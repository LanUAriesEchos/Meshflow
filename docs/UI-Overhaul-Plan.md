# UI Overhaul Plan

## Problem

The current GUI is Qt Widgets (~12K lines, 8 dialogs). It looks dated, the dev loop is compile-restart with no hot reload, and modern UI patterns (animations, fluid layouts, dark mode transitions) require fighting the toolkit.

## Current State (audited 2026-09-09)

### GUI surface
- `src/lib/gui/` — 88 cpp/h files, ~7,858 lines (non-IPC)
- 8 `.ui` dialogs, ~3,975 lines:
  - `MainWindow.ui` (523) — main app window, tray, log dock, status bar
  - `ServerConfigDialog.ui` (926) — screen layout drag-and-drop grid (signature feature)
  - `SettingsDialog.ui` (871) — app settings
  - `ScreenSettingsDialog.ui` (766) — per-screen config
  - `AboutDialog.ui` (448) — about/version info
  - `ActionDialog.ui` (188) — hotkey/action config
  - `ClientConfigDialog.ui` (171) — client config
  - `HotkeyDialog.ui` (81) — hotkey editor

### IPC bridge (already decoupled)
The GUI talks to the C++ core/daemon over Windows named pipes using newline-delimited `key=value` text. This is the key enabler — the core does not change regardless of UI choice.

**Socket names:** `meshflow-daemon`, `meshflow-core` (named pipes)

**GUI → Daemon commands:**
- `start`, `stop`, `clearSettings`
- `logLevel=<level>`, `configFile=<path>`, `logPath`

**GUI → Core commands:**
- `stop`

**Daemon/Core → GUI events:**
- `hello=<version>`, `bye`, `error=<detail>`, `versionMismatch`
- `logPath=<path>`, arbitrary `command=args` from core

**Total bridge code:** 346 lines (GUI-side IPC client). The core-side IPC server (404 lines) stays untouched.

### Toolchain (this machine)
- Qt 6.8.3 at `C:\Qt\6.8.3\msvc2022_64`
- vcpkg at `C:\dev\vcpkg` (OpenSSL 3.4.1 cached)
- VS 2026 / MSVC 14.51 / CMake 4.4
- Node v24.14.0, pnpm, bun installed
- WebView2 runtime v152 installed
- **Rust/cargo: NOT installed** (needed for Tauri)
- **Tauri CLI: NOT installed**

## Options Considered

### Option A: Tauri (web UI) — RECOMMENDED
Rewrite the GUI as a web frontend (React/Svelte + Vite), bridge to the existing C++ core via Rust/Tauri commands over the existing IPC named pipes.

**Pros:**
- Full web dev loop (Vite HMR, hot reload, devtools)
- Access to the entire web UI ecosystem (Tailwind, shadcn/Radix, framer-motion, etc.)
- Modern UI is the default, not a fight
- WebView2 preinstalled on Windows 10+
- IPC seam already exists — the hard part is done
- C++ core/daemon stays 100% unchanged

**Cons:**
- ~12K lines of UI to rewrite (estimated 4-6K lines of React/Svelte)
- Adds Rust (bridge) + TS (UI) to the stack
- Screen layout drag-and-drop grid needs reimplementation (dnd-kit or similar)
- System tray needs Tauri platform plugin

**Effort estimate:** Medium. The IPC protocol is simple enough that the Rust bridge is ~300 lines. The UI is mostly forms + one drag-and-drop grid.

### Option B: QML rewrite
Rewrite the GUI in Qt's declarative QML language. Stays within Qt ecosystem, gets live reload via `qmlscene`.

**Pros:**
- GPU-accelerated, animation-friendly
- Live reload for UI (logic changes still need recompile)
- Stays in one toolkit (Qt), no new languages

**Cons:**
- Small component ecosystem compared to web
- Still Qt-land, not as modern as web
- Same rewrite effort (~12K lines) but worse ecosystem payoff

### Option C: Stay Qt Widgets + speed up build
Install Ninja + ccache, keep Qt Widgets, accept the compile-restart loop.

**Pros:**
- Zero rewrite, zero risk
- Incremental builds drop to a few seconds

**Cons:**
- No hot reload
- Still fighting Qt Widgets for modern UI
- Doesn't solve the core motivation (modern UI)

## Decision

**Chosen: Option A (Tauri)** — pending confirmation after PC reset.

Rationale: the IPC seam already exists, modern UI is a stated goal, and the web ecosystem is the strongest path to a premium look. The dev loop benefit (HMR) is a side effect of the primary goal.

## Migration Plan

### Phase 0: Prerequisites (post PC reset)
- [ ] Install Rust via `rustup`
- [ ] Install Tauri CLI (`cargo install tauri-cli` or `npm install -g @tauri-apps/cli`)
- [ ] Scaffold Tauri project in `Meshflow-rebrand/ui/` (or separate repo)
- [ ] Verify WebView2 runtime available (preinstalled on Win10+)

### Phase 1: Rust bridge (validate the seam)
- [ ] Implement named pipe client in Rust (connect to `meshflow-daemon` and `meshflow-core`)
- [ ] Map IPC commands to Tauri commands:
  - `start_core`, `stop_core`, `start_daemon`, `stop_daemon`
  - `set_log_level`, `set_config_file`, `get_log_path`, `clear_settings`
- [ ] Map IPC events to Tauri events:
  - `connection_state`, `log_path`, `server_shutdown`, `version_mismatch`
- [ ] Validate: run Tauri shell against the existing built daemon, confirm commands work

### Phase 2: Core UI (get to feature parity)
- [ ] MainWindow shell — sidebar/nav, log panel, status bar, start/stop controls
- [ ] SettingsDialog — general settings form
- [ ] ServerConfigDialog — server mode config
- [ ] ClientConfigDialog — client mode config
- [ ] Screen layout grid — drag-and-drop screen arrangement (dnd-kit)
  - This is the hardest single piece. Plan a dedicated spike.
- [ ] System tray integration (Tauri tray plugin)
- [ ] About dialog

### Phase 3: Polish
- [ ] Dark mode / theme system
- [ ] Animations and transitions
- [ ] Responsive layout
- [ ] Icon system (Lucide or custom SVG)
- [ ] Empty states, loading states, error states
- [ ] Accessibility (keyboard nav, ARIA, focus management)

### Phase 4: Integration & shipping
- [ ] Build pipeline: Tauri builds web + Rust, CMake builds C++ core
- [ ] Packaging: bundle Tauri app + C++ daemon into single installer
- [ ] Auto-start on login
- [ ] Auto-update (Tauri updater plugin)
- [ ] Replace old Qt GUI in builds (or keep as fallback during transition)

## What does NOT change
- `src/lib/meshflow/` — core protocol, IPC server, input capture, network
- `src/apps/meshflow-core/` — core binary
- `src/apps/meshflow-daemon/` — daemon binary
- `src/lib/{base,arch,net,io,mt,platform,server,client,common}/` — all internals
- `deploy/` — packaging (may need updates for Tauri bundling)
- IPC protocol — the named pipe text protocol stays as-is

## Open Questions
- [ ] Separate repo for the Tauri UI, or monorepo subfolder (`Meshflow-rebrand/ui/`)?
- [ ] React vs Svelte for the frontend?
- [ ] Keep the old Qt GUI as a fallback during transition, or hard switch?
- [ ] Does the screen layout grid need to match the current Qt behavior exactly, or can we redesign it?
- [ ] PC reset timeline — should we write a setup script for the fresh machine?
