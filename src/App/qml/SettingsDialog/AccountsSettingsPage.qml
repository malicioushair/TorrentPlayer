import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import "colors.js" as Colors

StyledScrollView {
    id: rootID

    property string subDlApiKey: ""
    property string openSubtitlesUsername: ""
    property string openSubtitlesPassword: ""

    signal subDlApiKeyEdited(string apiKey)
    signal openSubtitlesUsernameEdited(string username)
    signal openSubtitlesPasswordEdited(string password)

    SettingsPageHeader {
        title: qsTr("Accounts")
        description: qsTr("Configure credentials for subtitle providers.")
    }

    SettingsSection {
        Layout.topMargin: 8

        title: qsTr("SubDL")
        statusText: rootID.subDlApiKey.trim() !== ""
            ? qsTr("Custom key configured")
            : qsTr("Using app default")
        statusColor: rootID.subDlApiKey.trim() !== ""
            ? Colors.SettingsDialog.configured
            : Colors.SettingsDialog.secondaryText

        Label {
            Layout.fillWidth: true

            text: qsTr("Leave this empty to use the API key included with the app.")
            color: Colors.SettingsDialog.secondaryText
            wrapMode: Text.WordWrap
        }

        TextField {
            Layout.fillWidth: true

            text: rootID.subDlApiKey
            placeholderText: qsTr("SubDL API key")
            echoMode: TextInput.Password
            onTextEdited: rootID.subDlApiKeyEdited(text)
        }
    }

    SettingsSection {
        title: qsTr("OpenSubtitles")
        statusText: rootID.openSubtitlesUsername.trim() !== ""
            && rootID.openSubtitlesPassword !== ""
            ? qsTr("Credentials configured")
            : qsTr("Not configured")
        statusColor: rootID.openSubtitlesUsername.trim() !== ""
            && rootID.openSubtitlesPassword !== ""
            ? Colors.SettingsDialog.configured
            : Colors.SettingsDialog.secondaryText

        TextField {
            Layout.fillWidth: true

            text: rootID.openSubtitlesUsername
            placeholderText: qsTr("OpenSubtitles username")
            onTextEdited: rootID.openSubtitlesUsernameEdited(text)
        }

        TextField {
            Layout.fillWidth: true

            text: rootID.openSubtitlesPassword
            placeholderText: qsTr("OpenSubtitles password")
            echoMode: TextInput.Password
            onTextEdited: rootID.openSubtitlesPasswordEdited(text)
        }
    }
}
