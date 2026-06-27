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
