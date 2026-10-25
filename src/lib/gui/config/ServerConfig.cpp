/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfig.h"

#include "common/Hotkey.h"
#include "common/Settings.h"
#include "gui/config/DisplayLayout.h"

#include <QAbstractButton>
#include <QGuiApplication>
#include <QPushButton>
#include <QScreen>

using enum ScreenConfig::Modifier;
using enum ScreenConfig::SwitchCorner;
using enum ScreenConfig::Fix;

const int serverDefaultIndex = 7;

ServerConfig::ServerConfig(int columns, int rows) : m_Screens(columns), m_columns(columns), m_rows(rows)
{
  recall();
}

bool ServerConfig::save(const QString &fileName) const
{
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;

  save(file);
  file.close();

  return true;
}

bool ServerConfig::operator==(const ServerConfig &sc) const
{
  return m_Screens == sc.m_Screens && //
         m_Hotkeys == sc.m_Hotkeys;   //
}

void ServerConfig::save(QFile &file) const
{
  QTextStream outStream(&file);
  outStream << *this;
}

void ServerConfig::setupScreens()
{
  screens().clear();
  hotkeys().clear();

  // There must always be screen objects for each cell in the screens QList.
  // Unused screens are identified by having an empty name.
  for (int i = 0; i < m_columns * m_rows; i++)
    addScreen(Screen());
}

void ServerConfig::commit()
{
  qDebug("committing server config");

  settings().beginGroup("internalConfig");
  settings().remove("");

  settings().beginWriteArray("screens");
  for (int i = 0; i < screens().size(); i++) {
    settings().setArrayIndex(i);
    const auto &screen = screens()[i];
    screen.saveSettings(settings());
    auto screenName = Settings::value(Settings::Core::ComputerName).toString();
    if (screen.isServer() && screenName != screen.name()) {
      Settings::setValue(Settings::Core::ComputerName, screen.name());
    }
  }
  settings().endArray();

  settings().beginWriteArray("hotkeys");
  for (int i = 0; i < hotkeys().size(); i++) {
    settings().setArrayIndex(i);
    hotkeys()[i].saveSettings(settings().get());
  }
  settings().endArray();

  settings().endGroup();
}

void ServerConfig::recall()
{
  qDebug("recalling server config");

  settings().beginGroup("internalConfig");

  m_columns = Settings::value(Settings::Server::GridWidth).toInt();
  m_rows = Settings::value(Settings::Server::GridHeight).toInt();

  // we need to know the number of columns and rows before we can set up
  // ourselves
  setupScreens();

  int numScreens = settings().beginReadArray("screens");
  Q_ASSERT(numScreens <= screens().size());
  for (int i = 0; i < numScreens; i++) {
    settings().setArrayIndex(i);
    screens()[i].loadSettings(settings());
    if (getServerName() == screens()[i].name()) {
      screens()[i].markAsServer();
    }
  }
  settings().endArray();

  int numHotkeys = settings().beginReadArray("hotkeys");
  for (int i = 0; i < numHotkeys; i++) {
    settings().setArrayIndex(i);
    Hotkey h;
    h.loadSettings(settings().get());
    hotkeys().append(h);
  }
  settings().endArray();

  settings().endGroup();

  ensureGeometry();
}

int ServerConfig::adjacentScreenIndex(int idx, int deltaColumn, int deltaRow) const
{
  if (screens()[idx].isNull())
    return -1;

  // if we're at the left or right end of the table, don't find results going
  // further left or right
  if ((deltaColumn > 0 && (idx + 1) % m_columns == 0) || (deltaColumn < 0 && idx % m_columns == 0))
    return -1;

  int arrayPos = idx + deltaColumn + deltaRow * m_columns;

  if (arrayPos >= screens().size() || arrayPos < 0)
    return -1;

  return arrayPos;
}

QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config)
{
  outStream << "section: screens" << Qt::endl;

  for (const Screen &s : config.screens()) {
    if (!s.isNull())
      outStream << s.screensSection();
  }

  outStream << "end" << Qt::endl << Qt::endl;

  outStream << "section: links" << Qt::endl;

  const auto links = displaylayout::computeLinks(config.layoutComputers());
  for (int i = 0; i < config.screens().size(); i++) {
    const auto &screen = config.screens()[i];
    if (screen.isNull())
      continue;

    outStream << "\t" << screen.name() << ":\n";
    for (const auto &link : links) {
      if (link.from != i)
        continue;

      QString srcInterval;
      QString dstInterval;
      if (!displaylayout::formatInterval(link.srcStart, link.srcEnd, srcInterval) ||
          !displaylayout::formatInterval(link.dstStart, link.dstEnd, dstInterval))
        continue;

      outStream << "\t\t" << displaylayout::sideName(link.side) << srcInterval << " = "
                << config.screens()[link.to].name() << dstInterval << Qt::endl;
    }
  }

  outStream << "end" << Qt::endl << Qt::endl;

  outStream << "section: options" << Qt::endl;

  for (const Hotkey &hotkey : config.hotkeys())
    outStream << hotkey;

  if (const auto sharedPorts = Settings::value(Settings::Server::SharedPorts).toString(); !sharedPorts.isEmpty()) {
    outStream << "\tsharedPorts = " << sharedPorts << Qt::endl;
  }

  outStream << "end" << Qt::endl << Qt::endl;

  return outStream;
}

int ServerConfig::numScreens() const
{
  int rval = 0;

  for (const Screen &s : screens()) {
    if (!s.isNull())
      rval++;
  }

  return rval;
}

QString ServerConfig::getServerName() const
{
  return Settings::value(Settings::Core::ComputerName).toString();
}

void ServerConfig::updateServerName()
{
  for (auto &screen : screens()) {
    if (screen.isServer()) {
      screen.setName(Settings::value(Settings::Core::ComputerName).toString());
      break;
    }
  }
}

QString ServerConfig::configFile() const
{
  return Settings::value(Settings::Server::ExternalConfigFile).toString();
}

bool ServerConfig::useExternalConfig() const
{
  return Settings::value(Settings::Server::ExternalConfig).toBool();
}

bool ServerConfig::isFull() const
{
  bool isFull = true;

  for (const auto &screen : screens()) {
    if (screen.isNull()) {
      isFull = false;
      break;
    }
  }

  return isFull;
}

bool ServerConfig::screenExists(const QString &screenName) const
{
  bool isExists = false;

  for (const auto &screen : screens()) {
    if (!screen.isNull() && screen.name() == screenName) {
      isExists = true;
      break;
    }
  }

  return isExists;
}

void ServerConfig::addClient(const QString &clientName)
{
  int serverIndex = -1;
  const auto screenName = Settings::value(Settings::Core::ComputerName).toString();

  if (findScreenName(screenName, serverIndex)) {
    m_Screens[serverIndex].markAsServer();
  } else {
    fixNoServer(screenName, serverIndex);
  }

  m_Screens.addScreenByPriority(Screen(clientName));
  ensureGeometry();
}

QList<QRect> ServerConfig::layoutRects() const
{
  QList<QRect> rects;
  rects.reserve(m_Screens.size());
  QRect bounds;

  for (const auto &screen : m_Screens) {
    rects.append(!screen.isNull() && screen.hasGeometry() ? screen.geometry() : QRect());
    bounds = bounds.united(rects.last());
  }

  // Screens that were never positioned keep the legacy grid cell when nothing else is placed yet,
  // otherwise they are appended to the right of the existing layout.
  const bool anyPositioned = !bounds.isNull();
  int nextX = bounds.x() + bounds.width();
  int nextY = bounds.y();
  for (int i = 0; i < m_Screens.size(); i++) {
    if (m_Screens[i].isServer() && !rects[i].isNull())
      nextY = rects[i].y();
  }
  for (int i = 0; i < m_Screens.size(); i++) {
    const auto &screen = m_Screens[i];
    if (screen.isNull() || screen.hasGeometry())
      continue;

    if (!anyPositioned) {
      rects[i] = displaylayout::defaultRectForSlot(i, m_columns);
    } else {
      rects[i] = QRect(nextX, nextY, displaylayout::kDefaultWidth, displaylayout::kDefaultHeight);
      nextX += displaylayout::kDefaultWidth;
    }
  }
  return rects;
}

void ServerConfig::ensureGeometry()
{
  const auto rects = layoutRects();
  for (int i = 0; i < m_Screens.size(); i++) {
    if (!m_Screens[i].isNull() && !m_Screens[i].hasGeometry())
      m_Screens[i].setGeometry(rects[i]);
  }
}

QList<displaylayout::Computer> ServerConfig::layoutComputers() const
{
  const auto rects = layoutRects();
  QList<displaylayout::Computer> computers;
  computers.reserve(rects.size());

  for (int i = 0; i < m_Screens.size(); i++) {
    displaylayout::Computer c;
    c.bounds = rects[i];
    if (!rects[i].isNull()) {
      if (m_Screens[i].monitors().isEmpty()) {
        c.monitors.append(rects[i]);
      } else {
        for (const QRect &m : m_Screens[i].monitors())
          c.monitors.append(m.translated(rects[i].topLeft()));
      }
    }
    computers.append(c);
  }
  return computers;
}

bool ServerConfig::detectLocalMonitors(bool force)
{
  if (!QGuiApplication::instance())
    return false;

  QList<QRect> monitors;
  QRect bounds;
  for (const QScreen *screen : QGuiApplication::screens()) {
    const qreal dpr = screen->devicePixelRatio();
    const QRect g = screen->geometry();
    const QRect pixels(qRound(g.x() * dpr), qRound(g.y() * dpr), qRound(g.width() * dpr), qRound(g.height() * dpr));
    monitors.append(pixels);
    bounds = bounds.united(pixels);
  }
  if (monitors.isEmpty())
    return false;

  for (QRect &m : monitors)
    m.translate(-bounds.topLeft());

  ensureGeometry();
  auto server = std::ranges::find_if(m_Screens, [](const Screen &s) { return !s.isNull() && s.isServer(); });
  if (server == m_Screens.end())
    return false;

  // on load keep an arrangement the user has already edited
  if (!force && (!server->monitors().isEmpty() ||
                 server->geometry().size() != QSize(displaylayout::kDefaultWidth, displaylayout::kDefaultHeight)))
    return false;

  const QList<QRect> storedMonitors = monitors.size() > 1 ? monitors : QList<QRect>();
  const QRect newBounds(server->geometry().topLeft(), bounds.size());
  if (server->monitors() == storedMonitors && server->geometry() == newBounds)
    return false;

  server->setMonitors(storedMonitors);
  server->setGeometry(newBounds);

  // the server may have grown over its neighbours: push those to the right of it
  for (int pass = 0; pass < 32; pass++) {
    bool moved = false;
    for (auto &other : m_Screens) {
      if (other.isNull() || other.isServer() || !other.hasGeometry())
        continue;
      if (displaylayout::overlapsAny(other.geometry(), {newBounds})) {
        other.setGeometry(QRect(
            QPoint(newBounds.x() + newBounds.width(), other.geometry().y()), other.geometry().size()
        ));
        moved = true;
      }
    }
    if (!moved)
      break;
  }
  return true;
}

void ServerConfig::setConfigFile(const QString &configFile) const
{
  Settings::setValue(Settings::Server::ExternalConfigFile, configFile);
}

void ServerConfig::setUseExternalConfig(bool useExternalConfig) const
{
  Settings::setValue(Settings::Server::ExternalConfig, useExternalConfig);
}

bool ServerConfig::findScreenName(const QString &name, int &index)
{
  bool found = false;
  for (int i = 0; i < screens().size(); i++) {
    if (!screens()[i].isNull() && screens()[i].name().compare(name) == 0) {
      index = i;
      found = true;
      break;
    }
  }
  return found;
}

bool ServerConfig::fixNoServer(const QString &name, int &index)
{
  bool fixed = false;
  if (screens()[serverDefaultIndex].isNull()) {
    m_Screens[serverDefaultIndex].setName(name);
    m_Screens[serverDefaultIndex].markAsServer();
    index = serverDefaultIndex;
    fixed = true;
  }

  return fixed;
}

QSettingsProxy &ServerConfig::settings()
{
  return Settings::proxy();
}
