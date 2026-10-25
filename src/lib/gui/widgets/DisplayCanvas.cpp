/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DisplayCanvas.h"

#include "ScreenSetupModel.h"
#include "dialogs/ScreenSettingsDialog.h"
#include "gui/config/DisplayLayout.h"

#include <QContextMenuEvent>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

namespace {
constexpr int kSnapPixels = 10;
constexpr int kRemoveDragMargin = 24;
constexpr int kMargin = 16;
} // namespace

DisplayCanvas::DisplayCanvas(QWidget *parent) : QWidget(parent)
{
  setFocusPolicy(Qt::StrongFocus);
  setAcceptDrops(true);
  setMinimumHeight(180);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setToolTip(
      tr("Drag a monitor to arrange it. Hold Shift to move the whole computer, Ctrl to move without snapping.\n"
         "Arrow keys nudge the selected monitor by 1 pixel (Shift: 10 pixels). Double click for the computer's settings.")
  );
}

void DisplayCanvas::setModel(ScreenSetupModel *model)
{
  if (m_model)
    disconnect(m_model, nullptr, this, nullptr);
  m_model = model;
  m_selected = -1;
  if (m_model)
    connect(m_model, &ScreenSetupModel::screensChanged, this, &DisplayCanvas::refresh);
  refresh();
  Q_EMIT selectionChanged();
}

ScreenList &DisplayCanvas::screens() const
{
  return m_model->m_Screens;
}

QRect DisplayCanvas::slotRect(int slot) const
{
  if (!m_model || slot < 0 || slot >= screens().size())
    return {};

  const Screen &s = screens().at(slot);
  if (s.isNull())
    return {};
  return s.hasGeometry() ? s.geometry() : displaylayout::defaultRectForSlot(slot, m_model->columnCount());
}

QList<QRect> DisplayCanvas::currentRects() const
{
  QList<QRect> rects;
  if (!m_model)
    return rects;

  for (int i = 0; i < screens().size(); i++) {
    if (i == m_dragSlot && m_dragMoved) {
      QRect bounds;
      for (const QRect &m : monitorsFor(i))
        bounds = bounds.united(m);
      rects.append(bounds);
    } else {
      rects.append(slotRect(i));
    }
  }
  return rects;
}

QList<QRect> DisplayCanvas::storedMonitors(int slot) const
{
  const QRect g = slotRect(slot);
  if (g.isNull())
    return {};

  const auto &stored = screens().at(slot).monitors();
  if (stored.isEmpty())
    return {g};

  QList<QRect> result;
  for (const QRect &m : stored)
    result.append(m.translated(g.topLeft()));
  return result;
}

bool DisplayCanvas::commitMonitors(int slot, const QList<QRect> &monitors)
{
  if (monitors.isEmpty())
    return false;

  QRect bounds;
  for (const QRect &m : monitors)
    bounds = bounds.united(m);

  QList<QRect> relative;
  if (monitors.size() > 1) {
    for (const QRect &m : monitors)
      relative.append(m.translated(-bounds.topLeft()));
  }

  screens()[slot].setMonitors(relative);
  screens()[slot].setGeometry(bounds);
  Q_EMIT m_model->screensChanged();
  Q_EMIT geometryEdited();
  return true;
}

int DisplayCanvas::selectedMonitor() const
{
  if (!m_model || m_selected < 0 || m_selected >= screens().size())
    return -1;
  const auto count = storedMonitors(m_selected).size();
  return count == 0 ? -1 : static_cast<int>(std::clamp<qsizetype>(m_selectedMonitor, 0, count - 1));
}

QList<QRect> DisplayCanvas::othersOf(int slot) const
{
  auto rects = currentRects();
  if (slot >= 0 && slot < rects.size())
    rects[slot] = QRect();
  return rects;
}

QRect DisplayCanvas::selectedGeometry() const
{
  const int index = selectedMonitor();
  return index < 0 ? QRect() : storedMonitors(m_selected).at(index);
}

QString DisplayCanvas::selectedName() const
{
  if (!m_model || m_selected < 0 || m_selected >= screens().size())
    return {};
  return screens().at(m_selected).name();
}

int DisplayCanvas::screenCount() const
{
  int count = 0;
  for (const auto &r : currentRects())
    count += r.isNull() ? 0 : 1;
  return count;
}

QList<QRect> DisplayCanvas::monitorsFor(int slot) const
{
  auto monitors = storedMonitors(slot);
  if (monitors.isEmpty() || slot != m_dragSlot || !m_dragMoved)
    return monitors;

  if (m_dragMonitor >= 0 && m_dragMonitor < monitors.size()) {
    monitors[m_dragMonitor] = m_dragRect;
    return monitors;
  }

  const QPoint delta = m_dragRect.topLeft() - slotRect(slot).topLeft();
  for (QRect &m : monitors)
    m.translate(delta);
  return monitors;
}

QList<displaylayout::Computer> DisplayCanvas::currentComputers() const
{
  const auto rects = currentRects();
  QList<displaylayout::Computer> computers;
  for (int i = 0; i < rects.size(); i++)
    computers.append({rects[i], monitorsFor(i)});
  return computers;
}

QList<int> DisplayCanvas::monitorNumbers(int slot) const
{
  if (!m_model || slotRect(slot).isNull())
    return {};

  QList<int> order;
  for (int i = 0; i < screens().size(); i++) {
    if (!screens().at(i).isNull())
      order.append(i);
  }
  std::ranges::stable_sort(order, [this](int a, int b) {
    if (screens().at(a).isServer() != screens().at(b).isServer())
      return screens().at(a).isServer();
    const QRect ra = slotRect(a);
    const QRect rb = slotRect(b);
    return ra.x() != rb.x() ? ra.x() < rb.x() : ra.y() < rb.y();
  });

  int next = 1;
  for (const int s : order) {
    const auto monitors = monitorsFor(s);
    QList<int> idx(monitors.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::ranges::stable_sort(idx, [&](int a, int b) {
      return monitors[a].x() != monitors[b].x() ? monitors[a].x() < monitors[b].x() : monitors[a].y() < monitors[b].y();
    });

    QList<int> numbers(monitors.size());
    for (const int i : idx)
      numbers[i] = next++;
    if (s == slot)
      return numbers;
  }
  return {};
}

int DisplayCanvas::displayNumber(int slot) const
{
  const auto numbers = monitorNumbers(slot);
  return numbers.isEmpty() ? 0 : *std::ranges::min_element(numbers);
}

int DisplayCanvas::monitorCount(int slot) const
{
  return static_cast<int>(monitorsFor(slot).size());
}

bool DisplayCanvas::isServer(int slot) const
{
  return m_model && slot >= 0 && slot < screens().size() && screens().at(slot).isServer();
}

bool DisplayCanvas::isIsolated(int slot) const
{
  return screenCount() > 1 && displaylayout::isolatedScreens(currentComputers()).contains(slot);
}

bool DisplayCanvas::setSelectedMonitorCount(int count)
{
  const int index = selectedMonitor();
  if (index < 0)
    return false;
  count = std::clamp(count, 1, 8);

  auto monitors = storedMonitors(m_selected);
  if (count == monitors.size())
    return true;

  int newSelection = index;
  if (count < monitors.size()) {
    while (monitors.size() > count) {
      monitors.removeAt(newSelection);
      newSelection = std::min(newSelection, static_cast<int>(monitors.size()) - 1);
    }
  } else {
    const QSize unit = monitors.at(index).size();
    while (monitors.size() < count) {
      QRect bounds;
      for (const QRect &m : monitors)
        bounds = bounds.united(m);
      monitors.append(QRect(QPoint(bounds.right() + 1, bounds.y()), unit));
      newSelection = static_cast<int>(monitors.size()) - 1;
    }
  }

  QRect bounds;
  for (const QRect &m : monitors)
    bounds = bounds.united(m);
  if (displaylayout::overlapsAny(bounds, othersOf(m_selected)))
    return false;

  m_selectedMonitor = newSelection;
  return commitMonitors(m_selected, monitors);
}

void DisplayCanvas::refresh()
{
  if (m_selected >= 0 && slotRect(m_selected).isNull())
    m_selected = -1;
  updateTransform();
  update();
  Q_EMIT selectionChanged();
}

void DisplayCanvas::updateTransform()
{
  if (m_frozen)
    return;

  QRect bounds;
  for (const auto &r : currentRects())
    bounds = bounds.united(r);
  if (bounds.isNull())
    bounds = QRect(0, 0, displaylayout::kDefaultWidth, displaylayout::kDefaultHeight);

  // leave room around the layout so that a display can be dragged to any side of it
  const int padX = std::max(bounds.width() / 5, displaylayout::kDefaultWidth / 4);
  const int padY = std::max(bounds.height() / 5, displaylayout::kDefaultHeight / 4);
  const QRect padded = bounds.adjusted(-padX, -padY, padX, padY);

  const qreal availW = std::max(width() - 2 * kMargin, 1);
  const qreal availH = std::max(height() - 2 * kMargin, 1);
  m_scale = std::min(availW / padded.width(), availH / padded.height());
  m_offset = QPointF(
      kMargin + (availW - padded.width() * m_scale) / 2.0 - padded.x() * m_scale,
      kMargin + (availH - padded.height() * m_scale) / 2.0 - padded.y() * m_scale
  );
}

QRectF DisplayCanvas::toView(const QRect &rect) const
{
  return QRectF(
      rect.x() * m_scale + m_offset.x(), rect.y() * m_scale + m_offset.y(), rect.width() * m_scale,
      rect.height() * m_scale
  );
}

QPointF DisplayCanvas::toVirtual(const QPointF &point) const
{
  return QPointF((point.x() - m_offset.x()) / m_scale, (point.y() - m_offset.y()) / m_scale);
}

int DisplayCanvas::monitorAt(int slot, const QPoint &pos) const
{
  const auto monitors = monitorsFor(slot);
  for (int i = static_cast<int>(monitors.size()) - 1; i >= 0; i--) {
    if (toView(monitors[i]).contains(pos))
      return i;
  }
  return -1;
}

int DisplayCanvas::slotAt(const QPoint &pos) const
{
  const int count = static_cast<int>(screens().size());
  // the selected display is drawn last, so it wins
  if (m_selected >= 0 && m_selected < count && monitorAt(m_selected, pos) >= 0)
    return m_selected;
  for (int i = count - 1; i >= 0; i--) {
    if (monitorAt(i, pos) >= 0)
      return i;
  }
  return -1;
}

void DisplayCanvas::select(int slot, int monitor)
{
  if (slot == m_selected && monitor == m_selectedMonitor)
    return;
  m_selected = slot;
  m_selectedMonitor = monitor;
  update();
  Q_EMIT selectionChanged();
}

void DisplayCanvas::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  p.fillRect(rect(), palette().color(QPalette::Base));

  if (!m_model)
    return;

  const auto rects = currentRects();
  const auto isolated = displaylayout::isolatedScreens(currentComputers());
  const QColor accent = palette().color(QPalette::Highlight);
  const QColor text = palette().color(QPalette::HighlightedText);

  auto drawScreen = [&](int slot) {
    const QRect &vr = rects[slot];
    if (vr.isNull())
      return;

    const bool selected = slot == m_selected;
    const bool dragging = slot == m_dragSlot && m_dragMoved;
    const auto monitors = monitorsFor(slot);
    const auto numbers = monitorNumbers(slot);
    const bool multi = monitors.size() > 1;

    QColor fill = selected ? accent : accent.darker(170);
    if (dragging && !m_dragValid)
      fill = QColor(0xb0, 0x30, 0x30);

    if (multi) {
      p.setBrush(Qt::NoBrush);
      p.setPen(QPen(selected ? accent.lighter(150) : palette().color(QPalette::Mid), 1, Qt::DotLine));
      p.drawRect(toView(vr).adjusted(-3, -3, 3, 3));
    }

    int labelIndex = 0;
    for (int i = 1; i < numbers.size(); i++) {
      if (numbers[i] < numbers[labelIndex])
        labelIndex = i;
    }

    for (int mi = 0; mi < monitors.size(); mi++) {
      const QRect &mr = monitors[mi];
      const QRectF r = toView(mr).adjusted(1, 1, -1, -1);
      const bool activeMonitor = selected && (!multi || mi == selectedMonitor());

      p.setPen(QPen(activeMonitor ? accent.lighter(150) : palette().color(QPalette::Mid), 2));
      p.setBrush(selected && multi && !activeMonitor && !(dragging && !m_dragValid) ? accent.darker(125) : fill);
      p.drawRoundedRect(r, 3, 3);

      QFont small = font();
      small.setPixelSize(std::clamp(static_cast<int>(r.height() * 0.14), 8, 13));
      const QFontMetrics fm(small);
      const bool showLabel = r.width() > 60 && r.height() > fm.height() * 5;

      p.setPen(text);
      QFont numberFont = font();
      numberFont.setPixelSize(std::clamp(static_cast<int>(r.height() * (showLabel ? 0.34 : 0.5)), 10, 72));
      p.setFont(numberFont);
      const QRectF numberRect = showLabel ? r.adjusted(0, 0, 0, -fm.height() * 2 - 6) : r;
      p.drawText(numberRect, Qt::AlignCenter, QString::number(numbers[mi]));

      p.setFont(small);
      const QRectF labelRect(r.left() + 4, r.bottom() - fm.height() * 2 - 6, r.width() - 8, fm.height() * 2);
      if (showLabel) {
        const QString name = mi == labelIndex ? screens().at(slot).name() : QString();
        const auto elided = fm.elidedText(name, Qt::ElideMiddle, static_cast<int>(labelRect.width()));
        const auto size = QStringLiteral("%1 \u00d7 %2").arg(mr.width()).arg(mr.height());
        p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, elided + QLatin1Char('\n') + size);
      }

      if (screens().at(slot).isServer()) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 150));
        p.drawRect(QRectF(r.left() + r.width() * 0.2, r.bottom() - 4, r.width() * 0.6, 3));
      }
    }

    if (isolated.contains(slot) && !dragging) {
      p.setBrush(Qt::NoBrush);
      p.setPen(QPen(QColor(0xe0, 0x50, 0x50), 2, Qt::DashLine));
      p.drawRoundedRect(toView(vr).adjusted(-4, -4, 4, 4), 4, 4);
    }
  };

  for (int i = 0; i < rects.size(); i++) {
    if (i != m_selected)
      drawScreen(i);
  }
  if (m_selected >= 0 && m_selected < rects.size())
    drawScreen(m_selected);
}

void DisplayCanvas::mousePressEvent(QMouseEvent *event)
{
  setFocus();
  if (event->button() != Qt::LeftButton || !m_model)
    return;

  const int slot = slotAt(event->pos());
  select(slot, slot < 0 ? -1 : monitorAt(slot, event->pos()));
  if (slot < 0)
    return;

  m_dragSlot = slot;
  m_dragStart = event->pos();
  m_dragMonitor = -1;
  const auto monitors = storedMonitors(slot);
  if (monitors.size() > 1 && !(event->modifiers() & Qt::ShiftModifier))
    m_dragMonitor = selectedMonitor();
  m_dragOrigin = m_dragMonitor >= 0 ? monitors.at(m_dragMonitor) : slotRect(slot);
  m_dragRect = m_dragOrigin;
  m_dragValid = true;
  m_dragMoved = false;
}

void DisplayCanvas::mouseMoveEvent(QMouseEvent *event)
{
  if (m_dragSlot < 0 || !(event->buttons() & Qt::LeftButton))
    return;

  if (!m_dragMoved) {
    if ((event->pos() - m_dragStart).manhattanLength() < 4)
      return;
    m_dragMoved = true;
    m_frozen = true;
  }

  if (!rect().adjusted(-kRemoveDragMargin, -kRemoveDragMargin, kRemoveDragMargin, kRemoveDragMargin)
           .contains(event->pos())) {
    startRemovalDrag();
    return;
  }

  const QPointF delta = QPointF(event->pos() - m_dragStart) / m_scale;
  QRect moved = m_dragOrigin.translated(qRound(delta.x()), qRound(delta.y()));
  auto others = othersOf(m_dragSlot);
  QList<QRect> siblings;
  if (m_dragMonitor >= 0) {
    siblings = storedMonitors(m_dragSlot);
    siblings[m_dragMonitor] = QRect();
  }

  if (m_snap && !(event->modifiers() & Qt::ControlModifier))
    moved = displaylayout::snap(moved, others + siblings, std::max(1, qRound(kSnapPixels / m_scale)));

  m_dragRect = moved;
  if (m_dragMonitor >= 0) {
    auto monitors = siblings;
    monitors[m_dragMonitor] = moved;
    QRect bounds;
    for (const QRect &m : monitors)
      bounds = bounds.united(m);
    m_dragValid = !displaylayout::overlapsAny(bounds, others) && !displaylayout::overlapsAny(moved, siblings);
  } else {
    m_dragValid = !displaylayout::overlapsAny(moved, others);
  }
  update();
}

void DisplayCanvas::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
    endDrag(true);
}

void DisplayCanvas::endDrag(bool commit)
{
  const int slot = m_dragSlot;
  const int monitor = m_dragMonitor;
  const bool moved = m_dragMoved;
  const bool valid = m_dragValid;
  const QRect target = m_dragRect;
  auto monitors = monitor >= 0 && slot >= 0 ? storedMonitors(slot) : QList<QRect>();

  m_dragSlot = -1;
  m_dragMonitor = -1;
  m_dragMoved = false;
  m_frozen = false;

  if (slot >= 0 && moved && commit && valid && target != m_dragOrigin) {
    if (monitor >= 0 && monitor < monitors.size()) {
      monitors[monitor] = target;
      commitMonitors(slot, monitors);
    } else {
      screens()[slot].setGeometry(target);
      Q_EMIT m_model->screensChanged();
      Q_EMIT geometryEdited();
    }
    return;
  }
  refresh();
}

void DisplayCanvas::startRemovalDrag()
{
  const int slot = m_dragSlot;
  if (slot < 0)
    return;

  QByteArray data;
  QDataStream stream(&data, QIODevice::WriteOnly);
  stream << m_selected << slot << screens().at(slot);

  auto *mime = new QMimeData;
  mime->setData(ScreenSetupModel::mimeType(), data);

  auto *drag = new QDrag(this);
  drag->setMimeData(mime);
  drag->setPixmap(screens().at(slot).pixmap());
  drag->setHotSpot(QPoint(screens().at(slot).pixmap().width() / 2, screens().at(slot).pixmap().height() / 2));

  endDrag(false);
  if (drag->exec(Qt::MoveAction, Qt::MoveAction) == Qt::MoveAction) {
    screens()[slot] = Screen();
    m_selected = -1;
    Q_EMIT m_model->screensChanged();
    Q_EMIT screenRemoved();
  }
}

void DisplayCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (event->button() != Qt::LeftButton)
    return;
  endDrag(false);
  if (slotAt(event->pos()) >= 0)
    editSelected();
}

void DisplayCanvas::editSelected()
{
  if (!m_model || m_selected < 0 || slotRect(m_selected).isNull())
    return;

  ScreenSettingsDialog dlg(this, &screens()[m_selected], &screens());
  dlg.exec();
  Q_EMIT m_model->screensChanged();
}

void DisplayCanvas::removeSelected()
{
  if (!m_model || m_selected < 0 || slotRect(m_selected).isNull())
    return;

  screens()[m_selected] = Screen();
  m_selected = -1;
  Q_EMIT m_model->screensChanged();
  Q_EMIT screenRemoved();
}

bool DisplayCanvas::setSelectedGeometry(const QRect &geometry)
{
  const int index = selectedMonitor();
  if (index < 0)
    return false;
  if (geometry.width() < displaylayout::kMinSize || geometry.height() < displaylayout::kMinSize)
    return false;

  auto monitors = storedMonitors(m_selected);
  if (monitors.at(index) == geometry)
    return true;

  monitors[index] = geometry;
  QRect bounds;
  for (const QRect &m : monitors)
    bounds = bounds.united(m);
  if (displaylayout::overlapsAny(bounds, othersOf(m_selected)))
    return false;

  auto siblings = monitors;
  siblings[index] = QRect();
  if (displaylayout::overlapsAny(geometry, siblings))
    return false;

  return commitMonitors(m_selected, monitors);
}

void DisplayCanvas::keyPressEvent(QKeyEvent *event)
{
  if (m_selected < 0) {
    QWidget::keyPressEvent(event);
    return;
  }

  const int step = (event->modifiers() & Qt::ShiftModifier) ? 10 : 1;
  QPoint d;
  switch (event->key()) {
  case Qt::Key_Left:
    d = {-step, 0};
    break;
  case Qt::Key_Right:
    d = {step, 0};
    break;
  case Qt::Key_Up:
    d = {0, -step};
    break;
  case Qt::Key_Down:
    d = {0, step};
    break;
  case Qt::Key_Delete:
    removeSelected();
    return;
  case Qt::Key_Return:
  case Qt::Key_Enter:
    editSelected();
    return;
  default:
    QWidget::keyPressEvent(event);
    return;
  }
  setSelectedGeometry(selectedGeometry().translated(d));
}

void DisplayCanvas::contextMenuEvent(QContextMenuEvent *event)
{
  const int slot = slotAt(event->pos());
  if (slot < 0)
    return;
  select(slot, monitorAt(slot, event->pos()));

  QMenu menu(this);
  menu.addAction(tr("Settings..."), this, &DisplayCanvas::editSelected);
  menu.addAction(tr("Remove"), this, &DisplayCanvas::removeSelected);
  menu.exec(event->globalPos());
}

void DisplayCanvas::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
  updateTransform();
}

void DisplayCanvas::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->source() != this && event->mimeData()->hasFormat(ScreenSetupModel::mimeType()))
    event->acceptProposedAction();
  else
    event->ignore();
}

void DisplayCanvas::dragMoveEvent(QDragMoveEvent *event)
{
  if (event->source() != this && event->mimeData()->hasFormat(ScreenSetupModel::mimeType()))
    event->acceptProposedAction();
  else
    event->ignore();
}

QRect DisplayCanvas::findFreeRect(const QRect &wanted) const
{
  const auto others = currentRects();
  const QRect snapped = m_snap ? displaylayout::snap(wanted, others, std::max(1, qRound(kSnapPixels / m_scale))) : wanted;
  if (!displaylayout::overlapsAny(snapped, others))
    return snapped;

  QRect bounds;
  for (const auto &r : others)
    bounds = bounds.united(r);
  return QRect(bounds.x() + bounds.width(), bounds.y(), wanted.width(), wanted.height());
}

void DisplayCanvas::dropEvent(QDropEvent *event)
{
  if (!m_model || !event->mimeData()->hasFormat(ScreenSetupModel::mimeType())) {
    event->ignore();
    return;
  }

  int slot = -1;
  for (int i = 0; i < screens().size(); i++) {
    if (screens().at(i).isNull()) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    event->ignore();
    return;
  }

  QByteArray data = event->mimeData()->data(ScreenSetupModel::mimeType());
  QDataStream stream(&data, QIODevice::ReadOnly);
  int column = -1;
  int row = -1;
  Screen dropped;
  stream >> column >> row >> dropped;

  const QPointF center = toVirtual(event->position());
  const QRect wanted(
      qRound(center.x()) - displaylayout::kDefaultWidth / 2, qRound(center.y()) - displaylayout::kDefaultHeight / 2,
      displaylayout::kDefaultWidth, displaylayout::kDefaultHeight
  );
  dropped.setGeometry(findFreeRect(wanted));
  screens()[slot] = dropped;
  m_selected = slot;

  event->acceptProposedAction();
  Q_EMIT m_model->screensChanged();
  Q_EMIT geometryEdited();
}
