# roller

Pick a wallpaper from the keyboard: browse local files in a centered coverflow carousel, search by name, apply through whichever daemon you already run.

[![CI](https://img.shields.io/github/actions/workflow/status/zyrophix/roller/ci.yml)](https://github.com/zyrophix/roller/actions)
[![Release](https://img.shields.io/github/v/release/zyrophix/roller)](https://github.com/zyrophix/roller/releases)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

<video src="https://github.com/user-attachments/assets/29ac2fb7-6548-4c0f-ac7c-2a6e0863d89a" autoplay loop muted playsinline width="900">Demo: stepping through wallpapers in the coverflow carousel with h/l and applying one with Enter</video>

> **0.x — expect movement.** Config keys, carousel behaviour and defaults may
> change without notice until 1.0; that is when they stop. The binary name, the
> command line flags, the config path and the cache location do not change. Pin
> a commit if you depend on exact behaviour.

## Why roller?

| What you need | roller | Alternative |
|---|---|---|
| "flip through wallpapers fast" | GPU coverflow, `h`/`l` and wheel, no reload | file manager, or right-clicking the desktop |
| "work with the daemon I already run" | `awww`, `hyprpaper`, `waypaper`, `swaybg`, video via `mpvpaper` | one daemon, or a per-desktop script |
| "keep it small and hackable" | ~600 KB, one C++ binary, one JSON file | a shell component needing a Quickshell install |

## Install

**Wayland only.** roller draws itself as a `wlr-layer-shell` overlay, so it needs a compositor implementing that protocol — Hyprland, Sway, Wayfire, labwc, niri, river. It will not start on X11; the shell import in `qml/Main.qml` is unconditional and there is no fallback window type.

Build needs Qt **6.8+** (`Quick`, `Gui`, `Core`, `Concurrent`) and `layer-shell-qt`. Runtime needs one backend from the table below. `ffmpeg` is used only for video poster frames, `sh` only for `post_apply_command`. No `jq`, no ImageMagick.

### Arch Linux

The repository ships a `PKGBUILD`, so it installs through pacman and resolves
its own dependencies:

```sh
git clone https://github.com/zyrophix/roller.git
cd roller
makepkg -si
```

It is not in the AUR — clone and build, no helper needed. The version comes
from the git tags, so you get the newest release rather than whatever `main`
happens to be. Afterwards `roller` is a pacman package, so `pacman -R roller`
removes it.

### Any distribution

```sh
sudo pacman -S qt6-base qt6-declarative layer-shell-qt awww   # or your distro's equivalents

git clone https://github.com/zyrophix/roller.git
cd roller
cmake -B build -S . && cmake --build build
sudo install -Dm755 build/roller /usr/local/bin/roller
```

A plain clone gives you `main`. To pin a release, clone the tag instead —
`git clone --branch v0.1.0 https://github.com/zyrophix/roller.git` — or pick a
version from [releases](https://github.com/zyrophix/roller/releases).

### Updating

Neither path follows a `git pull` on its own. With the package:

```sh
git pull && makepkg -su
```

Installed by hand, the binary lives at a path rather than in pacman's
database, so it has to be installed again, and a `roller` that is already
running keeps serving the old build until it is restarted:

```sh
git pull && cmake --build build
sudo install -Dm755 build/roller /usr/local/bin/roller
pkill -x roller
```

## Quickstart

Layer rule plus a keybind, so the overlay is not blurred and a key opens it:

```lua
hl.layer_rule({
  match = { namespace = "^(roller)$" },
  blur = false,
  ignore_alpha = 0,
})

hl.bind("SUPER + W", hl.dsp.exec_cmd("roller"))
```

Point it at your wallpapers:

```sh
mkdir -p ~/.config/roller
cp config.example.json ~/.config/roller/config.json
$EDITOR ~/.config/roller/config.json
roller
```

Expected output on start:

```text
roller: config /home/you/.config/roller/config.json
```

The resolved config path is printed every launch. A second `roller` exits and reports the pid that already holds the lock.

## Usage

Press `h` and `l`, or scroll, to move; `Enter` applies.

Applying sets the wallpaper on **the focused monitor only**. A second monitor is
left alone — open an issue if you need both, it is a missing feature rather
than a bug, and an issue saying so is what gets it built.

### Keybinds

| Key | Action |
|---|---|
| `h` / `←` | Previous wallpaper |
| `l` / `→` | Next wallpaper |
| `d` / `u` | Jump one page forward / back |
| `Enter` / `Space` | Apply selected — inside search it only confirms the query |
| `Ctrl+F` / `/` | Toggle search. `/` types normally while the field has focus |
| `Esc` | Clear search, or quit |
| `Wheel` / `Drag` | Scroll |
| `Click` | Select, or apply if already selected |

### Command line

| Flag | Effect |
|---|---|
| `-h`, `--help` | Usage |
| `-v`, `--version` | Version |
| `--allow-multiple` | Skip the single-instance lock, for development |

### Backends

Set with `"backend"` in `config.json`:

| Value | Needs | Notes |
|---|---|---|
| `auto` *(default)* | — | Picks the first backend actually installed, in this order |
| `awww` / `swww` | `awww` + `awww-daemon` | Supports transitions. `swww` is the former name, same binary |
| `hyprpaper` | `hyprpaper` daemon | Hyprland native, via `hyprctl` |
| `waypaper` | `waypaper` | |
| `swaybg` | `swaybg` | roller replaces the instance it started on the next change |

An unknown or unavailable name falls back to the first installed backend. The name actually used is in the startup log.

`awww` is the only backend exercised on a real desktop. The other three are implemented from their documented interfaces and their commands are correct, but none has been run against a live compositor.

There is no X11 backend. roller needs `wlr-layer-shell`, and no compositor
providing it draws an X11 root window as the desktop, so `feh` and similar
could only ever report success and change nothing.

### Video wallpapers

Files whose extension is in `video_extensions` always go through `mpvpaper`, whatever the configured backend — none of the static-image backends can render video. The focused monitor comes from `hyprctl`, falling back to `swaymsg`.

Hardware decoding is off unless you ask for it — mpv ships with `hwdec=no`, so 4K video would otherwise be decoded in software on the CPU. `video_hwdec` defaults to `auto`, which is mpv's whitelisted alias; a codec your GPU cannot decode falls back to software on its own, so it is safe to leave on. Set it to `""` to keep mpv's default.

`mpvpaper` is started **without** `-f`, so its pid is known and the previous instance can be replaced precisely. Any later wallpaper change stops a running video wallpaper; the match is limited to files under `wallpaper_path`, so an `mpvpaper` you started yourself is untouched.

Video costs CPU. Where the GPU has no decoder for the codec — 4K AV1 on AMD Vega, for instance — `auto` falls back to software and can still pin several cores.

### Configuration

Read from, in order:

1. `$XDG_CONFIG_HOME/roller/config.json` — canonical, usually `~/.config/roller/config.json`
2. `../config.json` next to the binary — portable tree installs

The working directory is **not** searched, so a stray `config.json` cannot silently change behaviour. `config.example.json` in this repo is a template, not a live config. `~` in a path expands to `$HOME`.

| Key | Default | Meaning |
|---|---|---|
| `wallpaper_path` | `~/Pictures/Wallpapers` | Scanned recursively |
| `cache_path` | `~/.cache/roller/thumbs` | Thumbnail cache |
| `number_of_pictures` | `5` | Visible tiles, clamped 1–64 |
| `panel_height` | `500` | Carousel height, clamped 1–2160 |
| `selected_horizontal_scale` | `1.6` | Selected tile width scale, 1.0–4.0 |
| `selected_vertical_scale` | `1.1` | Selected tile height scale, 1.0–4.0 |
| `carousel_selected_border` | `#b4befe` | Selected tile border colour |
| `border_color` | `#b4befe` | Legacy alias for the above |
| `border_width` | `2` | Selected tile border |
| `idle_border_width` | `2` | Unselected tile border, `0` hides it |
| `idle_border_color` | `#585b70` | Unselected tile border colour |
| `thumbnail_height` | `512` | Thumbnail height px, 128–2160 |
| `cache_max_mb` | `256` | On-disk cache bound, oldest evicted first |
| `restore_last` | `true` | Reopen on the last applied wallpaper |
| `backend` | `auto` | See [Backends](#backends) |
| `video_extensions` | `mp4 webm mov avi mkv gif m4v flv wmv mpeg 3gp` | Matched case-insensitively |
| `transition_type` | `grow` | awww only, unknown values fall back to `grow` |
| `transition_pos` | `0.5,0.5` | awww only |
| `transition_duration` | `1.2` | awww only, clamped 0–10 |
| `transition_fps` | `60` | awww only, clamped 1–240 |
| `video_hwdec` | `auto` | mpv `hwdec` for video wallpapers; `""` keeps mpv's default of no hardware decoding |
| `stable_copy_path` | `""` | Copy the current wallpaper here for lockscreens or bars |
| `post_apply_command` | `""` | Shell hook after every change, e.g. `wallust run` |
| `search_background_color` | `#313244` | |
| `search_text_color` | `#cdd6f4` | |
| `search_hint_color` | `#a6adc8` | |
| `search_hint_text` | `Press Ctrl + F or / to search` | |
| `show_search_hint` | `true` | |

### Thumbnails

Generated in the background on launch into `cache_path`, keyed by `md5(absolute source path)`, always stored as JPEG. Sizing is height-driven, like `magick -thumbnail x512`; a thumb is regenerated when its height is more than 6% off the target. Videos get a poster frame from `ffmpeg`, seeked to 1s because the first frames of a clip are often black.

`thumbnail_height` defaults to 512 deliberately: at 16:9 a 910×512 thumb is 1.86 MB as ARGB32, which fits Qt's 2 MiB unreferenced pixmap cache limit, while 550 does not. It is also a tier in the [freedesktop thumbnail standard](https://specifications.freedesktop.org/thumbnail/latest/thumbsave.html).

Thumbs whose wallpaper is gone are deleted, and the oldest are evicted once the directory passes `cache_max_mb`. `make clean-cache` drops the whole cache.

### Notes for contributors

Two decisions are not guessable from the code alone:

- Delegate slots are pinned to absolute indices (`slotVIdx` plus `syncSlots`) rather than derived from the rounded centre. Deriving them shifts every pooled delegate on each half-step, so every tile changes source mid-slide and paints the previous wallpaper. The carousel is not linear, which is why a `ListView` does not fit here.
- QML bindings that call `Q_INVOKABLE` getters never re-evaluate on `dataChanged`. The delegate bindings read `model.rev` for that reason.

## Repo overview

- `qml/` — `Main.qml` overlay, layout and keybinds; `Carousel.qml` coverflow and delegate pool; `SearchBar.qml` debounced field
- `src/` — `main.cpp` CLI, single-instance lock, config resolution, QML engine
- `src/backend/` — repository scan, model, filter proxy, thumbnail worker, backend dispatch
- `tests/` — `QtTest` regression tests over the pure logic; run with `ctest --test-dir build`
- `config.example.json` — config template
- `PKGBUILD` — Arch package; builds the newest tag, see [Install](#install)
- `.github/workflows/ci.yml` — build, QML lint, tests

## Contributing

PRs welcome: fork → branch → PR. Build and lint before submitting:

```sh
cmake -B build -S . && cmake --build build
make lint
```

Full guide: [CONTRIBUTING.md](CONTRIBUTING.md).

## License

MIT — see [LICENSE](LICENSE).
