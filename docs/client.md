---
layout: page
title: Client
permalink: /client/
nav_order: 4
---

## Stack

- **raylib 5.x** for windowing, input, 3D models, audio, and shaders (CMake **FetchContent** when a system raylib is not found).
- **Emscripten** for the shipped target: WebGL 2 / GLES3, **ASYNCIFY**, preloaded **`assets/`**, and **Emscripten WebSockets** for gameplay.
- **nlohmann/json** (vendored under `client_wasm/include/nlohmann/`) for parsing server messages.

Entry point is **`main.cpp`**: allocates the WebSocket URL from the current page (night query, JWT from `localStorage`), initializes **`WsContext`** with a global **`GameState`**, and runs **`init_menu`** as the first **`Scene`**.

## Why mostly `.hpp` (header-only organization)

Almost all game code lives in **`.hpp`** files included from **`main.cpp`** (for example `menu.hpp`, `MainScene.hpp`, `ws_init.hpp`). There is **no `.cpp` per translation unit** for those modules—only **`main.cpp`** pulls them in.

This is an **organizational choice**, not a runtime architecture:

- **Compile time:** the compiler still sees one translation unit’s worth of code linked into the WASM module. Splitting into `.cpp` files would mainly change **build parallelism** and **incremental compile** behavior, not generated machine code quality by itself.
- **Runtime:** there is **no** “header-only library” penalty at play time. Templates and `inline` functions aside, non-inline functions defined in headers included in a **single** `.cpp` behave like ordinary definitions for that TU.
- **Tradeoffs:** faster to sketch and navigate for a small team; **clean rebuilds** can be slower as `MainScene.hpp` grows. If compile times become painful, hot headers can be moved to `.cpp` files with stable declarations in headers—without changing gameplay semantics.

## Major client components

| Unit | Role |
|------|------|
| `Scene.hpp` | Abstract **`Scene`**: `update(Scene*&, GameState&, socket)` and optional `listen()` |
| `menu.hpp` | Lobby UI, create/join, host start; **lazy `MainScene` construction** |
| `MainScene.hpp` | 3D office, doors, cameras, P2 UI, rendering; reads **`GameState`** updated from WS |
| `ws_init.hpp` | Socket open/onmessage; dispatches server JSON into **`GameState`** |
| `GameState.hpp` | Plain struct of mirrored server fields (power, time, flags, `sim_entities`, etc.) |

## Web build flags (CMake)

Notable linker settings for Web (see `client_wasm/CMakeLists.txt`):

- **OpenGL ES 3** for raylib on Web (VAO / batch compatibility).
- **`ASYNCIFY`** for blocking-style waits that Emscripten can legalize.
- **Large WASM stack** (`STACK_SIZE`) to avoid stack exhaustion during heavy scene construction under Asyncify.
- **Exported heap views** (`HEAPF32`, etc.) for audio paths that read WASM memory from JavaScript.

## Assets

Models and textures live under **`client_wasm/assets/`** and are packaged into **`game.data`** for the browser preload.

## Local WebSocket URL

`main.cpp` builds `ws://` or `wss://` against the current host, maps **emrun** port **6931** to **6789** for the Go server, and appends **`/echo?night=…`** plus **`&token=`** when a JWT exists.
