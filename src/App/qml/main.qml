import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material

ApplicationWindow {
    id: mainWindowID

    visible: true

    Material.theme: Material.Dark
    Material.accent: '#9C6BFF'

    minimumWidth: 900
    minimumHeight: 620
    title: "Torrent Video Player"

    Loader {
        id: mainWindowLoaderID

        anchors.fill: parent
        source: "MainWindow.qml"
        focus: true
    }
}