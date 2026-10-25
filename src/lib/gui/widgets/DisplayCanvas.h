/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QHash>
#include <QList>
#include <QRect>
#include <QWidget>

#include "gui/config/DisplayLayout.h"

class ScreenSetupModel;
class ScreenList;

/// @brief Free-form canvas that shows the computers as numbered rectangles on a shared virtual desktop,
/// like the Windows display settings page.
class DisplayCanvas : public QWidget
{
  Q_OBJECT

public:
  explicit DisplayCanvas(QWidget *parent = nullptr);

  void setModel(ScreenSetupModel *model);
  ScreenSetupModel *model() const
  {
    return m_model;
  }

  int selectedSlot() const
  {
    return m_selected;
  }
  /// Index of the selected monitor within the selected computer (matches monitorsFor), or -1.
  int selectedMonitor() const;
  /// Geometry of the selected monitor in the shared desktop.
  QRect selectedGeometry() const;
  QString selectedName() const;
  int displayNumber(int slot) const;
  /// Absolute monitor rectangles of a computer.
  QList<QRect> monitorsFor(int slot) const;
  /// Display numbers matching monitorsFor(slot).
  QList<int> monitorNumbers(int slot) const;
  int monitorCount(int slot) const;
  bool isServer(int slot) const;
  bool isIsolated(int slot) const;
  int screenCount() const;

  bool snapEnabled() const
  {
    return m_snap;
  }
  void setSnapEnabled(bool enabled)
  {
    m_snap = enabled;
  }

  /// Applies a new position / size to the selected monitor. Returns false when it would overlap another monitor.
  bool setSelectedGeometry(const QRect &geometry);
  /// Adds monitors to, or removes monitors from, the selected computer. Returns false on overlap.
  bool setSelectedMonitorCount(int count);
  void editSelected();
  void removeSelected();

public Q_SLOTS:
  void refresh();

Q_SIGNALS:
  void selectionChanged();
  void geometryEdited();
  void screenRemoved();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  ScreenList &screens() const;
  QRect slotRect(int slot) const;
  QList<QRect> storedMonitors(int slot) const;
  bool commitMonitors(int slot, const QList<QRect> &monitors);
  QList<QRect> currentRects() const;
  QList<displaylayout::Computer> currentComputers() const;
  QList<QRect> othersOf(int slot) const;
  void updateTransform();
  QRectF toView(const QRect &rect) const;
  QPointF toVirtual(const QPointF &point) const;
  int slotAt(const QPoint &pos) const;
  int monitorAt(int slot, const QPoint &pos) const;
  void select(int slot, int monitor = -1);
  void endDrag(bool commit);
  void startRemovalDrag();
  QRect findFreeRect(const QRect &wanted) const;

  ScreenSetupModel *m_model = nullptr;
  int m_selected = -1;
  int m_selectedMonitor = -1;
  bool m_snap = true;

  qreal m_scale = 0.1;
  QPointF m_offset;
  bool m_frozen = false;

  int m_dragSlot = -1;
  int m_dragMonitor = -1;
  QPoint m_dragStart;
  QRect m_dragOrigin;
  QRect m_dragRect;
  bool m_dragValid = true;
  bool m_dragMoved = false;
};
