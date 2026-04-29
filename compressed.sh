#!/usr/bin/env bash

set -e

SRC_DIR="client_wasm/assets-uncompressed"
DST_DIR="client_wasm/assets"

echo "Cleaning output directory..."

mkdir -p "$DST_DIR"

echo "Compressing GLB assets..."

find "$SRC_DIR" -type f -name "*.glb" | while read -r file; do
    # Compute relative path
    rel_path="${file#$SRC_DIR/}"
    out_file="$DST_DIR/$rel_path"

    # Ensure destination directory exists
    mkdir -p "$(dirname "$out_file")"

    echo "Processing $rel_path"

    # Run a chain of optimizations
    gltf-transform prune "$file" "$out_file.tmp.glb"
    gltf-transform dedup "$out_file.tmp.glb" "$out_file.tmp2.glb"
    gltf-transform draco "$out_file.tmp2.glb" "$out_file.tmp3.glb"
   gltf-transform resize --width 1024 --height 1024 "$out_file.tmp3.glb" "$out_file"

    # Clean temp files
    rm "$out_file.tmp.glb" "$out_file.tmp2.glb" "$out_file.tmp3.glb"
done

echo "Done!"