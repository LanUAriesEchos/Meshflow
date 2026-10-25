/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "meshflow/StreamPump.h"

#include "base/EventTypes.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "io/IStream.h"

StreamPump::StreamPump(meshflow::IStream *a, meshflow::IStream *b, IEventQueue *events, std::function<void()> onDone)
    : m_events(events),
      m_a(a),
      m_b(b),
      m_onDone(std::move(onDone))
{
  for (auto *stream : {m_a, m_b}) {
    m_events->addHandler(EventTypes::StreamInputReady, stream->getEventTarget(), [this, stream](const auto &) {
      handleData(stream, stream == m_a ? m_b : m_a);
    });
    m_events->addHandler(EventTypes::StreamInputShutdown, stream->getEventTarget(), [this, stream](const auto &) {
      handleShutdown(stream, stream == m_a ? m_b : m_a);
    });
    m_events->addHandler(EventTypes::StreamInputFormatError, stream->getEventTarget(), [this](const auto &) {
      handleError();
    });
    m_events->addHandler(EventTypes::StreamOutputError, stream->getEventTarget(), [this](const auto &) {
      handleError();
    });
    m_events->addHandler(EventTypes::StreamOutputShutdown, stream->getEventTarget(), [this](const auto &) {
      handleError();
    });
  }
}

StreamPump::~StreamPump()
{
  using enum EventTypes;
  for (auto *stream : {m_a, m_b}) {
    if (stream == nullptr) {
      continue;
    }
    m_events->removeHandler(StreamInputReady, stream->getEventTarget());
    m_events->removeHandler(StreamInputShutdown, stream->getEventTarget());
    m_events->removeHandler(StreamInputFormatError, stream->getEventTarget());
    m_events->removeHandler(StreamOutputError, stream->getEventTarget());
    m_events->removeHandler(StreamOutputShutdown, stream->getEventTarget());
  }
}

void StreamPump::handleData(meshflow::IStream *from, meshflow::IStream *to)
{
  uint8_t buffer[kChunkSize];
  while (!m_done) {
    const uint32_t n = from->read(buffer, sizeof(buffer));
    if (n == 0) {
      break;
    }
    to->write(buffer, n);
  }
}

void StreamPump::handleShutdown(meshflow::IStream *from, meshflow::IStream *to)
{
  if (from == m_a) {
    m_aInputShutdown = true;
  } else {
    m_bInputShutdown = true;
  }
  to->flush();
  to->shutdownOutput();
  if (m_aInputShutdown && m_bInputShutdown) {
    finish();
  }
}

void StreamPump::handleError()
{
  LOG_INFO("port share relay error, closing connection");
  finish();
}

void StreamPump::finish()
{
  if (m_done) {
    return;
  }
  m_done = true;
  m_a->close();
  m_b->close();
  if (m_onDone) {
    m_onDone();
  }
}
