/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "meshflow/StreamPump.h"

#include "base/Event.h"
#include "base/EventQueueTimer.h"
#include "base/EventTypes.h"
#include "base/IEventQueue.h"
#include "common/ExitCodes.h"
#include "io/IStream.h"

#include <deque>
#include <map>
#include <vector>

#include "StreamPumpTests.h"

namespace {

class FakeStream : public meshflow::IStream
{
public:
  void close() override
  {
    closed = true;
  }

  uint32_t read(void *buffer, uint32_t n) override
  {
    const uint32_t count = std::min<uint32_t>(n, static_cast<uint32_t>(in.size()));
    std::copy_n(in.begin(), count, static_cast<uint8_t *>(buffer));
    in.erase(in.begin(), in.begin() + count);
    return count;
  }

  void write(const void *buffer, uint32_t n) override
  {
    const auto *bytes = static_cast<const uint8_t *>(buffer);
    out.insert(out.end(), bytes, bytes + n);
  }

  void flush() override
  {
    flushed = true;
  }

  void shutdownInput() override
  {
  }

  void shutdownOutput() override
  {
    outputShutdown = true;
  }

  void *getEventTarget() const override
  {
    return const_cast<FakeStream *>(this);
  }

  bool isReady() const override
  {
    return !in.empty();
  }

  uint32_t getSize() const override
  {
    return static_cast<uint32_t>(in.size());
  }

  std::deque<uint8_t> in;
  std::deque<uint8_t> out;
  bool closed = false;
  bool flushed = false;
  bool outputShutdown = false;
};

class FakeEventQueue : public IEventQueue
{
public:
  EventQueueTimer *newOneShotTimer(double, void *) override
  {
    return nullptr;
  }

  EventQueueTimer *newTimer(double, void *) override
  {
    return nullptr;
  }

  bool getEvent(Event &, double) override
  {
    return true;
  }

  int loop() override
  {
    return s_exitSuccess;
  }

  void adoptBuffer(IEventQueueBuffer *) override
  {
  }

  void removeHandlers(void *) override
  {
  }

  EventTypes registerType(const char *)
  {
    return EventTypes::Unknown;
  }

  void addHandler(EventTypes type, void *target, const EventHandler &handler) override
  {
    m_handlers[{type, target}] = handler;
  }

  void addEvent(Event &&) override
  {
  }

  void removeHandler(EventTypes type, void *target) override
  {
    m_handlers.erase({type, target});
  }

  bool dispatchEvent(const Event &) override
  {
    return true;
  }

  void deleteTimer(EventQueueTimer *) override
  {
  }

  void waitForReady() const override
  {
  }

  void *getSystemTarget() override
  {
    return nullptr;
  }

  void fire(EventTypes type, void *target)
  {
    auto it = m_handlers.find({type, target});
    if (it != m_handlers.end()) {
      it->second(Event(type, target));
    }
  }

private:
  std::map<std::pair<EventTypes, void *>, EventHandler> m_handlers;
};

} // namespace

void StreamPumpTests::relaysDataBothWays()
{
  FakeEventQueue events;
  FakeStream a, b;
  a.in = {'h', 'i'};
  b.in = {'y', 'o'};

  StreamPump pump(&a, &b, &events, [] {});

  events.fire(EventTypes::StreamInputReady, &a);
  QCOMPARE(std::deque<uint8_t>({'h', 'i'}), b.out);
  QVERIFY(a.in.empty());

  events.fire(EventTypes::StreamInputReady, &b);
  QCOMPARE(std::deque<uint8_t>({'y', 'o'}), a.out);
}

void StreamPumpTests::inputShutdownPropagates()
{
  FakeEventQueue events;
  FakeStream a, b;
  StreamPump pump(&a, &b, &events, [] {});

  events.fire(EventTypes::StreamInputShutdown, &a);
  QVERIFY(b.flushed);
  QVERIFY(b.outputShutdown);
  QVERIFY(!a.closed);
}

void StreamPumpTests::bothShutdownClosesAndFinishes()
{
  FakeEventQueue events;
  FakeStream a, b;
  bool done = false;
  StreamPump pump(&a, &b, &events, [&done] { done = true; });

  events.fire(EventTypes::StreamInputShutdown, &a);
  events.fire(EventTypes::StreamInputShutdown, &b);
  QVERIFY(done);
  QVERIFY(a.closed);
  QVERIFY(b.closed);
}

void StreamPumpTests::outputErrorFinishes()
{
  FakeEventQueue events;
  FakeStream a, b;
  bool done = false;
  StreamPump pump(&a, &b, &events, [&done] { done = true; });

  events.fire(EventTypes::StreamOutputError, &b);
  QVERIFY(done);
  QVERIFY(a.closed);
  QVERIFY(b.closed);
}

QTEST_MAIN(StreamPumpTests)
#include "StreamPumpTests.moc"
