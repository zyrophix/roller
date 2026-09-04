# hyprroll

> Inspired by [serpantinum](https://github.com/ilyamiro/serpantinum) — wallpaper picker from his Hyprland shell.

A fast, keyboard-driven wallpaper picker for Hyprland. Browse local wallpapers in a centered carousel, search by filename, and apply instantly with `awww` — pure `QML` + `C++` `LayerShell` overlay.

![Hyprland 0.56 + awww](https://codeberg.org/LGFae/awww)

## Features

- Horizontal carousel with centered selection — `5` visible, `500px` panel, `1.6x`/`1.1x` expansion, shear `-0.3` (parallelogram clip, cover-fit)
- Smooth `60Hz` lerp (`contentX *0.15`, `visualSelection *0.22`)
- Name search — case-insensitive, live, `Ctrl+F`/`/` to toggle, `Esc` to clear (window stays)
- Thumbnail cache `x500` via ImageMagick + `metadata.json` (LAB `k-means`)
- `h`/`←` `l`/`→` `d`/`u` `Enter`/`Space` `Esc` `wheel` `drag` `click` with circular wrap
- `LayerShell` overlay `1920x1080` transparent, `1200x678` centered content, `lavender #b4befe` border `4px`
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

```bash
sudo pacman -S qt6-base qt6-declarative layer-shell-qt jq imagemagick awww
```

## Build

```bash
git clone https://github.com/zyrophix/hyprroll
cd hyprroll
cmake -B build -S . && cmake --build build
install -Dm755 build/hyprroll ~/.local/bin/hyprroll
./scripts/cache.sh ~/Projects/hyprroll
```

## Hyprland Setup

Add to `~/.config/hypr/hyprland.lua`:

```lua
hl.layer_rule({
  match = { namespace = "^(hyprroll)$" },
  blur = false,
  ignore_alpha = 0,
})

hl.bind("SUPER + W", hl.dsp.exec_cmd("hyprroll"))

hl.on("hyprland.start", function()
  hl.exec_cmd("awww-daemon")
end)
```

```bash
hyprctl reload
```

## Usage

```bash
hyprroll
./scripts/cache.sh ~/Projects/hyprroll
```

Controls: `h/←` prev, `l/→` next, `d` +5, `u` -5, `Enter`/`Space` apply, `Esc` quit, `Ctrl+F`/`/` search, `Esc` in search clears, wheel/drag.

Wallpaper via:

```bash
awww img <path> --transition-type grow --transition-pos 0.5,0.5 --transition-duration 1.2 --transition-fps 60
```

## Configuration

`config.json`:

```json
{
  "wallpaper_path": "~/Pictures/Wallpapers",
  "cache_path": "~/.cache/hyprroll/thumbs",
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

Wallpapers searched recursively under `wallpaper_path` (`.jpg` `.jpeg` `.png` `.webp` `.bmp`), cache `~/.cache/hyprroll/thumbs`.

## Structure

```
hyprroll/
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
