import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import "../colors.js" as Colors

Button {
    id: rootID

    property bool selected: false

    Layout.fillWidth: true
    implicitHeight: 48

    flat: true

    background: Rectangle {
        radius: 6
        color: rootID.selected
            ? Colors.SettingsDialog.navigationSelected
            : rootID.hovered
                ? Colors.SettingsDialog.navigationHovered
                : Colors.App.transparent

        Rectangle {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter

            width: 3
            height: 28
            radius: 2

            color: Material.accent
            visible: rootID.selected
        }
    }

    contentItem: Label {
        leftPadding: 18

        text: rootID.text
        color: rootID.selected ? Material.accent : Material.foreground
        font.pixelSize: 15
        font.weight: rootID.selected ? Font.DemiBold : Font.Normal
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
