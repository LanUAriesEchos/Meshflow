# Meshflow: Unified Distributed Workspace

Product vision, architecture, roadmap, and technical direction.

Working concept document. September 2026.

---

## 1. Executive Summary

Meshflow currently provides shared mouse and keyboard control between computers, with basic clipboard synchronization. The proposed evolution is not simply to turn one PC into a remote monitor or to copy an existing remote-desktop product. The larger vision is a **Unified Distributed Workspace**: multiple computers remain independent systems but cooperate as one user-facing workspace.

The central idea is that workload should remain distributed. A powerful desktop should not be forced to render every application for a weaker laptop, and a laptop should not be reduced to an expensive display endpoint. Instead, each device can contribute its own CPU, GPU, memory, storage, display, input devices, and applications while Meshflow provides the coordination layer that makes the collection feel coherent.

**Core vision**

```
Meshflow
  = Input Sharing
  + Clipboard / Data Sharing
  + Display Sharing
  + Remote Application / Execution Coordination
  + Resource-Aware Workload Distribution
  + Unified Workspace UX
```

---

## 2. Current Meshflow Foundation

The existing Meshflow implementation already solves an important part of the problem: computers can communicate and share user interaction. This foundation should be preserved rather than replaced.

- Shared mouse input between computers.
- Shared keyboard input between computers.
- Basic clipboard synchronization.
- Networked communication between participating machines.
- Cross-platform application architecture.
- A user-facing GUI for configuring and operating the system.
- A topology in which the pointer can move between computers as though they are adjacent.

This means the next stage is primarily an expansion of the transport and workspace model, not a restart of the project.

---

## 3. Problem Statement

People increasingly own several capable computers at the same time: a desktop, laptop, workstation, secondary PC, home server, or specialized machine. Existing tools generally force a choice between two models.

- **Input-sharing tools** make several computers easier to control, but each computer remains a separate workspace.
- **Remote-desktop tools** centralize execution on the remote machine and stream the result, which can waste local compute resources.
- **Virtual-display solutions** can make another computer behave like a monitor, but the host carries most of the application workload.
- **Traditional distributed-computing systems** can distribute workload, but they generally do not provide a seamless desktop workspace.

Meshflow's opportunity is to combine the useful parts of these models without forcing all workload onto a single machine.

---

## 4. Product Vision: One Workspace, Multiple Computers

Meshflow should make the user think about a workspace rather than individual computers. The machines remain technically independent, but the user can interact with them through a unified spatial and interaction model.

```
                  MESHFLOW WORKSPACE
                         |
          +--------------+--------------+
          |              |              |
       Desktop         Laptop        Secondary PC
          |              |              |
       CPU/GPU        CPU/GPU        CPU/GPU
       RAM/SSD        RAM/SSD        RAM/SSD
          |              |              |
          +--------------+--------------+
                         |
              Unified interaction layer
```

The important distinction is that Meshflow should not pretend the machines are literally one operating system. Instead, it should provide a distributed workspace layer above the existing operating systems.

---

## 5. Product Modes

### 5.1 Input Sharing

Existing capability.

- Move the pointer between computers.
- Send keyboard input to the active computer.
- Synchronize basic clipboard content.
- Treat multiple machines as adjacent workspace regions.

### 5.2 Virtual Display

A host computer exposes an additional virtual display. A child computer renders that display on its physical screen. The important technical goal is a **true extended desktop** rather than merely opening a remote-desktop window.

- The host OS sees a second display.
- Windows can be dragged from the host's primary display onto the virtual display.
- Meshflow transports the resulting display frames to the child device.
- The child primarily performs decoding and presentation.
- LAN-first operation should be the initial target.

### 5.3 Remote Desktop

Meshflow can later support conventional remote control: the remote machine renders its own desktop while Meshflow transports display data back to the controller and sends input in the opposite direction.

### 5.4 Hybrid / Unified Workspace

This is the long-term differentiator. A display may belong to one machine while an application runs on another. Input, clipboard, files, displays, and application execution become separate capabilities that Meshflow can route independently.

---

## 6. Distributed Workload Model

The key design principle is: **do not make the main PC carry all workload simply because it owns the unified desktop.** Each application should ideally execute where it makes the most sense.

Example workspace:

```
Desktop
  - VS Code
  - Docker
  - AI inference
  - GPU-heavy workloads

Laptop
  - Browser
  - Communication apps
  - Mobile testing
  - Local GPU/CPU workloads

Secondary PC
  - Build jobs
  - Background services
  - Storage
  - Batch processing

Meshflow
  - Coordinates interaction
  - Routes display/input/data
  - Maintains workspace topology
  - Helps select execution location
```

This does not mean every application must become remotely executable immediately. The architecture should first establish the transport and identity primitives needed to support that capability later.

---

## 7. High-Level Architecture

```
                         MESHFLOW WORKSPACE
                                |
             +------------------+------------------+
             |                  |                  |
          CONTROL             DISPLAY             DATA
             |                  |                  |
       Mouse / Keyboard     Video Frames       Clipboard
       Focus / Pointer      Resolution         Files
       Topology             Refresh Rate       Metadata
             |                  |                  |
             +------------------+------------------+
                                |
                       MESHFLOW TRANSPORT
                                |
              +-----------------+-----------------+
              |                 |                 |
           Windows            Linux             macOS
              |                 |                 |
        Native runtime    Native runtime    Native runtime
```

### 7.1 Session Layer

- Device identity and discovery.
- Pairing and authentication.
- Session establishment and teardown.
- Encryption and key management.
- Capability negotiation.
- Connection health and reconnection.

### 7.2 Control Channel

- Mouse events.
- Keyboard events.
- Focus changes.
- Pointer topology.
- Display configuration changes.
- Workspace commands.

### 7.3 Display Channel

- Virtual-display metadata.
- Frame or dirty-region transport.
- Hardware-accelerated encoding/decoding where available.
- Adaptive quality and bitrate.
- Frame prioritization and latency control.

### 7.4 Data Channel

- Text clipboard.
- Image clipboard.
- File transfer.
- Future drag-and-drop semantics.
- Application metadata.

---

## 8. The Window-Movement Problem

The existing shared-input implementation can move the cursor between computers, but an application window belongs to the operating system that created it. Therefore, moving the mouse onto another computer does not inherently move the application.

A true virtual-display implementation changes this. If the host OS recognizes a remote screen as a real additional display, the normal window manager can place an application there.

```
Host OS
  +-- Monitor 1
  +-- Virtual Monitor 2
           |
           +-- Meshflow Display Transport
                         |
                         v
                  Child physical screen
```

Result: User drags application from Monitor 1 to Monitor 2. Application remains owned by the host OS. Child computer displays Monitor 2.

This is an important milestone because it demonstrates a visually obvious transition from simple input sharing to workspace composition.

---

## 9. Why Virtual Display Alone Is Not the Final Goal

A virtual display solves the window-placement problem, but it has a major architectural cost: the host performs most application work. The child machine becomes mostly a decoder and display endpoint.

- The host renders applications.
- The host consumes CPU/GPU/RAM for those applications.
- The child contributes little beyond decoding and presentation.
- The approach can be inefficient when the child has useful compute resources.

Therefore, virtual display should be treated as a **capability** within Meshflow, not as the definition of the product.

---

## 10. Distributed Application Model

The long-term model is to allow an application to execute on one machine while its user-facing window is integrated into another machine's workspace.

```
User Workspace
      |
      +-- Window: VS Code
      |       +-- executes on Desktop
      |
      +-- Window: Browser
      |       +-- executes on Laptop
      |
      +-- Window: Build Task
              +-- executes on Secondary PC
```

Achieving this perfectly would require significant OS integration and application remoting. It should therefore be considered a long-term research direction rather than a near-term promise.

---

## 11. Relationship to Remote Desktop / AnyDesk-Like Software

Meshflow can eventually support many of the technical primitives found in remote-desktop software, including display streaming and remote input. However, the product abstraction should remain different.

- **Remote desktop**: "I am controlling another computer."
- **Meshflow unified workspace**: "I am using several computers as one workspace."
- Remote desktop typically moves the user's interaction into a remote machine.
- Meshflow can route display, input, data, and eventually workload independently.
- The goal is cooperation between machines rather than simply control of one machine by another.

---

## 12. Major Technical Challenges

1. Virtual display drivers and OS integration across Windows, Linux/Wayland, X11, and macOS.
2. Low-latency screen capture and transport.
3. GPU-accelerated encoding and decoding.
4. Efficient dirty-region or changed-frame detection.
5. Adaptive bitrate and quality control.
6. Input latency and event ordering.
7. Network loss, jitter, reconnect, and session recovery.
8. Multi-monitor geometry, scaling, DPI, rotation, and refresh-rate differences.
9. Security, device authentication, and permission boundaries.
10. Application identity and process placement if remote application execution is introduced.
11. Cross-platform packaging, permissions, and driver installation.
12. Resource scheduling if workload distribution becomes automatic.

---

## 13. Suggested Protocol Design

The protocol should be capability-oriented. A connection should not assume that every device supports every feature.

```
HELLO
  device_id
  platform
  version
  capabilities

CAPABILITIES
  input
  clipboard
  display
  file_transfer
  remote_execution
  hardware_decode
  hardware_encode

SESSION
  authentication
  encryption
  negotiated_transport

RUNTIME
  input events
  clipboard events
  display frames
  workspace commands
  health / heartbeat
```

This makes it possible to add capabilities without redesigning the entire protocol.

---

## 14. Security Model

- Explicit device pairing rather than accepting arbitrary clients.
- Strong device identity.
- Encrypted transport.
- Per-session authorization.
- User-visible permission prompts for sensitive operations.
- Separate permissions for input control, display access, clipboard, files, and future remote execution.
- Ability to revoke a paired device.
- No requirement for cloud accounts for local-network operation.

Remote execution and file transfer should receive stricter authorization than basic input sharing.

---

## 15. Performance Targets

Initial targets should prioritize a reliable LAN experience over maximum resolution or refresh rate.

- 1080p at 60 FPS as an initial virtual-display target.
- Low perceived input latency on wired LAN.
- Hardware encoding/decoding when available.
- Graceful degradation on Wi-Fi or weaker hardware.
- Minimal idle CPU usage.
- Fast connection establishment.
- Stable operation across long-running sessions.
- Clear diagnostics for bandwidth, latency, dropped frames, and decoder load.

---

## 16. User Experience

The product should avoid exposing networking complexity to ordinary users.

```
Meshflow
+--------------------------------------+
| Devices                              |
|                                      |
|  * Desktop        Connected          |
|  * Laptop         Connected          |
|  * Workstation    Available          |
|                                      |
| Workspace                            |
|                                      |
|  [Desktop] [Laptop] [Workstation]    |
|                                      |
| Capabilities                         |
|  [x] Mouse & Keyboard               |
|  [x] Clipboard                       |
|  [x] Use as Display                  |
|  [ ] File Transfer                   |
|  [ ] Remote Execution                |
+--------------------------------------+
```

- Automatic discovery on the local network.
- Simple pairing.
- Visual workspace layout.
- Per-device capability toggles.
- Clear status and connection diagnostics.
- One-click "Use as Display" flow.
- No need to understand IP addresses for normal use.

---

## 17. Proposed Roadmap

### Phase 1: Strengthen the Foundation

- Stabilize current mouse/keyboard sharing.
- Improve clipboard synchronization.
- Formalize device identity and capability negotiation.
- Improve connection recovery and diagnostics.
- Document the protocol and architecture.

### Phase 2: Meshflow Display

- Implement virtual-display support for one priority OS first.
- Create a display transport channel.
- Support 1080p/60 FPS LAN operation.
- Implement resolution and display-geometry negotiation.
- Demonstrate real window movement onto the remote screen.

### Phase 3: Hybrid Workspace

- Combine input sharing and virtual displays.
- Make display endpoints independently configurable.
- Support multiple displays and multiple child devices.
- Improve workspace topology and pointer transitions.
- Add file transfer and richer clipboard support.

### Phase 4: Remote Application Research

- Investigate application/process execution on another node.
- Prototype a controlled remote application launcher.
- Explore GPU/resource-aware placement.
- Study window remoting versus full-display streaming.
- Keep the prototype opt-in and security-isolated.

### Phase 5: Distributed Workspace

- Unified device/resource model.
- Application placement policies.
- Resource-aware workload scheduling.
- Persistent workspace state.
- Cross-device drag/drop semantics.
- Advanced remote execution and session migration where technically feasible.

---

## 18. What Not to Build Too Early

- Do not immediately attempt to recreate every feature of commercial remote-desktop software.
- Do not start with internet-wide NAT traversal before LAN reliability is excellent.
- Do not make virtual display the only architecture.
- Do not build a complex cloud backend if local networking is the core use case.
- Do not automatically distribute arbitrary applications before security and process isolation are understood.
- Do not sacrifice the existing input-sharing experience while pursuing advanced features.

---

## 19. Differentiation Strategy

Meshflow should avoid competing purely on "another keyboard and mouse sharing app." Its strongest story is the progression from input sharing to a distributed workspace.

```
Shared Input
     |
     v
Shared Workspace
     |
     v
Virtual Displays
     |
     v
Hybrid Workspace
     |
     v
Distributed Workload
     |
     v
Personal Distributed Computer Environment
```

The product's long-term differentiation is not a particular codec or driver. It is the abstraction that multiple personal computers can cooperate without forcing the user to manage each one as a completely separate workspace.

---

## 20. Killer Demo

The first major demonstration should be simple enough to understand without technical explanation.

1. Show a desktop PC and laptop on the same desk.
2. Meshflow detects the laptop.
3. Select "Use as Display".
4. The host desktop gains a second display.
5. Drag VS Code from the desktop monitor onto the laptop screen.
6. Move the mouse naturally across the display boundary.
7. Copy text on one machine and paste it on the other.
8. Then show a second machine running an independent workload, demonstrating that the workspace is not merely a dumb display.

The demo should communicate the difference between "remote desktop" and "distributed workspace" within seconds.

---

## 21. Success Metrics

- A new user can connect two machines without reading technical documentation.
- Mouse and keyboard sharing feels instantaneous on a local network.
- Clipboard synchronization works reliably for common content.
- A child machine can become a usable extended display with minimal setup.
- Applications can be dragged onto the virtual display naturally.
- The system remains stable during long sessions.
- CPU, GPU, memory, and network usage are observable and reasonable.
- The architecture can add new capabilities without breaking existing connections.
- Early users understand Meshflow as a unified workspace rather than simply a mouse-sharing tool.

---

## 22. Key Risks

- Cross-platform virtual-display implementation may become the largest engineering effort.
- Wayland and macOS display integration may require platform-specific approaches.
- Video streaming can consume substantial bandwidth and GPU resources.
- Distributed application execution introduces difficult OS, security, and compatibility problems.
- Trying to solve all platforms simultaneously can slow development dramatically.
- The product can become too broad if every remote-computing feature is accepted without a clear architecture.

---

## 23. Core Architectural Principle

**Separate capabilities from machines.**

Meshflow should not hard-code the assumption that the machine providing input must also provide display, compute, storage, or application execution. Treat each capability as something that can be provided by a node and routed through the workspace.

```
Node A
  provides: input + compute + display

Node B
  provides: display + compute

Node C
  provides: storage + compute

Meshflow
  coordinates capabilities
  and presents them as one workspace
```

---

## 24. Final Product Definition

Meshflow is an open, cross-platform system for connecting multiple personal computers into a single interactive workspace. It begins with shared mouse, keyboard, and clipboard functionality. It can extend into virtual displays and remote desktop capabilities, but its defining long-term principle is **distributed workload**: each connected machine remains useful and contributes its own resources instead of becoming merely a passive endpoint.

**One workspace. Many computers. Distributed by design.**
