<!--
SPDX-FileCopyrightText: (C) 2026 Rimora Studio
SPDX-License-Identifier: MIT
-->

# Meshflow Port Share — implementation plan

Forward `localhost` TCP ports between peers. A dev server running on PC A's
`localhost:3000` becomes reachable at `localhost:3000` on connected PC B.

```
PC A (server)                          PC B (client)
localhost:3000 (dev server)            localhost:3000 → tunnel → A:3000
```

Think VS Code port forwarding / `ssh -L`, but LAN-native and part of the
distributed workspace.

## Goals

- v1: forward a configured list of TCP ports, localhost-only both ends, over
  the existing TLS-encrypted link (inherits fingerprint trust).
- Later: per-share permission prompts, UDP, named shares, session multiplexing.

## Wire design (v1, as implemented)

No new listen ports, no new protocol version, no new messages. A forward is
a normal Meshflow TCP+TLS connection on port 24800 whose hello-back client
name is the port-share prefix plus the target port:

```
Client → Server:  hello-back (protocol, version, name="meshflowfwd:<port>")
Then: raw bidirectional byte pump to 127.0.0.1:<port> on the server side
```

Server-side dispatch: `ClientProxyUnknown::handleData` parses the hello-back,
sees the `meshflowfwd:` name prefix (`kPortShareNamePrefix`), validates the port
against the whitelist, and hands the socket to `PortForwardSession` instead of
a `ClientProxy`. There is no ack message — a rejected port or failed dial
simply closes the connection, which the client surfaces as a closed local
socket.

This keeps `ClientListener`, the hello exchange, TLS upgrade, and fingerprint
verification untouched, and old servers reject the unknown screen name the
same way they reject any unconfigured client.

## Components

### `src/lib/meshflow` — shared
- `ProtocolTypes.h/.cpp`: `kPortShareNamePrefix` (`"meshflowfwd:"`).
- `StreamPump.{h,cpp}`: event-driven bidirectional relay between two
  `meshflow::IStream`s. Half-close semantics: input shutdown on one side →
  flush + output shutdown on the peer; both shutdown or any stream error →
  close both + done callback.
- `unittests/meshflow/StreamPumpTests.cpp`: relay both directions, shutdown
  propagation, close-on-error.

### `src/lib/server` — `PortForwardSession.{h,cpp}`
- Receives the post-hello socket + requested port.
- Whitelist: `Server::sharedPorts()` → `Config::m_sharedPorts`, seeded from
  `Settings::Server::SharedPorts` and the `sharedPorts = <csv>` line in the
  generated config's `options:` section. Empty = all requests refused.
- Dials `127.0.0.1:<port>` with a `TCPSocket` (plaintext — loopback).
- Runs `StreamPump`; logs open/close at INFO.

### `src/lib/client` — `PortShareListener.{h,cpp}`
- For each configured entry (from `Settings::Client::ForwardPorts`):
  binds `127.0.0.1:<local>` with `TCPListenSocket` (loopback only).
  Entries are either `P` (same port both ends) or `remote:local`
  (server port `remote` reachable at this machine's `localhost:<local>`).
- On accept: opens a new socket via the client's `ISocketFactory` (same TLS
  security level as the main connection), answers the server hello with
  name `meshflowfwd:<remote>`, then pumps.
- Caps: 8 concurrent forwards per port, 10s hello timeout.
- Bind failure (port in use): WARN log, other ports still start.
- Wired in `ClientApp`: created on client connect, torn down on disconnect.

### GUI
- Dedicated "Port Share" page in the main window nav rail with two fields:
  `server/sharedPorts` (ports peers may reach on this machine) and
  `client/forwardPorts` (ports tunneled from the server while connected).
- `ServerConfig` writes `sharedPorts = <csv>` into the generated config.

## Security model (v1)

- Forwarding is opt-in on the server (`sharedPorts` empty by default → all
  forward connections refused).
- Both ends bind `127.0.0.1` only — nothing is exposed on the LAN.
- TLS + fingerprint trust: only verified peers can open forwards.
- Cap concurrent forwards per port (8) to limit fd exhaustion.
- Rejection is silent close (no reason codes) — acceptable for v1 since the
  GUI owns configuration on both ends.

## Testing

- `StreamPumpTests` — byte relay both directions, half-close, error close.
- Manual: `node -e "http server"` on A :3000 → B `curl localhost:3000`.
- Two-machine: dev server (vite/next) on A, browser on B hits
  `localhost:3000` — HMR websockets must work (they are just TCP).

## Milestones

1. **Protocol + pump** — `kPortShareNamePrefix`, `StreamPump`, tests. ✅
2. **Server side** — name-prefix dispatch, whitelist check, dial. ✅
3. **Client side** — `PortShareListener`, reconnect lifecycle. ✅
4. **Config/GUI** — Port Share page + settings keys. ✅
5. **Dogfood** — vite/next dev server across two machines incl. websockets.

## Post-v1 ideas

- Server advertises `sharedPorts` in the session; client shows
  "Available shared ports" with one-click enable (no manual port typing).
- Allow/deny prompt on the server when a peer first requests a port.
- Named shares (`my-app:3000`) surfaced in the workspace UI.
- Channel multiplexing inside the input session (single connection).
- UDP forwarding.
