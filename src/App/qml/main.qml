import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material

import "ErrorMessageDialog"
import "SettingsDialog"

ApplicationWindow {
    id: mainWindowID

    Connections {
        target: guiController
        function onShowErrorMessage(text, informativeText) {
            errorMessageLoaderID.setSource("ErrorMessageDialog/ErrorMessageDialog.qml", {
                "text": text,
                "informativeText": informativeText
            })
        }
    }

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

    Shortcut {
        sequences: ["Ctrl+R"]
        context: Qt.ApplicationShortcut
        onActivated: {
            if (!guiController.IsDebug())
                return

            guiController.BumpHotReloadToken()
            const base = Qt.resolvedUrl("MainWindow.qml")
            mainWindowLoaderID.source = ""
            mainWindowLoaderID.source = base + "?r=" + Date.now()
        }
    }

    Loader {
        id: errorMessageLoaderID
        onLoaded: item.open()
    }

    ErrorMessageDialog {
        id: errorMessageDialogID
    }

    Loader {
        id: mainWindowLoaderID

        anchors.fill: parent
        source: "MainWindow.qml"
        focus: true
    }
}