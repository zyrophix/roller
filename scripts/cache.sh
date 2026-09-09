#!/usr/bin/env bash
set -Eeuo pipefail

APP_DIR="${1:-$(cd "$(dirname "$0")/.." && pwd)}"

if [[ ! -f "$APP_DIR/config.json" ]]; then
  echo "Usage: $0 /path/to/hyprroll" >&2
  exit 1
fi

CONFIG="$APP_DIR/config.json"
wallpaper_path=$(jq -r '.wallpaper_path // empty' "$CONFIG")
cache_path=$(jq -r '.cache_path // empty' "$CONFIG")
cache_batch_size=$(jq -r '.cache_batch_size // 20' "$CONFIG")

# Safe ~ expansion without eval (prefix only)
expand_path() {
  local p="$1"
  if [[ "$p" == "~/"* ]]; then
    p="${HOME}${p:1}"
  elif [[ "$p" == "~" ]]; then
    p="$HOME"
  fi
  printf '%s' "$p"
}
wallpaper_path=$(expand_path "$wallpaper_path")
cache_path=$(expand_path "$cache_path")

mkdir -p "$cache_path"

if command -v magick >/dev/null 2>&1; then
  IM_BIN="magick"
elif command -v convert >/dev/null 2>&1; then
  IM_BIN="convert"
else
  echo "ImageMagick not found (magick or convert)" >&2
  exit 1
fi

find "$wallpaper_path" -type f \( -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.png" -o -iname "*.webp" -o -iname "*.bmp" \) -print0 | while IFS= read -r -d '' img; do
  ext="${img##*.}"; ext=$(printf '%s' "$ext" | tr '[:upper:]' '[:lower:]')
  hash=$(printf '%s' "$img" | md5sum | cut -d' ' -f1)
  out="$cache_path/$hash.$ext"
  out_old="$cache_path/$(basename "$img")"
  if [[ -f "$out" && "$out" -nt "$img" ]]; then
    continue
  fi
  # fallback: keep old thumb if exists and newer
  if [[ -f "$out_old" && "$out_old" -nt "$img" && ! -f "$out" ]]; then
    cp -a "$out_old" "$out"
    continue
  fi
  "$IM_BIN" "$img" -thumbnail x500 -strip -quality 85 "$out" &
  if (( cache_batch_size > 0 )); then
    while (( $(jobs -rp | wc -l) >= cache_batch_size )); do
      wait -n
    done
  fi
done
wait

# cleanup orphaned thumbs (both old basename and new hashed)
tmp_expected=$(mktemp)
find "$wallpaper_path" -type f \( -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.png" -o -iname "*.webp" -o -iname "*.bmp" \) -print0 | while IFS= read -r -d '' img; do
  ext="${img##*.}"; ext=$(printf '%s' "$ext" | tr '[:upper:]' '[:lower:]')
  hash=$(printf '%s' "$img" | md5sum | cut -d' ' -f1)
  printf '%s\n' "$hash.$ext" >> "$tmp_expected"
  printf '%s\n' "$(basename "$img")" >> "$tmp_expected"
done
find "$cache_path" -maxdepth 1 -type f \( -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.png" -o -iname "*.webp" -o -iname "*.bmp" \) -print0 | while IFS= read -r -d '' cached; do
  base=$(basename "$cached")
  if ! grep -qxF "$base" "$tmp_expected"; then
    rm -f "$cached"
  fi
done
rm -f "$tmp_expected"
