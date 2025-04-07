import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Frame {
    id: root

    property SessionWrapper session: null
    property AudioTrackWrapper micTrack: null
    property CameraTrackWrapper cameraTrack: null
    readonly property bool connecting: session != null && session.connecting
    readonly property int state: {
        if (session == null) {
            return SessionWrapper.TransportDisconnected
        }
        return session.state
    }
    readonly property string identity : session == null ? "" : session.identity

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
                        addMicrophoneTrack(checked)
                    }
                }
                CheckBox {
                    id: micMuted
                    text: qsTr("Muted")
                    enabled: micTrack != null
                    checked: false
                    onCheckedChanged: {
                        if (micTrack != null) {
                            micTrack.muted = checked
                        }
                    }
                }

                ToolSeparator {}

                Switch {
                    id: cameraAdded
                    text: qsTr("Camera")
                    enabled: session != null
                    checked: false
                    onCheckedChanged: {
                        addCameraTrack(checked)
                    }
                }
                CheckBox {
                    id: cameraMuted
                    text: qsTr("Muted")
                    enabled: cameraTrack != null
                    checked: false
                    onCheckedChanged: {
                        if (cameraTrack != null) {
                            cameraTrack.muted = checked
                        }
                    }
                }
                ComboBox {
                    model: app.camerasModel
                    textRole: "display"
                    Layout.fillWidth: true
                    enabled: cameraTrack != null
                    visible: model.rowCount > 0
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
                VideoOutput {
                    id: localCameraView
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
            if (session.connectToSfu(url, token)) {
                if (micAdded.checked) {
                    addMicrophoneTrack()
                }
                if (cameraAdded.checked) {
                    addCameraTrack()
                }
            }
        }
        else {
            error(qsTr("Failed to create session"), "")
        }
    }

    function disconnect() {
        if (session != null) {
            session.disconnectFromSfu()
            destroySession()
        }
    }

    function destroySession() {
        if (session != null) {
            session.removeMicrophoneTrack(micTrack)
            session.removeCameraTrack(cameraTrack)
            micTrack = null
            cameraTrack = null
        }
    }

    function addMicrophoneTrack(add = true) {
        if (session != null) {
            if (add) {
                micTrack = session.addMicrophoneTrack()
                if (micTrack != null) {
                    micTrack.muted = micMuted.checked;
                }
            }
            else {
                session.removeMicrophoneTrack(micTrack)
                micTrack = null
            }
        }
    }

    function addCameraTrack(add = true) {
        if (session != null) {
            if (add) {
                cameraTrack = session.addCameraTrack()
                if (cameraTrack != null) {
                    cameraTrack.muted = cameraMuted.checked
                    cameraTrack.videoOutput = localCameraView.videoSink
                }
            }
            else {
                session.removeCameraTrack(cameraTrack)
                cameraTrack = null
            }
        }
    }

    onStateChanged: {
        switch (state) {
            case SessionWrapper.TransportDisconnected:
            case SessionWrapper.RtcClosed:
                destroySession()
                break
            default:
                break
        }
    }

    Connections {
        target: session
        function onError(desc, details) {
            destroySession()
            root.error(desc, details)
        }
        function onChatMessageReceived(participantIdentity, message, deleted) {
            chatView.append(participantIdentity, message)
        }
    }
}
