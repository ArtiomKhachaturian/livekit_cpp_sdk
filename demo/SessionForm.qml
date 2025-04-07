import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Frame {
    id: root

    property SessionWrapper session: null
    readonly property bool connecting: session !== null && session.connecting
    readonly property int state: {
        if (session === null) {
            return SessionWrapper.TransportDisconnected
        }
        return session.state
    }
    property string identity : session === null ? objectName : session.identity

    signal error(string desc, string details)


    ColumnLayout {
        anchors.fill: parent
        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Switch {
                    id: micAdded
                    text: qsTr("Microphone")
                    checked: false
                    enabled: session != null
                    onCheckedChanged: {
                        localMediaView.microphoneAdded = checked
                    }
                }
                CheckBox {
                    id: micMuted
                    text: qsTr("Muted")
                    enabled: localMediaView.microphoneAdded
                    checked: false
                    onCheckedChanged: {
                        localMediaView.microphoneMuted = checked
                    }
                }

                ToolSeparator {}

                Switch {
                    id: cameraAdded
                    text: qsTr("Camera")
                    enabled: session != null
                    checked: false
                    onCheckedChanged: {
                        localMediaView.cameraAdded = checked
                    }
                }
                CheckBox {
                    id: cameraMuted
                    text: qsTr("Muted")
                    enabled: localMediaView.cameraAdded
                    checked: false
                    onCheckedChanged: {
                        localMediaView.cameraMuted = checked
                    }
                }
                ComboBox {
                    id: cameraModelComboBox
                    model: app.camerasModel
                    readonly property var deviceInfo: {
                        return model.itemAt(currentIndex)
                    }
                    textRole: "display"
                    Layout.fillWidth: true
                    visible: count > 1
                    onCurrentIndexChanged: {
                        localMediaView.cameraDeviceInfo = model.itemAt(currentIndex)
                    }
                }

                ComboBox {
                    id: cameraOptionsComboBox
                    textRole: "display"
                    Layout.fillWidth: true
                    enabled: localMediaView.cameraAdded && null !== model
                    Component.onCompleted: {
                        model = app.createCameraOptionsModel(this)
                        if (null != model) {
                            currentIndex = Math.max(0, model.indexOf(app.defaultCameraOptions))
                            model.deviceInfo = Qt.binding(function() {
                                return cameraModelComboBox.deviceInfo
                            })
                        }
                    }
                    onCurrentIndexChanged: {
                        localMediaView.cameraOptions = model.itemAt(currentIndex)
                    }
                }

                Item { // spacer
                    Layout.fillWidth: true
                }

                ToolButton {
                    Layout.alignment: Qt.AlignRight
                    id: chatButton
                    text: qsTr("Chat")
                    checkable: true
                    checked: false
                }
                ToolButton {
                    Layout.alignment: Qt.AlignRight
                    icon.name: "network-offline"
                    onClicked: disconnect()
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Pane {
                Layout.fillWidth: true
                Layout.fillHeight: true
                LocalMediaView {
                    id: localMediaView
                    anchors.fill: parent
                }
            }
            ChatView {
                id: chatView
                visible: chatButton.checked
                Layout.preferredWidth: 300
                Layout.maximumWidth: 600
                Layout.fillHeight: true
                onTextEntered: text => {
                    if (session != null) {
                        session.sendChatMessage(text)
                    }
                }
            }
        }
    }

    function connect(url, token) {
        session = app.createSession(this)
        if (session != null) {
            if (!session.connectToSfu(url, token)) {
                // TODO: log error
            }
        }
        else {
            error(qsTr("Failed to create session"), "")
        }
    }

    function disconnect() {
        if (session != null) {
            session.disconnectFromSfu()
            session = null
        }
    }

    onSessionChanged: {
        localMediaView.session = session
    }

    onStateChanged: {
        switch (state) {
            case SessionWrapper.TransportDisconnected:
            case SessionWrapper.RtcClosed:
                localMediaView.session = null
                break
            default:
                break
        }
    }

    Connections {
        target: session
        function onError(desc, details) {
            session = null
            root.error(desc, details)
        }
        function onChatMessageReceived(participantIdentity, message, deleted) {
            chatView.append(participantIdentity, message)
        }
    }
}
