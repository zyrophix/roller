# Contributing

PRs welcome: fork → branch → PR.

## Setup

```sh
git clone https://github.com/zyrophix/roller
cd roller
sudo pacman -S qt6-base qt6-declarative layer-shell-qt   # plus a backend
cmake -B build -S . && cmake --build build
```

## Before you submit

```sh
cmake -B build -S . && cmake --build build   # must build without new warnings
make lint                                     # qmllint, informational
```

There is no test suite yet. Changes to rendering, animation or the delegate
pool cannot be verified automatically, so describe in the PR what you saw on
screen, not only what compiled.

## Manual checks that matter

Ask a human to run these; synthetic key injection cannot reach the overlay.

| Area | What to confirm |
|---|---|
| Navigation | `h`/`l`, wrap at both ends, `d`/`u`, wheel, drag |
| Search | `/` and `Ctrl+F` enter and leave, `/` types inside the field, filter result order |
| Apply | `Enter` applies what is highlighted, not the previous tile mid-slide |
| Video | Apply a video, then an image — the video must stop |
| Second launch | `SUPER+W` twice leaves one process and one overlay |

## Rules that are easy to break

- Keep `thumbnail_height` and the QML `sourceSize.height` equal, and keep the
  value at or below 543px so one thumbnail fits Qt's pixmap cache.
- Do not recompute `vIdx` from the rounded centre in `Carousel.qml`.
- Do not add `jq` or ImageMagick to the build or runtime path.
- `AGENTS.md` records the reasoning; read it before touching rendering.
