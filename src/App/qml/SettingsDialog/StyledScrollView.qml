import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

ScrollView {
    id: rootID

    default property alias items: contentLayoutID.data

    contentWidth: availableWidth
    clip: true
    leftPadding: 28
    rightPadding: leftPadding
    topPadding: 0
    bottomPadding: 0

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ColumnLayout {
        id: contentLayoutID

        width: rootID.availableWidth
        spacing: 8
    }
}
