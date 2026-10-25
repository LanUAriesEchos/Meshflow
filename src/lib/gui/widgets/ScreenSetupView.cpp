/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenSetupView.h"

#include "DisplayCanvas.h"
#include "ScreenSetupModel.h"
#include "gui/config/DisplayLayout.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
QSpinBox *makeSpin(int min, int max, const QString &suffix)
{
  auto *spin = new QSpinBox;
  spin->setRange(min, max);
  spin->setSuffix(suffix);
  // apply when editing finishes instead of on every keystroke
  spin->setKeyboardTracking(false);
  spin->setAlignment(Qt::AlignRight);
  return spin;
}
} // namespace

ScreenSetupView::ScreenSetupView(QWidget *parent)
    : QWidget(parent),
      m_canvas(new DisplayCanvas(this)),
      m_title(new QLabel(this)),
      m_status(new QLabel(this)),
      m_x(makeSpin(-displaylayout::kMaxCoordinate, displaylayout::kMaxCoordinate, tr(" px"))),
      m_y(makeSpin(-displaylayout::kMaxCoordinate, displaylayout::kMaxCoordinate, tr(" px"))),
      m_width(makeSpin(displaylayout::kMinSize, displaylayout::kMaxSize, tr(" px"))),
      m_height(makeSpin(displaylayout::kMinSize, displaylayout::kMaxSize, tr(" px"))),
      m_monitors(makeSpin(1, 8, QString())),
      m_snap(new QCheckBox(tr("Snap to edges"), this)),
      m_settings(new QPushButton(tr("Settings..."), this)),
      m_remove(new QPushButton(tr("Remove"), this)),
      m_detect(new QPushButton(tr("Detect displays"), this)),
      m_undo(new QPushButton(tr("Undo"), this)),
      m_redo(new QPushButton(tr("Redo"), this)),
      m_fit(new QPushButton(tr("Reset view"), this)),
      m_presets(new QToolButton(this)),
      m_presetMenu(new QMenu(this))
{
  m_undo->setToolTip(tr("Undo the last layout change (Ctrl+Z)"));
  m_redo->setToolTip(tr("Redo (Ctrl+Y)"));
  m_fit->setToolTip(tr("Fit the whole layout in view. Mouse wheel zooms, drag empty space to pan."));
  m_presets->setText(tr("Layouts"));
  m_presets->setToolTip(tr("Save the current arrangement and switch between saved layouts"));
  m_presets->setPopupMode(QToolButton::InstantPopup);
  m_presets->setMenu(m_presetMenu);
  m_monitors->setToolTip(tr("Number of monitors this computer has. Select a monitor to position or resize it."));
  m_detect->setToolTip(tr("Reset this PC's monitors to the arrangement Windows reports"));
  m_snap->setChecked(true);
  m_snap->setToolTip(tr("Align displays to each other's edges while dragging. Hold Ctrl to move freely."));
  m_title->setStyleSheet(QStringLiteral("font-weight: bold;"));
  m_status->setWordWrap(true);

  auto *fields = new QGridLayout;
  fields->addWidget(new QLabel(tr("Position X")), 0, 0);
  fields->addWidget(m_x, 0, 1);
  fields->addWidget(new QLabel(tr("Y")), 0, 2);
  fields->addWidget(m_y, 0, 3);
  fields->addWidget(new QLabel(tr("Resolution")), 1, 0);
  fields->addWidget(m_width, 1, 1);
  fields->addWidget(new QLabel(QStringLiteral("\u00d7")), 1, 2, Qt::AlignCenter);
  fields->addWidget(m_height, 1, 3);
  fields->addWidget(new QLabel(tr("Monitors")), 2, 0);
  fields->addWidget(m_monitors, 2, 1);
  fields->setColumnStretch(4, 1);

  auto *buttons = new QVBoxLayout;
  buttons->addWidget(m_settings);
  buttons->addWidget(m_remove);
  buttons->addWidget(m_detect);
  auto *history = new QHBoxLayout;
  history->addWidget(m_undo);
  history->addWidget(m_redo);
  buttons->addLayout(history);
  buttons->addWidget(m_fit);
  buttons->addWidget(m_presets);
  buttons->addWidget(m_snap);
  buttons->addStretch();

  auto *info = new QVBoxLayout;
  info->addWidget(m_title);
  info->addLayout(fields);
  info->addWidget(m_status);
  info->addStretch();

  auto *panel = new QHBoxLayout;
  panel->addLayout(info, 1);
  panel->addLayout(buttons);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_canvas, 1);
  layout->addLayout(panel);

  setFocusProxy(m_canvas);

  connect(m_canvas, &DisplayCanvas::selectionChanged, this, &ScreenSetupView::syncPanel);
  connect(m_canvas, &DisplayCanvas::screenRemoved, this, &ScreenSetupView::screenRemoved);
  connect(m_snap, &QCheckBox::toggled, m_canvas, &DisplayCanvas::setSnapEnabled);
  connect(m_settings, &QPushButton::clicked, m_canvas, &DisplayCanvas::editSelected);
  connect(m_remove, &QPushButton::clicked, m_canvas, &DisplayCanvas::removeSelected);
  connect(m_detect, &QPushButton::clicked, this, &ScreenSetupView::detectRequested);
  connect(m_undo, &QPushButton::clicked, m_canvas, &DisplayCanvas::undo);
  connect(m_redo, &QPushButton::clicked, m_canvas, &DisplayCanvas::redo);
  connect(m_fit, &QPushButton::clicked, m_canvas, &DisplayCanvas::resetView);
  connect(m_canvas, &DisplayCanvas::historyChanged, this, &ScreenSetupView::updateHistoryButtons);
  connect(m_presetMenu, &QMenu::aboutToShow, this, &ScreenSetupView::populatePresetMenu);
  connect(m_monitors, &QSpinBox::valueChanged, this, [this](int count) {
    if (!m_canvas->setSelectedMonitorCount(count)) {
      syncPanel();
      m_status->setText(tr("Displays cannot overlap."));
    }
  });
  for (auto *spin : {m_x, m_y, m_width, m_height})
    connect(spin, &QSpinBox::valueChanged, this, &ScreenSetupView::applyGeometryFromFields);

  syncPanel();
  updateHistoryButtons();
}

void ScreenSetupView::recordUndo()
{
  m_canvas->pushUndo();
}

void ScreenSetupView::clearHistory()
{
  m_canvas->clearHistory();
}

void ScreenSetupView::updateHistoryButtons()
{
  m_undo->setEnabled(m_canvas->canUndo());
  m_redo->setEnabled(m_canvas->canRedo());
}

void ScreenSetupView::populatePresetMenu()
{
  m_presetMenu->clear();
  m_presetMenu->addAction(tr("Save current layout as..."), this, [this] {
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Save layout"), tr("Layout name:"), QLineEdit::Normal, QString(), &ok).trimmed();
    if (ok && !name.isEmpty())
      m_canvas->savePreset(name);
  });

  const auto names = DisplayCanvas::presetNames();
  if (names.isEmpty())
    return;

  m_presetMenu->addSeparator();
  for (const QString &name : names) {
    m_presetMenu->addAction(tr("Load \"%1\"").arg(name), this, [this, name] {
      if (!m_canvas->loadPreset(name))
        QMessageBox::warning(this, tr("Load layout"), tr("This layout does not match the computers currently configured."));
    });
  }
  m_presetMenu->addSeparator();
  auto *remove = m_presetMenu->addMenu(tr("Delete layout"));
  for (const QString &name : names)
    remove->addAction(name, this, [name] { DisplayCanvas::deletePreset(name); });
}

void ScreenSetupView::setModel(ScreenSetupModel *model)
{
  m_canvas->setModel(model);
}

ScreenSetupModel *ScreenSetupView::model() const
{
  return m_canvas->model();
}

void ScreenSetupView::reset()
{
  m_canvas->setModel(m_canvas->model());
}

void ScreenSetupView::syncPanel()
{
  const int slot = m_canvas->selectedSlot();
  const bool hasSelection = slot >= 0;

  for (auto *w : std::initializer_list<QWidget *>{m_x, m_y, m_width, m_height, m_monitors, m_settings, m_remove})
    w->setEnabled(hasSelection);

  if (!hasSelection) {
    m_title->setText(tr("No display selected"));
    m_status->setText(tr("Select a display to change its position and resolution."));
    return;
  }

  const QRect g = m_canvas->selectedGeometry();
  for (auto *spin : {m_x, m_y, m_width, m_height, m_monitors})
    spin->blockSignals(true);
  m_x->setValue(g.x());
  m_y->setValue(g.y());
  m_width->setValue(g.width());
  m_height->setValue(g.height());
  m_monitors->setValue(m_canvas->monitorCount(slot));
  for (auto *spin : {m_x, m_y, m_width, m_height, m_monitors})
    spin->blockSignals(false);

  const int monitor = m_canvas->selectedMonitor();
  QStringList numbers;
  const auto all = m_canvas->monitorNumbers(slot);
  for (const int n : all)
    numbers << QString::number(n);
  const QString shown = all.size() > 1 && monitor >= 0 ? QString::number(all.at(monitor)) : numbers.join(QStringLiteral(", "));
  m_title->setText(tr("%1: %2").arg(shown).arg(m_canvas->selectedName()));
  m_status->setText(
      m_canvas->isIsolated(slot) ? tr("Not connected to any other display. The cursor can only cross on an outer edge of a computer, so place "
         "it so that an outer edge of one of its monitors touches an outer edge of another computer's monitor.")
                                 : QString()
  );
}

void ScreenSetupView::applyGeometryFromFields()
{
  const QRect wanted(m_x->value(), m_y->value(), m_width->value(), m_height->value());
  if (!m_canvas->setSelectedGeometry(wanted)) {
    syncPanel();
    m_status->setText(tr("Displays cannot overlap."));
  }
}
