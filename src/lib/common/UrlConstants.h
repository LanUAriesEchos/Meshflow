/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

// important: this is used for settings paths on some platforms,
// and must not be a url. qt automatically converts this to reverse domain
// notation (rdn), e.g. org.meshflow
const auto kOrgDomain = QStringLiteral("github.com/RimoraStudio/Aether");

const auto kUrlSourceQuery = QStringLiteral("source=gui");
const auto kUrlApp = QStringLiteral("https://github.com/LanUAriesEchos/Meshflow");
const auto kUrlHelp = QStringLiteral("%1#readme").arg(kUrlApp);
const auto kUrlDownload = QStringLiteral("%1/releases/latest").arg(kUrlApp);
const auto kUrlWiki = QStringLiteral("%1/wiki").arg(kUrlApp);
const auto kUrlUpdateCheck = QStringLiteral("https://api.github.com/repos/LanUAriesEchos/Meshflow/releases/latest");

#if defined(Q_OS_LINUX)
const auto kUrlGnomeTrayFix = QStringLiteral("https://extensions.gnome.org/extension/615/appindicator-support/");
#endif
