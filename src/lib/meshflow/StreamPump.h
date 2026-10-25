/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <functional>

class IEventQueue;

namespace meshflow {
class IStream;
}

//! Bidirectional byte relay between two streams
/*!
Pumps bytes between two streams using the event queue. Data read from
one stream is written to the other, and input shutdown on one side is
propagated as output shutdown on the peer. When the relay ends the
streams are closed and the done callback fires once.
*/
class StreamPump
{
public:
  StreamPump(meshflow::IStream *a, meshflow::IStream *b, IEventQueue *events, std::function<void()> onDone);
  StreamPump(const StreamPump &) = delete;
  StreamPump &operator=(const StreamPump &) = delete;
  ~StreamPump();

private:
  void handleData(meshflow::IStream *from, meshflow::IStream *to);
  void handleShutdown(meshflow::IStream *from, meshflow::IStream *to);
  void handleError();
  void finish();

  static constexpr uint32_t kChunkSize = 64 * 1024;

  IEventQueue *m_events;
  meshflow::IStream *m_a;
  meshflow::IStream *m_b;
  std::function<void()> m_onDone;
  bool m_aInputShutdown = false;
  bool m_bInputShutdown = false;
  bool m_done = false;
};
