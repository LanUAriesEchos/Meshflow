/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/PortShareListener.h"

#include "arch/Arch.h"
#include "base/BaseException.h"
#include "base/Event.h"
#include "base/EventTypes.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "meshflow/ProtocolTypes.h"
#include "meshflow/ProtocolUtil.h"
#include "meshflow/StreamPump.h"
#include "common/NetworkProtocol.h"
#include "net/IDataSocket.h"
#include "net/IListenSocket.h"
#include "net/ISocketFactory.h"
#include "net/TCPListenSocket.h"

#include <QString>

#include <algorithm>

PortShareListener::PortShareListener(
    IEventQueue *events, SocketMultiplexer *multiplexer, std::unique_ptr<ISocketFactory> socketFactory,
    const NetworkAddress &serverAddress, bool secure
)
    : m_events(events),
      m_multiplexer(multiplexer),
      m_socketFactory(std::move(socketFactory)),
      m_serverAddress(serverAddress),
      m_securityLevel(secure ? SecurityLevel::PeerAuth : SecurityLevel::PlainText)
{
  assert(m_events != nullptr);
  assert(m_multiplexer != nullptr);
  assert(m_socketFactory != nullptr);
}

PortShareListener::~PortShareListener()
{
  stop();
}

void PortShareListener::setPorts(std::vector<PortMap> ports)
{
  const bool running = !m_listeners.empty();
  if (running) {
    stop();
  }
  m_ports = std::move(ports);
  if (running) {
    start();
  }
}

void PortShareListener::start()
{
  stop();
  if (m_ports.empty()) {
    return;
  }

  try {
    m_serverAddress.resolve();
  } catch (BaseException &e) {
    LOG_WARN("port share: cannot resolve server address: %s", e.what());
  }

  for (const auto &map : m_ports) {
    try {
      NetworkAddress local("127.0.0.1", map.local);
      local.resolve();

      auto listener = std::make_unique<Listener>();
      listener->localPort = map.local;
      listener->remotePort = map.remote;
      listener->socket = new TCPListenSocket(m_events, m_multiplexer, ARCH->getAddrFamily(local.getAddress()));

      auto *listenSocket = listener->socket;
      auto *rawListener = listener.get();
      m_events->addHandler(EventTypes::ListenSocketConnecting, listenSocket, [this, rawListener](const auto &) {
        handleAccept(rawListener);
      });
      listenSocket->bind(local);

      m_listeners.push_back(std::move(listener));
      LOG_INFO("port share: forwarding localhost:%d to server port %d", map.local, map.remote);
    } catch (BaseException &e) {
      LOG_WARN("port share: cannot listen on localhost:%d: %s", map.local, e.what());
    }
  }
}

void PortShareListener::stop()
{
  while (!m_conns.empty()) {
    removeConn(m_conns.back().get());
  }

  for (auto &listener : m_listeners) {
    m_events->removeHandler(EventTypes::ListenSocketConnecting, listener->socket);
    delete listener->socket;
  }
  m_listeners.clear();
}

size_t PortShareListener::connCount(uint16_t port) const
{
  return static_cast<size_t>(std::count_if(m_conns.begin(), m_conns.end(), [port](const auto &conn) {
    return conn->localPort == port;
  }));
}

void PortShareListener::handleAccept(Listener *listener)
{
  while (auto socket = listener->socket->accept()) {
    if (connCount(listener->localPort) >= kMaxForwardsPerPort) {
      LOG_WARN("port share: too many forwards on port %d, refusing connection", listener->localPort);
      socket->close();
      continue;
    }

    auto conn = std::make_unique<ForwardConn>();
    conn->local = socket.release();
    conn->localPort = listener->localPort;
    conn->remotePort = listener->remotePort;

    try {
      conn->forward = m_socketFactory->create(ARCH->getAddrFamily(m_serverAddress.getAddress()), m_securityLevel);
    } catch (BaseException &e) {
      LOG_WARN("port share: cannot create forward socket: %s", e.what());
      delete conn->local;
      continue;
    }

    auto *c = conn.get();
    m_conns.push_back(std::move(conn));

    auto *target = c->forward->getEventTarget();
    const auto connectedType =
        m_securityLevel == SecurityLevel::PlainText ? EventTypes::DataSocketConnected
                                                    : EventTypes::DataSocketSecureConnected;
    m_events->addHandler(connectedType, target, [this, c](const auto &) { handleForwardConnected(c); });
    m_events->addHandler(EventTypes::DataSocketConnectionFailed, target, [this, c](const auto &e) {
      handleForwardFailed(c, e);
    });
    m_events->addHandler(EventTypes::SocketDisconnected, target, [this, c](const auto &) { removeConn(c); });
    m_events->addHandler(EventTypes::StreamInputReady, target, [this, c](const auto &) { handleForwardHello(c); });

    c->timer = m_events->newOneShotTimer(kHelloTimeout, nullptr);
    m_events->addHandler(EventTypes::Timer, c->timer, [this, c](const auto &) {
      LOG_WARN("port share: timed out waiting for server hello on port %d", c->localPort);
      removeConn(c);
    });

    try {
      c->forward->connect(m_serverAddress);
    } catch (BaseException &e) {
      LOG_WARN("port share: forward connect failed on port %d: %s", c->localPort, e.what());
      removeConn(c);
    }
  }
}

void PortShareListener::handleForwardConnected(ForwardConn *conn)
{
  auto *target = conn->forward->getEventTarget();
  m_events->removeHandler(EventTypes::DataSocketConnected, target);
  m_events->removeHandler(EventTypes::DataSocketSecureConnected, target);
  m_events->removeHandler(EventTypes::DataSocketConnectionFailed, target);

  LOG_VERBOSE("port share: forward connected for port %d, waiting for hello", conn->localPort);
  if (conn->forward->isReady()) {
    m_events->addEvent(Event(EventTypes::StreamInputReady, target));
  }
}

void PortShareListener::handleForwardFailed(ForwardConn *conn, const Event &event)
{
  auto *info = static_cast<IDataSocket::ConnectionFailedInfo *>(event.getData());
  LOG_WARN("port share: forward connect failed on port %d: %s", conn->localPort, info ? info->m_what.c_str() : "unknown");
  delete info;
  removeConn(conn);
}

void PortShareListener::handleForwardHello(ForwardConn *conn)
{
  int16_t serverMajor;
  int16_t serverMinor;
  std::string protocolName;

  if (!ProtocolUtil::readf(conn->forward, kMsgHello, &protocolName, &serverMajor, &serverMinor) ||
      networkProtocolFromString(QString::fromStdString(protocolName)) == NetworkProtocol::Unknown ||
      serverMajor != kProtocolMajorVersion) {
    LOG_WARN("port share: bad hello from server on port %d", conn->localPort);
    removeConn(conn);
    return;
  }

  const int16_t helloBackMinor = serverMinor < kProtocolMinorVersion ? serverMinor : kProtocolMinorVersion;
  std::string name = std::string(kPortShareNamePrefix) + std::to_string(conn->remotePort);
  const std::string helloBackMessage = protocolName + kMsgHelloBackArgs;

  try {
    ProtocolUtil::writef(conn->forward, helloBackMessage.c_str(), kProtocolMajorVersion, helloBackMinor, &name);
  } catch (BaseException &e) {
    LOG_WARN("port share: hello-back write failed on port %d: %s", conn->localPort, e.what());
    removeConn(conn);
    return;
  }

  auto *forwardTarget = conn->forward->getEventTarget();
  m_events->removeHandler(EventTypes::StreamInputReady, forwardTarget);
  m_events->removeHandler(EventTypes::SocketDisconnected, forwardTarget);
  if (conn->timer != nullptr) {
    m_events->removeHandler(EventTypes::Timer, conn->timer);
    m_events->deleteTimer(conn->timer);
    conn->timer = nullptr;
  }

  LOG_INFO("port share: tunneling localhost:%d to server port %d", conn->localPort, conn->remotePort);
  conn->pump = new StreamPump(conn->local, conn->forward, m_events, [this, conn]() { removeConn(conn); });

  if (conn->forward->isReady()) {
    m_events->addEvent(Event(EventTypes::StreamInputReady, forwardTarget));
  }
  if (conn->local->isReady()) {
    m_events->addEvent(Event(EventTypes::StreamInputReady, conn->local->getEventTarget()));
  }
}

void PortShareListener::removeConn(ForwardConn *conn)
{
  const auto it = std::find_if(m_conns.begin(), m_conns.end(), [conn](const auto &c) {
    return c.get() == conn;
  });
  if (it == m_conns.end()) {
    return;
  }

  if (conn->timer != nullptr) {
    m_events->removeHandler(EventTypes::Timer, conn->timer);
    m_events->deleteTimer(conn->timer);
  }

  delete conn->pump;
  for (auto *socket : {conn->forward, conn->local}) {
    if (socket != nullptr) {
      m_events->removeHandlers(socket->getEventTarget());
      delete socket;
    }
  }

  m_conns.erase(it);
}
