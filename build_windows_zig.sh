#!/bin/sh
# Cross-compile the Windows (Win32) backend from a Linux host using Zig.
# Requirements: zig, cmake, ninja.
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="${ROOT_DIR}/build_windows_zig"
CC="${ROOT_DIR}/scripts/zig-cc-windows"

rm -rf "${BUILD_DIR}"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCUI_PLATFORM=win32 \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
cmake --build "${BUILD_DIR}" --parallel 2

echo "Build OK: ${BUILD_DIR}/cuiviewer.exe"
