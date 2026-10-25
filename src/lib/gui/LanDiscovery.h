/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QHostAddress>
#include <QMap>
#include <QObject>
#include <QTimer>

class QUdpSocket;

namespace meshflow::gui {

// LAN discovery: servers announce their screen name over UDP broadcast,
// clients listen and resolve names to the currently advertised address.
// This only provides addressing; authentication stays with TLS fingerprints.
class LanDiscovery : public QObject
{
  Q_OBJECT

public:
  explicit LanDiscovery(QObject *parent = nullptr);

  void startListening();
  void stopListening();

  void startAnnounce(const QString &screenName, quint16 corePort);
  void stopAnnounce();

  // Latest advertised address for a server name, empty if unknown.
  QString addressFor(const QString &screenName) const;
  bool knows(const QString &screenName) const;

Q_SIGNALS:
  void serverFound(const QString &screenName, const QString &address);

private:
  void announce();
  void readDatagrams();

  QUdpSocket *m_socket = nullptr;
  QTimer m_announceTimer;
  QString m_screenName;
  quint16 m_corePort = 0;
  QMap<QString, QString> m_servers;
};

} // namespace meshflow::gui
