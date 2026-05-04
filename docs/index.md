---
layout: home
title: Home
nav_order: 1
---

This site documents the **2026 Spring Project**: a cooperative night-survival game with a **Go** authoritative server, **raylib** graphics, and a **WebAssembly** client (Emscripten). Gameplay state, animatronic movement, power, and win/lose rules run on the server; the browser renders 3D and sends player actions over WebSockets.

The project is also framed as an **MIS Independent Study**; the formal proposal (summary, scope, learning objectives, deliverables) lives on [Independent study]({{ site.baseurl }}/independent-study/).

## Sections

| Topic | What you will find |
|--------|---------------------|
| [Independent study]({{ site.baseurl }}/independent-study/) | MIS proposal: scope, learning goals, deliverables, mapping to this repo |
| [Build]({{ site.baseurl }}/build/) | Local dev, WASM build, Docker, syncing artifacts into the server |
| [Server]({{ site.baseurl }}/server/) | HTTP routes, WebSocket protocol, sim ticker, auth, persistence |
| [Client]({{ site.baseurl }}/client/) | raylib + Emscripten, header-only style, WebSocket helpers |
| [Game state]({{ site.baseurl }}/game-state/) | Server `GameRoomState` / sim vs client `GameState`, JSON wire model |
| [Economy]({{ site.baseurl }}/economy/) | Power, doors, P2 music box, queues drained per tick |
| [Scene swapping]({{ site.baseurl }}/scene-swapping/) | `Scene` base class, menu → main handoff, loading strategy |

## Source repository

[github.com/jrny6996/2026-spring-project](https://github.com/jrny6996/2026-spring-project)

## Progress notes

Older progress write-ups (for example [April 16 on GitHub](https://github.com/jrny6996/2026-spring-project/blob/master/progress/apr_16.md)) live under `progress/` in the repository; this site focuses on architecture and operations.
