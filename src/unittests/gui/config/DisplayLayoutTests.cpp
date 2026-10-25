/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DisplayLayoutTests.h"

#include "gui/config/DisplayLayout.h"

using namespace displaylayout;

namespace {
const Link *find(const QList<Link> &links, int from, Side side, int to)
{
  for (const auto &l : links) {
    if (l.from == from && l.side == side && l.to == to)
      return &l;
  }
  return nullptr;
}
} // namespace

void DisplayLayoutTests::gridNeighboursHaveFullEdges()
{
  const QList<QRect> rects = {QRect(0, 0, 1920, 1080), QRect(1920, 0, 1920, 1080)};
  const auto links = computeLinks(rects);

  QCOMPARE(links.size(), 2);
  const auto *right = find(links, 0, Side::Right, 1);
  QVERIFY(right);
  QCOMPARE(right->srcStart, 0.0);
  QCOMPARE(right->srcEnd, 1.0);
  QVERIFY(find(links, 1, Side::Left, 0));
}

void DisplayLayoutTests::multiMonitorLinksUseOuterEdges()
{
  // two 1920x1080 monitors side by side, a laptop touching the right monitor
  Computer pc{QRect(0, 0, 3840, 1080), {QRect(0, 0, 1920, 1080), QRect(1920, 0, 1920, 1080)}};
  Computer laptop{QRect(3840, 0, 1920, 1080), {QRect(3840, 0, 1920, 1080)}};
  const auto links = computeLinks(QList<Computer>{pc, laptop});

  QCOMPARE(links.size(), 2);
  const auto *right = find(links, 0, Side::Right, 1);
  QVERIFY(right);
  QCOMPARE(right->srcStart, 0.0);
  QCOMPARE(right->srcEnd, 1.0);
  QVERIFY(find(links, 1, Side::Left, 0));

  // a computer touching the inner seam of the PC's monitors is not linked
  Computer below{QRect(1920, 1080, 1920, 1080), {QRect(1920, 1080, 1920, 1080)}};
  const auto below2 = computeLinks(QList<Computer>{pc, below});
  const auto *down = find(below2, 0, Side::Down, 1);
  QVERIFY(down);
  QCOMPARE(down->srcStart, 0.5);
  QCOMPARE(down->srcEnd, 1.0);
}

void DisplayLayoutTests::innerEdgeAboveCenterMonitorLinks()
{
  // friend's PC: left and right monitors stick up, the centre one is lower with empty space above it
  Computer friendPc{
      QRect(0, 0, 3000, 1080), {QRect(0, 0, 1000, 1080), QRect(1000, 200, 1000, 880), QRect(2000, 0, 1000, 1080)}
  };
  // my display sits in that empty space, right above the centre monitor
  Computer mine{QRect(1000, 0, 1000, 200), {QRect(1000, 0, 1000, 200)}};
  const auto links = computeLinks(QList<Computer>{friendPc, mine});

  const auto *up = find(links, 0, Side::Up, 1);
  QVERIFY(up);
  QCOMPARE(up->srcStart, 1.0 / 3.0);
  QCOMPARE(up->srcEnd, 2.0 / 3.0);
  QVERIFY(find(links, 1, Side::Down, 0));

  // the side monitors are not adjacent to it across open space, so no link to them
  QCOMPARE(links.size(), 2);
}

void DisplayLayoutTests::partialOverlapProducesIntervals()
{
  // a laptop (1000 high) sitting half way down the right side of a 2000 high monitor
  const QList<QRect> rects = {QRect(0, 0, 2000, 2000), QRect(2000, 500, 1000, 1000)};
  const auto links = computeLinks(rects);

  const auto *right = find(links, 0, Side::Right, 1);
  QVERIFY(right);
  QCOMPARE(right->srcStart, 0.25);
  QCOMPARE(right->srcEnd, 0.75);
  QCOMPARE(right->dstStart, 0.0);
  QCOMPARE(right->dstEnd, 1.0);

  const auto *left = find(links, 1, Side::Left, 0);
  QVERIFY(left);
  QCOMPARE(left->srcStart, 0.0);
  QCOMPARE(left->srcEnd, 1.0);
  QCOMPARE(left->dstStart, 0.25);
  QCOMPARE(left->dstEnd, 0.75);
}

void DisplayLayoutTests::touchingAtCornerIsNotALink()
{
  const QList<QRect> rects = {QRect(0, 0, 1000, 1000), QRect(1000, 1000, 1000, 1000)};
  QVERIFY(computeLinks(rects).isEmpty());
}

void DisplayLayoutTests::stackedNeighboursDoNotOverlap()
{
  const QList<QRect> rects = {QRect(0, 0, 1000, 2000), QRect(1000, 0, 1000, 1000), QRect(1000, 1000, 1000, 1000)};
  const auto links = computeLinks(rects);

  const auto *top = find(links, 0, Side::Right, 1);
  const auto *bottom = find(links, 0, Side::Right, 2);
  QVERIFY(top && bottom);
  QCOMPARE(top->srcEnd, bottom->srcStart);
}

void DisplayLayoutTests::snapMovesFlushAgainstEdge()
{
  const QList<QRect> others = {QRect(0, 0, 1920, 1080)};
  const QRect snapped = snap(QRect(1930, 0, 1920, 1080), others, 20);
  QCOMPARE(snapped.x(), 1920);

  const QRect far = snap(QRect(2100, 0, 1920, 1080), others, 20);
  QCOMPARE(far.x(), 2100);
}

void DisplayLayoutTests::snapAlignsTopEdges()
{
  const QList<QRect> others = {QRect(0, 0, 1920, 1080)};
  const QRect snapped = snap(QRect(1920, 12, 1280, 720), others, 20);
  QCOMPARE(snapped.y(), 0);
  QCOMPARE(snapped.x(), 1920);
}

void DisplayLayoutTests::overlapDetection()
{
  const QList<QRect> others = {QRect(0, 0, 1000, 1000)};
  QVERIFY(overlapsAny(QRect(999, 0, 1000, 1000), others));
  QVERIFY(!overlapsAny(QRect(1000, 0, 1000, 1000), others));
  QVERIFY(!overlapsAny(QRect(0, 0, 10, 10), {QRect()}));
}

void DisplayLayoutTests::isolatedScreens()
{
  const QList<QRect> rects = {QRect(0, 0, 1000, 1000), QRect(1000, 0, 1000, 1000), QRect(5000, 0, 1000, 1000), QRect()};
  QCOMPARE(displaylayout::isolatedScreens(rects), QList<int>{2});
}

void DisplayLayoutTests::intervalFormatting()
{
  QString out;
  QVERIFY(formatInterval(0.0, 1.0, out));
  QVERIFY(out.isEmpty());

  QVERIFY(formatInterval(0.25, 0.755, out));
  QCOMPARE(out, QStringLiteral("(25,75.5)"));

  QVERIFY(!formatInterval(0.5, 0.50000001, out));

  QCOMPARE(sideName(Side::Up), QStringLiteral("up"));
}

QTEST_MAIN(DisplayLayoutTests)
