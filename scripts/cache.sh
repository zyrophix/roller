#!/usr/bin/env bash
set -u

APP_DIR="${1:-$(cd "$(dirname "$0")/.." && pwd)}"

if [[ ! -f "$APP_DIR/config.json" ]]; then
  echo "Usage: $0 /path/to/hyprroll" >&2
  exit 1
fi

CONFIG="$APP_DIR/config.json"
wallpaper_path=$(jq -r '.wallpaper_path' "$CONFIG")
cache_path=$(jq -r '.cache_path' "$CONFIG")
cache_batch_size=$(jq -r '.cache_batch_size' "$CONFIG")

wallpaper_path=$(eval echo "$wallpaper_path")
cache_path=$(eval echo "$cache_path")

mkdir -p "$cache_path"

if command -v magick >/dev/null 2>&1; then
  IM_BIN="magick"
elif command -v convert >/dev/null 2>&1; then
  IM_BIN="convert"
else
  echo "ImageMagick not found (magick or convert)" >&2
  exit 1
fi

find "$wallpaper_path" -type f \( -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.png" -o -iname "*.webp" -o -iname "*.bmp" \) | while read -r img; do
  filename=$(basename "$img")
  out="$cache_path/$filename"
  if [[ -f "$out" && "$out" -nt "$img" ]]; then
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

find "$cache_path" -maxdepth 1 -type f \( -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.png" -o -iname "*.webp" -o -iname "*.bmp" \) | while read -r cached; do
  filename=$(basename "$cached")
  if ! find "$wallpaper_path" -type f -name "$filename" -print -quit | grep -q .; then
    rm -f "$cached"
  fi
done

cd "$APP_DIR"
PYTHONPATH="$APP_DIR" python3 -m backend.metadata "$wallpaper_path" "$cache_path"
