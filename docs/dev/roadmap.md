# Meshflow Development Roadmap

Centralized plan for all features and improvements. Not published on the public docs site.

---

## Current State (v1.0.0)

### Shipped
- Keyboard and mouse sharing across Windows and Linux
- Clipboard sharing (text and images)
- TLS encrypted network traffic
- Synergy/Barrier protocol compatibility
- Qt6 GUI with screen layout editor
- System tray integration
- Version checker (notification only, no auto-update)
- Multi-language UI (7 languages)
- MSI installer, EXE bootstrapper, 7Z portable (Windows)
- DEB, RPM, tar.gz (Linux)
- GitHub Actions CI/CD with Telegram notifications

### Known Limitations
- No code signing (Windows SmartScreen warning)
- No auto-updater (only version check notification)
- Linux build lacks Wayland input capture (libei 1.3+ required, Ubuntu 24.04 has 1.2.1)
- No macOS support
- GUI is dated (2010s era Qt look)
- No dark mode
- No mDNS discovery (manual IP entry required)

---

## v1.1.0 - Auto-Updater

### Goal
Click update notification, confirm, app downloads and installs the new version automatically.

### Scope
- Download latest installer from GitHub releases
- Verify SHA-256 checksum against sums.txt
- Windows: run MSI silently (`msiexec /i ... /quiet /norestart`)
- Linux DEB: install via pkexec (`apt install -y`)
- Linux RPM: install via pkexec (`dnf install -y`)
- Linux tar.gz: prompt manual extraction
- Restart app after install
- Setting to disable update checks
- Proxy support via HTTPS_PROXY env var

### Files to create
```
src/lib/meshflow/AutoUpdater.h
src/lib/meshflow/AutoUpdater.cpp
src/lib/gui/dialogs/UpdateDialog.h
src/lib/gui/dialogs/UpdateDialog.cpp
```

### Files to modify
```
src/lib/meshflow/VersionChecker.cpp    # Add download + install logic
src/lib/gui/MainWindow.cpp           # Add update dialog
src/lib/gui/Settings.h/cpp           # Add update settings
```

### Dependencies
- Qt Network (already linked)
- JSON parsing (Qt has QJsonDocument)

### Estimated effort
2-3 days

---

## v1.2.0 - Virtual Display (Windows first)

### Goal
Turn a second machine into a virtual monitor for the main machine. No HDMI cable. Main machine creates a virtual display, streams content over LAN, second machine renders it fullscreen. Input flows back to main.

### Architecture
```
Main machine (server)              Second machine (client)
  VDD creates virtual monitor       Fullscreen Qt viewer
  -> DXGI captures display           <- Receives H.264 stream
  -> FFmpeg encodes (NVENC)          <- Qt renders fullscreen
  -> TLS stream over LAN             -> Input sent back
  -> TCP 24800 or UDP                -> kMsgDisplayInput
```

### Phases

**Phase 1: Virtual Display Driver (VDD)**
- Bundle VDD from https://github.com/VirtualDrivers/Virtual-Display-Driver
- Detect if VDD is installed
- Create/destroy virtual displays on demand
- Configure resolution and refresh rate
- Files: `src/lib/display/DisplayManager.h/cpp`

**Phase 2: Screen Capture + Encoding**
- Capture virtual display via DXGI Desktop Duplication
- Encode with FFmpeg (H.264, hardware accelerated via NVENC/AMF/QSV)
- Target 30-60 FPS
- Files: `src/lib/display/ScreenCapture.h/cpp`, `VideoEncoder.h/cpp`

**Phase 3: Network Streaming**
- New protocol messages: kMsgDisplayInit, kMsgDisplayFrame, kMsgDisplayEnd, kMsgDisplayInput, kMsgDisplayReady
- Reuse existing StreamChunker for large frame chunking
- Reuse existing SecureSocket for TLS
- Decision: multiplex on TCP 24800 or separate port
- Files: `src/lib/display/DisplayStreamServer.h/cpp`

**Phase 4: Display Client**
- Fullscreen borderless Qt window on client
- Decode H.264 stream via FFmpeg
- Render frames
- Capture local input, forward as kMsgDisplayInput
- Files: `src/lib/display/DisplayStreamClient.h/cpp`, `DisplayViewer.h/cpp`

**Phase 5: Input Loop**
- Client captures mouse/keyboard
- Sends input events back to server
- Server injects into virtual display coordinate space
- Files: modify `src/lib/client/Client.h/cpp`, `src/lib/server/Server.h/cpp`

**Phase 6: GUI + Bundling**
- "Add Virtual Display" / "Remove Virtual Display" menu items
- Display config dialog (resolution, refresh rate)
- Status bar shows active streams
- Bundle VDD driver in MSI installer
- Files: `src/lib/gui/dialogs/DisplayConfigDialog.h/cpp`, `deploy/windows/vdd/`

### Decisions made
- Use TCP 24800 (multiplex with existing protocol, simpler, TLS already works)
- VDD is optional (prompt to install on first use)
- Display client is a mode in meshflow-gui, not a separate executable
- New Meshflow-specific protocol extension (Synergy/Barrier don't support video)
- FFmpeg for encoding (cross-platform, hardware accel support)
- Windows first, Linux later

### Dependencies
| Dependency | Purpose | License |
|---|---|---|
| Virtual Display Driver | Virtual monitor creation | MIT |
| FFmpeg (libavcodec) | Video encoding/decoding | LGPL 2.1 |
| DXGI (Windows SDK) | Screen capture | Proprietary |

### Risks
- Latency: use hardware encoding, consider UDP fallback
- Bandwidth: adaptive bitrate, configurable quality
- FFmpeg LGPL: dynamic linking, separate DLL
- VDD signing: VDD is already signed by maintainers

### Estimated effort
4-6 weeks (all phases)

---

## v1.2.1 - Clipboard Improvements

### Goal
Modernize clipboard sharing to handle real-world copy/paste scenarios.

### Current state
- Text: works
- HTML: works
- BMP images: works (but most apps copy as PNG, not BMP)
- PNG/JPEG: not supported
- Files: not supported (drag file from Explorer, paste on other machine)
- Rich text with embedded images: not supported

### Scope

**PNG support:**
- Add PNG converter alongside existing BMP converter
- Windows: register `CF_PNG` or use `RegisterClipboardFormat("PNG")`
- Linux: add `image/png` atom to XWindowsClipboard
- Convert PNG <-> BMP internally so both formats work cross-platform
- Files: `src/lib/platform/MSWindowsClipboardPNGConverter.h/cpp`, `src/lib/platform/XWindowsClipboardPNGConverter.h/cpp`

**JPEG support:**
- Add JPEG converter (decode to BMP/PNG for internal transfer)
- Windows: `RegisterClipboardFormat("JFIF")`
- Linux: `image/jpeg` atom
- Files: `src/lib/platform/MSWindowsClipboardJPEGConverter.h/cpp`, `src/lib/platform/XWindowsClipboardJPEGConverter.h/cpp`

**File copy/paste:**
- Detect file clipboard format (Windows: `CF_HDROP`, Linux: `text/uri-list`)
- Serialize file paths as URI list
- Transfer file contents over existing TLS connection (chunked)
- Write to temp dir on receiving machine, set clipboard to local paths
- Progress indicator for large files
- Configurable max file size (default 100MB)
- Files: `src/lib/meshflow/FileTransfer.h/cpp`, `src/lib/platform/MSWindowsClipboardFileConverter.h/cpp`, `src/lib/platform/XWindowsClipboardFileConverter.h/cpp`

### Files to modify
```
src/lib/platform/MSWindowsClipboard.cpp    # Register PNG/JPEG/file converters
src/lib/platform/XWindowsClipboard.cpp     # Add PNG/JPEG/file atoms
src/lib/meshflow/Clipboard.h/cpp              # Add File transfer format
src/lib/meshflow/ProtocolTypes.h              # File transfer message types
```

### Dependencies
- Qt GUI (QImage for PNG/JPEG decode) - already linked
- No new external dependencies

### Estimated effort
1 week

---

## v1.3.0 - UX Improvements

### Network resilience (WiFi lag fix)
- **Problem**: TCP head-of-line blocking causes mouse freezes and keystroke bursts on laggy WiFi
- **UDP transport option**: Add UDP mode for input events (no head-of-line blocking)
  - Mouse moves use UDP (lossy OK, latest position wins)
  - Keystrokes use TCP (must be reliable)
  - Auto-fallback to TCP if UDP blocked
  - Files: `src/lib/net/UdpStream.h/cpp`, `src/lib/meshflow/ProtocolTypes.h`
- **Input coalescing**: Merge rapid mouse events to reduce bandwidth
  - Drop intermediate mouse moves, send only latest position
  - Configurable rate (default 125Hz)
  - Files: `src/lib/server/Server.cpp`, `src/lib/client/Client.cpp`
- **Connection quality indicator**: Show latency in status bar
  - Measure RTT via heartbeat
  - Color: green (<50ms), yellow (50-150ms), red (>150ms)
  - Files: `src/lib/gui/StatusBar.h/cpp`, `src/lib/net/ConnectionMonitor.h/cpp`
- **Local input fallback**: If connection drops, immediately return control to local machine
  - Detect connection loss within 500ms
  - Release keyboard/mouse grab instantly
  - Files: `src/lib/server/Server.cpp`, `src/lib/client/Client.cpp`
- **Adaptive timeout**: Detect lag early and reconnect faster
  - Dynamic timeout based on measured RTT
  - Exponential backoff for reconnect attempts
  - Files: `src/lib/net/SocketMultiplexer.cpp`, `src/lib/meshflow/ConnectionManager.h/cpp`
- **QoS marking**: Tag packets for router priority (DSCP EF)
  - Optional, enabled by default
  - Files: `src/lib/net/TCPSocket.cpp`, `src/lib/net/UDPSocket.cpp`

### Dark mode
- Add theme toggle to GUI
- Qt6 supports dark palettes natively
- Persist preference in settings
- Files: `src/lib/gui/MainWindow.cpp`, new `src/lib/gui/Theme.h/cpp`

### Onboarding wizard
- First-run setup wizard
- Detect if server or client
- Auto-detect screen layout
- Guide through configuration
- Files: `src/lib/gui/dialogs/SetupWizard.h/cpp`

### mDNS discovery
- Auto-find Meshflow machines on LAN
- No manual IP entry
- List available servers in GUI
- Files: `src/lib/net/MdnsDiscovery.h/cpp` (use Qt DNS-SD or Avahi)

### Status dashboard
- Show all connected clients
- Show latency, connection status
- Show clipboard transfer history
- Files: `src/lib/gui/dialogs/StatusDashboard.h/cpp`

### Estimated effort
2-3 weeks

---

## v1.4.0 - Platform Expansion

### macOS support
- Add macOS build job to CI
- Homebrew Qt6, app bundle, DMG
- Requires Apple Developer cert ($99/yr) for notarization
- Without cert: "app is damaged" error on Apple Silicon
- Files: `deploy/macos/`, `.github/workflows/build-release.yml`

### Linux Wayland full support
- Build libei 1.3+ from source in CI (Ubuntu 24.04 has 1.2.1)
- Or use Ubuntu 24.10+ when available on GitHub Actions
- Enables portal-based input capture on Wayland
- Files: `.github/workflows/build-release.yml`, `src/lib/platform/CMakeLists.txt`

### Windows ARM64
- Add windows-arm64 build job
- Qt 6.11.2 supports ARM64
- MSVC supports ARM64 cross-compile
- Files: `.github/workflows/build-release.yml`

### Code signing
- Buy OV cert (~$100/yr) or EV cert (~$300/yr)
- Add signtool step to CI
- Store cert as GitHub secret
- Files: `.github/workflows/build-release.yml`

### Estimated effort
1-2 weeks (per platform)

---

## v2.0.0 - Advanced Features

### File drag and drop
- Drag files from one machine to another
- File transfer over existing TLS connection
- Progress indicator
- Files: `src/lib/meshflow/FileTransfer.h/cpp`

### Audio streaming
- Stream system audio to display client
- Opus codec for low latency
- Sync with video stream

### Multi-display streaming
- One server streams to multiple display clients
- Each client renders a portion or full display
- Load balancing for bandwidth

### Mobile companion
- Android/iOS app as input client
- Use phone touchscreen as trackpad
- Use phone keyboard for text input

### Adaptive bitrate
- Detect network conditions
- Adjust video quality dynamically
- Fallback to lower resolution on congestion

### Estimated effort
3-6 months

---

## CI/CD Backlog

Not versioned, ongoing improvements:

- [ ] Matrix build for Linux (Ubuntu 22.04, 24.04, Debian 12)
- [ ] clang-format check in verify job
- [ ] clang-tidy static analysis
- [ ] REUSE license compliance check
- [ ] Release notes template (.github/release-notes.md)
- [ ] Lower build timeout from 30 to 20 min
- [ ] Linux AppImage output
- [ ] GPG sign Linux packages
- [ ] Code signing for Windows (when cert is purchased)

---

## Priority Order

1. **Ship v1.0.0** (current - waiting on build)
2. **v1.1.0 Auto-updater** (quick win, high user value)
3. **v1.2.0 Virtual Display** (killer feature, differentiator)
4. **v1.2.1 Clipboard improvements** (PNG/JPEG/file transfer)
5. **v1.3.0 UX improvements** (dark mode, mDNS, onboarding)
6. **v1.4.0 Platform expansion** (macOS, Wayland, ARM64, signing)
7. **v2.0.0 Advanced** (file transfer, audio, mobile)
