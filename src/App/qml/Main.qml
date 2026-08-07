import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material

import "colors.js" as Colors

ApplicationWindow {
    id: mainWindowID

    function openSettings() {
        if (mainWindowLoaderID.item)
            mainWindowLoaderID.item.openSettings()
    }

    width: 1200
    height: 760

    visible: true

    Material.theme: Material.Dark
    Material.accent: Colors.App.accent
    Material.background: Colors.App.background
    Material.foreground: Colors.App.foreground

    minimumWidth: 900
    minimumHeight: 620
    color: Colors.App.background
    title: "TorrentPlayer"

    Action {
        id: settingsActionID

        text: qsTr("Settings…")
        shortcut: "Ctrl+,"
        onTriggered: mainWindowID.openSettings()
    }

    menuBar: guiController.IsMacOS() ? macOSMenuID : null

    MenuBar {
        id: macOSMenuID

        Menu {
            title: qsTr("TorrentPlayer")

            MenuItem {
                action: settingsActionID
            }
        }
    }

    Loader {
        id: mainWindowLoaderID

        anchors.fill: parent
        source: "MainWindow.qml"
        focus: true
    }
}
