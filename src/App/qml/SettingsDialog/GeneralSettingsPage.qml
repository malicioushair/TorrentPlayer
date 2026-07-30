import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import "colors.js" as Colors

StyledScrollView {
    id: rootID

    property string savePath: ""

    signal chooseFolderRequested

    SettingsPageHeader {
        title: qsTr("General")
        description: qsTr("Choose where downloaded videos and torrent data are stored.")
    }

    SettingsSection {
        Layout.topMargin: 8

        title: qsTr("Download location")

        Label {
            Layout.fillWidth: true

            text: qsTr("New downloads will be saved in this folder.")
            color: Colors.SettingsDialog.secondaryText
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                color: Colors.SettingsDialog.inputBackground
                radius: 4
                border.color: Colors.SettingsDialog.border

                Label {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12

                    text: rootID.savePath !== ""
                        ? rootID.savePath
                        : qsTr("Default Movies folder")
                    color: rootID.savePath !== ""
                        ? Material.foreground
                        : Colors.SettingsDialog.secondaryText
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideMiddle
                }
            }

            Button {
                Layout.preferredWidth: 113
                Layout.preferredHeight: 40

                text: qsTr("Choose…")
                onClicked: rootID.chooseFolderRequested()
            }
        }
    }
}
