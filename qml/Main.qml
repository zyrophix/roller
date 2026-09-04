import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: win
    title: "hyprroll"
    width: Screen.width
    height: Screen.height
    visible: true
    visibility: Window.FullScreen
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    property int selectedIndex: 0
    property int prevCount: 0

    function applySelected() {
        if (!wallpaperModel || wallpaperModel.countProp === 0) return
        let path = wallpaperModel.get_path_at(carousel.selectedIndex)
        if (path && backend) backend.applyWallpaper(path)
    }

    function updateFilter(color) {
        if (backend) backend.setFilter(color)
    }
    function updateSearch(query) {
        if (backend) backend.setSearch(query)
    }

    // Click on empty area closes picker
    MouseArea {
        anchors.fill: parent
        onClicked: Qt.quit()
    }

    // Centered content 678 tall on fullscreen transparent
    Item {
        id: content
        width: win.width
        height: 678
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 0
            spacing: 0

            // Search bar — always visible as requested
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                visible: true
                ColorFilter {
                    id: colorFilter
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
                borderWidth: config ? config.borderWidth : 4
                panelHeight: config ? config.panelHeight : 500
                horizontalScale: config ? config.horizontalScale : 1.6
                verticalScale: config ? config.verticalScale : 1.1
                countVisible: config ? config.numberOfPictures || 5 : 5
                onSelectedIndexChanged: win.selectedIndex = selectedIndex
                onWallpaperClicked: (idx) => win.selectedIndex = idx
                onApplyRequested: (idx) => {
                    win.selectedIndex = idx
                    win.applySelected()
                }
            }

            // Selected wallpaper name — above counter as requested
            Label {
                id: nameLabel
                Layout.alignment: Qt.AlignHCenter
                text: {
                    let cnt = wallpaperModel ? wallpaperModel.countProp : 0
                    if (cnt > 0) return wallpaperModel.get_name_at(carousel.selectedIndex)
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
                        let idx = ((Math.round(carousel.visualSelection) % cnt) + cnt) % cnt
                        return (idx + 1) + " / " + cnt
                    }
                    return "No wallpapers — check wallpaper_path in config.json"
                }
                color: "#aaaaaa"
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
            if (k === Qt.Key_Escape && colorFilter.searchMode) {
                colorFilter.searchMode = false
                win.updateSearch("")
                keyHandler.forceActiveFocus()
                event.accepted = true
                return
            }
            if (k === Qt.Key_Slash || key === "/" || (ctrl && k === Qt.Key_F)) {
                colorFilter.searchMode = !colorFilter.searchMode
                if (!colorFilter.searchMode) win.updateSearch("")
                event.accepted = true
                return
            }
            if (key === "j" || k === Qt.Key_Right) { carousel.next(); event.accepted = true }
            else if (key === "k" || k === Qt.Key_Left) { carousel.prev(); event.accepted = true }
            else if (key === "d") { carousel.jumpForward(); event.accepted = true }
            else if (key === "u") { carousel.jumpBack(); event.accepted = true }
            else if (k === Qt.Key_Return || k === Qt.Key_Enter || key === " ") { win.applySelected(); event.accepted = true }
            else if (k === Qt.Key_Escape) { Qt.quit(); event.accepted = true }
        }
        Component.onCompleted: forceActiveFocus()
    }

    // Keep focus after interactions — don't steal from search field
    Connections {
        target: backend
        function onAvailableColorsChanged(colors) { colorFilter.setAvailable(colors) }
        function onActiveColorChanged(c) { colorFilter.setActive(c) }
        function onWallpapersChanged() {
            let cnt = wallpaperModel ? wallpaperModel.countProp : 0
            if (win.prevCount === 0 && cnt > 0) {
                let start = Math.floor(carousel.countVisible / 2) % cnt
                carousel.setSelectedImmediate(start)
                win.selectedIndex = start
            } else {
                carousel.setSelectedImmediate(0)
                win.selectedIndex = 0
            }
            win.prevCount = cnt
            if (!colorFilter.searchMode) {
                keyHandler.forceActiveFocus()
            }
            // when in searchMode, keep focus on searchField (handled by ColorFilter)
        }
        function onWallpaperApplied(path) {
            // Uncomment to auto-close after apply:
            // Qt.quit()
        }
    }

    Component.onCompleted: {
        keyHandler.forceActiveFocus()
    }

    // Click outside carousel but inside dim also closes (via background MouseArea)
}
