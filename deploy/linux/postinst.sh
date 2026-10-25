#!/bin/sh
# SPDX-FileCopyrightText: (C) 2026 Rimora Studio
# SPDX-License-Identifier: MIT
#
# Shared by the deb and rpm packages. Enables the Meshflow user service for every user
# (input capture needs the user's own graphical session, so it cannot be a system service).

if command -v systemctl >/dev/null 2>&1; then
  systemctl --global enable meshflow.service >/dev/null 2>&1 || true

  # Start it right away for users who are already logged in
  if command -v loginctl >/dev/null 2>&1; then
    for uid in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $1}'); do
      user=$(id -nu "$uid" 2>/dev/null) || continue
      runtime="/run/user/$uid"
      [ -d "$runtime" ] || continue
      runuser -u "$user" -- env XDG_RUNTIME_DIR="$runtime" systemctl --user daemon-reload >/dev/null 2>&1 || true
      runuser -u "$user" -- env XDG_RUNTIME_DIR="$runtime" systemctl --user try-restart meshflow.service >/dev/null 2>&1 || true
    done
  fi
fi

if command -v firewall-cmd >/dev/null 2>&1 && firewall-cmd --state >/dev/null 2>&1; then
  firewall-cmd --reload >/dev/null 2>&1 || true
fi

exit 0
