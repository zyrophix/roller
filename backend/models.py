from pathlib import Path

from PySide6.QtCore import QAbstractListModel, Qt, QModelIndex, Signal, Slot, Property


class WallpaperModel(QAbstractListModel):
    PathRole = Qt.UserRole + 1
    NameRole = Qt.UserRole + 2
    ThumbRole = Qt.UserRole + 3
    ColorRole = Qt.UserRole + 4

    countChanged = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._items = []  # list[dict]
        self._wallpaper_dir = Path.home() / "Pictures/Wallpapers"
        self._cache_dir = Path.home() / ".cache/hyprroll/thumbs"

    def roleNames(self):
        return {
            self.PathRole: b"wallpaperPath",
            self.NameRole: b"wallpaperName",
            self.ThumbRole: b"thumbnailPath",
            self.ColorRole: b"colorGroup",
        }

    def rowCount(self, parent=QModelIndex()):
        return len(self._items)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid() or index.row() >= len(self._items):
            return None
        item = self._items[index.row()]
        if role == self.PathRole:
            return item["path"]
        if role == self.NameRole:
            return item["name"]
        if role == self.ThumbRole:
            return item["thumb"]
        if role == self.ColorRole:
            return item["color"] or ""
        return None

    def set_dirs(self, wallpaper_dir: Path, cache_dir: Path):
        self._wallpaper_dir = Path(wallpaper_dir).expanduser()
        self._cache_dir = Path(cache_dir).expanduser()

    def set_items(self, paths, metadata):
        self.beginResetModel()
        items = []
        for p in paths:
            name = p.name
            thumb = str(self._cache_dir / name)
            # fallback to original if thumb missing
            thumb_path = Path(thumb)
            if not thumb_path.is_file():
                thumb = str(p)
            color = metadata.get(name, {}).get("color_group") if metadata else None
            items.append({
                "path": str(p),
                "name": name,
                "thumb": "file://" + thumb if not thumb.startswith("file://") else thumb,
                "color": color,
            })
        self._items = items
        self.endResetModel()
        self.countChanged.emit()

    @Slot(result=int)
    def count(self):
        return len(self._items)

    @Property(int, notify=countChanged)
    def countProp(self):
        return len(self._items)

    @Slot(int, result=str)
    def get_path_at(self, idx: int):
        if 0 <= idx < len(self._items):
            return self._items[idx]["path"]
        return ""

    @Slot(int, result=str)
    def get_name_at(self, idx: int):
        if 0 <= idx < len(self._items):
            return self._items[idx]["name"]
        return ""

    @Slot(int, result=str)
    def get_thumb_at(self, idx: int):
        if 0 <= idx < len(self._items):
            return self._items[idx]["thumb"]
        return ""
