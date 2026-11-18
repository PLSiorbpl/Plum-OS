#!/usr/bin/env bash
set -Eeuo pipefail

BUILD_DIR="../build"
TARGET="${1:-run}"

echo "================================"
echo "            Plum-OS             "
echo "================================"

if [[ ! -f "$BUILD_DIR/Makefile" ]]; then
    echo "[*] Configuring CMake..."
    cmake -S .. -B "$BUILD_DIR" -G "Unix Makefiles"
fi

echo "[*] Building target: $TARGET"

cmake --build "$BUILD_DIR" --target "$TARGET" --parallel "$(nproc)"