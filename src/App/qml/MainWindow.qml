import QtQuick

import "ErrorMessageDialog"
import "Player"

Item {
    id: rootID

    function openSettings() {
        settingsDialogLoaderID.setSource("SettingsDialog/SettingsDialog.qml")
    }

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

    Player {
        id: playerID

        anchors.fill: parent

        onOpenSettingsRequested: rootID.openSettings()
    }
}
