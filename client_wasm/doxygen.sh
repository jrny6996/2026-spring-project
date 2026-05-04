#!/usr/bin/env bash
# Run from this directory: ./doxygen.sh
set -euo pipefail
cd "$(dirname "$0")"
if ! command -v doxygen >/dev/null 2>&1; then
  echo "doxygen: not installed."
  echo "  Debian/Ubuntu: sudo apt install doxygen"
  echo "  Fedora:        sudo dnf install doxygen"
  echo "  macOS:         brew install doxygen"
  echo "  Or see:        https://www.doxygen.nl/download.html"
  exit 127
fi
exec doxygen Doxyfile
