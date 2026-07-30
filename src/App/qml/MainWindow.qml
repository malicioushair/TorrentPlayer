import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import "ErrorMessageDialog"
import "SettingsDialog"
import "Player"

Item {
    id: rootID

    Connections {
        target: guiController
        function onShowErrorMessage(text, informativeText) {
            errorMessageLoaderID.setSource("ErrorMessageDialog/ErrorMessageDialog.qml", {
                "text": text,
                "informativeText": informativeText
            })
        }
    }

    Connections {
        target: settingsDialogLoaderID.item
        ignoreUnknownSignals: true

        function onClosed() {
            settingsDialogLoaderID.source = ""
        }
    }

    MenuBar {
        Menu {
            title: qsTr("TorrentPlayer")
            MenuItem {
                text: qsTr("Settings…")
                onTriggered: {
                    settingsDialogLoaderID.setSource("SettingsDialog/SettingsDialog.qml")
                }
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Quit")
                onTriggered: Qt.quit()
            }
        }
    }

    Loader {
        id: settingsDialogLoaderID

        onLoaded: item.open()
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

    ColumnLayout {
        anchors.fill: parent

        Player {
            id: playerID

            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
