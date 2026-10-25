/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class DisplayLayoutTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void gridNeighboursHaveFullEdges();
  void partialOverlapProducesIntervals();
  void touchingAtCornerIsNotALink();
  void stackedNeighboursDoNotOverlap();
  void snapMovesFlushAgainstEdge();
  void snapAlignsTopEdges();
  void overlapDetection();
  void isolatedScreens();
  void intervalFormatting();
  void multiMonitorLinksUseOuterEdges();
};
