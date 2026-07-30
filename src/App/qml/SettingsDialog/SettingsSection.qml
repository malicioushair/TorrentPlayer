import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import "colors.js" as Colors

Frame {
    id: rootID

    default property alias items: contentLayoutID.data
    property alias title: titleID.text
    property string statusText: ""
    property color statusColor: Colors.SettingsDialog.secondaryText
    property bool separatorVisible: true

    Layout.fillWidth: true

    padding: 20
    background: Rectangle {
        color: Colors.SettingsDialog.sectionBackground
        radius: 8
        border.color: Colors.SettingsDialog.border
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Label {
                id: titleID

                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: rootID.statusText
                color: rootID.statusColor
                visible: rootID.statusText !== ""
            }
        }

        SettingsSeparator {
            visible: rootID.separatorVisible
        }

        ColumnLayout {
            id: contentLayoutID

            Layout.fillWidth: true
            spacing: 12
        }
    }
}
