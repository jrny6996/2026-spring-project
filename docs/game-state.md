---
layout: page
title: Game state
permalink: /game-state/
nav_order: 6
---

## Two “game states” (by design)

The codebase uses the same conceptual name at different layers:

### Server: `GameRoomState` + `GameState` (sim)

In **`server/main.go`**, **`GameRoomState`** is the lobby row: night clock, **`Power`**, **`MusicBoxWind`**, door booleans, P2 mask, **`P2Lost`**, **`P2OfficeDanger`**, one-shot **`DoorPound*`** cues, **`NightNum`**, acceptance fields, and pointers into the live sim.

The pointer field **`Sim *GameState`** (in `server/game.go`) is the **animatronic graph + tracker**: rooms, edges, blocked doors, entity positions, and **`Step`** logic. It is created when a night **starts**, not at lobby creation.

### Client: `GameState` class

**`client_wasm/GameState.hpp`** is a **mirror** of what the UI needs from JSON: lobby id, **`gameStarted`**, **`gameTime`**, **`night_end_game_sec`**, role flags (**`is_player_one`**, **`p2_in_lobby`**, **`p2_lost`**), power/music, door-pound flags, camera-check bookkeeping, **`sim_entities`**, and menu fields (**`menu_error`**, **`menu_creating_lobby`**, etc.).

**`ws_init.hpp`** parses incoming **`state`**, **`checkCamera`**, **`status`**, and errors, and mutates this struct. **`MainScene`** and **`Menu`** read it; they do not run the sim locally.

## Wire model (`state` message)

The server sends **`type: "state"`** with **`data`** shaped like **`gameStateWire`**: embedded lobby fields plus **`isPlayerOne`**, **`p2InLobby`**, and filtered **`simEntities`** (each with `entityId`, `name`, `roomAlias`).

Important derived fields:

- **`nightEndGameSec`** — Survive-to-win duration in **in-game seconds** for the HUD clock (12 AM → 6 AM mapped linearly over `[0, nightEndGameSec]`).
- **`time`** — Cumulative in-game seconds advanced in **`gameSecondsPerTick`** steps each server tick.

## Fog of war

**`simEntities`** is not always the full sim: the server applies **`filterSimEntitiesForFog`** using the client’s last **`view`** message. Offices, both office doors, and certain halls are **always** included; other rooms only appear when the client’s feed matches that **`roomAlias`**.

## Camera check vs live feed

**`check`** messages return a **`checkCamera`** (or error) payload used for HUD / one-shot entity lists, while periodic **`state`** updates carry **`sim_entities`** for continuous awareness subject to fog rules.
