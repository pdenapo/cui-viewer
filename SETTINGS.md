# Settings file format (`settings.json`)

`cuiviewer` stores its configuration in a single JSON file named
`settings.json` located in the application data directory:

- Linux: `$XDG_DATA_HOME/cuiviewer/settings.json` or
  `~/.local/share/cuiviewer/settings.json`
- macOS: `~/Library/Application Support/cuiviewer/settings.json`
- Windows: `%APPDATA%\cuiviewer\settings.json`

The directory is created on first save. Missing or invalid fields fall back to
their defaults and a clean file is rewritten on the next save.

## Schema

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
    "slow_update": false,
    "preview": true
  },
  "custom_commands": [
    {
      "name": "Process file",
      "command": "process_file %s",
      "shell": "/bin/sh",
      "terminal": true
    }
  ]
}
```

## Field reference

| Field | Type | Default | Description |
|---|---|---|---|
| `language` | string | `"en"` | UI language: `en`, `de`, `es`, `fr`, `it`, `pt` |
| `theme` | string | `"light"` | `"light"` or `"dark"` |
| `last_directory` | string | home/Pictures | Last browsed directory |
| `window.width` | int | 1280 | Window width |
| `window.height` | int | 800 | Window height |
| `window.maximized` | bool | true | Start maximized |
| `viewer.checkerboard` | bool | true | Show transparency grid |
| `viewer.max_zoom` | float | 16.0 | Maximum zoom factor |
| `slideshow.interval_seconds` | double | 5.0 | Seconds between images (min 0.5) |
| `slideshow.shuffle` | bool | false | Shuffle image order |
| `slideshow.recursive` | bool | false | Include subdirectories |
| `slideshow.wrap` | bool | true | Wrap around at the end |
| `thumbnails.enabled` | bool | true | Enable thumbnails |
| `thumbnails.location` | string | `"per-directory"` | `per-directory`, `global` or `none` |
| `thumbnails.size` | int | 128 | Thumbnail dimension in pixels |
| `thumbnails.quality` | int | 75 | Quality/compression setting |
| `thumbnails.show_directories` | bool | false | Generate directory thumbnails |
| `thumbnails.slow_update` | bool | false | Slow thumbnail update |
| `thumbnails.preview` | bool | true | Show preview pane |
| `custom_commands` | array | `[]` | User-defined commands (see below) |

## Custom commands

Each entry in `custom_commands` is an object:

| Field | Type | Default | Description |
|---|---|---|---|
| `name` | string | required | Label shown in the Actions menu |
| `command` | string | required | Shell command template; `%s` is replaced with the full path of the selected file |
| `shell` | string | `/bin/sh` | Shell used to execute the command |
| `terminal` | bool | false | Launch in a terminal emulator to show output |

Commands appear in the Actions menu of the browser and the viewer, as well as in
right-click context menus.