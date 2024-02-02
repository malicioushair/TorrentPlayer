import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

import "Player"

ApplicationWindow {
    id: mainWindowID

    visible: true

    Material.theme: Material.Dark
    Material.accent: Material.Purple

    minimumWidth: 640
    minimumHeight: 530
    title: "Torrent Video Player"

    ColumnLayout {
        anchors.fill: parent

        Player {
            id: playerID

            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}