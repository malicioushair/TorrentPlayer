pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls.Material

import "colors.js" as Colors

Dialog {
    id: rootID

    property int currentSection: 0
    property string pendingSavePath: guiController.savePath
    property int pendingAudioTrack: guiController.activeAudioTrack

    function localPath(folderUrl) {
        const encodedPath = folderUrl.toString()
        if (!encodedPath.startsWith("file://"))
            return decodeURIComponent(encodedPath)

        let path = decodeURIComponent(encodedPath.substring(7))
        if (Qt.platform.os === "windows"
                && path.length >= 3
                && path[0] === "/"
                && path[2] === ":") {
            path = path.substring(1)
        }
        return path
    }

    parent: Overlay.overlay
    anchors.centerIn: parent

    width: Math.min(900, parent.width - 32)
    height: Math.min(620, parent.height - 32)
    padding: 0
    topPadding: 0
    spacing: 0

    title: qsTr("Settings")
    modal: true
    focus: true

    background: Rectangle {
        color: Colors.SettingsDialog.background
        radius: 8
        border.color: Colors.SettingsDialog.border
    }

    onOpened: currentSection = 0
    onAccepted: {
        guiController.savePath = generalSettingsID.savePath
        guiController.activeAudioTrack = playbackSettingsID.activeAudioTrack
        subtitlesSettingsID.applySubtitleSettings()
    }

    header: Rectangle {
        implicitHeight: 68
        color: Colors.SettingsDialog.headerBackground

        topLeftRadius: 10
        topRightRadius: topLeftRadius

        Rectangle {
            anchors.bottom: parent.bottom

            width: parent.width
            height: 1

            color: Colors.SettingsDialog.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 16

            Label {
                text: rootID.title
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                text: "×"
                font.pixelSize: 26
                Accessible.name: qsTr("Close settings")
                onClicked: rootID.reject()
            }
        }
    }

    footer: DialogButtonBox {
        leftPadding: 16
        rightPadding: 16
        topPadding: 12
        bottomPadding: 12
        spacing: 12

        background: Rectangle {
            color: Colors.SettingsDialog.background

            bottomLeftRadius: 10
            bottomRightRadius: bottomLeftRadius

            Rectangle {
                anchors.top: parent.top

                width: parent.width
                height: 1

                color: Colors.SettingsDialog.border
            }
        }

        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }

        Button {
            text: qsTr("Apply")
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    FolderDialog {
        id: folderDialogID

        currentFolder: rootID.pendingSavePath
        onAccepted: rootID.pendingSavePath = rootID.localPath(selectedFolder)
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: Math.min(210, rootID.width * 0.28)
            Layout.fillHeight: true

            color: Colors.SettingsDialog.sidebarBackground

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 4

                Repeater {
                    model: [
                        qsTr("General"),
                        qsTr("Playback"),
                        qsTr("Subtitles"),
                    ]

                    delegate: SettingsNavigationButton {
                        required property int index
                        required property string modelData

                        text: modelData
                        selected: rootID.currentSection === index
                        onClicked: rootID.currentSection = index
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 10

            currentIndex: rootID.currentSection

            GeneralSettingsPage {
                id: generalSettingsID

                savePath: rootID.pendingSavePath
                onChooseFolderRequested: folderDialogID.open()
            }

            PlaybackSettingsPage {
                id: playbackSettingsID

                audioTracks: guiController.audioTracks
                activeAudioTrack: rootID.pendingAudioTrack
                onAudioTrackActivated: function(index) {
                    rootID.pendingAudioTrack = index
                }
            }

            SubtitlesSettingsPage {
                id: subtitlesSettingsID
            }
        }
    }
}
