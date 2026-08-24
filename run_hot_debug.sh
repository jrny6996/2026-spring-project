#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT_DIR="$ROOT_DIR/client_wasm"
SERVER_DIR="$ROOT_DIR/server"
WASM_DEST="$SERVER_DIR/wasmdist"

build_client() {
  echo "[hot-debug] building Emscripten client (Debug) and syncing to server/wasmdist..."
  (
    cd "$CLIENT_DIR"
    BUILD_TYPE=Debug ./run.sh
  )
  echo "[hot-debug] client debug build synced to $WASM_DEST"
}

run_server() {
  echo "[hot-debug] starting Go server on http://localhost:6789"
  (
    cd "$SERVER_DIR"
    WASMDIST="$WASM_DEST" go run .
  )
}

watch_client_changes() {
  echo "[hot-debug] watching client files for changes..."
  while true; do
    inotifywait -q -r \
      -e modify -e create -e delete -e move \
      --exclude '(^|/)(build|build-debug|\.git|doxygen-html)(/|$)' \
      "$CLIENT_DIR" >/dev/null
    build_client || echo "[hot-debug] rebuild failed; waiting for next change..."
  done
}

build_client

if command -v inotifywait >/dev/null 2>&1; then
  watch_client_changes &
  WATCH_PID=$!
  trap 'kill "$WATCH_PID" 2>/dev/null || true' EXIT INT TERM
else
  echo "[hot-debug] inotifywait not found; running one-time build before server start."
fi

run_server
