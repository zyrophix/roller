#!/usr/bin/env python3
import json
import sys
from pathlib import Path

from PySide6.QtCore import QObject, Signal, Slot, Property, QUrl, Qt
from PySide6.QtGui import QGuiApplication, QSurfaceFormat
from PySide6.QtQml import QQmlApplicationEngine

from backend.repository import WallpaperRepository
from backend.metadata import MetadataStore
from backend.models import WallpaperModel
from backend import awww_backend

APP_DIR = Path(__file__).resolve().parent
CONFIG_FILE = APP_DIR / "config.json"
QML_MAIN = APP_DIR / "qml" / "Main.qml"


class Config(QObject):
    borderColorChanged = Signal()
    numberOfPicturesChanged = Signal()
    borderWidthChanged = Signal()
    panelHeightChanged = Signal()
    horizontalScaleChanged = Signal()
    verticalScaleChanged = Signal()
    searchHintChanged = Signal()
    showSearchHintChanged = Signal()
    searchBgChanged = Signal()
    searchTextChanged = Signal()

    def __init__(self, data, parent=None):
        super().__init__(parent)
        self._data = data

    def get(self, key, default=None):
        return self._data.get(key, default)

    @Property(str, notify=borderColorChanged)
    def borderColor(self):
        return self._data.get("border_color", "#b4befe")

    @Property(int, notify=borderWidthChanged)
    def borderWidth(self):
        try:
            return int(self._data.get("border_width", 4))
        except:
            return 4

    @Property(int, notify=numberOfPicturesChanged)
    def numberOfPictures(self):
        try:
            return int(self._data.get("number_of_pictures", 7))
        except:
            return 7

    @Property(int, notify=panelHeightChanged)
    def panelHeight(self):
        try:
            return int(self._data.get("panel_height", 500))
        except:
            return 500

    @Property(float, notify=horizontalScaleChanged)
    def horizontalScale(self):
        try:
            return float(self._data.get("selected_horizontal_scale", 1.6))
        except:
            return 1.6

    @Property(float, notify=verticalScaleChanged)
    def verticalScale(self):
        try:
            return float(self._data.get("selected_vertical_scale", 1.1))
        except:
            return 1.1

    @Property(str, notify=searchHintChanged)
    def searchHintText(self):
        return self._data.get("search_hint_text", "Press Ctrl + F or / to search")

    @Property(bool, notify=showSearchHintChanged)
    def showSearchHint(self):
        return bool(self._data.get("show_search_hint", True))

    @Property(str, notify=searchHintChanged)
    def searchToggleKey(self):
        return self._data.get("search_toggle_key", "Ctrl+F or /")

    @Property(str, notify=searchBgChanged)
    def searchBackgroundColor(self):
        return self._data.get("search_background_color", "#313244")

    @Property(str, notify=searchTextChanged)
    def searchTextColor(self):
        return self._data.get("search_text_color", "#cdd6f4")

    @Property(str, notify=searchHintChanged)
    def searchHintColor(self):
        return self._data.get("search_hint_color", "#a6adc8")

    @Property(str, notify=borderColorChanged)
    def carouselSelectedBorder(self):
        return self._data.get("carousel_selected_border", "#b4befe")

    # Transition — defaults are grow 0.5,0.5 1.2 60 as requested, but any awww type is allowed
    @Property(str, notify=searchHintChanged)
    def transitionType(self):
        return self._data.get("transition_type", "grow")

    @Property(str, notify=searchHintChanged)
    def transitionPos(self):
        return self._data.get("transition_pos", "0.5,0.5")

    @Property(float, notify=searchHintChanged)
    def transitionDuration(self):
        try:
            return float(self._data.get("transition_duration", 1.2))
        except:
            return 1.2

    @Property(int, notify=searchHintChanged)
    def transitionFps(self):
        try:
            return int(self._data.get("transition_fps", 60))
        except:
            return 60


class Backend(QObject):
    availableColorsChanged = Signal(list)
    activeColorChanged = Signal(str)
    wallpapersChanged = Signal()
    wallpaperApplied = Signal(str)

    def __init__(self, repo: WallpaperRepository, store: MetadataStore, model: WallpaperModel, config: dict):
        super().__init__()
        self.repo = repo
        self.store = store
        self.model = model
        self.config = config
        self.active_color = None
        self.search_query = ""
        self._available = []

    def refresh(self):
        self.store.load()
        self.repo.set_metadata(self.store.data)
        self._available = self.repo.get_available_colors()
        self.availableColorsChanged.emit(self._available)
        self.activeColorChanged.emit(self.active_color or "")
        self._apply_filters()

    def _apply_filters(self):
        # base: all wallpapers
        wallpapers = self.repo.get_all()
        # color filter
        if self.active_color:
            wallpapers = [p for p in wallpapers if self.store.data.get(p.name, {}).get("color_group") == self.active_color]
        # search filter
        if self.search_query:
            q = self.search_query.casefold().strip()
            wallpapers = [p for p in wallpapers if q in p.name.casefold()]
        self.model.set_items(wallpapers, self.store.data)
        self.wallpapersChanged.emit()

    @Slot(str)
    def setFilter(self, color: str):
        self.active_color = color if color else None
        self.activeColorChanged.emit(self.active_color or "")
        self._apply_filters()

    @Slot(str)
    def setSearch(self, query: str):
        self.search_query = query or ""
        self._apply_filters()

    @Slot(str)
    def applyWallpaper(self, path: str):
        try:
            t_type = self.config.get("transition_type", "grow")
            t_pos = self.config.get("transition_pos", "0.5,0.5")
            t_dur = self.config.get("transition_duration", 1.2)
            try:
                t_dur = float(t_dur)
            except:
                t_dur = 1.2
            fps = int(self.config.get("transition_fps", 60))
            awww_backend.apply_wallpaper(
                Path(path),
                transition_type=t_type,
                transition_pos=t_pos,
                transition_duration=t_dur,
                transition_fps=fps,
            )
            self.wallpaperApplied.emit(path)
        except Exception as e:
            print(f"apply failed: {e}", file=sys.stderr)


def load_config():
    if not CONFIG_FILE.is_file():
        return {
            "wallpaper_path": str(Path.home() / "Pictures/Wallpapers"),
            "cache_path": str(Path.home() / ".cache/hyprroll/thumbs"),
            "number_of_pictures": 7,
            "border_color": "#b4befe",
            "border_width": 4,
            "panel_height": 500,
            "selected_horizontal_scale": 1.6,
            "selected_vertical_scale": 1.1,
            "cache_batch_size": 20,
            "backend": "awww",
            "transition_type": "wipe",
            "transition_fps": 30,
        }
    with CONFIG_FILE.open("r", encoding="utf-8") as fh:
        return json.load(fh)


def main():
    config_data = load_config()
    wallpaper_dir = Path(config_data.get("wallpaper_path", "~/Pictures/Wallpapers")).expanduser()
    cache_dir = Path(config_data.get("cache_path", "~/.cache/hyprroll/thumbs")).expanduser()

    wallpaper_dir.mkdir(parents=True, exist_ok=True)
    cache_dir.mkdir(parents=True, exist_ok=True)

    # Enable alpha buffer for transparent overlay
    fmt = QSurfaceFormat.defaultFormat()
    fmt.setAlphaBufferSize(8)
    QSurfaceFormat.setDefaultFormat(fmt)
    app = QGuiApplication(sys.argv)
    app.setApplicationName("hyprroll")
    app.setApplicationDisplayName("hyprroll")
    try:
        app.setDesktopFileName("hyprroll")
    except AttributeError:
        pass
    app.setOrganizationName("hyprroll")


    repo = WallpaperRepository(wallpaper_dir)
    store = MetadataStore(cache_dir / "metadata.json")
    model = WallpaperModel()
    model.set_dirs(wallpaper_dir, cache_dir)

    repo.refresh()
    # ensure metadata loaded
    store.load()
    repo.set_metadata(store.data)

    backend = Backend(repo, store, model, config_data)
    config_qobj = Config(config_data)

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("wallpaperModel", model)
    engine.rootContext().setContextProperty("backend", backend)
    engine.rootContext().setContextProperty("config", config_qobj)

    engine.load(QUrl.fromLocalFile(str(QML_MAIN)))
    if not engine.rootObjects():
        print("Failed to load QML", file=sys.stderr)
        return 1

    # initial populate after QML loaded
    backend.refresh()

    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
