/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

class QApplication;
class QColor;
class QWidget;

namespace meshflow::gui {

// Installs the Qlementine style on the application and applies the
// light or dark theme following the "gui/theme" setting (system/light/dark).
// Also refreshes the icon theme and follows live system color-scheme changes.
void applyTheme(QApplication &app);

// Re-reads the "gui/theme" setting and applies it. No-op before applyTheme.
void reapplyTheme();

// Muted text color from the active Qlementine theme.
// Falls back to the regular text color when the app style isn't Qlementine.
QColor secondaryTextColor();

// Makes a widget render in the theme's muted text color.
// Prefer this over stylesheet `color:` so the widget follows theme switches.
void applySecondaryText(QWidget *widget);

} // namespace meshflow::gui
