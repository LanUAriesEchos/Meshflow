/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QList>
#include <QRect>
#include <QString>

/// @brief Pure geometry helpers for the free-form (Windows style) display layout.
namespace displaylayout {

constexpr int kDefaultWidth = 1920;
constexpr int kDefaultHeight = 1080;
constexpr int kMinSize = 320;
constexpr int kMaxSize = 16384;
constexpr int kMaxCoordinate = 1000000;

enum class Side
{
  Right,
  Left,
  Up,
  Down
};

struct Link
{
  int from = -1;
  int to = -1;
  Side side = Side::Right;
  // fractions (0..1) of the edge of the source / destination screen that is shared
  double srcStart = 0;
  double srcEnd = 1;
  double dstStart = 0;
  double dstEnd = 1;
};

/// One computer: its bounding rectangle in the shared desktop and its monitors (absolute coordinates).
struct Computer
{
  QRect bounds;
  QList<QRect> monitors;
};

/// Rectangle used for a screen that has never been positioned: one default sized cell per grid slot.
QRect defaultRectForSlot(int slot, int columns);

/// Links between computers whose monitors touch along the outer edge of their bounding rectangles.
/// Null bounds are ignored.
QList<Link> computeLinks(const QList<Computer> &computers);

/// Same as above for computers that each consist of a single monitor.
QList<Link> computeLinks(const QList<QRect> &rects);

/// Configuration keyword for a side ("right", "left", "up", "down").
QString sideName(Side side);

/// "(start,end)" in percent, or an empty string for a full edge. Returns false if the range collapses.
bool formatInterval(double start, double end, QString &out);

bool overlapsAny(const QRect &rect, const QList<QRect> &others);

/// Moves @p rect so that it sits flush against, or aligned with, the nearest edges of @p others
/// when closer than @p threshold virtual pixels.
QRect snap(const QRect &rect, const QList<QRect> &others, int threshold);

/// Indexes of screens that have no link to any other screen.
QList<int> isolatedScreens(const QList<QRect> &rects);
QList<int> isolatedScreens(const QList<Computer> &computers);

} // namespace displaylayout
