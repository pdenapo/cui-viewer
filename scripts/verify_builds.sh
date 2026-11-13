#!/bin/sh
# Pre-flight verification for all build targets that can be produced from the
# current host. Linux targets are built natively; the Windows/Win32 target is
# cross-compiled with Zig when zig is available.
#
# Usage:
#   scripts/verify_builds.sh            # build every available target
#   scripts/verify_builds.sh --linux     # only Linux backends
#   scripts/verify_builds.sh --windows   # only the Win32 backend (Zig)
set -u

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DO_LINUX=1
DO_WINDOWS=1

for arg in "$@"; do
    case "$arg" in
        --linux) DO_WINDOWS=0 ;;
        --windows) DO_LINUX=0 ;;
        --all) ;;
        -h|--help)
            sed -n '1,18p' "$0"
            exit 0
            ;;
        *)
            echo "unknown option: $arg" >&2
            exit 2
            ;;
    esac
done

if [ "$DO_LINUX" -eq 0 ] && [ "$DO_WINDOWS" -eq 0 ]; then
    DO_LINUX=1
    DO_WINDOWS=1
fi

FAILED=0
run_build() {
    name="$1"
    shift
    echo "===== $name ====="
    if "$@"; then
        echo "OK: $name"
    else
        echo "FAILED: $name" >&2
        FAILED=1
    fi
}

build_linux_backend() {
    backend="$1"
    build_dir="$2"
    log="/tmp/verify_${backend}.log"
    rm -rf "$build_dir" &&
    cmake -S "$ROOT_DIR" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DCUI_PLATFORM="$backend" >"$log" 2>&1 &&
    cmake --build "$build_dir" --parallel 2 >>"$log" 2>&1
}

if [ "$DO_LINUX" -eq 1 ]; then
    for backend in x11 glfw sdl2; do
        build_dir="${ROOT_DIR}/build_${backend}_verify"
        run_build "linux/${backend}" build_linux_backend "$backend" "$build_dir"
    done
fi

if [ "$DO_WINDOWS" -eq 1 ]; then
    if command -v zig >/dev/null 2>&1; then
        run_build "windows/win32 (zig)" "${ROOT_DIR}/build_windows_zig.sh"
    else
        echo "SKIP: windows/win32 (zig) - zig not found" >&2
    fi
fi

if [ "$FAILED" -ne 0 ]; then
    echo "One or more verification builds failed." >&2
    exit 1
fi

echo "All requested verification builds succeeded."
