import QtMultimedia
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Layouts

import TorrentPlayer

import "../CustomControls"
import "../colors.js" as Colors

Item {
    id: rootID

    signal openSettingsRequested()

    readonly property bool hasMedia: {
        return videoID.mediaStatus !== MediaPlayer.NoMedia
    }
    readonly property bool fullScreen: rootID.Window.window
        && rootID.Window.window.visibility === Window.FullScreen
    property bool downloading: false
    property string infoBubbleText
    property bool playerOverlayVisible: true
    property real lastAudibleVolume: 1

    focus: true

    function displayFileName() {
        if (!hasMedia)
            return qsTr("Open a video or torrent")

        const path = videoID.source.toString()
        const name = path.substring(path.lastIndexOf("/") + 1)
        return decodeURIComponent(name)
    }

    function formatTime(milliseconds) {
        if (!Number.isFinite(milliseconds) || milliseconds < 0)
            return "0:00"

        const totalSeconds = Math.floor(milliseconds / 1000)
        const hours = Math.floor(totalSeconds / 3600)
        const minutes = Math.floor((totalSeconds % 3600) / 60)
        const seconds = totalSeconds % 60
        const minuteText = hours > 0
            ? minutes.toString().padStart(2, "0")
            : minutes.toString()
        const prefix = hours > 0 ? `${hours}:` : ""
        return `${prefix}${minuteText}:${seconds.toString().padStart(2, "0")}`
    }

    function mediaStatusText() {
        if (!hasMedia)
            return qsTr("Choose a local video or .torrent file")
        if (downloading)
            return qsTr("Streaming while the download continues")
        if (videoID.duration > 0)
            return qsTr("Ready to play")
        return qsTr("Preparing playback…")
    }

    function showPlayerOverlay() {
        playerOverlayVisible = true
        if (fullScreen)
            hideControlsTimerID.restart()
    }

    function showInfoBubble(message) {
        infoBubbleText = message
        infoBubbleID.visible = true
        infoBubbleVisibilityTimerID.restart()
    }

    function toggleFullScreen() {
        const window = rootID.Window.window
        if (!window)
            return

        const enterFullScreen = !fullScreen
        window.visibility = enterFullScreen ? Window.FullScreen : Window.Windowed
        showPlayerOverlay()
        if (!enterFullScreen)
            hideControlsTimerID.stop()
    }

    Rectangle {
        anchors.fill: parent

        color: Colors.Player.background
    }

    MediaPlayer {
        id: videoID

        activeAudioTrack: guiController.activeAudioTrack
        audioOutput: AudioOutput {
            volume: volumeControlID.value
        }
        videoOutput: videoOutputID

        onPositionChanged: SubtitlesController.SetPlaybackPosition(position)
    }

    VideoOutput {
        id: videoOutputID

        anchors.fill: parent

        fillMode: VideoOutput.PreserveAspectFit
    }

    Column {
        anchors.centerIn: parent
        z: 99

        spacing: 12
        visible: !rootID.hasMedia

        PlayerControlButton {
            anchors.horizontalCenter: parent.horizontalCenter

            prominent: true
            iconName: "folder"
            tooltipText: qsTr("Open video or torrent")
            onClicked: fileDialogID.open()
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter

            text: qsTr("Open a video or torrent")
            color: Colors.Player.emptyStateText
            font.pixelSize: 20
            font.weight: Font.DemiBold
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter

            text: qsTr("Choose a file to start watching")
            color: Colors.Player.emptyStateSecondaryText
            font.pixelSize: 14
        }
    }

    Rectangle {
        id: topScrimID

        z: 1
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        height: 150
        opacity: rootID.playerOverlayVisible ? 1 : 0

        gradient: Gradient {
            GradientStop {
                position: 0
                color: Colors.Player.topScrimStart
            }
            GradientStop {
                position: 1
                color: Colors.Player.scrimTransparent
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 160
            }
        }
    }

    Column {
        z: 2
        anchors {
            top: parent.top
            left: parent.left
            topMargin: 28
            leftMargin: 32
        }

        spacing: 5
        opacity: rootID.playerOverlayVisible ? 1 : 0

        Label {
            width: Math.min(implicitWidth, rootID.width * 0.58)

            text: rootID.displayFileName()
            color: Colors.Player.titleText
            elide: Text.ElideMiddle
            font.pixelSize: 20
            font.weight: Font.DemiBold
        }

        Label {
            text: rootID.mediaStatusText()
            color: Colors.Player.statusText
            font.pixelSize: 14
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 160
            }
        }
    }

    Rectangle {
        z: 2
        anchors {
            top: parent.top
            right: parent.right
            topMargin: 26
            rightMargin: 32
        }

        width: downloadStatusRowID.implicitWidth + 32
        height: 42
        radius: height / 2

        color: Colors.Player.downloadBadgeBackground
        border.width: 1
        border.color: Colors.Player.downloadBadgeBorder
        opacity: rootID.playerOverlayVisible && rootID.downloading ? 1 : 0
        visible: opacity > 0

        RowLayout {
            id: downloadStatusRowID

            anchors.centerIn: parent

            spacing: 9

            Label {
                text: "↓"
                color: Colors.Player.downloadIcon
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            Label {
                text: qsTr("Downloading %1%").arg(guiController.downloadProgress)
                color: Colors.Player.downloadText
                font.pixelSize: 14
                font.weight: Font.Medium
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 160
            }
        }
    }

    OutlinedText {
        id: subtitleTextID

        z: 2
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: rootID.playerOverlayVisible ? 202 : 36
        }

        width: parent.width * 0.8

        text: SubtitlesController.currentSubtitleText
        color: Colors.Player.subtitleText
        font.pixelSize: SubtitlesController.fontSize
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        visible: text.length > 0

        Behavior on anchors.bottomMargin {
            NumberAnimation {
                duration: 160
            }
        }
    }

    Rectangle {
        id: infoBubbleID

        z: 3
        anchors {
            top: parent.top
            right: parent.right
            topMargin: 82
            rightMargin: 32
        }

        width: infoBubbleTextID.implicitWidth + 24
        height: 38
        radius: height / 2

        color: Colors.Player.subtitleOffsetBackground
        border.width: 1
        border.color: Colors.Player.subtitleOffsetBorder
        visible: false

        Label {
            id: infoBubbleTextID

            anchors.centerIn: parent

            text: rootID.infoBubbleText
            color: Colors.Player.subtitleText
            font.pixelSize: 14
            font.weight: Font.Medium
        }
    }

    Rectangle {
        z: 1
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        height: 280
        opacity: rootID.playerOverlayVisible ? 1 : 0

        gradient: Gradient {
            GradientStop {
                position: 0
                color: Colors.Player.scrimTransparent
            }
            GradientStop {
                position: 1
                color: Colors.Player.bottomScrimEnd
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 160
            }
        }
    }

    MouseArea {
        id: videoAreaID

        z: 1
        anchors.fill: parent

        hoverEnabled: true
        cursorShape: rootID.playerOverlayVisible
            ? Qt.ArrowCursor
            : Qt.BlankCursor

        onClicked: rootID.forceActiveFocus()
        onDoubleClicked: rootID.toggleFullScreen()
        onEntered: rootID.showPlayerOverlay()
        onPositionChanged: rootID.showPlayerOverlay()
    }

    Item {
        id: controlsID

        property real radius: 8

        z: 3
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            leftMargin: 28
            rightMargin: 28
            bottomMargin: 26
        }

        height: 144
        opacity: rootID.playerOverlayVisible ? 1 : 0
        enabled: rootID.playerOverlayVisible

        Behavior on opacity {
            NumberAnimation {
                duration: 160
            }
        }

        MultiEffect {
            id: frostedControlsEffectID

            anchors.fill: parent

            visible: guiController.frostedGlassEnabled
                && rootID.hasMedia
                && controlsID.opacity > 0
            blurEnabled: true
            blur: 1.0
            blurMax: 64
            brightness: -0.3
            autoPaddingEnabled: false
            maskEnabled: true

            source: ShaderEffectSource {
                readonly property point panelPosition: controlsID.mapToItem(videoOutputID, 0, 0)

                width: controlsID.width
                height: controlsID.height

                sourceItem: frostedControlsEffectID.visible ? videoOutputID : null
                sourceRect: Qt.rect(
                    controlsID.x - videoOutputID.x,
                    controlsID.y - videoOutputID.y,
                    controlsID.width,
                    controlsID.height)
                textureSize: Qt.size(
                    Math.max(1, Math.round(width / 4)),
                    Math.max(1, Math.round(height / 4)))
                live: true
            }

            maskSource: ShaderEffectSource {
                width: controlsID.width
                height: controlsID.height

                sourceItem: Rectangle {
                    width: controlsID.width
                    height: controlsID.height
                    radius: controlsID.radius
                    color: Colors.Player.controlsFrostedMask
                }
            }
        }

        Rectangle {
            anchors.fill: parent

            radius: controlsID.radius
            color: Colors.Player.controlsBackground
            border {
                width: 1
                color: Colors.Player.controlsBorder
            }
        }

        ColumnLayout {
            anchors {
                fill: parent
                leftMargin: 24
                rightMargin: 24
                topMargin: 16
                bottomMargin: 12
            }

            spacing: 6

            RowLayout {
                Layout.fillWidth: true

                spacing: 14

                Label {
                    text: rootID.formatTime(videoID.position)
                    color: Colors.Player.currentTimeText
                    font.pixelSize: 13
                    font.weight: Font.Medium
                }

                Slider {
                    id: seekBarID

                    Layout.fillWidth: true
                    Layout.preferredHeight: 24

                    from: 0
                    to: Math.max(videoID.duration, 1)
                    value: videoID.position

                    onMoved: videoID.position = value

                    background: Item {
                        x: seekBarID.leftPadding
                        y: seekBarID.topPadding + seekBarID.availableHeight / 2 - height / 2

                        width: seekBarID.availableWidth
                        height: 7

                        Rectangle {
                            anchors.fill: parent

                            radius: height / 2
                            color: Colors.Player.trackBackground
                        }

                        Rectangle {
                            width: parent.width * Math.max(0, Math.min(1, guiController.downloadProgress / 100))
                            height: parent.height
                            radius: height / 2

                            color: Colors.Player.downloadTrack
                        }

                        Rectangle {
                            width: parent.width * (videoID.duration > 0
                                ? Math.max(0, Math.min(1, videoID.position / videoID.duration))
                                : 0)
                            height: parent.height
                            radius: height / 2

                            color: Colors.App.accent
                        }
                    }

                    handle: Rectangle {
                        x: seekBarID.leftPadding
                            + seekBarID.visualPosition * (seekBarID.availableWidth - width)
                        y: seekBarID.topPadding + seekBarID.availableHeight / 2 - height / 2

                        width: 15
                        height: 15
                        radius: width / 2

                        color: seekBarID.pressed
                            ? Colors.Player.seekHandlePressed
                            : Colors.Player.seekHandle
                        border.width: 1
                        border.color: Colors.App.accent
                    }
                }

                Label {
                    text: rootID.formatTime(videoID.duration)
                    color: Colors.Player.durationText
                    font.pixelSize: 13
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                spacing: 7

                PlayerControlButton {
                    iconName: volumeControlID.value > 0 ? "volume" : "muted"
                    tooltipText: volumeControlID.value > 0 ? qsTr("Mute") : qsTr("Unmute")
                    onClicked: {
                        if (volumeControlID.value > 0) {
                            rootID.lastAudibleVolume = volumeControlID.value
                            volumeControlID.value = 0
                        } else {
                            volumeControlID.value = rootID.lastAudibleVolume
                        }
                    }
                }

                Slider {
                    id: volumeControlID

                    Layout.preferredWidth: 112

                    from: 0
                    value: 1
                    to: 1

                    onMoved: {
                        if (value > 0)
                            rootID.lastAudibleVolume = value
                    }

                    background: Rectangle {
                        x: volumeControlID.leftPadding
                        y: volumeControlID.topPadding + volumeControlID.availableHeight / 2 - height / 2

                        width: volumeControlID.availableWidth
                        height: 4
                        radius: height / 2

                        color: Colors.Player.trackBackground

                        Rectangle {
                            width: parent.width * volumeControlID.visualPosition
                            height: parent.height
                            radius: height / 2

                            color: Colors.App.accent
                        }
                    }

                    handle: Rectangle {
                        x: volumeControlID.leftPadding
                            + volumeControlID.visualPosition * (volumeControlID.availableWidth - width)
                        y: volumeControlID.topPadding + volumeControlID.availableHeight / 2 - height / 2

                        width: 14
                        height: 14
                        radius: width / 2

                        color: Colors.Player.volumeHandle
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                PlayerControlButton {
                    iconName: "rewind"
                    tooltipText: qsTr("Back 5 seconds")
                    enabled: rootID.hasMedia
                    onClicked: videoID.position = Math.max(0, videoID.position - 5000)
                }

                PlayerControlButton {
                    prominent: true
                    iconName: videoID.playbackState === MediaPlayer.PlayingState ? "pause" : "play"
                    tooltipText: videoID.playbackState === MediaPlayer.PlayingState
                        ? qsTr("Pause")
                        : qsTr("Play")
                    enabled: rootID.hasMedia
                    onClicked: videoID.playbackState === MediaPlayer.PlayingState
                        ? videoID.pause()
                        : videoID.play()
                }

                PlayerControlButton {
                    iconName: "forward"
                    tooltipText: qsTr("Forward 5 seconds")
                    enabled: rootID.hasMedia
                    onClicked: videoID.position = Math.min(videoID.duration, videoID.position + 5000)
                }

                Item {
                    Layout.fillWidth: true
                }

                PlayerControlButton {
                    iconName: "folder"
                    tooltipText: qsTr("Open video or torrent")
                    onClicked: fileDialogID.open()
                }

                ComboBox {
                    id: subtitleTrackID

                    Layout.preferredWidth: 148
                    Layout.preferredHeight: 42

                    model: SubtitlesController.subtitleTracks
                    currentIndex: SubtitlesController.activeSubtitleTrack
                    visible: count > 0

                    onActivated: index => SubtitlesController.activeSubtitleTrack = index

                    background: Rectangle {
                        radius: 8
                        color: subtitleTrackID.hovered
                            ? Colors.Player.subtitleSelectorHovered
                            : Colors.Player.subtitleSelectorBackground
                        border.width: 1
                        border.color: subtitleTrackID.activeFocus
                            ? Colors.App.accent
                            : Colors.Player.trackBackground
                    }
                }

                PlayerControlButton {
                    iconName: "settings"
                    tooltipText: qsTr("Settings")
                    onClicked: rootID.openSettingsRequested()
                }

                PlayerControlButton {
                    iconName: rootID.fullScreen ? "opened_fullscreen" : "fullscreen"
                    tooltipText: rootID.fullScreen
                        ? qsTr("Exit full screen")
                        : qsTr("Full screen")
                    onClicked: rootID.toggleFullScreen()
                }
            }
        }
    }

    Timer {
        id: infoBubbleVisibilityTimerID

        interval: 1000
        onTriggered: infoBubbleID.visible = false
    }

    Timer {
        id: hideControlsTimerID

        interval: 3000
        onTriggered: {
            if (rootID.fullScreen)
                rootID.playerOverlayVisible = false
        }
    }

    Connections {
        target: guiController

        function onVideoFileUpdated() {
            videoID.source = guiController.videoFile
            if (videoID.playbackState !== MediaPlayer.PlayingState) {
                videoID.play()
                SubtitlesController.DownloadSubtitles()
            }
        }
        function onTorrentDownloadStarted() {
            rootID.downloading = true
        }
        function onTorrentDownloadFinished() {
            rootID.downloading = false
        }
    }

    FileDialog {
        id: fileDialogID

        title: qsTr("Open a video or torrent")
        nameFilters: [
            qsTr("Video and torrent files (*.torrent *.mkv *.mp4 *.mov *.avi *.webm)"),
            qsTr("All files (*)"),
        ]

        onAccepted: {
            guiController.AddFile(selectedFile)
            videoID.source = guiController.videoFile
            videoID.play()
            videoID.pause()
            rootID.forceActiveFocus()
        }
    }

    Keys.onSpacePressed: {
        if (rootID.hasMedia)
            videoID.playbackState === MediaPlayer.PlayingState ? videoID.pause() : videoID.play()
    }
    Keys.onLeftPressed: videoID.position = Math.max(0, videoID.position - 5000)
    Keys.onRightPressed: videoID.position = Math.min(videoID.duration, videoID.position + 5000)
    Keys.onEscapePressed: {
        if (rootID.fullScreen) {
            rootID.Window.window.visibility = Window.Windowed
            rootID.showPlayerOverlay()
        }
    }
    Keys.onUpPressed: {
        volumeControlID.value = Math.min(volumeControlID.to, volumeControlID.value + 0.1)
        rootID.showInfoBubble(qsTr("Volume: %1%").arg(Math.round(volumeControlID.value * 100)))
    }
    Keys.onDownPressed: {
        volumeControlID.value = Math.max(volumeControlID.from, volumeControlID.value - 0.1)
        rootID.showInfoBubble(qsTr("Volume: %1%").arg(Math.round(volumeControlID.value * 100)))
    }
    Keys.onPressed: event => {
        switch (event.key) {
        case Qt.Key_F:
            rootID.toggleFullScreen()
            event.accepted = true
            break
        case Qt.Key_G:
            SubtitlesController.DecreaseOffset()
            rootID.showInfoBubble(`${SubtitlesController.subtitleOffset > 0 ? "+" : ""}${SubtitlesController.subtitleOffset} ms`)
            event.accepted = true
            break
        case Qt.Key_H:
            SubtitlesController.IncreaseOffset()
            rootID.showInfoBubble(`${SubtitlesController.subtitleOffset > 0 ? "+" : ""}${SubtitlesController.subtitleOffset} ms`)
            event.accepted = true
            break
        }
    }
}
