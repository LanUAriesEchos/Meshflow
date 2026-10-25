/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LanDiscovery.h"

#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QUdpSocket>

namespace meshflow::gui {

namespace {
const quint16 kDiscoveryPort = 24801;
const auto kMagic = QByteArrayLiteral("MESHFLOW_DISCOVER/1");
const auto kAnnounceInterval = 2000;
} // namespace

LanDiscovery::LanDiscovery(QObject *parent) : QObject(parent)
{
  m_socket = new QUdpSocket(this);
  connect(&m_announceTimer, &QTimer::timeout, this, &LanDiscovery::announce);
  connect(m_socket, &QUdpSocket::readyRead, this, &LanDiscovery::readDatagrams);
}

void LanDiscovery::startListening()
{
  if (m_socket->state() == QAbstractSocket::BoundState)
    return;

  if (!m_socket->bind(kDiscoveryPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
    qWarning("lan discovery: failed to bind port %d", kDiscoveryPort);
  }
}

void LanDiscovery::stopListening()
{
  m_socket->close();
}

void LanDiscovery::startAnnounce(const QString &screenName, quint16 corePort)
{
  m_screenName = screenName;
  m_corePort = corePort;
  announce();
  m_announceTimer.start(kAnnounceInterval);
}

void LanDiscovery::stopAnnounce()
{
  m_announceTimer.stop();
}

QString LanDiscovery::addressFor(const QString &screenName) const
{
  return m_servers.value(screenName);
}

bool LanDiscovery::knows(const QString &screenName) const
{
  return m_servers.contains(screenName);
}

void LanDiscovery::announce()
{
  if (m_screenName.isEmpty())
    return;

  const auto payload = kMagic + '|' + m_screenName.toUtf8() + '|' + QByteArray::number(m_corePort);
  for (const auto &iface : QNetworkInterface::allInterfaces()) {
    if (!(iface.flags() & QNetworkInterface::IsUp) || !(iface.flags() & QNetworkInterface::CanBroadcast))
      continue;
    for (const auto &entry : iface.addressEntries()) {
      const auto broadcast = entry.broadcast();
      if (broadcast.isNull() || broadcast.protocol() != QAbstractSocket::IPv4Protocol)
        continue;
      m_socket->writeDatagram(payload, broadcast, kDiscoveryPort);
    }
  }
}

void LanDiscovery::readDatagrams()
{
  while (m_socket->hasPendingDatagrams()) {
    const auto datagram = m_socket->receiveDatagram();
    const auto parts = datagram.data().split('|');
    if (parts.size() != 3 || parts[0] != kMagic)
      continue;

    const auto name = QString::fromUtf8(parts[1]);
    if (name.isEmpty())
      continue;

    // report plain IPv4 rather than the IPv4-mapped IPv6 form (::ffff:a.b.c.d) and drop any IPv6 zone id
    auto sender = datagram.senderAddress();
    sender.setScopeId(QString());
    bool isV4 = false;
    const quint32 v4 = sender.toIPv4Address(&isV4);
    const auto address = isV4 ? QHostAddress(v4).toString() : sender.toString();
    const auto previous = m_servers.value(name);
    m_servers[name] = address;
    if (previous != address) {
      Q_EMIT serverFound(name, address);
    }
  }
}

} // namespace meshflow::gui
