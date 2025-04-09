import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root

    property Session session: null
    property bool activeCamera: false
    property bool activeMicrophone: false
    property alias camerDeviceInfo: localMediaView.cameraDeviceInfo
    property alias cameraOptions: localMediaView.cameraOptions
    property alias cameraMuted: localMediaView.cameraMuted
    property alias microphoneMuted: localMediaView.microphoneMuted
    property alias hasMicrophoneTrack: localMediaView.hasMicrophoneTrack
    property alias hasCameraTrack: localMediaView.hasCameraTrack
    readonly property bool connecting: session !== null && session.connecting
    readonly property int state: {
        if (session === null) {
            return Session.TransportDisconnected
        }
        return session.state
    }
    property string identity : session === null ? objectName : session.identity

    signal error(string desc, string details)


    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Pane {
                Layout.fillWidth: true
                Layout.fillHeight: true
                LocalMediaView {
                    id: localMediaView
                    cameraAdded: activeCamera
                    microphoneAdded: activeMicrophone
                    //cameraDeviceInfo: cameraModelComboBox.deviceInfo
                    //cameraOptions: cameraOptionsComboBox.cameraOptions
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

    onSessionChanged: {
        localMediaView.session = session
    }

    onStateChanged: {
        switch (state) {
            case Session.TransportDisconnected:
            case Session.RtcClosed:
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
