import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls.Material

import "Player"
import "SettingsDialog"

ApplicationWindow {
    id: mainWindowID

    visible: true

    Material.theme: Material.Dark
    Material.accent: Material.Purple

    minimumWidth: 640
    minimumHeight: 530
    title: "Torrent Video Player"

    MenuBar {
        Menu {
            title: qsTr("TorrentPlayer")
            MenuItem {
                text: qsTr("Settings…")
                onTriggered: settingsDialogID.open()
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Quit")
                onTriggered: Qt.quit()
            }
        }
    }

    SettingsDialog {
        id: settingsDialogID
    }

    ColumnLayout {
        anchors.fill: parent

        Player {
            id: playerID

            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}