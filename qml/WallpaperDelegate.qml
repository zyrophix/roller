import QtQuick
import QtQuick.Controls

Item {
    id: delegate
    required property string wallpaperPath
    required property string wallpaperName
    required property string thumbnailPath
    required property string colorGroup
    required property int index
    property bool isSelected: false
    property string borderColor: "#4B4B50"
    signal clicked(int idx)

    width: 180
    height: 120

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 14
        color: "#1e1e1e"
        border.width: isSelected ? 3 : 0
        border.color: borderColor
        clip: true

        Image {
            id: img
            anchors.fill: parent
            anchors.margins: isSelected ? 3 : 0
            source: delegate.thumbnailPath
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            smooth: true
            mipmap: true
        }

        // shear effect container
        transform: Scale {
            // horizontal/vertical expansion for selected
            xScale: isSelected ? 1.0 : 1.0
            yScale: isSelected ? 1.0 : 1.0
        }

        // subtle overlay for non-selected
        Rectangle {
            anchors.fill: parent
            color: "#000000"
            opacity: isSelected ? 0 : 0.18
            radius: bg.radius
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 6
            text: delegate.wallpaperName
            color: "white"
            elide: Text.ElideMiddle
            font.pixelSize: 9
            opacity: isSelected ? 0.95 : 0
            visible: isSelected
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: delegate.clicked(delegate.index)
    }
}
