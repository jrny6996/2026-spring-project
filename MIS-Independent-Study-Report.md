# MIS Independent Study — Project & Deliverables Report

**Student:** Jonathan Clark  
**Topic:** Browser-based 3D multiplayer horror game (WebGL stack + custom game server)  
**Repository:** [2026-spring-project](https://github.com/jrny6996/2026-spring-project)  
**Living documentation (GitHub Pages):** [Documentation site](https://jrny6996.github.io/2026-spring-project/)

This report maps the **approved independent study proposal** to what is **implemented in the codebase today**, and states **deliverable status** for grading or demo planning.

---

## Executive summary

The project delivers a **full-stack, browser-playable cooperative survival game** with **authoritative Go simulation**, **WebSocket multiplayer (two roles per lobby)**, **seven escalating “nights” (levels)** with **time-gated win conditions**, **3D rendering** via **raylib compiled to WebGL 2 (Emscripten)**, **SQLite-backed accounts**, **JWT-gated progression** for higher nights, and **published technical documentation** suitable for reuse in MIS-adjacent courses. A **final presentation** is outside the repository and should be scheduled separately.

---

## Proposal scope vs. implementation

### 5–7 playable levels, time-gated difficulty

| Proposal item | Status | Evidence in project |
|---------------|--------|---------------------|
| Multiple levels | **Met** | Nights **1–7** (`clampNightNum` in `server/game.go`; progression and UI use `night` query / WS). |
| Time-gated win | **Met** | Server advances in-game time on a fixed tick; win when `room.Time >= winTimeGameSeconds(night)` (`server/main.go`). Higher nights use a **shorter** survival window (escalating difficulty). |
| Escalating difficulty | **Partially / met** | Shorter nights plus **night-scoped entity move probabilities** (`moveChanceNumerator` in `server/game.go`). Map construction also varies by night (`CreateInitialMap` / `CreateP2Map`). |

### Multiplayer (2–3 players) via WebSockets

| Proposal item | Status | Notes |
|---------------|--------|--------|
| Real-time WebSockets | **Met** | `/echo` JSON protocol; gorilla/websocket (`server/main.go`). |
| 2-player coop | **Met** | Lobby: host `invite`, guest `join`, host `start`; roles **Player 1 / Player 2** with asymmetric duties (doors/power vs. mask/music box per server design). |
| 3 players | **Not implemented** | Lobbies are modeled for **two** connected clients (host + acceptor). |

### Dedicated game server

| Proposal item | Status | Notes |
|---------------|--------|--------|
| Player synchronization | **Met** | Authoritative sim; periodic **`state`** broadcasts; per-client **fog-of-war** filtering of `simEntities` (`docs/server.md`, `docs/game-state.md`). |
| Matchmaking / lobby | **Met** | UUID lobbies, join/start flow; not automated global matchmaking—**intentional lobby codes** style. |
| Game state management | **Met** | `GameRoomState` + `GameState` / `EntityTracker` (`server/main.go`, `server/game.go`). |

### 3D rendering (WebGL), lighting, textures, models

| Proposal item | Status | Notes |
|---------------|--------|--------|
| WebGL in browser | **Met** | **WebGL 2 / GLES3** through **raylib + Emscripten** (`docs/client.md`, `client_wasm/CMakeLists.txt`). |
| Models & textures | **Met** | Preloaded `client_wasm/assets/` → `game.data`; third-party map models attributed in `readme.md` / `client_wasm/Attribution.md`. |
| Shaders / lighting | **Met** | raylib material/shader path; client docs reference PBR-style work in scene code. |

### Basic AI / hazards

| Proposal item | Status | Notes |
|---------------|--------|--------|
| Enemy-like behavior | **Met** | Graph-based **animatronic simulation** with movement rules, doors blocking paths, office danger, lose conditions (`server/game.go`, lose handlers in `server/main.go`). |
| Environmental / cooperative hazards | **Met** | Shared **power** drain, **music box** wind, door/mask actions tied to server economy (`docs/economy.md`). |

### User interface

| Proposal item | Status | Notes |
|---------------|--------|--------|
| Menus, lobby | **Met** | `client_wasm/menu.hpp`; auth HTML/API on server. |
| HUD, timers, status | **Met** | Night clock mapping, power, role indicators; server-driven state mirrored in `client_wasm/GameState.hpp` / `ws_init.hpp`. |

### Browser optimization

| Proposal item | Status | Notes |
|---------------|--------|--------|
| Performance-conscious build | **Met** | Asyncify, stack sizing, GLES3 choice documented in `docs/client.md` / `client_wasm/CMakeLists.txt`. |
| Systematic profiling write-up | **Limited** | Engineering tradeoffs are documented; formal profiling reports are not a separate artifact in-repo. |

### Open source & documentation

| Proposal item | Status | Notes |
|---------------|--------|--------|
| Public repository | **Met** | GitHub link in `readme.md` / `package.json`. |
| Clear architecture docs | **Met** | Jekyll site under `docs/` (build, server, client, game state, economy, scene flow). |
| Explicit `LICENSE` file | **Gap** | No top-level `LICENSE` file was present at report time; adding a standard OSS license would fully align with the proposal’s “open source release” wording. |

---

## Learning objectives — coverage

1. **Graphics rendering** — **Strong:** WebGL via Emscripten, 3D scene graph usage through raylib, asset preload pipeline, shader/material usage consistent with raylib workflows.  
2. **Real-time networking** — **Strong for authoritative snapshots:** JSON over WebSocket, lobby protocol, server-driven state. **Weaker vs. proposal wording:** dedicated **lag compensation**, **interpolation/extrapolation** of remote entities are not emphasized in docs as first-class systems (design is tick-based authoritative sim, not client-prediction FPS).  
3. **Software architecture** — **Strong:** Clear split **Go server** / **C++ WASM client**, documented wire model and fog-of-war, modular headers (`menu`, `MainScene`, `ws_init`, `GameState`).  
4. **Optimization** — **Moderate–strong in implementation** (build flags, authoritative sim frequency); **lighter** on formal measurement narrative.  
5. **Open source contribution** — **Strong** on docs + repo visibility; **complete** once a **license file** and any desired **contribution guidelines** are added.

---

## Educational value (as realized)

- **Builds on web/MIS foundations:** HTTP, sessions/JWT, HTML shells (`server/auth.html`, `server/play.html`), and API design extend typical web coursework into **games + real-time systems**.  
- **Industry-relevant patterns:** Authoritative server, structured message protocol, persistence for progression, container packaging (`Dockerfile`, `docker-compose.yml`).  
- **Departmental reuse:** The `docs/` site is suitable as a **teaching reference** for WebSocket multiplayer, WASM clients, and small-game server architecture.

---

## Deliverables checklist

| # | Deliverable | Status |
|---|-------------|--------|
| 1 | **Fully playable browser-based 3D multiplayer game with 5–7 levels** | **Met** (7 nights; 3-player not in scope). |
| 2 | **Game server implementation with documentation** | **Met** (`server/*.go`, `docs/server.md`, related pages). |
| 3 | **Final presentation/demo** | **Out of band** — prepare slides/live demo from build artifacts and GitHub Pages; not stored in this repo. |

---

## Suggested demo narrative (for presentation)

1. **Auth & progression:** register/login → JWT → nights 3–7 require prior wins when logged in (`server/main.go` messages reference this).  
2. **Lobby & roles:** create/join lobby → host starts → asymmetric two-player UI.  
3. **Technical depth:** authoritative tick, fog-of-war on `simEntities`, economy (power / music).  
4. **Stack:** Emscripten + raylib (client), Go + SQLite + WebSockets (server).  
5. **Honest stretch goals:** third seat in lobby, deep netcode (prediction/interpolation), formal perf study — good “future work” slides.

---

## References (in-repo)

- Root overview: `readme.md`  
- Server architecture: `docs/server.md`  
- Client architecture: `docs/client.md`  
- Wire/state model: `docs/game-state.md`  
- Economy: `docs/economy.md`  
- Progress snapshot (may predate current sim wiring): `progress/apr_16.md`  
- Asset licensing: `client_wasm/Attribution.md`, CC BY 4.0 links in `readme.md`

---

*Report generated to align MIS independent study proposal language with the 2026-spring-project codebase and documentation.*
