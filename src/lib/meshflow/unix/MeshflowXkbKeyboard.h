/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#if WINAPI_XWINDOWS

#pragma once

#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

namespace meshflow::linux {

class MeshflowXkbKeyboard
{
  XkbRF_VarDefsRec m_data = {};

public:
  MeshflowXkbKeyboard();
  MeshflowXkbKeyboard(const MeshflowXkbKeyboard &) = delete;
  MeshflowXkbKeyboard &operator=(const MeshflowXkbKeyboard &) = delete;

  const char *getLayout() const;
  const char *getVariant() const;

  ~MeshflowXkbKeyboard();
};

} // namespace meshflow::linux

#endif // WINAPI_XWINDOWS
