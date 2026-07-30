import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material

import "colors.js" as Colors

ApplicationWindow {
    id: mainWindowID

    width: 1200
    height: 760

    visible: true

    Material.theme: Material.Dark
    Material.accent: Colors.App.accent
    Material.background: Colors.App.background
    Material.foreground: Colors.App.foreground

    minimumWidth: 900
    minimumHeight: 620
    color: Colors.App.background
    title: "TorrentPlayer"

    Loader {
        id: mainWindowLoaderID

        anchors.fill: parent
        source: "MainWindow.qml"
        focus: true
    }
}
