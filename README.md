# roller

> Inspired by [serpantinum](https://github.com/ilyamiro/serpantinum) — wallpaper picker from his Hyprland shell.

A keyboard-driven wallpaper picker. Browse local wallpapers in a centered coverflow carousel, search by filename, apply instantly through whichever wallpaper daemon you run — a `QML` + `C++` `LayerShell` overlay.

![roller](assets/demo.png)

[![CI](https://github.com/zyrophix/roller/actions/workflows/ci.yml/badge.svg)](https://github.com/zyrophix/roller/actions) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE) [![Qt 6.8+](https://img.shields.io/badge/Qt-6.8%2B-green.svg)](https://www.qt.io) [![Wayland](https://img.shields.io/badge/Wayland-only-blue.svg)](https://wayland.app)

## Features

- Centered coverflow carousel, GPU rendered via `Matrix4x4` shear
- Search by filename (`Ctrl+F` / `/`)
- Thumbnail cache with size-bounded eviction and orphan collection
- Keyboard, wheel, drag and click navigation
- Reopens on the wallpaper you applied last
- Multiple wallpaper backends, plus video wallpapers through `mpvpaper`
- Single instance: launching it twice will not stack two overlays
- LayerShell overlay — does not disturb tiling
- ~600 KB binary, no runtime dependency on a script

## Wayland only

roller draws itself as a `wlr-layer-shell` overlay, so it **requires Wayland and a compositor that implements that protocol**: Hyprland, Sway, Wayfire, labwc, niri, river. It will not start on X11 — the shell import in `qml/Main.qml` is unconditional, there is no fallback window type.

## Backends

Set with `"backend"` in `config.json`:

| Value | Needs | Notes |
|-------|-------|-------|
| `auto` *(default)* | — | picks the first backend actually installed, in the order below |
| `awww` / `swww` | `awww` + `awww-daemon` | supports transitions; `swww` is the former name and resolves to the same binary |
| `hyprpaper` | `hyprpaper` daemon | Hyprland native, IPC via `hyprctl` |
| `waypaper` | `waypaper` | |
| `swaybg` | `swaybg` | roller replaces the instance it started on the next change |
| `feh` | `feh` | **X11 only** — it sets the X root window, which a Wayland compositor draws over, so under Hyprland it reports success and changes nothing |

An unknown or unavailable name falls back to the first installed backend; the name actually used is in the startup log.

### Video wallpapers

Files whose extension is in `video_extensions` are always applied through `mpvpaper`, regardless of the configured backend — none of the static-image backends can render video. The focused monitor is taken from `hyprctl`, falling back to `swaymsg`.

`mpvpaper` is started **without** `-f`, so its pid is known and the previous instance can be replaced precisely. Any later wallpaper change stops a running video wallpaper; matching is limited to files under `wallpaper_path`, so an `mpvpaper` you started yourself is not touched.

Video costs CPU. `mpv` defaults to `--hwdec=no`, so decoding happens on the processor; on a machine without hardware decoding for the codec (4K AV1 on AMD Vega, for instance) this can pin several cores.

## Requirements

Build:

- `cmake` 3.20+
- Qt **6.8+**: `Quick`, `Gui`, `Core`, `Concurrent`
- `layer-shell-qt`

Runtime:

- a Wayland compositor with `wlr-layer-shell`
- one of the backends above
- `ffmpeg` *(optional)* — only to render poster frames for video wallpapers
- `sh` *(optional)* — only if you set `post_apply_command`

No `jq`, no ImageMagick.

## Quick start

```bash
sudo pacman -S qt6-base qt6-declarative layer-shell-qt awww   # backend of your choice
```

Build and install:

```bash
git clone https://github.com/zyrophix/roller
cd roller
cmake -B build -S . && cmake --build build
install -Dm755 build/roller ~/.local/bin/roller
```

Hyprland config — a layer rule so the overlay is not blurred, and a keybind:

```lua
hl.layer_rule({
  match = { namespace = "^(roller)$" },
  blur = false,
  ignore_alpha = 0,
})

hl.bind("SUPER + W", hl.dsp.exec_cmd("roller"))
```

```bash
roller          # or SUPER + W
```

The resolved config path and any warnings are printed on every start.

### Command line

```
-h, --help           usage
-v, --version        version
    --allow-multiple do not enforce the single-instance lock
```

## Keybinds

| Key | Action |
|-----|--------|
| `h` / `←` | Previous wallpaper |
| `l` / `→` | Next wallpaper |
| `d` / `u` | Jump one page forward / back |
| `Enter` / `Space` | Apply selected (a second Enter confirms a search) |
| `Ctrl+F` / `/` | Toggle search — `/` types normally while the field has focus |
| `Esc` | Clear search, or quit |
| `Wheel` / `Drag` | Scroll |
| `Click` | Select, or apply if already selected |

## Configuration

Read from, in order:

1. `$XDG_CONFIG_HOME/roller/config.json` — canonical, usually `~/.config/roller/config.json`
2. `../config.json` next to the binary — portable tree installs

The working directory is **not** searched. `config.example.json` in this repo is a template, not a live config.

`~` in a path is expanded to `$HOME`.

| Key | Default | Meaning |
|-----|---------|---------|
| `wallpaper_path` | `~/Pictures/Wallpapers` | scanned recursively |
| `cache_path` | `~/.cache/roller/thumbs` | thumbnail cache |
| `number_of_pictures` | `5` | visible tiles, clamped to 1–64 |
| `panel_height` | `500` | carousel height, clamped to 1–2160 |
| `selected_horizontal_scale` | `1.6` | selected tile width scale, 1.0–4.0 |
| `selected_vertical_scale` | `1.1` | selected tile height scale, 1.0–4.0 |
| `border_color` | `#b4befe` | legacy alias, prefer `carousel_selected_border` |
| `border_width` | `2` | selected tile border |
| `carousel_selected_border` | `#b4befe` | selected tile border colour |
| `idle_border_width` | `2` | unselected tile border, `0` to hide |
| `idle_border_color` | `#585b70` | unselected tile border colour |
| `thumbnail_height` | `512` | thumbnail height in px, 128–2160 |
| `cache_max_mb` | `256` | on-disk cache bound, oldest evicted first |
| `restore_last` | `true` | reopen on the last applied wallpaper |
| `backend` | `auto` | see Backends |
| `video_extensions` | `mp4 webm mov avi mkv gif m4v flv wmv mpeg 3gp` | matched case-insensitively |
| `transition_type` | `grow` | awww only; unknown values fall back to `grow` |
| `transition_pos` | `0.5,0.5` | awww only |
| `transition_duration` | `1.2` | awww only, clamped to 0–10 |
| `transition_fps` | `60` | awww only, clamped to 1–240 |
| `stable_copy_path` | `""` | copy the current wallpaper here, for lockscreens or bars |
| `post_apply_command` | `""` | shell hook run after every change, e.g. `wallust run` |
| `search_background_color` | `#313244` | |
| `search_text_color` | `#cdd6f4` | |
| `search_hint_color` | `#a6adc8` | |
| `search_hint_text` | `Press Ctrl + F or / to search` | |
| `show_search_hint` | `true` | |

### Thumbnails

Generated in the background on launch into `cache_path`, keyed by `md5(absolute source path)` and always stored as JPEG. Sizing is height-driven, like `magick -thumbnail x512`; a cached thumbnail is regenerated when its height is more than 6% off the target. Videos get a poster frame from `ffmpeg`, seeked to 1s because the first frames of a clip are often black.

`thumbnail_height` defaults to 512 deliberately: at 16:9 a 910×512 thumbnail is 1.86 MB as ARGB32, which fits inside Qt's 2 MiB unreferenced pixmap cache limit, while 550 does not. It is also a [freedesktop thumbnail tier](https://specifications.freedesktop.org/thumbnail/latest/thumbsave.html).

Thumbs whose wallpaper is gone are deleted, and the oldest are evicted once the directory passes `cache_max_mb`. `make clean-cache` removes the whole cache.

## Structure

```
roller/
├── qml/Main.qml                        # LayerShell overlay, layout, keybinds, search
├── qml/Carousel.qml                    # coverflow, FrameAnimation, delegate pool
├── qml/SearchBar.qml                   # search field, debounced
├── src/main.cpp                        # CLI, single-instance lock, config, QML engine
├── src/backend/AppConfig.{h,cpp}        # config.json wrapper, current-wallpaper state
├── src/backend/WallpaperRepository.*    # recursive scan
├── src/backend/WallpaperModel.*        # QAbstractListModel, path/name/thumb roles
├── src/backend/WallpaperFilterProxy.*  # QSortFilterProxyModel, name search
├── src/backend/ThumbnailCache.*        # background thumbnails (QtConcurrent)
├── src/backend/PickerController.*      # search, apply, refresh
├── src/backend/WallpaperBackend.*      # backend dispatch, video, monitor detection
└── config.example.json
```

## Notes for contributors

- Delegate slots are pinned to absolute indices (`slotVIdx` + `syncSlots`) rather than derived from the rounded centre. Deriving them shifts every pooled delegate on each half-step, which makes every tile change source mid-slide and paints the previous wallpaper — the carousel is not linear, so a `ListView` does not apply here.
- QML bindings that call `Q_INVOKABLE` getters never re-evaluate on `dataChanged`. The delegate bindings read `model.rev` for that reason.
- `make lint` runs `qmllint`. It is not yet wired into CI as a hard failure.
