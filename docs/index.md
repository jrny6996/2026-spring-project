---
layout: home
title: Home
nav_order: 1
---
## Architecture
### Requirements
Within the scope of the project outline, this repo needs to support real-time 3D multiplayer gameplay delivered entirely through the browser. To accomplish this, we need:
* Some form of client-side rendering
* A way of sending communication between clients
* A centralized way of communicating state changes to clients
* A client-side request and response system for these state messages

### Rendering
Due to modern browser advances, browsers have become capable of running code beyond the traditional web stack (HTML, CSS, and JavaScript) through the power of WebAssembly (WASM). WASM provides a sandboxed compilation target that can be compiled to from a variety of languages (Rust, C, C++, Go, and more). This offers many advantages, primarily performance and an expansion of possibilities not previously available on the web.

In addition, JavaScript (JS) has gained many new libraries and frameworks that assist in the development of 3D games in a browser-first context, notably Three.js and Babylon.js. JS has many advantages on the web, as it's the only target that can manipulate DOM nodes directly. However, in the context of a 3D game rendered to a canvas, this happens very little. Likewise, I knew that at some point this game would need shaders, and JS implementations rely on string interpolation, which does not provide great IntelliSense or other modern IDE support. These limitations, along with the performance improvements of some WASM targets and better IDE support for shaders, pushed me toward that direction.

Within a WASM context, the browser can still execute JavaScript on the page—in fact, that’s how we load our WASM and bind it to the canvas. With JS already running, I felt that shipping another garbage-collected language could lead to unpredictable performance characteristics that would be difficult to trace. Additionally, the lifetime of most objects in this game spans the entire duration of gameplay. While each language offers a variety of pros and cons, C++ was selected as the target source language due to its strong ecosystem for game libraries (specifically raylib), support for multiple programming paradigms, and most importantly, performance and manual memory management.

raylib has bindings in a variety of languages and provides an abstraction over OpenGL and WebGL. Rendering requires three separate components:
1. Drawing 3D models at their X, Y, and Z coordinates  
2. A camera with perspective, XYZ coordinates, and a target to look at  
3. A loop to update the camera and models, then clear and redraw the scene  

While the native OpenGL target compiled immediately, WebGL introduced several visual artifacts on textures. These were eventually resolved by adding CMake flags to specify the WebGL version and including default raylib textures in the main scene for compatibility.

### Communication
Now that we have a way of drawing to the screen, we need a way to send communication from the client to the server, as well as from the server to the client, without requiring additional user input. To handle this, I chose to use a WebSocket connection, which provides a **full-duplex communication** channel over TCP.

For some games, TCP would not be appropriate, and UDP via WebRTC would be preferred for lower latency. However, because our state updates occur every few seconds rather than multiple times per second, WebSockets are perfectly suitable. If more networking performance were required, Protocol Buffers (Protobufs)—a binary serialization format—would offer better performance than JSON. I chose JSON for its simplicity and compatibility.

Go has a very simple WebSocket library, as well as an HTTP server in its standard library. Combined with its speed, hot reloading, and static typing, this makes it easy to build something reliable quickly.

### Game State Changes
Our Go server acts as the source of truth between clients and broadcasts state changes over existing WebSocket connections. We track lobbies using an in-memory map and periodically call a function to update game state.

Entities are represented using a doubly linked list, where each node explicitly references the next and previous rooms. Based on the `"type"` field in the JSON messages, we map handlers on both the client and server. For example, there is a "door close" type that player one uses for defense, as well as server message types for updates, pings, and other game-specific events.

We also track time during each tick. Players win when a time value in their game state exceeds a specified number of minutes.

Overall, the client maintains very little state—primarily animation frame timing—and mainly reflects the server state, reconstructing the 3D scene from minimal data.

The concept of levels is implemented using formulas applied to each entity. The night number acts as a variable (`X`) in these formulas. As the game approaches night 7, the probability of entity movement increases toward a constant threshold. This system also allows restricting certain entities to appear only after a specific night (e.g., one appears starting on night 2). Players lose if an entity reaches their node in the linked list without them performing the appropriate defensive action.

### Client-Side Messages
I designed the game so that player one and player two have different protection mechanics and must eventually support each other as levels progress. This is implemented through an economy system: player one consumes power, while player two can generate it. To balance single-player and multiplayer modes, player two has a faster production rate when assisting player one.

Player one uses power as a defensive resource, while player two manages a separate economy involving a music box, which complements their defensive tool (a mask).

Clients send these messages to the server, and based on each tick, the server processes economy updates or determines loss conditions.

## Docs

| Topic | What you will find |
|--------|---------------------|
| [C++ API (Doxygen)]({{ site.baseurl }}/cpp/) | Generated reference for `client_wasm/` (classes, files, source browser) |
| [Build]({{ site.baseurl }}/build/) | Local dev, WASM build, Docker, syncing artifacts into the server |
| [Server]({{ site.baseurl }}/server/) | HTTP routes, WebSocket protocol, sim ticker, auth, persistence |
| [Client]({{ site.baseurl }}/client/) | raylib + Emscripten, header-only style, WebSocket helpers |
| [Game state]({{ site.baseurl }}/game-state/) | Server `GameRoomState` / sim vs client `GameState`, JSON wire model |
| [Economy]({{ site.baseurl }}/economy/) | Power, doors, P2 music box, queues drained per tick |
| [Scene swapping]({{ site.baseurl }}/scene-swapping/) | `Scene` base class, menu → main handoff, loading strategy |

## Source repository

[github.com/jrny6996/2026-spring-project](https://github.com/jrny6996/2026-spring-project)

## Progress notes
[April 16 on GitHub](https://github.com/jrny6996/2026-spring-project/blob/master/progress/apr_16.md)