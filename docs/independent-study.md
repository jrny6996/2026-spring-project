---
layout: page
title: Independent study
permalink: /independent-study/
nav_order: 2
---

This page records the **MIS Independent Study** proposal (Jonathan Clark): goals, scope, learning objectives, and how they map to this repository’s architecture.

## Summary

The independent study targets a **3D multiplayer horror game** that runs **entirely in the web browser**, combining:

- A **client-side 3D stack** (WebGL-oriented rendering in the browser; this repo uses **raylib** compiled to **WebAssembly** via Emscripten).
- A **custom game server** for **real-time multiplayer** (this repo uses a **Go** authoritative server and **WebSockets**).

The work is meant to demonstrate advanced web development, real-time networking, and efficient rendering within browser constraints.

## Project scope

The proposal calls for a shipped game with:

| Area | Proposal intent | How this repo aligns |
|------|-----------------|----------------------|
| **Levels** | 5–7 playable levels, time-gated wins, escalating difficulty | Server-driven rooms, phases, and win/lose rules; level/scene flow is documented under [Scene swapping]({{ site.baseurl }}/scene-swapping/). |
| **Multiplayer** | 2–3 players, WebSockets | WebSocket protocol and room lifecycle are covered in [Server]({{ site.baseurl }}/server/) and [Game state]({{ site.baseurl }}/game-state/). |
| **Dedicated server** | Sync, lobby/matchmaking, game state | Go server: HTTP + WebSockets, sim ticker, persistence—see [Server]({{ site.baseurl }}/server/). |
| **3D rendering** | WebGL, lighting, textures, models, animation | WASM client with raylib; asset and build flow in [Client]({{ site.baseurl }}/client/) and [Build]({{ site.baseurl }}/build/). |
| **AI / hazards** | Enemies or environmental hazards tied to player actions | Server-side sim and animatronic-style logic (authoritative state); details in [Game state]({{ site.baseurl }}/game-state/) and [Economy]({{ site.baseurl }}/economy/). |
| **UI** | Menus, HUD, timers, status | Client scenes and server-fed state; scene handoff in [Scene swapping]({{ site.baseurl }}/scene-swapping/). |
| **Optimization** | Memory and performance within the browser | Profiling and packaging considerations are part of the WASM/build docs and runtime design (server does heavy logic; client renders). |

## Learning objectives

### 1. Graphics rendering

- Use **WebGL-oriented** 3D in the canvas (here: **WebAssembly + raylib**, which targets WebGL in the browser).
- Understand **shader-oriented** graphics concepts (vertex/fragment pipeline) as used by the engine layer.
- **Models, lighting, materials**: covered practically in the client build and runtime paths documented in [Client]({{ site.baseurl }}/client/).

### 2. Real-time networking (WebSockets)

- **Persistent connections** between browser and server.
- **Fast serialization** and low-latency messages (JSON wire model and protocol notes in [Server]({{ site.baseurl }}/server/) and [Game state]({{ site.baseurl }}/game-state/)).
- **Multiplayer concerns**: lag, interpolation/extrapolation, entity sync—implemented and documented at the protocol and sim-ticker level on the server.

### 3. Software architecture and engineering

- **Full-stack** structure: WASM client, Go server, shared understanding of state vs presentation.
- **Modular systems**: rendering, input, networking, game logic, assets—reflected in repo layout (`client_wasm/`, `server/`) and the architecture pages on this site.
- Builds on prior MIS/RBS web coursework while extending into **engine- and server-level** design.

### 4. Optimization and performance

- Respect **browser memory** and target **smooth frame rates**.
- Profile **render loops**, **GC**, and **packet size** tradeoffs where applicable.
- Balance **fidelity vs performance** on typical student hardware.

### 5. Open source

- **Public repository** with clear documentation for reuse in teaching and demos.
- This **GitHub Pages** site documents engine and server architecture for courses, WebGL/WASM introductions, and WebSocket multiplayer examples.

## Educational value

### Builds on prior coursework

Extends web design and interactive media foundations into:

- WebGL/WASM graphics
- Real-time communication
- Full-stack game architecture
- Performance engineering

### Prepares for industry

Mirrors practical problems in **networking**, **rendering**, **asset pipelines**, and **scalable structure**—relevant to software, games, and interactive systems roles.

## Benefits for the MIS department

The finished project and documentation can support:

- **Class demos** for networking, graphics, or web technology topics
- A **reusable codebase** for future independent studies or student projects
- An **example** of what students can build with modern browser stacks (WASM, WebSockets, authoritative server)

## Deliverables

1. **Playable browser-based 3D multiplayer game** with multiple levels (per proposal: 5–7), consistent with server rules and client rendering.
2. **Game server** with **documentation**—HTTP/WebSocket behavior, persistence, and sim described on [Server]({{ site.baseurl }}/server/) and related pages.
3. **Final presentation / demo** (course requirement): gameplay, technical challenges, and solutions—can reference this site and the `progress/` notes in the repository.

## Related documentation

| Page | Role |
|------|------|
| [Build]({{ site.baseurl }}/build/) | Toolchain, WASM output, Docker |
| [Server]({{ site.baseurl }}/server/) | Routes, WebSockets, sim, auth |
| [Client]({{ site.baseurl }}/client/) | raylib/Emscripten client |
| [Game state]({{ site.baseurl }}/game-state/) | Authoritative vs client state |
| [Economy]({{ site.baseurl }}/economy/) | Power, doors, queues |
| [Scene swapping]({{ site.baseurl }}/scene-swapping/) | Menus → main game flow |
