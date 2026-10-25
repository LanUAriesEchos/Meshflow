/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/NetworkAddress.h"
#include "net/SecurityLevel.h"

#include <cstdint>
#include <memory>
#include <vector>

class Event;
class EventQueueTimer;
class IDataSocket;
class IEventQueue;
class IListenSocket;
class ISocketFactory;
class SocketMultiplexer;
class StreamPump;

//! Localhost TCP port forwarder for client port sharing
/*!
Binds one loopback-only TCP listen socket per configured port. Each
accepted connection opens a new connection to the server, identifies
itself with the `meshflowfwd:<port>` hello-back name, and then relays
bytes bidirectionally via a StreamPump.
*/
class PortShareListener
{
public:
  PortShareListener(
      IEventQueue *events, SocketMultiplexer *multiplexer, std::unique_ptr<ISocketFactory> socketFactory,
      const NetworkAddress &serverAddress, bool secure
  );
  PortShareListener(const PortShareListener &) = delete;
  PortShareListener &operator=(const PortShareListener &) = delete;
  ~PortShareListener();

  struct PortMap
  {
    uint16_t remote = 0; ///< port on the server's localhost
    uint16_t local = 0;  ///< port bound on this machine's localhost
  };

  void setPorts(std::vector<PortMap> ports);
  void start();
  void stop();

private:
  struct Listener
  {
    IListenSocket *socket = nullptr;
    uint16_t remotePort = 0;
    uint16_t localPort = 0;
  };

  struct ForwardConn
  {
    IDataSocket *forward = nullptr;
    IDataSocket *local = nullptr;
    StreamPump *pump = nullptr;
    EventQueueTimer *timer = nullptr;
    uint16_t remotePort = 0;
    uint16_t localPort = 0;
  };

  void handleAccept(Listener *listener);
  void handleForwardConnected(ForwardConn *conn);
  void handleForwardFailed(ForwardConn *conn, const Event &event);
  void handleForwardHello(ForwardConn *conn);
  void removeConn(ForwardConn *conn);
  size_t connCount(uint16_t port) const;

  static constexpr size_t kMaxForwardsPerPort = 8;
  static constexpr double kHelloTimeout = 10.0;

  IEventQueue *m_events;
  SocketMultiplexer *m_multiplexer;
  std::unique_ptr<ISocketFactory> m_socketFactory;
  NetworkAddress m_serverAddress;
  SecurityLevel m_securityLevel;
  std::vector<PortMap> m_ports;
  std::vector<std::unique_ptr<Listener>> m_listeners;
  std::vector<std::unique_ptr<ForwardConn>> m_conns;
};
