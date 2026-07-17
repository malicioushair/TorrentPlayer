pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import TorrentPlayer

Dialog {
    id: rootID

    readonly property var userData: SubtitlesController.userData

    parent: Overlay.overlay
    anchors.centerIn: parent

    width: 600
    height: 420

    title: qsTr("Subtitle settings")
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel

    StackView {
        id: stackViewID

        anchors.fill: parent
        initialItem: subtitleSettingsID

        Component {
            id: subtitleSettingsID

            ColumnLayout {
                width: stackViewID.availableWidth

                spacing: 16

                TextField {
                    id: imdbInputID

                    Layout.fillWidth: true
                    text: SubtitlesController.imdbId
                    placeholderText: qsTr("IMDB ID")
                    onEditingFinished: SubtitlesController.imdbId = text
                }

                RowLayout {
                    Layout.fillWidth: true

                    spacing: 10

                    Label {
                        text: qsTr("Subtitle language:")
                    }

                    TextField {
                        id: subtitleLanguageID

                        Layout.fillWidth: true

                        placeholderText: qsTr("EN")
                    }

                    Button {
                        text: qsTr("Download")
                        onClicked: {
							SubtitlesController.imdbId = imdbInputID.text
                            SubtitlesController.userData = rootID.userData
                            SubtitlesController.DownloadSubtitles(subtitleLanguageID.text)
                        }
                    }
                }

                Button {
                    id: customAccountSettingsButtonID

                    text: qsTr("Custom Account")
                    onClicked: stackViewID.push(accountsListID)
                }
            }
        }


        Component {
            id: accountsListID

            ColumnLayout {
                RowLayout {
                    Button {
                        id: subdlButtonID

                        text: qsTr("SubDL")
                        onClicked: stackViewID.push(subdlViewID)
                    }
                    Button {
                        id: opensubtitlesButtonID

                        text: qsTr("OpenSubtitles")
                        onClicked: stackViewID.push(opensubtitlesViewID)
                    }
                }
            }
        }

        Component {
            id: subdlViewID

            ColumnLayout {
                TextField {
                    id: subdlApiKeyFieldID

                    Layout.fillWidth: true

					text: rootID.userData.subDlApiKey
                    placeholderText: qsTr("SubDL API key")
                    onEditingFinished: rootID.userData.subDlApiKey = text
                }

                Item {
                    id: verticalSpacerID
                    Layout.fillHeight: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    Item {
                        Layout.fillWidth: true
                    }

                    Button {
                        text: qsTr("Apply")
                        onClicked: {
                            rootID.userData.subDlApiKey = subdlApiKeyFieldID.text
                            SubtitlesController.userData = rootID.userData
                        }
                    }
                }
            }
        }

        Component {
            id: opensubtitlesViewID

            ColumnLayout {
                TextField {
                    id: openSubtitlesApiKeyFieldID

                    Layout.fillWidth: true

                    text: rootID.userData.openSubtitlesApiKey
                    placeholderText: qsTr("OpenSubtitles API key")
                    onEditingFinished: rootID.userData.openSubtitlesApiKey = text
                }

                TextField {
                    id: openSubtitlesUsernameFieldID

                    Layout.fillWidth: true

                    text: rootID.userData.openSubtitlesUsername
                    placeholderText: qsTr("OpenSubtitles username")
                    onEditingFinished: rootID.userData.openSubtitlesUsername = text
                }

                TextField {
                    id: openSubtitlesPasswordFieldID

                    Layout.fillWidth: true

                    echoMode: TextInput.Password
                    text: rootID.userData.openSubtitlesPassword
                    placeholderText: qsTr("OpenSubtitles password")
                    onEditingFinished: rootID.userData.openSubtitlesPassword = text
                }

                RowLayout {
                    Layout.fillWidth: true

                    Item {
                        Layout.fillWidth: true
                    }

                    Button {
                        text: qsTr("Apply")
                        onClicked: {
                            rootID.userData.openSubtitlesApiKey = openSubtitlesApiKeyFieldID.text
                            rootID.userData.openSubtitlesUsername = openSubtitlesUsernameFieldID.text
                            rootID.userData.openSubtitlesPassword = openSubtitlesPasswordFieldID.text

                            SubtitlesController.userData = rootID.userData
                        }
                    }
                }
            }
        }
    }
}
