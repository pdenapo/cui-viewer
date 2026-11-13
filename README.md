# cuiviewer — native image viewer

A blazing-fast, lightweight, native image viewer inspired by the classic `zgv`.
The frontend is C99 on top of the [CUI](https://github.com/yappologistic/CUI)
framework; images are decoded with CUI's built-in stb_image integration (PNG
and JPEG). No web views, instant startup, tiny memory footprint.

Licensed under the GNU General Public License v3.0 (see `LICENSE`).

## Features

- File browser: lists the current directory, filters to PNG/JPEG, navigates
  with the arrow keys; `Enter` opens a picture or enters a directory,
  `Backspace`/`Esc` goes to the parent directory.
- Image viewer: opens a selected PNG/JPEG, displays it with `cui_image_viewer`,
  supports zoom in/out, fit, 100%, pan and keyboard shortcuts.
- Slideshow: view all supported images in the current directory (optionally
  recursively), in directory or random order, advancing after a configurable
  interval.
- Thumbnail cache: zgv-compatible `.xvpics` (or global) thumbnail creation,
  storage and recursive update.
- Light/dark theme and several languages (English, German, Spanish, French,
  Italian, Portuguese), switched in-app and persisted.
- Command palette (`Ctrl+K`), `Ctrl+T` to toggle theme.
- Settings stored as JSON in the application data directory and restored on
  startup.
- Starts maximized by default; maximized/windowed startup is configurable.

## Layout

```
cuiviewer/
├── include/
│   ├── settings.h            # settings struct + load/save/path helpers
│   └── thumbnails.h          # thumbnail cache paths, update and storage helpers
├── src/
│   ├── settings.c            # JSON settings file read/write
│   └── thumbnails.c          # zgv-compatible thumbnail cache implementation
├── source-frontend/
│   └── main.c                # CUI application, browser/viewer state, frame loop
├── vendor/
│   └── CUI/                  # CUI framework (submodule)
├── tests/
│   ├── test_settings.c       # settings path + JSON round trip tests
│   └── run_tests.sh
├── packaging/                # debian, rpm, arch packaging
├── scripts/                  # make_tarball.sh, release.sh
├── CMakeLists.txt            # pure C build
├── build_linux.sh
├── build_x11.sh
├── build_sdl2.sh
├── build_windows.bat
├── AGENTS.md
└── README.md
```

## Getting the code

```sh
git clone --recurse-submodules https://github.com/pdenapo/cuiviewer.git
```

## Prerequisites

### Linux (Arch)
```sh
sudo pacman -S base-devel cmake ninja glfw libglvnd libx11
```
### Linux (Debian / Ubuntu)
```sh
sudo apt-get install -y build-essential cmake ninja-build libglfw3-dev libgl-dev libx11-dev
```
### macOS
```sh
brew install cmake ninja
```
Xcode Command Line Tools are required for the compiler.

### Windows
- CMake + Ninja (`choco install cmake ninja`)
- Visual Studio Build Tools (MSVC) or MinGW

## Build

There are several native frontends (one per platform / display server). Pick the
one that matches your system.

| Platform | Recommended frontend | Build script | CMake flag |
|---|---|---|---|
| Linux (X11 or Wayland) | **GLFW** (native on both) | `./build_linux.sh` | `-DCUI_PLATFORM=glfw` |
| Linux (X11 only) | X11 | `./build_x11.sh` | `-DCUI_PLATFORM=x11` |
| Linux (SDL2) | SDL2 | `./build_sdl2.sh` | `-DCUI_PLATFORM=sdl2` |
| Windows | Win32 (or GLFW/SDL2) | `build_windows.bat` | `-DCUI_PLATFORM=win32` |
| macOS | Cocoa (or GLFW/SDL2) | `./build_macos.sh` | `-DCUI_PLATFORM=cocoa` |

On Linux we recommend **GLFW for everyone**: it speaks to X11 and Wayland
natively, so a single binary works on both without XWayland. Use X11/SDL2 only
if you have a specific reason.

```sh
# Linux (recommended)
./build_linux.sh
./build_linux/cuiviewer

# Windows (cmd)
build_windows.bat
build_windows\cuiviewer.exe

# macOS
./build_macos.sh
./build_macos/cuiviewer
```

Manual equivalent:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCUI_PLATFORM=glfw
cmake --build build -j 2
```

Standard workflow (X11 example):

```sh
cmake -S . -B build_x11 -DCMAKE_BUILD_TYPE=Release -DCUI_PLATFORM=x11
cmake --build build_x11 -j 2
./build_x11/cuiviewer
```

## Command line

```sh
cuiviewer [PATH...] [OPTIONS]

cuiviewer ~/Pictures
cuiviewer ~/Pictures --slideshow --interval 5 --shuffle --recursive
cuiviewer a.png b.jpg --slideshow --interval 2.5
cuiviewer --dir ~/Pictures --slideshow --interval 3 --order directory
```

See `AGENTS.md` section 4.1 for the full option list.

## Troubleshooting

### `XDG_RUNTIME_DIR is invalid or not set`

Some minimal desktop sessions (for example MATE launched over SSH) do not
export `XDG_RUNTIME_DIR`. cuiviewer falls back to `/tmp/xdg-runtime-<uid>`,
but you can also set it manually:

```sh
export XDG_RUNTIME_DIR=/run/user/$(id -u)
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
```

If you only need X11, build and run the X11 variant (`./build_x11.sh`) or
force the X11 backend with `GLFW_PLATFORM=x11 ./build_x11/cuiviewer`.

## Settings

Settings are stored as JSON in the application data directory:

- Linux: `$XDG_DATA_HOME/cuiviewer/settings.json` or
  `~/.local/share/cuiviewer/settings.json`
- Windows: `%APPDATA%\cuiviewer\settings.json`

## Packaging

Packaging files for Debian, RPM and Arch are under `packaging/`. See
`scripts/make_tarball.sh` and `scripts/release.sh` for release helpers.

## License

cuiviewer is licensed under the GNU General Public License v3.0 (see
`LICENSE`). Third-party components are listed in `THIRD_PARTY.md`.
The `zgv` project is referenced only as a behavioral inspiration for the
browser/viewer flow and the thumbnail cache layout; no zgv code is included
or compiled into cuiviewer.
