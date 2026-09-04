# hyprroll

> Inspired by [serpantinum](https://github.com/ilyamiro/serpantinum) — wallpaper picker from his Hyprland shell.

A fast, keyboard-driven wallpaper picker for Hyprland. Browse local wallpapers in a centered carousel, filter by dominant color or search by filename, and apply instantly with `awww`.

![Hyprland 0.56 + awww](https://codeberg.org/LGFae/awww)

## Features

- Horizontal carousel with centered selection — `7` visible items, `500px` panel height, `1.6x` horizontal and `1.1x` vertical expansion for the selected item
- Shear/perspective styling (`-0.3`) without distorting image content (parallelogram clip, cover-fit thumbnails)
- Smooth animated transitions (`contentX *0.15`, `visualSelection *0.22` at 60Hz)
- Color filtering — only groups present in the current collection are shown (LAB classification: `red`, `orange`, `yellow`, `green`, `cyan`, `blue`, `purple`, `pink`, `gray`)
- Name search — case-insensitive, live filtering, debounced input
- Thumbnail cache via ImageMagick (`x500`, `quality 85`) and `metadata.json` with dominant color + k-means clustering
- Keyboard, wheel, drag and click navigation with circular wrap-around
- Single-monitor focused, local wallpapers only, extensible to multiple backends

## Requirements

- Hyprland 0.56+ (Lua config)
- `awww` and `awww-daemon` (`swww` is the same project, renamed — `awww` binary is used, `swww` works via symlink)
- `python3`, `PySide6` 6.11+, `qt6-declarative`, `jq`, `ImageMagick` (`magick`), `Pillow`

```bash
sudo pacman -S python-pyside6 qt6-declarative jq imagemagick awww
# or from AUR
yay -S awww
```

## Installation

```bash
git clone <repo> ~/Projects/hyprroll
cd ~/Projects/hyprroll
cp config.example.json config.json
# edit wallpaper_path if needed
mkdir -p ~/Pictures/Wallpapers
./scripts/cache.sh ~/Projects/hyprroll
```

## Hyprland Setup (Lua)

Add to `~/.config/hypr/hyprland.lua` (see `hypr/hyprland.lua.example`):

```lua
hl.window_rule({
  name  = "hyprroll",
  match = { class = "hyprroll" },
  float = true,
  fullscreen = true,
  dim_around = false,
  rounding = 0,
  no_blur = true,
})

hl.bind("SUPER + W", hl.dsp.exec_cmd("hyprroll"))

hl.on("hyprland.start", function()
  hl.exec_cmd("awww-daemon")
end)
```

Reload:

```bash
hyprctl reload
```

## Usage

```bash
hyprroll                       # preferred — installed to ~/.local/bin/hyprroll
python3 main.py                # direct launch
./scripts/cache.sh ~/Projects/hyprroll  # rebuild thumbnail cache manually
```

Controls:

| Action | Key |
|---|---|
| Next wallpaper | `j` / `→` |
| Previous wallpaper | `k` / `←` |
| Jump forward | `d` (+5) |
| Jump backward | `u` (-5) |
| Apply selected | `Enter` / `Space` / click selected |
| Select | click unselected |
| Scroll | mouse wheel, drag |
| Search | click `⌕`, type to filter, `Esc` to exit search |
| Filter by color | click color swatch, `◉` for all |
| Quit | `Esc` (or click outside) |

Wallpaper is applied via:

```bash
awww img <path> --transition-type wipe --transition-fps 30
```

## Configuration

`config.json` (generated from `config.example.json`):

```json
{
  "wallpaper_path": "~/Pictures/Wallpapers",
  "cache_path": "~/.cache/hyprroll/thumbs",
  "number_of_pictures": 7,
  "border_color": "#C27B63",
  "cache_batch_size": 20,
  "backend": "awww",
  "transition_type": "wipe",
  "transition_fps": 30
}
```

- `number_of_pictures` — odd values (`5`, `7`, `9`) keep selection centered
- `border_color` — selected tile border
- `cache_batch_size` — parallel ImageMagick jobs

Wallpapers are searched recursively (`rglob`) under `wallpaper_path`. Supported extensions: `.jpg`, `.jpeg`, `.png`, `.webp`, `.bmp`.

Cache is stored in `cache_path` as `x500` thumbnails plus `metadata.json` (mtime, dominant color, color group).

## Project Structure

```
hyprroll/
├── main.py               # PySide6 entry, transparent fullscreen overlay, class hyprroll
├── qml/
│   ├── Main.qml          # Fullscreen transparent window, centered content (678px)
│   ├── Carousel.qml      # Repeater with exact panel geometry + Matrix4x4 shear
│   ├── ColorFilter.qml   # Filter bar rgba(57,58,58,0.60) radius 10
│   └── WallpaperDelegate.qml
├── backend/
│   ├── repository.py     # Recursive scan, filterByColor/filterByName
│   ├── color.py          # LAB color classification (9 groups)
│   ├── metadata.py       # Dominant color extraction (Pillow/QImage, k-means)
│   ├── models.py         # QAbstractListModel for QML
│   └── awww_backend.py   # awww/swww backend abstraction
├── scripts/
│   ├── cache.sh
│   └── open.sh
├── hypr/hyprland.lua.example
└── config.json
```

## How It Works

- `scripts/cache.sh` generates `x500` thumbnails and calls `backend.metadata` to compute dominant colors.
- `backend.metadata` samples thumbnails at `48px`, converts to LAB, weights by chroma, clusters into `3–7` groups, and classifies via LAB distance.
- `qml/Carousel.qml` replicates the original cairo geometry: `tileWidth = width/7 -10`, `step = tile+4`, `extra = tile*0.6`, `margin = tile*0.25`, shear `0.3*scaledH`, lerp animations.
- `qml/ColorFilter.qml` matches `ui/color_filter.css`: `32x32` swatches, `radius 8`, selected stroke `1.9px rgba(1,1,1,0.95)`.

## Roadmap

- `B` variant: `Qt6 C++` + `LayerShellQt` overlay (same `qml/*`, no Python)
- Multi-backend abstraction (`awww`, `hyprpaper`, `swww` compat)
- Per-output wallpaper selection (currently single monitor)
