/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QPushButton;
class QSpinBox;
class DisplayCanvas;
class ScreenSetupModel;

/// @brief Display arrangement editor: a canvas plus numeric fields for pixel accurate positioning.
class ScreenSetupView : public QWidget
{
  Q_OBJECT

public:
  explicit ScreenSetupView(QWidget *parent);
  void setModel(ScreenSetupModel *model);
  ScreenSetupModel *model() const;
  void reset();

Q_SIGNALS:
  void screenRemoved();
  void detectRequested();

private:
  void syncPanel();
  void applyGeometryFromFields();

  DisplayCanvas *m_canvas;
  QLabel *m_title;
  QLabel *m_status;
  QSpinBox *m_x;
  QSpinBox *m_y;
  QSpinBox *m_width;
  QSpinBox *m_height;
  QSpinBox *m_monitors;
  QCheckBox *m_snap;
  QPushButton *m_settings;
  QPushButton *m_remove;
  QPushButton *m_detect;
};
