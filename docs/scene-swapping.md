---
layout: page
title: Scene swapping
permalink: /scene-swapping/
nav_order: 8
---

## Pattern

The WASM main loop (**`main.cpp`**) holds a single pointer **`curr_scene`** to a **`Scene`** subclass. Each frame:

1. **`curr_scene->listen()`** — e.g. menu clipboard paste handling.
2. **`curr_scene->update(curr_scene, game_state, socket)`** — may **reassign `curr_scene`** to switch scenes.

There is no separate scene stack in code today: transition is a **plain pointer swap**.

## `Scene` API

**`Scene.hpp`** defines:

- **`update(Scene*& curr_scene, GameState& state, EMSCRIPTEN_WEBSOCKET_T& socket)`** — default no-op; subclasses override.
- **`listen()`** — optional per-frame input that should run even if drawing differs.

**`ThreeDScene`** exists as a thin typed wrapper over a 3D camera; **`MainScene`** ultimately drives the first-person office experience.

## Menu → main flow

**`menu.hpp`** implements the handoff:

1. On first **`update`** when **`main_scene` is null**, **`boot_load_main_scene`** runs: one **`BeginDrawing` / `EndDrawing`** frame with a **“loading assets”** message, then **`new MainScene(camera)`**. This keeps heavy GLB loading **off** the initial frame before the menu has shown meaningful UI, while still giving user feedback.
2. On subsequent frames, if **`state.gameStarted && state.has_player_slot`**, the menu sets **`curr_scene = main_scene`** and returns. **`MainScene::update`** then runs forever (until you add more scenes).
3. Until the game starts, the menu draws **welcome** (create / join) or **in-lobby** UI and sends WebSocket messages via **`ws::`** helpers.

## Why not destroy the menu?

The menu keeps a **`MainScene* main_scene`** member after construction. After swapping **`curr_scene`**, the **`Menu`** object is **not deleted** in current code: it remains allocated so the pointer to **`MainScene`** stays valid. A future refactor could use **`std::unique_ptr<Scene>`** and explicit ownership transfer if you add return-to-menu flows.

## Adding new scenes

1. Subclass **`Scene`** (or **`ThreeDScene`**).
2. Construct it from the scene that should hand off (or from a factory).
3. In **`update`**, assign **`curr_scene = next`** when conditions match **`GameState`** / input / server messages.

Keep **WebSocket callbacks** writing only into **`GameState`** so any scene can read the latest server truth without duplicating network logic.
