import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Search-only bar with hint
Rectangle {
    id: root
    signal searchChanged(string query)

    property bool searchMode: false
    property string hintText: "Press Ctrl + F or / to search"
    property bool showHint: true
    property string searchBg: "#313244"
    property string searchTextColor: "#cdd6f4"
    property string searchHintColor: "#a6adc8"

    color: searchBg
    radius: 10
    height: 48
    visible: true
    implicitWidth: 520

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Rectangle {
            width: 32; height: 32; radius: 8
            color: Qt.rgba(1,1,1,0.10)
            Text { anchors.centerIn: parent; text: "‹"; color: "white"; font.pixelSize: 18 }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.searchMode = false
                    searchField.text = ""
                    root.searchChanged("")
                }
            }
        }

        TextField {
            id: searchField
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            placeholderText: "Search wallpapers"
            color: searchTextColor
            placeholderTextColor: searchHintColor
            background: Rectangle {
                color: Qt.lighter(searchBg, 1.15)
                radius: 8
            }
            leftPadding: 10
            rightPadding: 10
            onTextChanged: root.searchChanged(text)
            onActiveFocusChanged: if (activeFocus) root.searchMode = true
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Escape) {
                    root.searchMode = false
                    searchField.text = ""
                    root.searchChanged("")
                    event.accepted = true
                }
            }
        }
    }

    // Hint below search field — shown when enabled
    Text {
        id: hint
        anchors.top: parent.bottom
        anchors.topMargin: 6
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.hintText
        color: searchHintColor
        font.pixelSize: 10
        visible: root.showHint
    }

    function setSearchMode(on) { root.searchMode = on }
    function isSearchMode() { return root.searchMode }

    onSearchModeChanged: {
        if (searchMode) searchField.forceActiveFocus()
        else {
            searchField.focus = false
            if (searchField.text !== "") {
                searchField.text = ""
                root.searchChanged("")
            }
        }
    }
}
