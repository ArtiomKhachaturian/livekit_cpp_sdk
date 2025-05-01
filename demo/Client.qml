import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    property bool closable: true
    property string videoFilter
    property alias urlText: connectionForm.urlText
    property alias tokenText: connectionForm.tokenText
    property alias identity: sessionForm.identity
    readonly property bool sessionActive: sessionForm.connecting || sessionForm.connected

    signal wantsToBeClosed()
    signal error(string desc, string details)

    ColumnLayout {
        anchors.fill: parent

        ToolBar {
            Layout.fillWidth: true
            Layout.minimumHeight: 50
            RowLayout {
                anchors.fill: parent
                Switch {
                    id: micAddSwitch
                    text: qsTr("Microphone")
                    checked: false
                }

                CheckBox {
                    id: micMuteCheckbox
                    text: qsTr("Muted")
                    enabled: sessionActive && sessionForm.activeMicrophone
                    checked: false
                }

                ToolSeparator {}

                Switch {
                    id: sharingAddSwitch
                    text: qsTr("Sharing")
                    checkable: false
                    checked: false
                    onClicked: {
                        if (!checked) {
                            sharingSelection.visible = true

                        }
                        else {
                            sharingAddSwitch.checked = false
                        }
                    }
                    onPressAndHold: {
                        if (checked) {
                            sharingSelection.visible = true
                        }
                    }
                }

                CheckBox {
                    id: sharingMuteCheckbox
                    text: qsTr("Muted")
                    enabled: sessionActive && sessionForm.activeSharing
                    checked: false
                }

                ToolSeparator {}

                Switch {
                    id: cameraAddSwitch
                    text: qsTr("Camera")
                    checked: false
                }

                CheckBox {
                    id: cameraMuteCheckbox
                    text: qsTr("Muted")
                    enabled: sessionActive && sessionForm.activeCamera
                    checked: false
                }                

                ToolSeparator {}

                ToolButton {
                    Layout.alignment: Qt.AlignRight
//                    icon.name: "preferences-desktop-multimedia"
                    text: qsTr("Options")
                    onClicked: {
                        mediaOptionsForm.visible = !mediaOptionsForm.visible
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                ToolButton {
                    Layout.alignment: Qt.AlignRight
                    id: chatButton
                    text: qsTr("Chat")
                    checkable: true
                    checked: false
                    visible: sessionForm.connected
                }

                ToolSeparator {}

                ToolButton {
                    Layout.alignment: Qt.AlignRight
                    icon.name: "network-offline"
                    visible: sessionActive
                    onClicked: sessionForm.disconnect()
                }

                ToolButton {
                    Layout.alignment: Qt.AlignRight
                    visible: closable
                    icon.name: "window-close"
                    onClicked: {
                        sessionForm.disconnect()
                        wantsToBeClosed()
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                id: stackView
                Layout.fillHeight: true
                Layout.fillWidth: true
                //Layout.horizontalStretchFactor: 6
                currentIndex: sessionActive ? 1 : 0
                ConnectForm {
                    id: connectionForm
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    objectName: root.objectName + "_client_form"
                    enabled: !sessionForm.connecting
                    activePreview: !sessionActive
                    activeCamera: cameraAddSwitch.checked
                    activeSharing: sharingAddSwitch.checked
                    videoFilter: root.videoFilter
                    onConnectClicked: {
                        sessionForm.connect(urlText, tokenText,
                                            autoSubscribe, adaptiveStream,
                                            e2e, iceTransportPolicy, passPhrase)
                    }
                }
                SessionForm {
                    id: sessionForm
                    objectName: root.objectName
                    activeCamera: cameraAddSwitch.checked
                    activeMicrophone: micAddSwitch.checked
                    activeSharing: sharingAddSwitch.checked
                    microphoneMuted: micMuteCheckbox.checked
                    cameraMuted: cameraMuteCheckbox.checked
                    sharingMuted: sharingMuteCheckbox.checked
                    localVideoFilter: root.videoFilter
                    prefferedAudioEncoder: connectionForm.prefferedAudioEncoder
                    prefferedVideoEncoder: connectionForm.prefferedVideoEncoder
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    onError: (desc, details) => {
                        root.error(desc, details)
                    }
                }
            }

            MediaOptionsForm {
                id: mediaOptionsForm
                Layout.fillHeight: true
                visible: false
                micGroupEnabled: !sessionActive || !micAddSwitch.checked
                onAccepted: {
                    // connection preview
                    connectionForm.cameraDeviceInfo = mediaOptionsForm.cameraDeviceInfo
                    connectionForm.cameraOptions = mediaOptionsForm.cameraOptions
                    connectionForm.sharingOptions = mediaOptionsForm.sharingOptions
                    // session
                    sessionForm.cameraDeviceInfo = mediaOptionsForm.cameraDeviceInfo
                    sessionForm.cameraOptions = mediaOptionsForm.cameraOptions
                    sessionForm.sharingOptions = mediaOptionsForm.sharingOptions
                    sessionForm.microphoneOptions = mediaOptionsForm.microphoneOptions
                    // sharing form
                    sharingSelection.options = mediaOptionsForm.sharingOptions
                    // video filter
                    root.videoFilter = mediaOptionsForm.videoFilter
                }
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: sessionForm.connecting
        z: 2.
    }

    SharingSelectionForm {
        id: sharingSelection
        anchors.fill: parent
        visible: false
        z: 1
        onAccepted: {
            connectionForm.sharingDeviceInfo = deviceInfo
            sessionForm.sharingDeviceInfo = deviceInfo
            sharingAddSwitch.checked = true
        }
        onRejected: {
            //sharingAddSwitch.checked = false
        }
    }
}
