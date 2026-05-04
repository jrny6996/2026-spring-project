---
layout: page
title: Server
permalink: /server/
nav_order: 3
---

## Role

The Go server is **authoritative**: it owns lobby lifecycle, night timing, power and music-box economy, door and mask flags, animatronic simulation (`GameState` in `server/game.go`), fog-of-war filtering for camera feeds, and win/lose outcomes. Clients receive periodic JSON **state** snapshots and send typed **actions** over a single WebSocket.

## Main modules

| File | Responsibility |
|------|----------------|
| `server/main.go` | HTTP router, WebSocket `/echo`, lobby map, sim ticker, economy tick, JSON wire types, win/lose handlers |
| `server/game.go` | Graph nodes (`GameGraphNode`), entities, `EntityTracker`, movement rules, night-scoped map construction |
| `server/auth.go` | JWT signing/validation, bcrypt passwords, SQLite-backed users, cookie and `?token=` handling |

## HTTP surface

- **`/`** — Serves the play shell if the request is authenticated; otherwise redirects to `/auth`.
- **`/game/*`** — Static files from `wasmdist/` (WASM, JS, data, HTML) behind the same auth gate.
- **`/auth`** — Login/register HTML (`auth.html` template).
- **`/api/register`**, **`/api/login`**, **`/api/me`**, **`/api/session/sync`**, **`/api/logout`** — Session and account APIs used by the auth page and client.

## WebSocket (`/echo`)

Clients open a WebSocket with query parameters:

- **`night`** — Requested night (1–7). Guests without a JWT are clamped to **night ≤ 2**. Logged-in users are clamped by **progression** (must have won the previous night in SQLite to select a higher night).
- **`token`** — Optional JWT (also accepted via `Authorization: Bearer` or auth cookie on HTTP; mirrored for WS URL in the client).

Messages are JSON objects with **`type`** and **`content`** (string). Handled types include:

| `type` | Typical `content` | Purpose |
|--------|-------------------|---------|
| `chat` | `invite` | Create a lobby; host receives a lobby id |
| `join` | lobby UUID string | Join an existing lobby |
| `status` | `start` | Host requests night start (server validates both roles) |
| `check` | room alias | Camera check: which entities the server sees in that sim room |
| `view` | room alias or empty | Security-feed room for **fog-of-war** `simEntities` filtering |
| `step` | `tick` | Manual sim step (debug / paused flows) |
| `action` | `door:...`, `mask:...`, `p1q:...`, `p2qp:...`, `p2qm:...` | Doors, P2 mask, power/music queues |

## Simulation loop

- A background goroutine runs **`runSimStepTicker`**: every **`gameSecondsPerTick`** real seconds (5 seconds today, aligned with 5 in-game seconds per tick), it calls **`advanceLobbySimOneTick`** for every lobby that has started.
- Each tick: advance **`Time`**, apply **door power drain**, run **`applyChargeEconomyTick`**, evaluate **music box** (P2 loss if wind hits zero while P2 is live), check **power ≤ 0** (lobby loss), run **`GameState.Step`**, office danger / mask logic, **broadcast `state`** to all sockets in the lobby, then check **survival time** against **`winTimeGameSeconds(night)`**.

## State to clients

**`stateForClient`** builds a per-connection view: copies `GameRoomState`, sets **`isPlayerOne`** from `RemoteAddr` vs host/acceptor, includes **`nightEndGameSec`**, and filters **`simEntities`** with **`filterSimEntitiesForFog`** using the last **`view`** alias (offices, doors, and selected choke halls are always visible).

## Persistence

- **SQLite** (`DATABASE_PATH`, default `app.db` in dev) stores users and **night wins** for progression and optional **`recordWin`** on successful night end.
- **`JWT_SECRET`** should be at least 32 bytes in production; shorter values fall back to a dev default in code (see `auth.go`).

## Deployment notes

The Docker image ships the server binary plus `wasmdist`. **Build and copy the WASM client into `wasmdist` before `docker build`** (or use a CI step that builds the client and copies artifacts), otherwise the play page will be missing assets.
