import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root

    property SessionWrapper session: null
    readonly property bool connecting: session != null && session.connecting
    readonly property int state: {
        if (session == null) {
            return SessionWrapper.TransportDisconnected
        }
        return session.state
    }

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
                        if (session != null) {
                            if (checked) {
                                session.addMicrophoneTrack()
                            }
                            else {
                                session.removeMicrophoneTrack()
                            }
                        }
                    }
                }
                CheckBox {
                    text: qsTr("Muted")
                    enabled: micAdded.enabled && micAdded.checked
                    checked: false
                }
                Label {
                    text: "|"
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                }
                Switch {
                    id: cameraAdded
                    text: qsTr("Camera")
                    enabled: session != null
                    checked: false
                    onCheckedChanged: {
                        if (session != null) {
                            if (checked) {
                                session.addCameraTrack()
                            }
                            else {
                                session.removeCameraTrack()
                            }
                        }
                    }
                }
                CheckBox {
                    text: qsTr("Muted")
                    enabled: cameraAdded.enabled && cameraAdded.checked
                    checked: false
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
                    onClicked: {
                        disconnect()
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Pane {
                Layout.fillWidth: true
                Layout.fillHeight: true
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
            session.connectToSfu(url, token)
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

    Connections {
        target: session
        function onError(desc, details) {
            root.error(desc, details)
        }
        function onChatMessageReceived(participantIdentity, message, deleted) {
            chatView.append(participantIdentity, message)
        }
    }
}
