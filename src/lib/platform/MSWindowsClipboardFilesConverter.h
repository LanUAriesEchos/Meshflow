/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/MSWindowsClipboard.h"

//! Convert between CF_HDROP and IClipboard::Format::Files
class MSWindowsClipboardFilesConverter : public IMSWindowsClipboardConverter
{
public:
  MSWindowsClipboardFilesConverter() = default;
  ~MSWindowsClipboardFilesConverter() override = default;

  // IMSWindowsClipboardConverter overrides
  IClipboard::Format getFormat() const override;
  UINT getWin32Format() const override;
  HANDLE fromIClipboard(const std::string &) const override;
  std::string toIClipboard(HANDLE) const override;
};
