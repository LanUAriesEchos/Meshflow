#!/bin/sh
# SPDX-FileCopyrightText: (C) 2026 Rimora Studio
# SPDX-License-Identifier: MIT
#
# Debian: called as "prerm remove|upgrade|...". RPM: $1 is the number of remaining installs, 0 on removal.
case "$1" in
  remove|purge|deconfigure|0) ;;
  *) exit 0 ;;
esac

if command -v systemctl >/dev/null 2>&1; then
  if command -v loginctl >/dev/null 2>&1; then
    for uid in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $1}'); do
      user=$(id -nu "$uid" 2>/dev/null) || continue
      runtime="/run/user/$uid"
      [ -d "$runtime" ] || continue
      runuser -u "$user" -- env XDG_RUNTIME_DIR="$runtime" systemctl --user stop meshflow.service >/dev/null 2>&1 || true
    done
  fi
  systemctl --global disable meshflow.service >/dev/null 2>&1 || true
fi

exit 0
