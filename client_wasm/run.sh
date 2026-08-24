#!/usr/bin/env bash
set -euo pipefail

# Builds WASM + packs assets into game.data (~300MB). Prefer the Go server to play:
#   cd ../server && go run .
#   open http://localhost:6789/game/game.html?night=1
#
# emrun (EMRUN=1) uses Python's http.server and often hits macOS "No buffer space available"
# (Errno 55) when serving huge single-file responses — use the Go static route instead.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE:-.}")" && pwd)"
cd "$SCRIPT_DIR"
BUILD_TYPE="${BUILD_TYPE:-Release}"

wasm_magic_ok() {
  local path="$1"
  [[ -f "$path" ]] || return 1
  local bytes
  bytes="$(od -An -t x1 -N 4 "$path" 2>/dev/null | tr -d '[:space:]')"
  [[ "$bytes" == "0061736d" ]]
}

copy_atomic() {
  local src="$1"
  local dest="$2"
  local tmp="${dest}.tmp.$$"
  cp "$src" "$tmp"
  mv -f "$tmp" "$dest"
}

mkdir -p build
cd build
emcmake cmake .. -DPLATFORM=Web -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" -DCMAKE_EXECUTABLE_SUFFIX=".html"
emmake make

if ! wasm_magic_ok "game.wasm"; then
  echo "error: build produced invalid wasm magic header in client_wasm/build/game.wasm" >&2
  exit 1
fi

WASM_DEST="$SCRIPT_DIR/../server/wasmdist"
mkdir -p "$WASM_DEST"
for f in game.html game.js game.wasm game.data; do
  if [[ -f "$f" ]]; then
    copy_atomic "$f" "$WASM_DEST/$f"
  fi
done

if ! wasm_magic_ok "$WASM_DEST/game.wasm"; then
  echo "error: copied wasm is invalid at $WASM_DEST/game.wasm (copy aborted)" >&2
  exit 1
fi

echo "Copied build artifacts to $WASM_DEST — use Go server at :6789 /game/ (not emrun) for ~300MB game.data."

if [[ "${EMRUN:-}" == "1" ]]; then
  echo "warning: EMRUN=1 can fail on macOS (Errno 55) with large game.data; prefer Go server + /game/." >&2
  exec emrun --port 6931 game.html
fi
