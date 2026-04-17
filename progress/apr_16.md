# Project progress — 4/16/25

This document summarizes the **indep** repository as of the current tree: layout, implemented features, the server-side map/entity graph model, and practical build-system notes.

---

## Repository layout

| Area | Role |
|------|------|
| `server/` | Go HTTP + WebSocket game server, SQLite auth DB, lobby/simulation logic (`main.go`, `game.go`, `auth.go`, `db.go`, HTML shells). |
| `client_wasm/` | Emscripten + raylib C++ client: 3D scenes, menu, WebSocket bridge to the server (`main.cpp`, `ws_init.hpp`, `GameState.hpp`, headers for scenes/camera). |

There is no single top-level `Makefile` tying server and client together; each half is built with its own toolchain.

---

## Features implemented so far

### Server (`server/`)

- **HTTP**
  - `/` — serves `index.html` (minimal WebSocket test UI: join + generic chat send).
  - `/auth` — serves `auth.html` for registration/login flows (paired with API below).
- **REST API**
  - `POST /api/register` — create user (username rules, password length, bcrypt hash), returns JWT + user id.
  - `POST /api/login` — verify credentials, returns JWT.
  - `GET /api/me` — JWT-protected; returns user id and username (token via query `?token=` or `Authorization: Bearer …`).
- **Persistence**
  - SQLite (`app.db`) via `github.com/mattn/go-sqlite3`; `users` table with unique username.
- **WebSocket** (`GET /echo`)
  - JSON messages shaped as `{ "type": "...", "content": "..." }` (see `Message` in `main.go`).
  - **Lobby lifecycle**
    - `type: "chat"`, `content: "invite"` — host creates lobby UUID, stored in `lobbySet`, user bound to lobby.
    - `type: "join"`, `content: "<lobbyId>"` — second player joins; `joined` response with lobby id.
    - `type: "status"`, `content: "start"` — **host only** starts match: random `HostIsPlayerOne`, constructs `GameState`, `SpawnEntities()`, sends `state`.
  - **Game tick / state broadcast**
    - Each connected client runs a **1 Hz ticker** on their socket goroutine: if they are in a started lobby, server pushes `state` (`gameStateWire`: lobby fields + per-client `isPlayerOne`).
    - **Night timer**: when `room.Time` exceeds `NIGHT_IN_MINUTES * 60`, clients receive `status: "win"` and the lobby is cleaned up.
  - **Security camera check**
    - `type: "check"`, `content: "<room alias>"` — resolves a `GameGraphNode` by `AliasName` on the active sim and returns `checkCamera` with `roomAlias` and `entities` (`entityId`, `name`). Errors use `type: "error"` with string `data` (e.g. `game-not-started`, `unknown-room`).
- **Background sim ticker** (`runSimStepTicker` in `main.go`)
  - A **global** goroutine increments `room.Time` each second for every started lobby that has a sim.
  - **`GameState.Step` is currently not invoked there** (call is commented); entity positions therefore do not advance from this ticker until that is re-enabled or moved back into a single authoritative step path.

### Client (`client_wasm/`)

- **Emscripten WebSocket** to `ws://localhost:6789/echo` (see `main.cpp`, `ws_init.hpp`).
- **Helpers** (`ws_init.hpp`): `send_invite`, `send_join`, `send_start`, `send_check_camera` — mirror server message types.
- **Parsed server events**: `invite`, `joined`, `error`, `checkCamera`, `state` (updates `GameState`: lobby id, host flag, `gameStarted`, `gameTime`, `is_player_one`, camera-check UI state).
- **Rendering / UX** (raylib): menu flow, scenes, camera navigation, PBR-style assets/shaders (see `MainScene.hpp`, `menu.hpp`, `camera_nav.hpp`, asset preload, etc.).

### Auth / security notes (current design)

- JWT secret from `JWT_SECRET` env var, with a **dev fallback** string in code if unset (documented in `auth.go` — production should always set a strong secret).
- WebSocket `CheckOrigin` returns `true` for all origins (convenient for local dev, not for hardened production).

---

## Map and entity data structures (server `game.go`)

The game world is modeled as a **directed graph of rooms** with **entities** stored per room and a **tracker** for fast lookup and movement history.

### `GameGraphNode` (map / room graph)

Each node is one navigable **room** (or chokepoint such as a door):

| Field | Meaning |
|-------|---------|
| `RoomID` | Numeric room id (some P2 nodes still use placeholder `100` in the graph builder). |
| `AliasName` | Stable string key used by the server and client (`"stage"`, `"party_room"`, `"lhs_door"`, …) for camera checks and tooling. |
| `Entities` | Slice of `Entity` currently **standing in this node** (denormalized list on the graph). |
| `NextNodes` | Outgoing edges: possible forward destinations from this room. |
| `PrevNode` | Set by `AddNext` to the **last** parent that linked this node (useful for debugging; not a full multi-parent representation — the graph is authored as a tree/DAG from code). |
| `IsBlocked` | If `true`, movement into this node from a parent is treated as blocked (e.g. **door closed**); `EntityTracker.MoveEntity` resets the entity toward its path start when the chosen next node is blocked. |

**Construction**

- `CreateInitialMap` builds the **player-one** side (stage → party → halls/closets → doors → office).
- `CreateP2Map` builds the **player-two** toy/facade side (facade → toy_stage / mangle → … → vents and center door → P2 office).
- `NewGameState` links graphs across players (`player_one_office` ↔ `rhs_vent`, `player_two_office` ↔ `lhs_door`) so the tracker can model cross-map connectivity.

**Lookup**

- `FindNodeByAlias(start, alias)` does DFS with a `visited` set (handles shared naming carefully only if the graph is acyclic from that root; cycles would need stronger semantics).

### `Entity` (character / animatronic record)

| Field | Meaning |
|-------|---------|
| `Id`, `Name` | Identity for logs and wire formats (camera check). |
| `AggressionMultiple`, `Position`, `TimeInRoom` | Reserved / future tuning (not central to current movement logic). |
| `PreferredNextIndex` | Chooses which `NextNodes[index]` to follow when moving; falls back to index `0` if out of range. |

### `EntityTracker`

- `EntityPositions map[int16]*GameGraphNode` — current node per entity id.
- `EntityPaths map[int16][]*GameGraphNode` — append-only path of nodes visited (supports `MoveBack` / `ResetToStart` when hitting a blocked door).

**`SpawnEntities`**

- P1 classic crew on `stage` / `party_room`; toy crew on P2 `toy_stage`.

### `GameState`

- Holds both graphs, the tracker, and convenient pointers (`LHSDoor`, `RHSDoor`, `P2Office`) for gameplay hooks.
- `Step(lobbyID string)` logs before/after positions and increments time + moves all entities once (intended to be called once per sim tick per lobby).
- `CheckEntityInPlayerRoom` — helper for “is this entity in a camera-visible room vs an office?” logic (for future rules/UI).

---

## Build system — challenges and how things fit together

### Two independent stacks

1. **Go server** — run from `server/` with Go 1.19+ (`go.mod`). Depends on **CGO** for SQLite (`github.com/mattn/go-sqlite3`): you need a C toolchain (`gcc`) on the build machine; cross-compiling with CGO is extra friction.
2. **C++ Web client** — CMake + Emscripten + raylib (`client_wasm/CMakeLists.txt`). Desktop builds can use system raylib or **FetchContent** to download raylib 5.5.

### `client_wasm` / Emscripten + raylib

- **Emscripten is mandatory** for the Web artifact path documented in `client_wasm/README.md` (`emcmake`, `emmake`, `PLATFORM=Web`).
- **FetchContent** pulls raylib from GitHub on first configure — slow, network-dependent, and sensitive to tag URLs.
- The CMake file forces **`GRAPHICS_API_OPENGL_ES3`** when `PLATFORM=Web` to avoid WebGL2 / VAO issues with GLES2 batching (comment in `CMakeLists.txt` explains the failure mode).
- Link flags bundle **Asyncify**, GLFW, WebSocket JS library, and **preload-file assets** — builds are sensitive to Emscripten version and flag ordering; debugging linker errors often means checking emsdk version vs raylib expectations.

### Go module naming

- `server/go.mod` declares `module main`, which is valid for a standalone binary package but is **non-idiomatic** for import paths and can confuse tooling if you later split packages — worth revisiting when the server grows.

### Operational gaps to be aware of

- **Simulation vs wall-clock**: `room.Time` is advanced in `runSimStepTicker`, while entity movement in `GameState.Step` is not wired to that ticker in the current snippet (commented). Until `Step` runs each tick in one place only, **camera checks can stay static** while night time still counts up.
- **Player identity**: lobbies key off `r.RemoteAddr` for host/acceptor — fine on a single machine, fragile behind proxies/NAT (same user, different addresses).

---

## Suggested next steps (optional)

- Reconcile **one authoritative sim step** per lobby per second (global ticker or host-only) and keep `room.Time` in sync with `GameState.Time` if desired.
- Add **`type: "action"`** door open/close on the server (if not present in your branch) and mirror helpers in `ws_init.hpp`.
- Harden WebSocket origin checks and bind lobby membership to authenticated user ids instead of raw `RemoteAddr`.

---

*This file is a snapshot of the repository structure and behavior described in source; re-run builds and grep for `Step(` / `action` when verifying sim and control features on your branch.*
