---
layout: page
title: Build
permalink: /build/
nav_order: 3
---

## Overview

The project has three moving parts: the **WASM client** (`client_wasm/`), the **Go server** (`server/`), and optional **Docker** / **npm** glue at the repository root.

## Prerequisites

- **Client (Web):** [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html), CMake 3.11+, and a C++20 toolchain visible to Emscripten.
- **Server:** Go 1.19+ (see `server/go.mod`). Production builds use **CGO** with SQLite (`github.com/mattn/go-sqlite3`), so a C compiler is required for `go build`.

## Web client (Emscripten)

From `client_wasm/`:

```bash
./run.sh
```

This script:

1. Configures CMake with `-DPLATFORM=Web`, Release, and `.html` output.
2. Runs `emmake make`.
3. Copies `game.html`, `game.js`, `game.wasm`, and `game.data` into `server/wasmdist/`.

Equivalent manual steps are described in `client_wasm/README.md` (`emcmake` / `emmake`).

## Go server

From `server/`:

```bash
go run .
```

Default listen address is `:6789` (override with `HTTP_ADDR`). The server expects WASM artifacts under `server/wasmdist/` (especially `wasmdist/game.html`). If they are missing, it logs a hint and still starts.

## Root npm scripts

From the repository root:

- **`npm run dev`** — runs the WASM build script and the Go server concurrently (see `package.json`).
- **`npm run sync`** — copies a pre-built `client_wasm/build/` output into `server/wasmdist/` without rebuilding.

Use `sync` when you already built in `client_wasm/build` and only need to refresh what the server serves.

## Docker

`Dockerfile` multi-stage build:

1. **Builder:** `golang:1.22-bookworm`, installs `gcc` for CGO, copies `server/`, runs `go build` with `CGO_ENABLED=1`, prepares `wasmdist`.
2. **Runtime:** `debian:bookworm-slim` with `ca-certificates` and `libsqlite3-0`; copies the binary, HTML shells, and `wasmdist`.

`docker-compose.yml` runs the app on port **6789** (internal) and can run **cloudflared** beside it for tunneling. Set `JWT_SECRET` (32+ characters in production), `HTTP_ADDR`, and `DATABASE_PATH` as needed.

## C++ API reference (Doxygen)

The WASM client under `client_wasm/` is documented with **[Doxygen](https://www.doxygen.nl/)**.

Install **Doxygen** (e.g. Debian/Ubuntu: `sudo apt install doxygen`). From `client_wasm/`:

```bash
./doxygen.sh
# or: doxygen Doxyfile
```

HTML is written to **`client_wasm/doxygen-html/`** (gitignored). Open `doxygen-html/index.html` in a browser.

Configuration is in **`client_wasm/Doxyfile`**. Third-party **nlohmann/json** under `include/nlohmann/` is excluded. `EXTRACT_ALL = YES` is on so classes and functions appear even before you add `///` comments; tighten that later with `WARN_IF_UNDOCUMENTED = YES` once you want stricter coverage.

## GitHub Pages (this documentation + C++ HTML)

Architecture Markdown lives in `docs/` and is built with Jekyll via GitHub Actions (`.github/workflows/jekyll-gh-pages.yml`). Jekyll output and Doxygen HTML are placed under **`_site/docs/`** so everything is served from the **`/docs`** path on GitHub Pages:

- Jekyll pages: **`/2026-spring-project/docs/`** (e.g. `/2026-spring-project/docs/build/`)
- C++ API: **`/2026-spring-project/docs/cpp/`**

A minimal **`_site/index.html`** redirects the repo site root to **`docs/`**.

`baseurl` in `docs/_config.yml` must stay **`/2026-spring-project/docs`** (change the repo slug if yours differs).

After enabling **GitHub Pages** from GitHub Actions in the repository settings, pushes to the default branch deploy to:

`https://jrny6996.github.io/2026-spring-project/docs/`
