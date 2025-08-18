import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls.Material

import "Player"
import "SettingsDialog"
import "ErrorMessageDialog"

ApplicationWindow {
    id: mainWindowID

    Connections {
        target: guiController
        function onShowErrorMessage(text, informativeText) {
            errorMessageLoaderID.setSource("qrc:/ErrorMessageDialog/ErrorMessageDialog.qml", {
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