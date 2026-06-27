import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material

ApplicationWindow {
    id: mainWindowID

    visible: true

    Material.theme: Material.Dark
    Material.accent: Material.Purple

    minimumWidth: 640
    minimumHeight: 530
    title: "Torrent Video Player"

    Loader {
        id: mainWindowLoaderID

        anchors.fill: parent
        source: "MainWindow.qml"
        focus: true
    }
}