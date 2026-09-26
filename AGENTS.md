# AGENTS.md

Operating rules for roller. Read this before editing; it records what cannot be
reliably inferred from the code.

## Build, run, lint

```sh
cmake -B build -S . && cmake --build build    # ~40s, the fast check
make lint                                      # qmllint, informational
```

There is no test suite yet. `make lint` is not wired into CI as a hard gate.

Run the built binary from the tree — `./build/roller`. It reads
`$XDG_CONFIG_HOME/roller/config.json` first, then `../config.json` next to the
binary, so the tree run and an installed run can pick different files. It
prints the path it resolved on every start; read that line before trusting
any observation about behaviour.

```sh
cmake --build build && install -Dm755 build/roller ~/.local/bin/roller
pkill -x roller; setsid nohup roller >/tmp/roller.log 2>&1 < /dev/null &
```

## Boundaries

- **Do not add a build step that needs `jq` or ImageMagick.** Config is parsed
  with `QJsonDocument`; thumbnails with `QImageReader` and `ffmpeg` for video.
  Those tools were removed from the dependency set on purpose.
- **`config.example.json` is a template, not a live config.** The user's file
  lives at `~/.config/roller/config.json`. Editing the template changes nothing
  for a running install.
- **Thumbs live outside the repo**, in `~/.cache/roller/thumbs`. Do not commit
  them. Wipe with `make clean-cache`.
- **A user is a real desktop.** Any change to the apply path changes what is
  on their screen immediately. `awww-daemon` is usually running; several
  backends will fight for the same surface if enabled together.

## Traps

- **The carousel is not linear.** A `ListView` cannot be dropped in: tile
  position depends on both index and the animated selection. The pooled
  delegates are pinned to absolute indices in `slotVIdx`, with `syncSlots()`
  recycling only the slot that leaves the window. Recomputing `vIdx` from the
  rounded centre is the single change that reintroduces the wrong-wallpaper
  flash.
- **QML never re-evaluates a binding that only calls a `Q_INVOKABLE`
  getter.** `get_thumb_at(realIdx)` depends on `root.model` and `realIdx`
  only, so `dataChanged` does not reach it. The bindings read `model.rev` for
  this reason; keep that read.
- **`Thumbnails are 512px because of a Qt limit.** `QQuickPixmapCache` allows
  2 MiB of unreferenced data and a 910x512 ARGB32 thumb is 1.86 MB, while
  550px is 2.15 MB and does not fit. `thumbnail_height` and the QML
  `sourceSize.height` must stay equal or the cache keys diverge.
- **mpvpaper is started without `-f` on purpose.** With the fork, the pid
  `startDetached` returns belongs to a parent that exits, and players pile up
  each software-decoding 4K AV1 at several hundred percent CPU. The previous
  instance is found by scanning `/proc` for an `mpvpaper` whose command line
  references a file under `wallpaper_path`, which is what makes it safe to
  stop a stale one from a previous run.
- **Synthetic keys do not reach the overlay.** The layer surface holds
  `KeyboardInteractivityExclusive`, so `wtype`, `ydotool` and
  `hyprctl dispatch sendshortcut` cannot drive it. Navigation and focus
  behaviour can only be verified by a human at the keyboard.
- **Message output was invisible.** Qt's default handler produced nothing when
  stderr was not a tty, so use `fprintf` for anything a user must read, and do
  not remove `rollerMessageHandler`.

## Layout

`qml/` holds the UI, `src/` the entry point, `src/backend/` the model, cache and
backend dispatch. Architecture detail is in the README § Notes for
contributors; do not duplicate it here.
