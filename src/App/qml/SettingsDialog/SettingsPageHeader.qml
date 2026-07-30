import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import "../colors.js" as Colors

ColumnLayout {
    property alias title: titleID.text
    property alias description: descriptionID.text

    Layout.fillWidth: true
    spacing: 8

    Label {
        id: titleID

        Layout.alignment: Qt.AlignTop

        font.pixelSize: 18
        font.weight: Font.DemiBold
    }

    Label {
        id: descriptionID

        Layout.fillWidth: true

        color: Colors.SettingsDialog.secondaryText
        wrapMode: Text.WordWrap
    }
}
