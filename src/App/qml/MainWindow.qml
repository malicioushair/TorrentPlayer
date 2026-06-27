import QtQuick
import QtQuick.Layouts

import "Player"

ColumnLayout {
    anchors.fill: parent

    Player {
        id: playerID

        Layout.fillWidth: true
        Layout.fillHeight: true
    }
}
