/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/PortForwardSession.h"

#include "base/EventTypes.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "meshflow/StreamPump.h"
#include "io/IStream.h"
#include "net/NetworkAddress.h"
#include "net/TCPSocket.h"

PortForwardSession::PortForwardSession(
    meshflow::IStream *clientStream, uint16_t port, Server *, IEventQueue *events, std::function<void()> onDone
)
    : m_clientStream(clientStream),
      m_events(events),
      m_onDone(std::move(onDone)),
      m_port(port)
{
  m_localSocket = new TCPSocket(m_events, &m_socketMultiplexer);

  m_events->addHandler(EventTypes::DataSocketConnected, m_localSocket->getEventTarget(), [this](const auto &) {
    handleConnected();
  });
  m_events->addHandler(EventTypes::DataSocketConnectionFailed, m_localSocket->getEventTarget(), [this](const auto &) {
    handleConnectFailed();
  });

  try {
    // loopback only: the forwarded port always targets this machine
    NetworkAddress address("127.0.0.1", port);
    address.resolve();
    m_localSocket->connect(address);
  } catch (...) {
    delete m_localSocket;
    m_localSocket = nullptr;
    throw;
  }
}

PortForwardSession::~PortForwardSession()
{
  cleanup();
}

void PortForwardSession::handleConnected()
{
  LOG_INFO("port share: forwarding client to localhost:%u", m_port);
  m_pump = new StreamPump(m_clientStream, m_localSocket, m_events, [this] { finish(); });
}

void PortForwardSession::handleConnectFailed()
{
  LOG_WARN("port share: could not connect to localhost:%u", m_port);
  finish();
}

void PortForwardSession::cleanup()
{
  if (m_localSocket != nullptr) {
    m_events->removeHandler(EventTypes::DataSocketConnected, m_localSocket->getEventTarget());
    m_events->removeHandler(EventTypes::DataSocketConnectionFailed, m_localSocket->getEventTarget());
  }
  delete m_pump;
  m_pump = nullptr;
  delete m_clientStream;
  m_clientStream = nullptr;
  delete m_localSocket;
  m_localSocket = nullptr;
}

void PortForwardSession::finish()
{
  cleanup();
  if (m_onDone) {
    auto onDone = std::move(m_onDone);
    m_onDone = nullptr;
    onDone();
  }
}
