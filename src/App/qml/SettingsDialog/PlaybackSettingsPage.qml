import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

StyledScrollView {
    id: rootID

    property var audioTracks: []
    property int activeAudioTrack: -1

    signal audioTrackActivated(int index)

    SettingsPageHeader {
        title: qsTr("Playback")
        description: qsTr("Select the audio stream used for the current video.")
    }

    SettingsSection {
        Layout.topMargin: 8

        title: qsTr("Audio")

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Label {
                text: qsTr("Audio track")
            }

            Item {
                Layout.fillWidth: true
            }

            ComboBox {
                Layout.preferredHeight: 30
                Layout.preferredWidth: Math.min(360, rootID.availableWidth * 0.62)

                enabled: rootID.audioTracks.length > 0
                model: rootID.audioTracks
                currentIndex: rootID.activeAudioTrack
                displayText: enabled && currentIndex >= 0
                    ? rootID.audioTracks[currentIndex]
                    : qsTr("No audio tracks")

                onActivated: function(index) {
                    rootID.audioTrackActivated(index)
                }
            }
        }
    }
}
