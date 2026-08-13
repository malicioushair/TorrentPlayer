import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import TorrentPlayer
import "../colors.js" as Colors

StyledScrollView {
    id: rootID

    property bool manualDownloadPending: false
    property var userData: SubtitlesController.userData

    signal imdbIdEdited(string imdbId)
    signal configureSubDlRequested
    signal configureOpenSubtitlesRequested
    signal downloadRequested

    function applySubtitleSettings() {
        const storedUserData = SubtitlesController.userData
        const updatedUserData = ({})

        for (const key in storedUserData)
            updatedUserData[key] = storedUserData[key]

        SubtitlesController.imdbId = imdbIDInputID.text.trim()
        SubtitlesController.userData = updatedUserData
        subtitleParamsSectionID.applySubtitleParams()
    }

    onImdbIdEdited: function(imdbId) {
        rootID.pendingImdbId = imdbId
    }

    onDownloadRequested: {
        rootID.applySubtitleSettings()
        rootID.manualDownloadPending = true
        SubtitlesController.DownloadSubtitles(preferedLanguageInputID.currentValue)
    }

    Connections {
        target: SubtitlesController

        function onShowErrorMessage() {
            rootID.manualDownloadPending = false
        }

        function onSubtitleDownloadSucceeded() {
            if (!rootID.manualDownloadPending)
                return

            rootID.manualDownloadPending = false
            downloadSuccessBubbleID.visible = true
            downloadSuccessTimerID.restart()
        }
    }

    Timer {
        id: downloadSuccessTimerID

        interval: 3000
        onTriggered: downloadSuccessBubbleID.visible = false
    }

    SettingsPageHeader {
        title: qsTr("Subtitles")
        description: qsTr("Find, download, and manage subtitles for the current video.")
    }

    SettingsSection {
        Layout.topMargin: 8

        title: qsTr("Search defaults")

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("Find subtitles automatically")
                color: Colors.SettingsDialog.secondaryText
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
                Layout.alignment: Qt.AlignRight
                Layout.rightMargin: -20

                scale: 0.75
                checked: SubtitlesController.autoFind
                onClicked: SubtitlesController.autoFind = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("IMDb ID")
                color: Colors.SettingsDialog.secondaryText
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillWidth: true
            }

            TextField {
                id: imdbIDInputID

                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: 300
                Layout.preferredHeight: 30

                padding: 0
                text: SubtitlesController.imdbId
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("Preferred language")
                color: Colors.SettingsDialog.secondaryText
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillWidth: true
            }

            ComboBox {
                id: preferedLanguageInputID
                Layout.preferredHeight: 30
                model: ["EN", "RU"]
                currentValue: SubtitlesController.preferredLanguage
                onActivated: SubtitlesController.preferredLanguage = currentValue
            }
        }

        RowLayout {
            Item {
                Layout.fillWidth: true
            }

            Rectangle {
                id: downloadSuccessBubbleID

                Layout.preferredWidth: downloadSuccessTextID.implicitWidth + 24
                Layout.preferredHeight: 38

                radius: height / 2
                color: Colors.SettingsDialog.inputBackground
                border.width: 1
                border.color: Colors.SettingsDialog.configured
                visible: false

                Label {
                    id: downloadSuccessTextID

                    anchors.centerIn: parent

                    text: qsTr("Subtitles downloaded")
                    color: Colors.SettingsDialog.configured
                    font.weight: Font.Medium
                }
            }

            Button {
                Layout.preferredWidth: 165
                Layout.preferredHeight: 40

                text: qsTr("Manual download")
                highlighted: true
                onClicked: rootID.downloadRequested()
            }
        }
    }

    SettingsSection {
        id: providersSectionID

        title: qsTr("Providers")

        ProviderRow {
            providerName: qsTr("SubDL")
            statusText: SubtitlesController.subdlConfigured
                ? qsTr("Configured")
                : qsTr("Use app defaults")
            configured: SubtitlesController.subdlConfigured
            onConfigureRequested: {
                providersSectionID.visible = !providersSectionID.visible
                subDLConfigSectionID.visible = !subDLConfigSectionID.visible
            }
        }

        ProviderRow {
            providerName: qsTr("OpenSubtitiles")
            statusText: SubtitlesController.openSubtitlesConfigured
                ? qsTr("Configured")
                : qsTr("Not configured")
            configured: SubtitlesController.openSubtitlesConfigured
            onConfigureRequested: openSubtitlesConfigSectionID.toggleConfig()
        }
    }

    SettingsSection {
        id: subDLConfigSectionID

        function toggleConfig() {
            providersSectionID.visible = !providersSectionID.visible
            subDLConfigSectionID.visible = !subDLConfigSectionID.visible
        }

        function applyChanges() {
            const storedUserData = SubtitlesController.userData
            const updatedUserData = ({})

            for (const key in storedUserData)
                updatedUserData[key] = storedUserData[key]

            updatedUserData.subDlApiKey = subdlApiKeyInputID.text.trim()

            SubtitlesController.userData = updatedUserData
        }

        title: qsTr("SubDL configuration")
        visible: false

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("SubDL API key")
                color: Colors.SettingsDialog.secondaryText
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredWidth: 30
            }

            TextField {
                id: subdlApiKeyInputID

                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                padding: 0
                text: rootID.userData.subDlApiKey
                placeholderText: qsTr("Leave this empty to use the app's default API.")
            }
        }

        RowLayout {
            Item {
                Layout.fillWidth: true
            }
            Button {
                Layout.preferredHeight: 40

                text: qsTr("Cancel")
                Material.foreground: Material.accent

                onClicked: subDLConfigSectionID.toggleConfig()
            }
            Button {
                Layout.preferredHeight: 40

                text: qsTr("Apply")
                highlighted: true
                onClicked: {
                    subDLConfigSectionID.applyChanges()
                    subDLConfigSectionID.toggleConfig()
                }
            }
        }
    }

    SettingsSection {
        id: openSubtitlesConfigSectionID

        function toggleConfig() {
            providersSectionID.visible = !providersSectionID.visible
            openSubtitlesConfigSectionID.visible = !openSubtitlesConfigSectionID.visible
        }

        function applyChanges() {
            const storedUserData = SubtitlesController.userData
            const updatedUserData = ({})

            for (const key in storedUserData)
                updatedUserData[key] = storedUserData[key]

            updatedUserData.openSubtitlesUsername = openSubtitlesUsernameInputID.text.trim()
            updatedUserData.openSubtitlesPassword = openSubtitlesPasswordInputID.text.trim()

            SubtitlesController.userData = updatedUserData
        }

        title: qsTr("OpenSubtitles configuration")
        visible: false

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("OpenSubtitles username")
                color: Colors.SettingsDialog.secondaryText
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredWidth: 30
            }

            TextField {
                id: openSubtitlesUsernameInputID

                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                padding: 0
                text: rootID.userData.openSubtitlesUsername
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("OpenSubtitles password")
                color: Colors.SettingsDialog.secondaryText
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredWidth: 30
            }

            TextField {
                id: openSubtitlesPasswordInputID

                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                padding: 0
                text: rootID.userData.openSubtitlesPassword
                echoMode: passwordToggleID.showPassword ? TextInput.Normal : TextInput.Password

                ToolButton {
                    id: passwordToggleID

                    property bool showPassword: false

                    anchors {
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                    }

                    width: 32
                    height: 32
                    text: showPassword ? "👁️" : "🙈"
                    onClicked: showPassword = !showPassword
                }
            }
        }

        RowLayout {
            Item {
                Layout.fillWidth: true
            }
            Button {
                Layout.preferredHeight: 40

                text: qsTr("Cancel")
                Material.foreground: Material.accent

                onClicked: openSubtitlesConfigSectionID.toggleConfig()
            }
            Button {
                Layout.preferredHeight: 40

                text: qsTr("Apply")
                highlighted: true
                onClicked: {
                    openSubtitlesConfigSectionID.applyChanges()
                    openSubtitlesConfigSectionID.toggleConfig()
                }
            }
        }
    }

    SettingsSection {
        id: subtitleParamsSectionID

        function applySubtitleParams() {
            SubtitlesController.fontSize = fontSizeParamID.text
        }

        title: qsTr("Parameters")
        separatorVisible: false

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: qsTr("Font size")
            }

            Item {
                Layout.fillWidth: true
            }

            TextField {
                id: fontSizeParamID

                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: 50
                Layout.preferredHeight: 30

                padding: 0
                text: SubtitlesController.fontSize

                onAccepted: SubtitlesController.fontSize = text
            }
        }
    }
}
