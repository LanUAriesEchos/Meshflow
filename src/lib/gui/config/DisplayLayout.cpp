/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DisplayLayout.h"

#include <QSet>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace displaylayout {

namespace {

struct Span
{
  int lo;
  int hi;
};

Span overlap(int aLo, int aHi, int bLo, int bHi)
{
  return {std::max(aLo, bLo), std::min(aHi, bHi)};
}

void addLink(QList<Link> &links, int from, int to, Side side, const QRect &a, const QRect &b, Span s)
{
  if (s.hi <= s.lo)
    return;

  const bool horizontalEdge = side == Side::Up || side == Side::Down;
  const int aStart = horizontalEdge ? a.x() : a.y();
  const int aLen = horizontalEdge ? a.width() : a.height();
  const int bStart = horizontalEdge ? b.x() : b.y();
  const int bLen = horizontalEdge ? b.width() : b.height();

  Link l;
  l.from = from;
  l.to = to;
  l.side = side;
  l.srcStart = double(s.lo - aStart) / aLen;
  l.srcEnd = double(s.hi - aStart) / aLen;
  l.dstStart = double(s.lo - bStart) / bLen;
  l.dstEnd = double(s.hi - bStart) / bLen;
  links.append(l);
}

// Best (smallest magnitude) adjustment among candidates, or 0 when none is within the threshold.
void consider(int delta, int threshold, int &best)
{
  if (std::abs(delta) <= threshold && (best == 0 || std::abs(delta) < std::abs(best)))
    best = delta;
}

} // namespace

QRect defaultRectForSlot(int slot, int columns)
{
  const int cols = std::max(columns, 1);
  return QRect((slot % cols) * kDefaultWidth, (slot / cols) * kDefaultHeight, kDefaultWidth, kDefaultHeight);
}

QList<Link> computeLinks(const QList<QRect> &rects)
{
  QList<Computer> computers;
  computers.reserve(rects.size());
  for (const QRect &r : rects)
    computers.append({r, r.isNull() ? QList<QRect>() : QList<QRect>{r}});
  return computeLinks(computers);
}

QList<Link> computeLinks(const QList<Computer> &computers)
{
  using enum Side;
  QList<Link> links;
  const int n = static_cast<int>(computers.size());

  auto valid = [](const QRect &r) { return !r.isNull() && !r.isEmpty(); };
  auto right = [](const QRect &r) { return r.x() + r.width(); };
  auto bottom = [](const QRect &r) { return r.y() + r.height(); };

  for (int i = 0; i < n; i++) {
    const QRect &a = computers[i].bounds;
    if (!valid(a))
      continue;

    for (int j = 0; j < n; j++) {
      const QRect &b = computers[j].bounds;
      if (i == j || !valid(b))
        continue;

      // an edge is only usable where a monitor sits on the outer edge of its computer
      for (const QRect &ma : computers[i].monitors) {
        for (const QRect &mb : computers[j].monitors) {
          if (right(ma) == right(a) && mb.x() == b.x() && right(ma) == mb.x())
            addLink(links, i, j, Right, a, b, overlap(ma.y(), bottom(ma), mb.y(), bottom(mb)));
          if (ma.x() == a.x() && right(mb) == right(b) && right(mb) == ma.x())
            addLink(links, i, j, Left, a, b, overlap(ma.y(), bottom(ma), mb.y(), bottom(mb)));
          if (ma.y() == a.y() && bottom(mb) == bottom(b) && bottom(mb) == ma.y())
            addLink(links, i, j, Up, a, b, overlap(ma.x(), right(ma), mb.x(), right(mb)));
          if (bottom(ma) == bottom(a) && mb.y() == b.y() && bottom(ma) == mb.y())
            addLink(links, i, j, Down, a, b, overlap(ma.x(), right(ma), mb.x(), right(mb)));
        }
      }
    }
  }
  return links;
}

QString sideName(Side side)
{
  switch (side) {
  case Side::Right:
    return QStringLiteral("right");
  case Side::Left:
    return QStringLiteral("left");
  case Side::Up:
    return QStringLiteral("up");
  case Side::Down:
    return QStringLiteral("down");
  }
  return {};
}

bool formatInterval(double start, double end, QString &out)
{
  const double s = std::round(start * 100.0 * 10000.0) / 10000.0;
  const double e = std::round(end * 100.0 * 10000.0) / 10000.0;
  if (s >= e)
    return false;

  if (s <= 0.0 && e >= 100.0) {
    out.clear();
    return true;
  }

  out = QStringLiteral("(%1,%2)").arg(QString::number(s, 'g', 10), QString::number(e, 'g', 10));
  return true;
}

bool overlapsAny(const QRect &rect, const QList<QRect> &others)
{
  return std::ranges::any_of(others, [&rect](const QRect &o) {
    if (o.isNull() || o.isEmpty())
      return false;
    return rect.x() < o.x() + o.width() && o.x() < rect.x() + rect.width() && rect.y() < o.y() + o.height() &&
           o.y() < rect.y() + rect.height();
  });
}

QRect snap(const QRect &rect, const QList<QRect> &others, int threshold)
{
  int dx = 0;
  int dy = 0;
  const int right = rect.x() + rect.width();
  const int bottom = rect.y() + rect.height();

  for (const QRect &o : others) {
    if (o.isNull() || o.isEmpty())
      continue;
    const int oRight = o.x() + o.width();
    const int oBottom = o.y() + o.height();

    // only snap along an axis when the screens are close enough on the other axis to be neighbours
    const bool nearVertically = rect.y() < oBottom + threshold && o.y() < bottom + threshold;
    const bool nearHorizontally = rect.x() < oRight + threshold && o.x() < right + threshold;

    if (nearVertically) {
      consider(oRight - rect.x(), threshold, dx);
      consider(o.x() - right, threshold, dx);
    }
    if (nearHorizontally) {
      consider(oBottom - rect.y(), threshold, dy);
      consider(o.y() - bottom, threshold, dy);
    }
    if (nearVertically || nearHorizontally) {
      consider(o.x() - rect.x(), threshold, dx);
      consider(oRight - right, threshold, dx);
      consider(o.y() - rect.y(), threshold, dy);
      consider(oBottom - bottom, threshold, dy);
    }
  }
  return rect.translated(dx, dy);
}

QList<int> isolatedScreens(const QList<QRect> &rects)
{
  QList<Computer> computers;
  for (const QRect &r : rects)
    computers.append({r, r.isNull() ? QList<QRect>() : QList<QRect>{r}});
  return isolatedScreens(computers);
}

QList<int> isolatedScreens(const QList<Computer> &computers)
{
  QSet<int> linked;
  for (const Link &l : computeLinks(computers)) {
    linked.insert(l.from);
    linked.insert(l.to);
  }

  QList<int> isolated;
  for (int i = 0; i < computers.size(); i++) {
    const QRect &r = computers[i].bounds;
    if (!r.isNull() && !r.isEmpty() && !linked.contains(i))
      isolated.append(i);
  }
  return isolated;
}

} // namespace displaylayout
