# Changelog

All notable changes to this project are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Versions below 1.0 carry no compatibility promise. Config keys and rendering
have already changed several times during development, without a major bump,
and are expected to keep doing so. Pin a commit if you depend on exact
behaviour.

Entries were not kept before 0.1.0; the earlier history is in the commit log.

## [0.1.0]

First tagged release. A coverflow wallpaper picker for Wayland, written in
QML and C++ as a single `LayerShell` overlay binary, with a thumbnail cache
and a pluggable wallpaper backend.

Known limitations at this release:

- No automated tests. Rendering, animation and delegate recycling are
  verified by hand only.
- `awww` is the only backend exercised on a real desktop. `hyprpaper`,
  `waypaper` and `swaybg` are implemented from their documented interfaces.
- The video path is implemented but lightly exercised. Decoding cost depends
  entirely on hardware support for the codec.

### Added

- Backends: `awww`, `hyprpaper`, `waypaper`, `swaybg`, with `auto` picking
  the first installed.
- Video wallpapers through `mpvpaper`, with `ffmpeg` poster frames.
- Single-instance lock, `--help`, `--version`, `--allow-multiple`.
- Reopen on the last applied wallpaper (`restore_last`).
- Unselected tile borders via `idle_border_width` / `idle_border_color`.
- `stable_copy_path` and `post_apply_command`.
- Thumbnail cache: orphan collection and a size bound (`cache_max_mb`).

### Fixed

- A still wallpaper now stops a running video wallpaper. The kill sat inside
  the video branch, so it only fired for video-over-video; `mpvpaper` loops
  forever and its layer outlives the change underneath, so nothing could take
  the screen back.
- A tile no longer shows the previous wallpaper during a slide. Delegate slots
  are pinned to absolute indices instead of shifting with the rounded centre.
- `selected_horizontal_scale` and `selected_vertical_scale` are honoured
  instead of hardcoded values.
- `border_width` means what it says. The Canvas stroked inside an active clip
  and lost half the stroke, so 4 looked like 2.
- Thumbnail sizing is height-driven, matching `-thumbnail xH`, and cached
  thumbs within 6% of the target are kept rather than regenerated forever.
- Config resolution no longer searches the working directory, and prints the
  path it used.
- `qInfo`/`qWarning` are visible again when stderr is not a terminal.

### Changed

- Per-tile `Canvas` replaced by a `Matrix4x4` shear on the GPU.
- Thumbnail cache is always JPEG, keyed by `md5(absolute path)`.
- `scripts/cache.sh` removed; the C++ worker does the job.
- Config is read from `$XDG_CONFIG_HOME/roller/config.json` first, then next
  to the binary. An installed binary in `~/.local/bin` previously fell through
  to a stale config and ignored the working tree's.
