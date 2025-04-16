import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    property bool closable: true
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

                CameraModelComboBox {
                    id: cameraModelComboBox
                    Layout.horizontalStretchFactor: 2
                    Layout.fillWidth: true
                }

                CameraOptionsComboBox {
                    id: cameraOptionsComboBox
                    visible: sessionActive
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

        StackLayout {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: sessionActive ? 1 : 0
            ConnectForm {
                id: connectionForm
                Layout.fillHeight: true
                Layout.fillWidth: true
                objectName: root.objectName + "_client_form"
                enabled: !sessionForm.connecting
                activeCamera: !sessionActive && cameraAddSwitch.checked
                cameraDeviceInfo: cameraModelComboBox.deviceInfo
                onConnectClicked: {
                    sessionForm.connect(urlText, tokenText,
                                        autoSubscribe, adaptiveStream,
                                        e2e, iceTransportPolicy)
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
        z: 1.
    }
}
