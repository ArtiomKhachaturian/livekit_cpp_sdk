import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LiveKitClient 1.0

Frame {
    id: root

    property alias activeCamera: session.activeCamera
    property alias activeMicrophone: session.activeMicrophone
    property alias camerDeviceInfo: session.cameraDeviceInfo
    property alias cameraOptions: session.cameraOptions
    property alias cameraMuted: session.cameraMuted
    property alias microphoneMuted: session.microphoneMuted
    readonly property bool connected: session.connected
    readonly property bool connecting: session.connecting
    readonly property string identity : connected ? session.identity : objectName

    signal error(string desc, string details)

    Session {
        id: session
        onError: (desc, details) => {
            root.error(desc, details)
        }
        onChatMessageReceived: (participantIdentity, message, deleted) => {
            chatView.append(participantIdentity, message)
        }
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Pane {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ParticipantView {
                    id: localMediaView
                    participant: session.localParticipant
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
                    session.sendChatMessage(text)
                }
            }
        }
    }

    function connect(url, token) {
        if (!session.connectToSfu(url, token)) {
            error(qsTr("Failed connect to SFU"), "")
        }
    }

    function disconnect() {
        session.disconnectFromSfu()
    }
}
