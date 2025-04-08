import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    property bool closable: true
    property alias urlText: connectionForm.urlText
    property alias tokenText: connectionForm.tokenText
    property alias identity: sessionForm.identity
    readonly property bool hasActiveSession: sessionForm.visible && sessionForm.session !== null

    signal wantsToBeClosed()
    signal error(string desc, string details)

    ToolButton {
        visible: closable
        anchors.right: parent.right
        anchors.top: parent.top
        icon.name: "window-close"
        z: 1
        onClicked: {
            sessionForm.disconnect()
            wantsToBeClosed()
        }
    }

    ColumnLayout {
        anchors.fill: parent

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Switch {
                    id: micAddSwitch
                    text: qsTr("Microphone")
                    checked: false
                    enabled: connectionForm.visible || hasActiveSession
                }

                CheckBox {
                    id: micMuteCheckbox
                    text: qsTr("Muted")
                    enabled: sessionForm.hasMicrophoneTrack
                    checked: false
                }

                ToolSeparator {}

                Switch {
                    id: cameraAddSwitch
                    text: qsTr("Camera")
                    enabled: connectionForm.visible || hasActiveSession
                    checked: false
                    onCheckedChanged: {
                        if (checked) {
                            connectionForm.cameraDeviceInfo = cameraModelComboBox.deviceInfo
                        }
                        else {
                            connectionForm.cameraDeviceInfo = undefined
                        }
                    }
                }

                CheckBox {
                    id: cameraMuteCheckbox
                    text: qsTr("Muted")
                    enabled: sessionForm.hasCameraTrack
                    checked: false
                }

                CameraModelComboBox {
                    id: cameraModelComboBox
                    Layout.horizontalStretchFactor: 2
                    Layout.fillWidth: true
                }

                CameraOptionsComboBox {
                    id: cameraOptionsComboBox
                    visible: !connectionForm.visible
                    deviceInfo: cameraModelComboBox.deviceInfo
                    Layout.horizontalStretchFactor: 1
                    Layout.fillWidth: true
                }

                ToolButton {
                    Layout.alignment: Qt.AlignRight
                    id: chatButton
                    text: qsTr("Chat")
                    checkable: true
                    checked: false
                    visible: !connectionForm.visible
                }

                ToolButton {
                    Layout.alignment: Qt.AlignRight
                    icon.name: "network-offline"
                    visible: !connectionForm.visible
                    onClicked: disconnect()
                }
            }
        }

        StackLayout {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            ConnectForm {
                id: connectionForm
                Layout.fillHeight: true
                Layout.fillWidth: true
                enabled: !sessionForm.connecting
                onConnectClicked: {
                    sessionForm.connect(urlText, tokenText)
                }
            }
            SessionForm {
                id: sessionForm
                objectName: root.objectName
                activeCamera: cameraAddSwitch.checked
                activeMicrophone: micAddSwitch.checked
                camerDeviceInfo: cameraModelComboBox.deviceInfo
                cameraOptions: cameraOptionsComboBox.options
                microphoneMuted: micMuteCheckbox.checked
                cameraMuted: cameraMuteCheckbox.checked
                Layout.fillHeight: true
                Layout.fillWidth: true
                onError: (desc, details) => {
                    root.error(desc, details)
                }
                onStateChanged: {
                    switch (state) {
                        case SessionWrapper.RtcConnecting:
                        case SessionWrapper.RtcConnected:
                        case SessionWrapper.RtcDisconnected:
                            stackView.currentIndex = 1
                            break
                        default:
                            stackView.currentIndex = 0
                            break
                    }
                }
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: sessionForm.connecting
    }

    function disconnect() {
        if (sessionForm.session != null) {
            sessionForm.session.disconnectFromSfu()
            sessionForm.session = null
        }
    }
}
