pragma ComponentBehavior: Bound

import QtQuick

import "../colors.js" as Colors

Item {
    id: rootID

    property alias text: textID.text
    property alias font: textID.font
    property alias horizontalAlignment: textID.horizontalAlignment
    property alias wrapMode: textID.wrapMode
    property alias color: textID.color
    property color outlineColor: Colors.Player.subtitleOutline

    readonly property real outlineWidth: 4

    implicitWidth: textID.implicitWidth + outlineWidth * 2
    implicitHeight: textID.implicitHeight + outlineWidth * 2
    // Keep wrapped text the same width as the outer item
    width: textID.width + outlineWidth * 2
    Repeater {
        model: [
            Qt.point(-1, 0), Qt.point(1, 0),
            Qt.point(0, -1), Qt.point(0, 1),
            Qt.point(-1, -1), Qt.point(1, -1),
            Qt.point(-1, 1), Qt.point(1, 1)
        ]
        Text {
            x: rootID.outlineWidth + modelData.x * rootID.outlineWidth
            y: rootID.outlineWidth + modelData.y * rootID.outlineWidth
            width: textID.width
            text: textID.text
            font: textID.font
            color: rootID.outlineColor
            horizontalAlignment: textID.horizontalAlignment
            wrapMode: textID.wrapMode
        }
    }
    Text {
        id: textID
        x: rootID.outlineWidth
        y: rootID.outlineWidth
        width: rootID.width > 0 ? rootID.width - rootID.outlineWidth * 2 : implicitWidth
        color: "white"
    }
}