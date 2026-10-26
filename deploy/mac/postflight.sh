#!/bin/sh
# SPDX-FileCopyrightText: (C) 2026 Rimora Studio
# SPDX-License-Identifier: MIT
#
# Installer postinstall: registers Meshflow as a login agent so it starts hidden in the menu bar
# for every user. It has to be a login agent (not a system daemon) because input capture needs
# the user's own GUI session and accessibility permission.

APP="/Applications/Meshflow.app"
PLIST="/Library/LaunchAgents/io.github.LanUAriesEchos.Meshflow.plist"

mkdir -p /Library/LaunchAgents
cat > "$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>Label</key>
	<string>io.github.LanUAriesEchos.Meshflow</string>
	<key>ProgramArguments</key>
	<array>
		<string>$APP/Contents/MacOS/meshflow</string>
		<string>--background</string>
	</array>
	<key>RunAtLoad</key>
	<true/>
	<key>KeepAlive</key>
	<dict>
		<key>SuccessfulExit</key>
		<false/>
	</dict>
	<key>LimitLoadToSessionType</key>
	<string>Aqua</string>
	<key>ProcessType</key>
	<string>Interactive</string>
</dict>
</plist>
EOF
chown root:wheel "$PLIST"
chmod 644 "$PLIST"

# Start it now for whoever is logged in at the console
CONSOLE_USER=$(stat -f%Su /dev/console 2>/dev/null)
if [ -n "$CONSOLE_USER" ] && [ "$CONSOLE_USER" != "root" ]; then
  CONSOLE_UID=$(id -u "$CONSOLE_USER")
  launchctl bootout "gui/$CONSOLE_UID" "$PLIST" >/dev/null 2>&1 || true
  launchctl bootstrap "gui/$CONSOLE_UID" "$PLIST" >/dev/null 2>&1 || true
fi

exit 0
