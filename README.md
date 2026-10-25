# Meshflow

**One keyboard. One mouse. Every machine on your desk.**

Meshflow lets you share a single keyboard, mouse, or trackpad across multiple computers. Move your cursor off the edge of one screen and it appears on the next. Copy and paste between systems. All traffic encrypted with TLS. No hardware, no KVM switch, no cloud account.

[![Download](https://img.shields.io/github/v/release/RimoraStudio/Aether?label=Download&color=7c3aed)](https://rimorastudio.github.io/Aether/download.html) [![License](https://img.shields.io/github/license/RimoraStudio/Aether?label=License&color=blue)](LICENSE) [![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-7c3aed)](https://rimorastudio.github.io/Aether/download.html) [![PRs welcome](https://img.shields.io/badge/PRs-welcome-brightgreen)](https://github.com/RimoraStudio/Aether/pulls)

[Website](https://rimorastudio.github.io/Aether/) | [Download](https://rimorastudio.github.io/Aether/download.html) | [Docs](https://rimorastudio.github.io/Aether/docs.html) | [Releases](https://github.com/RimoraStudio/Aether/releases)

---

## Table of contents

- [Why Meshflow](#why-meshflow)
- [Quick start](#quick-start)
- [Download and install](#download-and-install)
- [Getting started](#getting-started)
- [Features](#features)
- [System requirements](#system-requirements)
- [Build from source](#build-from-source)
- [Documentation](#documentation)
- [Changelog](#changelog)
- [Roadmap](#roadmap)
- [Security](#security)
- [License](#license)
- [Credits](#credits)

---

## Why Meshflow

- **No KVM switch.** Software-only. Install on each machine and connect over your LAN.
- **No cloud.** Direct peer-to-peer. Nothing leaves your network.
- **No subscription.** Free and open source under GPL-2.0 with OpenSSL exception.
- **Compatible.** Works with existing Synergy and Barrier peers.

## Quick start

1. Download Meshflow from the [releases page](https://github.com/RimoraStudio/Aether/releases).
2. Install on the machine with the keyboard and mouse you want to share. Set it as **Server**.
3. Install on the other machines. Set them as **Clients**.
4. On the server, arrange the screen grid to match your physical desk layout.
5. Start the server. Connect from the clients.
6. Move your cursor to the edge of the screen. It appears on the next machine.

That's it. No accounts, no configuration files, no network setup.

## Download and install

Prebuilt installers and packages are available on the [releases page](https://github.com/RimoraStudio/Aether/releases) or from the [download page](https://rimorastudio.github.io/Aether/download.html).

### Windows

1. Download `meshflow-<version>-win-x64.msi` (recommended) or the `.exe` bootstrapper.
2. Run the installer. Administrator privileges are required.
3. Launch **Meshflow** from the Start menu.
4. On the first PC choose **Server**, on the others choose **Client**.
5. Arrange your screens in the GUI to match your physical layout.
6. Start the server.

A portable `.7z` archive is also available if you prefer not to install.

### Linux

- **Debian, Ubuntu, Mint:** download the `.deb` and run `sudo apt install ./meshflow-*.deb`
- **Fedora, openSUSE:** download the `.rpm` and run `sudo dnf install meshflow-*.rpm` or `sudo zypper install meshflow-*.rpm`
- **Other distributions:** extract the `.tar.gz` archive and run the binary directly

Meshflow supports both X11 and Wayland. The correct backend is selected automatically based on your session type.

### macOS

macOS support is planned. See the [roadmap](#roadmap) for details.

## Getting started

After installing, you need at least two machines:

1. Pick one machine as the **server** (the one with the keyboard and mouse you want to share).
2. Install Meshflow on the other machines and set them as **clients**.
3. On the server, open Meshflow and arrange the screen grid to match your physical desk layout.
4. Enter the client computer names in the grid to match their hostnames.
5. Start the server on the main machine and connect from the clients.
6. Move your cursor to the edge of the server screen and it appears on the client.

See the [help guide](https://rimorastudio.github.io/Aether/helpmain.html) for detailed setup instructions.

## Features

### Input sharing
- One keyboard, mouse, or trackpad controls all machines
- Cursor moves seamlessly across screens by dragging to the edge
- Hotkeys for jumping to a specific screen
- Configurable screen layout with drag-and-drop grid

### Cross-platform
- Windows 10 or later (x64)
- Linux with X11 or Wayland (x64)
- macOS support planned
- Same protocol works with Synergy and Barrier peers

### Clipboard sharing
- Copy text between machines
- Copy images between machines
- Synchronized clipboard state across all connected systems

### Security
- TLS encrypted network traffic between server and clients
- Fingerprint-based peer verification
- Configurable firewall rules on Windows

### Quality of life
- System tray integration with quick start/stop
- Auto-start on login (optional)
- Version checker with update notification
- Multi-language UI (English, Italian, Spanish, Japanese, Korean, Russian, Chinese)

## System requirements

- Windows 10 or later, or a modern Linux distribution with X11 or Wayland
- All machines on the same local network
- Port 24800 accessible (TCP) between machines
- Qt 6.7+ and OpenSSL 3.0+ (bundled in prebuilt packages)

## Build from source

See the [build guide](https://rimorastudio.github.io/Aether/build.html) for the complete instructions. Quick start:

```bash
cmake -S . -B build
cmake --build build
```

Requirements: CMake 3.24+, Qt 6.7+, OpenSSL 3.0+. Most other dependencies are fetched automatically by CMake.

## Documentation

- [Configuration](https://rimorastudio.github.io/Aether/configuration.html)
- [Auto-update](docs/AutoUpdate.md)
- [Help guide](https://rimorastudio.github.io/Aether/helpmain.html)
- [Build guide](https://rimorastudio.github.io/Aether/build.html)
- [Protocol reference](https://rimorastudio.github.io/Aether/protocol-reference.html)
- [Security policy](https://rimorastudio.github.io/Aether/security.html)
- [Issue reporting](https://rimorastudio.github.io/Aether/issues.html)
- [About](https://rimorastudio.github.io/Aether/about.html)

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for the full version history. Each release is linked to its GitHub tag.

## Roadmap

Meshflow is evolving from a keyboard and mouse sharing tool into a **Unified Distributed Workspace**: multiple computers cooperating as one workspace, with each device contributing its own CPU, GPU, memory, and display.

- **Phase 1:** Strengthen the foundation (auto-updater, network resilience, clipboard modernization, macOS, dark mode, mDNS discovery)
- **Phase 2:** Meshflow Display (virtual display streaming, true extended desktop)
- **Phase 3:** Hybrid Workspace (multi-display, file drag and drop, audio streaming, mobile companion)
- **Phase 4:** Remote Application Research (remote execution, resource-aware placement)
- **Phase 5:** Distributed Workspace (unified resource model, workload scheduling)

See the [full roadmap](https://rimorastudio.github.io/Aether/about.html#roadmap) and the [vision document](docs/dev/vision.md) for details.

## Security

Security issues should follow the policy in [docs/Security.md](docs/Security.md).

## License

Meshflow is distributed under GPL-2.0 with an OpenSSL exception. Source code is licensed under GPL-2.0-only with OpenSSL exception, or as noted per file. See `LICENSE` and `LICENSES/` for details.

## Credits

Meshflow is a fork of the Synergy codebase, rebranded and maintained by Rimora Studio (developer: Xeleste). The original Synergy and Barrier projects deserve credit for the protocol and core implementation.
