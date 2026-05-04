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

mkdir -p build
cd build
emcmake cmake .. -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXECUTABLE_SUFFIX=".html"
emmake make

WASM_DEST="$SCRIPT_DIR/../server/wasmdist"
mkdir -p "$WASM_DEST"
for f in game.html game.js game.wasm game.data; do
  if [[ -f "$f" ]]; then
    cp "$f" "$WASM_DEST/"
  fi
done

echo "Copied build artifacts to $WASM_DEST — use Go server at :6789 /game/ (not emrun) for ~300MB game.data."

if [[ "${EMRUN:-}" == "1" ]]; then
  echo "warning: EMRUN=1 can fail on macOS (Errno 55) with large game.data; prefer Go server + /game/." >&2
  exec emrun --port 6931 game.html
fi
