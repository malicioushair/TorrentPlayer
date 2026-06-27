import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material

Dialog {
    id: settingsDialogID

    property string customSavePath: guiController.savePath

    anchors.centerIn: parent

    width: 600
    height: 260

    title: qsTr("Settings")
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel

    ColumnLayout {
        id: settingsDialogLayoutID

        anchors.fill: parent

        spacing: 16

        SettingsDialogItem {
            id: savePathItemID

            label.text: qsTr("Files saved to: ") + guiController.savePath
            button {
                text: "📂"
                onClicked: folderDialogID.open()
            }

            FolderDialog {
                id: folderDialogID

                currentFolder: guiController.savePath
                onAccepted: guiController.savePath = currentFolder
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            spacing: 10

            Label {
                text: qsTr("Audio track:")
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            ComboBox {
                id: audioTrackComboBoxID

                Layout.preferredWidth: 320

                enabled: guiController.audioTracks.length > 0
                model: guiController.audioTracks
                currentIndex: guiController.activeAudioTrack
                displayText: enabled && currentIndex >= 0
                    ? guiController.audioTracks[currentIndex]
                    : qsTr("No audio tracks")

                onActivated: function(index) {
                    guiController.activeAudioTrack = index
                }
            }
        }

    }
    onAccepted: guiController.savePath = folderDialogID.currentFolder
}