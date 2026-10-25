/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/SocketMultiplexer.h"

#include <cstdint>
#include <functional>

class IEventQueue;
class Server;
class StreamPump;
class TCPSocket;
namespace meshflow {
class IStream;
}

//! Localhost TCP port forwarding session
/*!
Connects an accepted forwarding client stream (identified by an
"meshflowfwd:<port>" hello-back name) to a service listening on the
server's loopback interface and pumps bytes between the two. The
session takes ownership of \c clientStream.
*/
class PortForwardSession
{
public:
  PortForwardSession(
      meshflow::IStream *clientStream, uint16_t port, Server *server, IEventQueue *events,
      std::function<void()> onDone
  );
  PortForwardSession(const PortForwardSession &) = delete;
  PortForwardSession(PortForwardSession &&) = delete;
  ~PortForwardSession();

  PortForwardSession &operator=(const PortForwardSession &) = delete;
  PortForwardSession &operator=(PortForwardSession &&) = delete;

private:
  void handleConnected();
  void handleConnectFailed();
  void cleanup();
  void finish();

private:
  SocketMultiplexer m_socketMultiplexer;
  meshflow::IStream *m_clientStream = nullptr;
  TCPSocket *m_localSocket = nullptr;
  StreamPump *m_pump = nullptr;
  IEventQueue *m_events = nullptr;
  std::function<void()> m_onDone;
  uint16_t m_port = 0;
};
