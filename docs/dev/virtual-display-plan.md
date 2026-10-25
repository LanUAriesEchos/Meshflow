# Meshflow Virtual Display Feature Plan

## Overview

Allow a second machine on the LAN to act as a virtual monitor for the main machine.
No HDMI cable. The main machine creates a virtual display, streams its content over
the network, and the sub machine renders it fullscreen. Input from the sub machine
flows back to the main machine.

## Current Architecture

Meshflow currently shares only input (keyboard, mouse, clipboard) using the
Synergy/Barrier wire protocol over TCP port 24800 with TLS encryption.

```
Main machine (server)          Sub machine (client)
  Platform hooks capture          Receives protocol messages
  input events                    Injects events via platform API
  -> Protocol messages            (SendInput, XTest, CGEvent)
  -> TLS socket
  -> TCP 24800
```

Key directories:
- `src/lib/server/` - Server logic, client proxy, connection handling
- `src/lib/client/` - Client logic, server proxy
- `src/lib/net/` - TCP, TLS, socket multiplexer
- `src/lib/platform/` - Platform-specific input capture and injection
- `src/lib/gui/` - Qt6 GUI, main window, dialogs, widgets
- `src/lib/meshflow/` - Protocol types, app logic, screen abstraction
- `src/apps/meshflow-core/` - Core daemon executable
- `src/apps/meshflow-gui/` - GUI executable

## Proposed Architecture

### Phase 1: Virtual Display Driver Integration (Windows)

Bundle or detect the open source Virtual Display Driver (VDD) from
https://github.com/VirtualDrivers/Virtual-Display-Driver

The VDD is an IddCx-based user-mode driver that creates a virtual monitor
in Windows without physical hardware. It is MIT licensed and signed.

**Server-side changes:**
- New module: `src/lib/display/DisplayManager.h/cpp`
  - Detect if VDD is installed
  - Create/destroy virtual displays on demand
  - Configure resolution and refresh rate
  - Communicate with VDD via IOCTL

- New module: `src/lib/display/ScreenCapture.h/cpp`
  - Capture the virtual display's content using DXGI Desktop Duplication
  - Provide frames as D3D11 textures or raw bitmaps
  - Frame rate target: 30-60 FPS

**Files to create:**
```
src/lib/display/
  DisplayManager.h        # Virtual display lifecycle
  DisplayManager.cpp
  ScreenCapture.h         # DXGI capture interface
  ScreenCapture.cpp
  DisplayStream.h         # Stream management
  DisplayStream.cpp
  CMakeLists.txt          # Build config for display module
```

**Files to modify:**
```
src/lib/meshflow/ServerApp.h/cpp     # Initialize display manager
src/lib/gui/MainWindow.h/cpp      # UI for display management
CMakeLists.txt                    # Add display module
```

### Phase 2: Video Encoding and Streaming

Encode captured frames and stream them over the network.

**Encoding options (pick one):**
1. FFmpeg (libavcodec) - cross-platform, software + hardware encoding
2. GStreamer - pipeline-based, plugin system
3. Windows Media Foundation - Windows-only, hardware accelerated

Recommendation: FFmpeg. It has a C API, supports NVENC/AMF/QSV hardware
encoders, and is already widely used in C++ projects.

**New modules:**
```
src/lib/display/
  VideoEncoder.h         # Encode frames to H.264/H.265
  VideoEncoder.cpp       # FFmpeg wrapper
  DisplayStreamServer.h  # Stream encoded frames over network
  DisplayStreamServer.cpp
```

**Network protocol additions:**
- New message types in `ProtocolTypes.h`:
  - `kMsgDisplayInit` - Start display stream (resolution, format)
  - `kMsgDisplayFrame` - Encoded video frame (chunked)
  - `kMsgDisplayEnd` - Stop display stream
  - `kMsgDisplayInput` - Input event from display client back to server

- Reuse existing `StreamChunker` for large frame chunking
- Reuse existing `SecureSocket` for TLS encryption
- New port or multiplex on existing 24800 connection

**Data flow:**
```
Server:
  VDD creates virtual monitor
  -> DXGI captures virtual display
  -> FFmpeg encodes to H.264
  -> StreamChunker splits into chunks
  -> SecureSocket encrypts
  -> TCP sends to client

Client:
  TCP receives chunks
  -> SecureSocket decrypts
  -> StreamChunker reassembles
  -> FFmpeg decodes H.264
  -> Qt renders in fullscreen window
```

### Phase 3: Display Client (Sub Machine)

Build a fullscreen viewer mode into the client.

**Client-side changes:**
```
src/lib/display/
  DisplayStreamClient.h    # Receive and decode stream
  DisplayStreamClient.cpp
  DisplayViewer.h           # Qt widget for fullscreen rendering
  DisplayViewer.cpp
```

**Files to modify:**
```
src/lib/client/Client.h/cpp       # Add display client mode
src/lib/meshflow/ClientApp.h/cpp     # Initialize display client
src/apps/meshflow-gui/               # Add viewer window
```

The viewer is a borderless Qt window that:
- Opens fullscreen on the client machine
- Renders decoded video frames
- Captures local mouse/keyboard
- Forwards input events back to server via `kMsgDisplayInput`
- Supports windowed mode for testing

### Phase 4: Input Forwarding from Display Client

When the user clicks or types on the display client, those events
must go back to the server so the main machine processes them.

**Reuse existing input infrastructure:**
- Client already captures local input via platform hooks
- Add a new mode: instead of injecting locally, send as
  `kMsgDisplayInput` messages to the server
- Server receives and injects into the virtual display's
  coordinate space

**Files to modify:**
```
src/lib/client/Client.h/cpp           # Input forwarding mode
src/lib/server/Server.h/cpp            # Receive display input
src/lib/meshflow/ProtocolTypes.h         # New message types
```

### Phase 5: GUI Integration

Add display management UI to the Meshflow GUI.

**New dialogs:**
```
src/lib/gui/dialogs/
  DisplayConfigDialog.h     # Configure virtual display
  DisplayConfigDialog.cpp   # Resolution, refresh rate, enable/disable
```

**MainWindow changes:**
- New menu item: "Display > Add Virtual Display"
- New menu item: "Display > Remove Virtual Display"
- Status bar shows active display streams
- Settings persist in existing config system

**Files to modify:**
```
src/lib/gui/MainWindow.h/cpp
src/lib/gui/Settings.h/cpp
```

### Phase 6: VDD Bundling and Installation

Package the VDD driver with Meshflow.

**Options:**
1. Bundle VDD installer in Meshflow's MSI package
2. Detect VDD on first launch, prompt to install
3. Auto-install VDD silently during Meshflow install

**Deployment changes:**
```
deploy/windows/
  vdd/                    # VDD driver files
  installer.nsi           # NSIS installer updates (or WiX)
```

## Dependency Summary

| Dependency | Purpose | License | Required |
|---|---|---|---|
| Virtual Display Driver | Virtual monitor creation | MIT | Phase 1 |
| FFmpeg (libavcodec) | Video encoding/decoding | LGPL 2.1 | Phase 2 |
| DXGI (Windows SDK) | Screen capture | Proprietary | Phase 1 |
| VDD driver | Virtual display | MIT | Phase 1 |

## Protocol Extension Summary

New messages added to the existing wire protocol:

| Message | Direction | Purpose |
|---|---|---|
| `kMsgDisplayInit` | Server -> Client | Start stream, send resolution |
| `kMsgDisplayFrame` | Server -> Client | Encoded video frame |
| `kMsgDisplayEnd` | Server -> Client | Stop stream |
| `kMsgDisplayInput` | Client -> Server | Input from display client |
| `kMsgDisplayReady` | Client -> Server | Client ready for frames |

All new messages use the existing chunked transfer and TLS encryption.

## Phasing and Milestones

### Milestone 1: Proof of Concept (Phase 1 + partial Phase 2)
- VDD detection and virtual display creation
- DXGI screen capture
- Raw frame transmission (no encoding)
- Verify latency and frame rate feasibility

### Milestone 2: Encoded Streaming (Phase 2 + Phase 3)
- FFmpeg H.264 encoding
- Network streaming with chunking
- Client-side decoding and rendering
- End-to-end display streaming working

### Milestone 3: Input Loop (Phase 4)
- Input capture on display client
- Forwarding to server
- Server injection into virtual display
- Full interactive loop working

### Milestone 4: Production (Phase 5 + Phase 6)
- GUI integration
- VDD bundling
- Configuration persistence
- Documentation
- Testing across Windows versions

## Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| VDD driver signing | Blocks installation | VDD is already signed by its maintainers |
| Latency too high | Unusable experience | Use hardware encoding (NVENC), UDP transport option |
| Bandwidth saturation | Network issues | Adaptive bitrate, configurable quality |
| FFmpeg LGPL compliance | Legal | Dynamic linking, separate DLL |
| Multi-platform support | Scope creep | Windows first, Linux/macOS later |

## Out of Scope (for now)

- Linux and macOS virtual display support
- Hardware cursor rendering on virtual display
- HDR support
- Multi-display streaming (one server to many display clients)
- Audio streaming
- Adaptive resolution based on network conditions

## Open Questions

1. Should display streaming use the existing TCP 24800 port or a separate port?
2. Should we use UDP for lower latency video frames?
3. Should VDD be a hard dependency or optional?
4. Should the display client be a separate executable or a mode in meshflow-core?
5. Should we support the existing Synergy/Barrier protocol for display or create a new Meshflow-specific extension?
