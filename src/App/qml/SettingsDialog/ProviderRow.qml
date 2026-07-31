import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import "../colors.js" as Colors

RowLayout {
    id: rootID

    property alias providerName: providerNameID.text
    property alias statusText: statusID.text
    property bool configured: false

    signal configureRequested

    Layout.fillWidth: true

    Label {
        id: providerNameID

        color: Colors.SettingsDialog.secondaryText
        wrapMode: Text.WordWrap
    }

    Item {
        Layout.fillWidth: true
    }

    Label {
        id: statusID

        color: rootID.configured
            ? Colors.SettingsDialog.configured
            : Colors.SettingsDialog.secondaryText
    }

    Button {
        Layout.preferredWidth: 113
        Layout.preferredHeight: 40

        text: qsTr("Configure")
        onClicked: rootID.configureRequested()
    }
}
