import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    property bool closable: true
    property alias urlText: connectionForm.urlText
    property alias tokenText: connectionForm.tokenText
    property alias identity: sessionForm.identity

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
                    enabled: (sessionForm.connecting || sessionForm.connected) && sessionForm.activeMicrophone
                    checked: false
                }

                ToolSeparator {}

                Switch {
                    id: cameraAddSwitch
                    text: qsTr("Camera")
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
                    enabled: (sessionForm.connecting || sessionForm.connected) && sessionForm.activeCamera
                    checked: false
                }

                CameraModelComboBox {
                    id: cameraModelComboBox
                    Layout.horizontalStretchFactor: 2
                    Layout.fillWidth: true
                    onCurrentIndexChanged: {
                        if (cameraAddSwitch.checked) {
                            connectionForm.cameraDeviceInfo = deviceInfoAt(currentIndex)
                        }
                    }
                }

                CameraOptionsComboBox {
                    id: cameraOptionsComboBox
                    visible: sessionForm.connecting || sessionForm.connected
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
                    visible: sessionForm.connected
                }

                ToolButton {
                    Layout.alignment: Qt.AlignRight
                    icon.name: "network-offline"
                    visible: sessionForm.connecting || sessionForm.connected
                    onClicked: sessionForm.disconnect()
                }
            }
        }

        StackLayout {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: sessionForm.connecting || sessionForm.connected ? 1 : 0
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
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: sessionForm.connecting
    }
}
