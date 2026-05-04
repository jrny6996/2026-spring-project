---
layout: page
title: Economy
permalink: /economy/
nav_order: 7
---

## Design goal

**Power** and the **music box** are shared, server-owned resources. Players influence them through **queued actions** drained on a fixed **sim tick**, so all clients stay in lockstep with the authoritative clock.

## Tick constants (`server/main.go`)

- **`gameSecondsPerTick`** — In-game seconds advanced each tick (currently **5**), matched to the real ticker interval so wall clock and night clock stay aligned.
- **`nightLenInMinutes`** — Baseline **6** in-game minutes for win-time math; higher nights shorten survival time via **`winTimeGameSeconds`** (see server comments).
- **`powerDrainPerClosedDoorPerTick`** — Shared power drops **per closed door per tick** (currently **1** per tick per door).

## Power

- **Baseline:** `GameRoomState.Power` starts at **30** on new room state (see `newGameRoomState`).
- **Drain:** Each tick, if **left** and/or **right** office doors are down, power decreases by the per-door rate.
- **Loss:** If power reaches **0**, **`handleLobbyLose`** runs for the lobby (`"power_out"`).

## Generator queues (P1 and P2)

Charging is **not** applied instantly when the client sends an action. Instead:

- **P1** sends **`p1q:N`** (`action` message); the server adds **`N`** to **`p1PowerQueued`** (capped).
- **P2** sends **`p2qp:N`** for power and **`p2qm:N`** for music wind contribution (separate caps).

On each tick, **`applyChargeEconomyTick`**:

1. Drains **`p1PowerQueued`** into **`Power`** (then zeros the queue).
2. If P2 is still active (**not** lost and mask not blocking economy path per server rules), drains **`p2PowerQueued`** into **`Power`** and converts **`p2MusicQueued`** into **music box wind** in discrete steps, respecting **`musicBoxWindMax`**.
3. If P2 is down or masked out of that path, P2 queues are cleared without benefit.
4. **Music wind decays** by **`gameSeconds`** for this tick (wind is reduced toward zero over time).
5. **`Power`** is clamped to **100** maximum.

So the economy is **discrete-tick**: doors cost every tick; queues flush once per tick; wind decays every tick.

## Music box and P2 elimination

- Wind is bounded **[0, `musicBoxWindMax`]** with a configurable starting baseline for design tuning (see server constants **`musicBoxStartingWind`**, **`musicBoxWindPerQueueUnit`**).
- If wind is **≤ 0**, a **live** player-two WebSocket is present, and P2 has not already lost, the server triggers **P2 loss** (`"music_box"` path)—the puppet mechanic.

## Client helpers

**`ws_init.hpp`** documents the string formats for **`send_p1_power_queued`**, **`send_p2_power_queued`**, **`send_p2_music_queued`**, **`send_door_state`**, and **`send_mask_state`**, keeping the browser and server contracts in one place.
