# roller

> Inspired by [serpantinum](https://github.com/ilyamiro/serpantinum) — wallpaper picker from his Hyprland shell.

A fast, keyboard-driven wallpaper picker for Hyprland. Browse local wallpapers in a centered carousel, search by filename, and apply instantly with `awww` — pure `QML` + `C++` `LayerShell` overlay.

![roller](assets/demo.png)

[![CI](https://github.com/zyrophix/roller/actions/workflows/ci.yml/badge.svg)](https://github.com/zyrophix/roller/actions) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE) [![Qt 6.5+](https://img.shields.io/badge/Qt-6.5%2B-green.svg)](https://www.qt.io) [![Hyprland 0.56+](https://img.shields.io/badge/Hyprland-0.56%2B-blue.svg)](https://hyprland.org)

## Features

- Centered carousel with smooth animations
- Search by filename (`Ctrl+F` / `/`)
- Thumbnail cache with dominant color detection
- Keyboard, wheel, drag and click navigation
- LayerShell overlay — doesn't disturb tiling
- `<1MB` binary, `~22ms` cold start

## Backends

| Backend | Status |
|---------|--------|
| awww | available |

Future backends possible — PRs welcomed.

## Requirements

- Hyprland 0.56+ (Lua)
- `awww` + `awww-daemon`
- `qt6-base`, `qt6-declarative`, `layer-shell-qt`, `jq`, `ImageMagick` (`magick`)

## Quick start

Install dependencies:

```bash
sudo pacman -S qt6-base qt6-declarative layer-shell-qt jq imagemagick awww
```

Build and install:

```bash
git clone https://github.com/zyrophix/roller
cd roller
cmake -B build -S . && cmake --build build
install -Dm755 build/roller ~/.local/bin/roller
```

Add to `~/.config/hypr/hyprland.lua` — rule and keybind:

```lua
hl.layer_rule({
  match = { namespace = "^(roller)$" },
  blur = false,
  ignore_alpha = 0,
})

hl.bind("SUPER + W", hl.dsp.exec_cmd("roller"))

hl.on("hyprland.start", function()
  hl.exec_cmd("awww-daemon")
end)
```

Run:

```bash
roller          # or SUPER + W
```

Cache is generated automatically on first launch (`~/.cache/roller/thumbs`). To rebuild manually:

```bash
./scripts/cache.sh ~/Projects/roller
```

### Keybinds

| Key | Action |
|-----|--------|
| `h` / `←` | Previous wallpaper |
| `l` / `→` | Next wallpaper |
| `d` | Jump +5 |
| `u` | Jump -5 |
| `Enter` / `Space` | Apply selected |
| `Ctrl+F` / `/` | Toggle search |
| `Esc` | Clear search or quit |
| `Wheel` / `Drag` / `Click` | Navigate |

Wallpaper is applied via:

```bash
awww img <path> --transition-type grow --transition-pos 0.5,0.5 --transition-duration 1.2 --transition-fps 60
```

## Configuration

`config.json`:

```json
{
  "wallpaper_path": "~/Pictures/Wallpapers",
  "cache_path": "~/.cache/roller/thumbs",
  "number_of_pictures": 5,
  "border_color": "#b4befe",
  "border_width": 4,
  "panel_height": 500,
  "selected_horizontal_scale": 1.6,
  "selected_vertical_scale": 1.1,
  "search_background_color": "#313244",
  "search_text_color": "#cdd6f4",
  "search_hint_color": "#a6adc8",
  "carousel_selected_border": "#b4befe",
  "search_hint_text": "Press Ctrl + F or / to search",
  "show_search_hint": true,
  "transition_type": "grow",
  "transition_pos": "0.5,0.5",
  "transition_duration": 1.2,
  "transition_fps": 60
}
```

Wallpapers searched recursively under `wallpaper_path` (`.jpg` `.jpeg` `.png` `.webp` `.bmp`), cache `~/.cache/roller/thumbs`.

## Structure

```
roller/
├── qml/Main.qml          # LayerShell overlay fullscreen transparent, 678 centered
├── qml/Carousel.qml      # Canvas 1.6/1.1 0.3 border 4
├── qml/ColorFilter.qml   # Search-only bar
├── src/main.cpp          # QGuiApplication + LayerShell
├── src/backend/Color.cpp
├── src/backend/Repository.cpp
├── src/backend/Metadata.cpp
├── src/backend/Model.cpp
├── src/backend/Config.cpp
├── src/backend/Awww.cpp
├── src/backend/Backend.cpp
├── scripts/cache.sh
└── config.json
```
