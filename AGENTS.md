# Project Specification: Native CUI Image Viewer (`cuiviewer`)

## 1. Overview & Goals

Build a lightweight, native image viewer for Linux (and other desktop platforms supported by CUI) with an experience inspired by the classic `zgv`: a keyboard-driven file browser where the arrow keys move through entries, `Enter` opens a picture or enters a directory, and the viewer supports zooming and panning. The application must feel instant, with a very low memory footprint and no web-view or heavy runtime.

- **Frontend:** C99 using the [CUI](https://github.com/yappologistic/CUI) framework, specifically leveraging native components such as `cui_file_browser`, `cui_image_viewer`, tables, menus, toasts and the command palette.
- **Image decoding:** PNG and JPEG through CUI's built-in `stb_image` integration (`cui_image_load`). No separate image library is required.
- **Persistence:** plain JSON settings file in the application data directory. No SQLite, no database, no networking library.
- **Core philosophy:** zero bloatware, instant startup (< 10 ms), low memory footprint, zero web-view dependencies, pure native compilation.
- **UI features:** light/dark theme and several languages (English, German, Spanish, French, Italian, Portuguese), switched in-app and persisted in the settings file. All documentation, code comments and identifier names are written in English.

The old `zgv` source tree in `zgv-5.9/` is the behavioral reference for the browser/viewer flow. The `cui-http` project is the architectural and coding reference (C99 + CUI, settings restore, theme/language switching, build scripts).

---

## 2. Non-Goals

- No database and no SQLite dependency.
- No HTTP/networking; this is a local file viewer.
- No image editing, tagging or copy/move/delete file management in the first version. Slideshows and the zgv-style thumbnail cache are supported; other file-management features may be added later but must not complicate the core viewer.
- No custom image decoder: rely on CUI/stb_image for PNG and JPEG.

---

## 3. Directory Layout

```text
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
├── CMakeLists.txt            # pure C build
├── build_linux.sh
├── build_x11.sh
├── build_sdl2.sh
├── build_macos.sh
├── build_windows.bat
├── AGENTS.md
└── README.md
```

### Notes about the build

- The whole project is pure C99. There is no backend process, no FFI bridge and no database.
- `include/settings.h` is the single source of truth for the settings struct and settings functions.
- Strings returned by settings helpers are `malloc`'ed and must be released with `free()`.
- Image textures are owned by the frontend and must be destroyed with `cui_image_destroy()` when closing an image or quitting.
- The CUI framework is built as a subdirectory through CMake, exactly as in `cui-http`.

---

## 4. Build Instructions

```sh
./build_linux.sh        # recommended on Linux (GLFW: X11 + Wayland)
./build_x11.sh          # X11 only
./build_sdl2.sh         # SDL2
./build_macos.sh        # macOS (Cocoa)
build_windows.bat       # Windows (Win32)
```

Standard CMake workflow:

```sh
cmake -S . -B build_x11 -DCMAKE_BUILD_TYPE=Release -DCUI_PLATFORM=x11
cmake --build build_x11 -j 2
./build_x11/cuiviewer
```

CMake steps:

1. The CUI framework is built as a subdirectory (`add_subdirectory(vendor/CUI)`).
2. `cuiviewer` is built from `source-frontend/main.c` and `src/settings.c`.
3. `cuiviewer_test` is built from `tests/test_settings.c` and `src/settings.c`.
4. No SQLite, CURL or pthread dependency is required.

---

## 4.1 Command Line Interface

The GUI can be launched with options that configure or immediately start a slideshow. Options may be given in any order. Paths may be a directory or one or more image files.

```sh
cuiviewer [PATH...] [OPTIONS]

cuiviewer ~/Pictures
cuiviewer ~/Pictures --slideshow --interval 5 --shuffle --recursive
cuiviewer a.png b.jpg --slideshow --interval 2.5
cuiviewer --dir ~/Pictures --slideshow --interval 3 --order directory
```

Supported options:

| Option | Argument | Default | Description |
|---|---|---|---|
| `--dir`, `--directory` | `PATH` | user's pictures/home directory | Starting directory for the browser and slideshow |
| `--slideshow`, `--slide-show` | none | off | Start in slideshow mode instead of the browser |
| `--interval`, `--delay` | `SECONDS` | `5.0` | Time between images, in seconds; minimum `0.5` |
| `--shuffle`, `--random` | none | off | Randomize image order |
| `--order` | `directory` or `random` | `directory` | Explicit image ordering |
| `--recursive`, `-R` | none | off | Include images from subdirectories |
| `--theme` | `light` or `dark` | persisted setting | Override theme for this session |
| `--lang`, `--language` | `en`, `de`, `es`, `fr`, `it`, `pt` | persisted setting | Override UI language for this session |
| `--fullscreen` | none | off | Start fullscreen |
| `--maximized` | none | persisted setting | Start maximized |
| `--windowed`, `--no-maximized` | none | off | Start windowed |
| `--thumbnail-location` | `per-directory`, `global` or `none` | persisted setting | Override thumbnail cache location for this session |
| `--no-thumbnails` | none | off | Disable thumbnail cache for this session |
| `--help`, `-h` | none | off | Print usage and exit |
| `--version` | none | off | Print version and exit |

Rules:

- Command line values override persisted settings for that session only; they are not automatically saved unless the user changes them in the UI.
- If both `--shuffle` and `--order` are given, the last one wins.
- If `PATH` arguments are image files, they form the slideshow list directly and `--recursive` is ignored.
- If a directory and `--recursive` are given, collect supported images from that directory and all subdirectories.
- Invalid option values are reported on stderr and the application exits with a non-zero status.
- Slideshow settings changed in the UI are persisted as defaults for future launches.

## 5. Application Features

- **File browser:** lists the current directory, supports folders and image files, filters to PNG/JPEG by default, navigates with the arrow keys, `Enter` and `Backspace`/`Esc`.
- **Image viewer:** opens a selected PNG/JPEG, displays it with `cui_image_viewer`, supports zoom in/out, fit, 100%, pan and keyboard shortcuts provided by CUI.
- **Formats:** PNG and JPEG (and any additional format enabled in CUI).
- **Theme:** light and dark mode, toggled from the top bar and persisted.
- **Languages:** English, German, Spanish, French, Italian and Portuguese, switched from the top bar and persisted.
- **Settings:** stored as JSON in the application data directory and restored on startup.
- **Window:** starts maximized by default; maximized/windowed startup is configurable by the user and command line.
- **Command palette:** `Ctrl+K` for open image, browser, slideshow, theme toggle, language and quit.
- **Slideshow:** view all supported images in the current directory, optionally including subdirectories, in directory order or shuffled, advancing automatically after a configurable interval.
- **Thumbnail cache:** zgv-compatible thumbnail creation, storage, recursive update and fallback storage, so directories can show image previews without decoding full images every time.
- **Keyboard first:** all common actions have keyboard shortcuts; the mouse is optional.

---

## 6. zgv-Inspired User Flow

The interaction model mirrors `zgv`'s file selector and viewer, adapted to a modern desktop UI:

1. The application starts in the file browser, showing the last used directory or the user's home/pictures directory.
2. The arrow keys move the selection. `Page Up`/`Page Down`, `Home` and `End` move by page or to the ends of the list.
3. `Enter` on a directory makes it the current directory.
4. `Enter` on a PNG/JPEG file opens the image viewer.
5. `Backspace` or `Esc` in the browser goes to the parent directory.
6. In the viewer, `Esc` or `Backspace` returns to the file browser at the same directory and selection.
7. In the viewer, `+`/`-` zoom, `0` fits, `1` shows actual pixels, arrow keys pan. Mouse wheel zooms around the pointer and dragging pans, as provided by `cui_image_viewer`.
8. Theme and language changes are applied immediately and saved.
9. Slideshow can be started from the browser, command palette or command line. It uses the current directory's supported images, optionally recursively, in directory or random order, advancing after the configured interval. Slideshow controls pause/resume, next/previous, shuffle toggle and interval adjustment.

---

## 7. Keyboard Map

| Context | Key | Action |
|---|---|---|
| Browser | Up/Down | Move selection |
| Browser | Left/Right | Move selection (when list is single column) or parent/child navigation |
| Browser | Enter | Open selected directory or image |
| Browser | Backspace | Parent directory |
| Browser | Esc | Parent directory / quit when at root (configurable) |
| Browser | Ctrl+K | Command palette |
| Browser | u | Update thumbnails in current directory |
| Browser | Alt+u | Recursive thumbnail update |
| Browser | d | Create directory thumbnails |
| Browser | Esc | Abort thumbnail update |
| Viewer | Esc / Backspace | Return to browser |
| Viewer | + / = | Zoom in |
| Viewer | - | Zoom out |
| Viewer | 0 | Fit image to view |
| Viewer | 1 | Actual pixels (100%) |
| Viewer | Arrow keys | Pan |
| Viewer | Ctrl+K | Command palette |
| Global | Ctrl+K | Command palette |
| Global | Ctrl+T | Toggle theme |

---

## 7.1 Slideshow

The slideshow reuses the image viewer and advances through a list of supported images.

Behavior:

- The list is built from the current browser directory, or from explicit file arguments.
- With recursion enabled, subdirectories are traversed in directory order and their images are inserted in the same order.
- Directory order: files sorted by path/name; directories visited in sorted order.
- Random order: shuffle the collected list once when the slideshow starts; reshuffle when requested.
- The interval is stored in seconds with a minimum of `0.5` seconds.
- Advancing uses a monotonic timer; pause stops the timer without resetting the current image.
- If an image fails to load, show a toast and skip to the next image after a short delay.
- At the end of the list, wrap to the first image by default. A setting may change this to stop.
- The browser selection follows the current slideshow image when returning to the browser.

Slideshow controls:

| Context | Key | Action |
|---|---|---|
| Slideshow | Space | Pause/resume |
| Slideshow | Right / Page Down / N | Next image |
| Slideshow | Left / Page Up / P | Previous image |
| Slideshow | Up | Increase interval |
| Slideshow | Down | Decrease interval |
| Slideshow | S | Toggle shuffle and rebuild remaining order |
| Slideshow | R | Toggle recursive collection |
| Slideshow | Esc | Exit slideshow and return to browser |
| Slideshow | F11 / F | Toggle fullscreen |

UI controls:

- Play/pause button.
- Previous/next buttons.
- Interval selector (seconds).
- Shuffle toggle.
- Recursive toggle.
- Wrap-around toggle.
- Progress indicator: current index and total count.

## 7.2 Thumbnail Cache (zgv Model)

The thumbnail cache follows zgv's model so that existing zgv/xv/GIMP-compatible thumbnails can be reused and updated consistently.

Storage model:

The cache location is configurable through `thumbnails.location`:

- `per-directory`: store thumbnails in a `.xvpics` subdirectory beside the source image, using the same filename as the source image. This is the zgv default and keeps thumbnails portable with the image directory.
- `global`: store all thumbnails in one central cache under the user's cache directory:
  - Linux: `$XDG_CACHE_HOME/cuiviewer/thumbnails/` or `~/.cache/cuiviewer/thumbnails/`
  - macOS: `~/Library/Caches/cuiviewer/thumbnails/`
  - Windows: `%LOCALAPPDATA%\cuiviewer\thumbnails\`
  The central path is derived from the absolute source path so different directories do not collide.
- `none`: do not create, read or write thumbnail files. The browser uses generic file/folder icons and decodes nothing until an image is opened.

When `per-directory` storage is selected but the image directory is read-only, lacks permissions, does not support the `.xvpics` name, or thumbnail creation fails there, the implementation may fall back to `global` storage for that directory and record the fallback in the thumbnail index. If `thumbnails.location` is `none`, no fallback is attempted.

Thumbnail files:

- Thumbnails are small preview images, stored as PNG by default in the modern stack. For compatibility with zgv/xv, reading existing JPEG/XV thumbnails may be supported when available.
- Each thumbnail should preserve enough metadata to avoid reopening the full image when possible, including source modification time and image dimensions.
- A missing or outdated thumbnail is not an error; the browser simply shows the default document/folder icon.

Update behavior:

- Thumbnails are never created or updated automatically during normal browsing. The user explicitly requests an update.
- `u`: create thumbnails for images that lack them and update thumbnails older than their source image in the current directory.
- `Alt+u` or `Ctrl+Shift+U`: recursive update for the current directory and all subdirectories, with a confirmation dialog.
- `d`: create thumbnails for subdirectories themselves, using a composite of the first four images in each subdirectory.
- Updates run with a visible "updating thumbnails" status and progress indicator.
- `Esc` aborts the current update after the active thumbnail finishes.
- If the console/window loses focus, the update may continue in the background and the UI is refreshed when it regains focus.
- Recursive updates honour the configured traversal order and report progress.

Settings:

- `thumbnails.enabled`: whether thumbnail display is enabled.
- `thumbnails.location`: `per-directory`, `global`, or `none`.
- `thumbnails.size`: target thumbnail dimension in pixels (default 128).
- `thumbnails.quality`: JPEG/PNG quality or compression setting when creating thumbnails (default medium).
- `thumbnails.show_directories`: whether directory thumbnails are generated by `d`.
- `thumbnails.slow_update`: walk through the directory slowly while updating, matching zgv's `fs-slow-thumbnail-update` behavior.

Performance:

- Full images are never decoded during normal directory browsing when a valid thumbnail exists.
- Thumbnail loading is asynchronous or batched to keep the UI responsive.
- Thumbnail caches are pruned only when explicitly requested; the application does not delete thumbnails automatically.

## 8. Program Structure

### 8.1 Main Module (`source-frontend/main.c`)

Responsible for:

- Application state.
- Frame loop and view switching.
- Browser state and selected image loading.
- Viewer state and texture lifecycle.
- Slideshow list collection, ordering, timer and navigation.
- Thumbnail cache path resolution, loading, creation and recursive updates.
- Top bar, menus, command palette and toasts.
- Theme and language application.
- Startup settings load and shutdown settings save.

### 8.2 Settings Module (`include/settings.h`, `src/settings.c`)

Responsible for:

- Resolving the application data directory.
- Creating the directory if missing.
- Loading `settings.json`.
- Saving `settings.json` atomically.
- Parsing and serializing the small fixed settings schema.
- Providing defaults when the file is missing or invalid.

### 8.3 CUI Framework (`vendor/CUI`)

Provides windowing, rendering, theming, file browsing, image decoding, image viewing, layout, text and input. The application does not reimplement these.

---

## 9. Core Data Structures

```c
enum { VIEW_BROWSER = 0, VIEW_VIEWER };
enum { LANG_EN = 0, LANG_DE, LANG_ES, LANG_FR, LANG_IT, LANG_PT, LANG_COUNT };

typedef struct {
    char path[4096];            /* current file path shown in the viewer */
    cui_texture texture;        /* loaded image texture, 0 when none */
    int image_w;                /* decoded image width in pixels */
    int image_h;                /* decoded image height in pixels */
    cui_image_view view;         /* zoom and pan state */
    bool checkerboard;           /* show transparency grid */
} viewer_state;

typedef struct {
    char current_dir[4096];     /* directory shown by the browser */
    char selected_path[4096];   /* currently selected entry */
} browser_state;

typedef struct {
    char **paths;               /* owned array of image paths */
    int count;                  /* number of images */
    int capacity;               /* allocated capacity */
    int index;                  /* current image index */
    bool active;                /* slideshow is running */
    bool paused;                /* advancement paused */
    bool shuffle;               /* random order */
    bool recursive;             /* include subdirectories */
    bool wrap;                  /* wrap at end */
    double interval_seconds;    /* delay between images */
    double elapsed_seconds;     /* time since last advance */
    bool fullscreen;            /* fullscreen slideshow */
} slideshow_state;

typedef struct {
    bool updating;              /* thumbnail update in progress */
    bool recursive;             /* recursive update in progress */
    char current_path[4096];    /* item currently being processed */
    int processed;              /* completed items */
    int total;                  /* total items to process */
    bool abort_requested;       /* Esc pressed during update */
    bool slow_update;           /* walk slowly, updating UI per item */
} thumbnail_update_state;

typedef struct {
    int view;                   /* VIEW_BROWSER or VIEW_VIEWER */
    int lang;                   /* LANG_* */
    bool dark;                  /* true for dark theme */

    browser_state browser;
    viewer_state viewer;
    slideshow_state slideshow;
    thumbnail_update_state thumbnail_update;

    bool palette;               /* command palette open */
    bool first_frame;           /* apply theme on first frame */
} app_state;
```

The settings struct is separate and maps directly to JSON:

```c
typedef struct {
    int lang;                   /* LANG_* */
    bool dark;
    char last_directory[4096];
    int window_width;
    int window_height;
    bool window_maximized;
    bool viewer_checkerboard;
    float viewer_max_zoom;

    bool slideshow_shuffle;
    bool slideshow_recursive;
    bool slideshow_wrap;
    double slideshow_interval_seconds;

    bool thumbnails_enabled;
    char thumbnails_location[16]; /* per-directory, global, none */
    int thumbnails_size;
    int thumbnails_quality;
    bool thumbnails_show_directories;
    bool thumbnails_slow_update;
} app_settings;
```

---

## 10. Settings File

### 10.1 Location

The settings file is `settings.json` in the application data directory:

- Linux: `$XDG_DATA_HOME/cuiviewer/settings.json`, or `~/.local/share/cuiviewer/settings.json` when `XDG_DATA_HOME` is unset.
- macOS: `~/Library/Application Support/cuiviewer/settings.json`.
- Windows: `%APPDATA%\cuiviewer\settings.json`.

The directory is created on first save.

### 10.2 Schema

```json
{
  "language": "en",
  "theme": "dark",
  "last_directory": "/home/user/Pictures",
  "window": {
    "width": 1280,
    "height": 800,
    "maximized": true
  },
  "viewer": {
    "checkerboard": true,
    "max_zoom": 16.0
  },
  "slideshow": {
    "interval_seconds": 5.0,
    "shuffle": false,
    "recursive": false,
    "wrap": true
  },
  "thumbnails": {
    "enabled": true,
    "location": "per-directory",
    "size": 128,
    "quality": 75,
    "show_directories": false,
    "slow_update": false
  }
}
```

### 10.3 Rules

- Missing file: use defaults (`language = "en"`, `theme = "light"`, browser starts in the user's pictures/home directory).
- Invalid or partial JSON: ignore invalid fields, keep defaults for missing fields, and rewrite a clean file on next save.
- Save is atomic: write to `settings.json.tmp`, flush, rename over `settings.json`.
- Settings are loaded once at startup and saved on change and on normal exit.
- No SQLite and no migration system is needed; new fields are simply added with defaults.

---

## 11. Internationalization

Follow the `cui-http` pattern:

- A fixed enum of translation keys (`T_*`).
- A `dict[LANG_COUNT][T_COUNT]` table of UTF-8 strings.
- A `msg(app_state *, int id)` helper returning the selected language string.
- Language is selected from a menu and persisted as a code (`en`, `de`, `es`, `fr`, `it`, `pt`).
- All source identifiers and comments remain in English; only user-facing strings are translated.

---

## 12. Theme Handling

- Use `cui_set_theme(ui, CUI_THEME_LIGHT)` and `CUI_THEME_DARK`.
- Store the choice as `dark` in state and as `"light"`/`"dark"` in JSON.
- Apply the theme on the first frame and whenever the user toggles it.
- The toggle is available in the top bar and command palette.

---

## 13. Image Loading and Lifecycle

1. When `Enter` is pressed on an image in the browser:
   - Resolve and store the full path.
   - If a texture is already loaded, destroy it first.
   - Call `cui_image_load(ui, path, &w, &h)`.
   - On failure, show a toast/dialog and remain in the browser.
   - On success, store texture, dimensions and path, reset `cui_image_view` to zero (fit), switch to `VIEW_VIEWER`.
2. When returning to the browser or opening another image, destroy the previous texture.
3. On application exit, destroy any loaded texture.
4. Never call `cui_image_load` every frame; load once and reuse the texture.

---

## 14. File Browser Integration

Use `cui_file_browser` with:

```c
cui_file_browser(ui, "browser", s->browser.current_dir, sizeof s->browser.current_dir,
    .filter = "*.png;*.jpg;*.jpeg",
    .height = CUI_FILL);
```

Behavior:

- The browser owns directory navigation and selection.
- `result.navigated` indicates the folder changed; update saved `last_directory` on change.
- `result.picked` is valid when the user presses `Enter` or double-clicks:
  - If it points to a directory, the browser already changes `current_dir`.
  - If it points to a supported image, load it as described above.
- `result.error` shows a toast and keeps the previous directory.
- Hidden files are hidden by default.

---

## 15. Error Handling

- File read/decode errors: show a toast or dialog and stay in the browser.
- Settings read/write errors: log to stderr, continue with defaults; do not crash.
- Directory read errors: show an empty/error state in the browser.
- All allocations are checked; on failure, show a user-facing message when possible.

---

## 16. Coding Conventions

- Pure C99, no compiler-specific extensions in shared code.
- Four-space indentation, braces on the same line, as in `cui-http`.
- All identifiers and comments in English.
- User-facing strings go through the translation table.
- Shared structs and function declarations live in headers; headers are the single source of truth.
- Heap-allocated strings are freed explicitly.
- No global mutable state except where required by CUI or the settings module.
- Keep the viewer fast: no per-frame file I/O, no per-frame image decoding.

---

## 17. Testing

- `tests/test_settings.c`: verify default path resolution, JSON serialization/parsing and round trips.
- Manual smoke test:
  1. Launch, browse to a folder with PNG/JPEG files.
  2. Navigate with arrows, open with `Enter`.
  3. Zoom in/out, pan, fit, actual size.
  4. Return with `Esc`.
  5. Toggle theme and language.
  6. Start a slideshow from a directory with nested folders, test recursive collection, shuffle, pause/resume, next/previous and interval changes.
  7. Test thumbnail update with `u`, recursive update with `Alt+u`, directory thumbnails with `d`, and abort with `Esc`. Verify fallback storage on a read-only directory.
  7. Launch with command line slideshow options and verify they override persisted settings.
  8. Quit and relaunch; verify settings restored.

---

## 18. Future Extensions

- Tagging and file management (zgv parity).
- Additional image formats via CUI configuration.
- Thumbnail cache.
- Drag and drop opening.
- Recent files list.
- Full-screen mode.
```