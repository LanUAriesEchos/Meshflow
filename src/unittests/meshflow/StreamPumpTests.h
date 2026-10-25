/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"

#include <QTest>

class StreamPumpTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void relaysDataBothWays();
  void inputShutdownPropagates();
  void bothShutdownClosesAndFinishes();
  void outputErrorFinishes();

private:
  Log m_log;
};
