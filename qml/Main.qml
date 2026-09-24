import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import org.kde.layershell 1.0 as LayerShell

ApplicationWindow {
    id: win
    title: "roller"
    width: Screen.width
    height: Screen.height
    visible: true
    color: "transparent"
    flags: Qt.FramelessWindowHint
    // LayerShell overlay fullscreen transparent — covers but doesn't shift windows
    LayerShell.Window.layer: LayerShell.Window.LayerOverlay
    LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorBottom | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
    LayerShell.Window.exclusionZone: -1
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityExclusive
    LayerShell.Window.scope: "roller"

    property int prevCount: 0
    property string applyError: ""

    function applySelected() {
        win.applyError = ""
        if (!wallpaperModel || wallpaperModel.countProp === 0) return
        let path = wallpaperModel.get_path_at(carousel.committedIndex)
        if (path && backend) backend.applyWallpaper(path)
    }

    // index of the restored wallpaper in the current (filtered) model, or 0
    function restoreIndex() {
        if (!config || !config.restoreLast) return 0
        let want = config.lastWallpaper
        if (!want) return 0
        let cnt = wallpaperModel ? wallpaperModel.countProp : 0
        for (let i = 0; i < cnt; ++i)
            if (wallpaperModel.get_path_at(i) === want) return i
        return 0
    }

    function updateSearch(query) {
        if (backend) backend.setSearch(query)
    }

    Timer { id: quitTimer; interval: 100; onTriggered: Qt.quit() }
    function fadeQuit() { if (quitTimer.running) return; content.opacity = 0; quitTimer.restart() }

    MouseArea {
        anchors.fill: parent
        onClicked: fadeQuit()
    }

    Item {
        id: content
        width: win.width
        height: 678
        anchors.centerIn: parent
        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
        Component.onCompleted: opacity = 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 0
            spacing: 0

            // Search bar — always visible as requested
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                visible: true
                SearchBar {
                    id: searchBar
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 16
                    width: Math.min(840, parent.width - 32)
                    hintText: config ? config.searchHintText : "Press Ctrl + F or / to search"
                    showHint: config ? config.showSearchHint : true
                    searchBg: config ? config.searchBackgroundColor : "#313244"
                    searchTextColor: config ? config.searchTextColor : "#cdd6f4"
                    searchHintColor: config ? config.searchHintColor : "#a6adc8"
                    onSearchChanged: (q) => win.updateSearch(q)
                }
            }

            // Carousel — centered, panel_y = 121 equivalent
            Carousel {
                id: carousel
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: 16
                Layout.bottomMargin: 16
                model: wallpaperModel
                borderColor: config ? config.carouselSelectedBorder : "#b4befe"
                borderWidth: config ? config.borderWidth : 2
                idleBorderWidth: config ? config.idleBorderWidth : 2
                idleBorderColor: config ? config.idleBorderColor : "#585b70"
                panelHeight: config ? config.panelHeight : 500
                horizontalScale: config ? config.horizontalScale : 1.6
                verticalScale: config ? config.verticalScale : 1.1
                thumbnailHeight: config ? config.thumbnailHeight : 512
                countVisible: config ? config.numberOfPictures || 5 : 5
                onWallpaperClicked: (idx) => { /* selection already set by the delegate */ }
                onApplyRequested: (idx) => {
                    carousel.setSelected(idx)
                    win.applySelected()
                }
            }

            // Selected wallpaper name — above counter as requested
            Label {
                id: nameLabel
                Layout.alignment: Qt.AlignHCenter
                text: {
                    let cnt = wallpaperModel ? wallpaperModel.countProp : 0
                    if (cnt > 0) return wallpaperModel.get_name_at(carousel.committedIndex)
                    return ""
                }
                color: "#cdd6f4"
                font.pixelSize: 13
                visible: wallpaperModel ? wallpaperModel.countProp > 0 : false
                elide: Text.ElideMiddle
                Layout.maximumWidth: parent.width - 32
            }
            // Bottom counter — no negative, uses modulo
            Label {
                id: countLabel
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 8
                text: {
                    let cnt = wallpaperModel ? wallpaperModel.countProp : 0
                    if (cnt > 0) {
                        let idx = carousel.committedIndex
                        return (idx + 1) + " / " + cnt
                    }
                    return "No wallpapers — check wallpaper_path in config.json"
                }
                color: "#aaaaaa"
                font.pixelSize: 11
            }
            // Backend failure toast
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: win.applyError
                visible: win.applyError !== ""
                color: "#f38ba8"
                font.pixelSize: 11
            }
        }
    }

    // Keyboard handler — Ctrl+F or / toggles search, Esc clears search or quits
    Item {
        id: keyHandler
        focus: true
        Keys.onPressed: (event) => {
            let key = event.text.toLowerCase()
            let k = event.key
            let ctrl = event.modifiers & Qt.ControlModifier
            if (k === Qt.Key_Escape && searchBar.searchMode) {
                searchBar.clearSearch()
                keyHandler.forceActiveFocus()
                event.accepted = true
                return
            }
            if (k === Qt.Key_Slash || key === "/" || (ctrl && k === Qt.Key_F)) {
                let willBeSearch = !searchBar.searchMode
                if (!willBeSearch) searchBar.clearSearch()
                else searchBar.searchMode = true
                event.accepted = true
                return
            }
            if (key === "l" || k === Qt.Key_Right) { carousel.next(); event.accepted = true }
            else if (key === "h" || k === Qt.Key_Left) { carousel.prev(); event.accepted = true }
            else if (key === "d") { carousel.jumpForward(); event.accepted = true }
            else if (key === "u") { carousel.jumpBack(); event.accepted = true }
            else if (k === Qt.Key_Return || k === Qt.Key_Enter || key === " ") {
                if (searchBar.searchMode) {
                    searchBar.searchMode = false
                    keyHandler.forceActiveFocus()
                    event.accepted = true
                    return
                }
                win.applySelected(); event.accepted = true
            }
            else if (k === Qt.Key_Escape) { fadeQuit(); event.accepted = true }
        }
        Component.onCompleted: forceActiveFocus()
    }

    Connections {
        target: searchBar
        function onSearchModeChanged() { if (!searchBar.searchMode) keyHandler.forceActiveFocus() }
    }

    Connections {
        target: backend
        function onWallpapersChanged() {
            let cnt = wallpaperModel ? wallpaperModel.countProp : 0
            if (cnt > 0) {
                if (win.prevCount === 0) carousel.setSelectedImmediate(win.restoreIndex())
                // filter keystrokes: re-center on the current index instead
                // of yanking selection back to 0 every time
                else carousel.setSelected(carousel.selectedIndex)
            }
            win.prevCount = cnt
            if (!searchBar.searchMode) {
                keyHandler.forceActiveFocus()
            }
            // when in searchMode, keep focus on searchField (handled by SearchBar)
        }
        function onWallpaperApplied(path, ok) {
            win.applyError = ok ? "" : "Failed to apply wallpaper — backend missing?"
            if (ok && config) config.saveLastWallpaper(path)
        }
    }

    Component.onCompleted: {
        keyHandler.forceActiveFocus()
    }

    // Click outside carousel but inside dim also closes (via background MouseArea)
}
