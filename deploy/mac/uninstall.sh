#!/bin/sh
# SPDX-FileCopyrightText: (C) 2026 Rimora Studio
# SPDX-License-Identifier: MIT
#
# Removes Meshflow and its login agent. Run with: sudo sh uninstall.sh

PLIST="/Library/LaunchAgents/io.github.LanUAriesEchos.Meshflow.plist"

for uid in $(ls -n /dev/console 2>/dev/null | awk '{print $3}'); do
  launchctl bootout "gui/$uid" "$PLIST" >/dev/null 2>&1 || true
done
rm -f "$PLIST"
rm -rf /Applications/Meshflow.app
pkgutil --forget io.github.LanUAriesEchos.Meshflow >/dev/null 2>&1 || true
echo "Meshflow removed."
