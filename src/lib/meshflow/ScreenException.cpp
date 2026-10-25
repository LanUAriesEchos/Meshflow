/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Rimora Studio
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "meshflow/ScreenException.h"

//
// ScreenOpenFailureException
//

QString ScreenOpenFailureException::getWhat() const throw()
{
  return format("ScreenOpenFailureException", "unable to open screen");
}

//
// ScreenUnavailableException
//

QString ScreenUnavailableException::getWhat() const throw()
{
  return format("ScreenUnavailableException", "unable to open screen");
}
