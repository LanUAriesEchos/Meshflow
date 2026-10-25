/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Theme.h"

#include "StyleUtils.h"
#include "common/Settings.h"

#include <oclero/qlementine/resources/ResourceInitialization.hpp>
#include <oclero/qlementine/style/QlementineStyle.hpp>
#include <oclero/qlementine/style/ThemeManager.hpp>

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QPointer>
#include <QStyleHints>
#include <QWidget>

namespace {
const auto kThemeDark = QStringLiteral("Dark");
const auto kThemeLight = QStringLiteral("Light");
const auto kPreferenceDark = QStringLiteral("dark");
const auto kPreferenceLight = QStringLiteral("light");

QPointer<oclero::qlementine::ThemeManager> s_themeManager;

bool systemPrefersDark()
{
  if (const auto scheme = QGuiApplication::styleHints()->colorScheme(); scheme != Qt::ColorScheme::Unknown)
    return scheme == Qt::ColorScheme::Dark;
  return meshflow::gui::isDarkMode();
}

void applyPreference()
{
  if (!s_themeManager)
    return;
  const auto preference = Settings::value(Settings::Gui::Theme).toString();
  const bool dark = (preference == kPreferenceDark) || (preference != kPreferenceLight && systemPrefersDark());
  s_themeManager->setCurrentTheme(dark ? kThemeDark : kThemeLight);
  meshflow::gui::updateIconTheme();
}
} // namespace

void meshflow::gui::applyTheme(QApplication &app)
{
  oclero::qlementine::resources::initializeResources();

  auto *style = new oclero::qlementine::QlementineStyle(&app);
  QApplication::setStyle(style);

  s_themeManager = new oclero::qlementine::ThemeManager(style, &app);
  s_themeManager->loadDirectory(QStringLiteral(":/themes"));

  applyPreference();

  QObject::connect(
      QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, s_themeManager,
      [](Qt::ColorScheme) { applyPreference(); }
  );
}

void meshflow::gui::reapplyTheme()
{
  applyPreference();
}

QColor meshflow::gui::secondaryTextColor()
{
  if (const auto *style = qobject_cast<oclero::qlementine::QlementineStyle *>(QApplication::style()))
    return style->theme().secondaryAlternativeColor;
  return QPalette().color(QPalette::WindowText);
}

void meshflow::gui::applySecondaryText(QWidget *widget)
{
  auto pal = widget->palette();
  pal.setColor(QPalette::WindowText, secondaryTextColor());
  widget->setPalette(pal);
}