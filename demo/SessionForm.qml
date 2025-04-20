import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LiveKitClient 1.0

Frame {
    id: root

    property alias activeCamera: session.activeCamera
    property alias activeMicrophone: session.activeMicrophone
    property alias activeSharing: session.activeSharing
    property alias camerDeviceInfo: session.cameraDeviceInfo
    property alias cameraOptions: session.cameraOptions
    property alias cameraMuted: session.cameraMuted
    property alias microphoneMuted: session.microphoneMuted
    property alias sharingDeviceInfo: session.sharingDeviceInfo
    property alias sharingMuted: session.sharingMuted
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
        onRemoteParticipantAdded: participant => {
            addParticipant(participant)
        }
        onRemoteParticipantRemoved:  participant => {
            removeParticipant(participant)
        }

        Component.onCompleted: {
            addParticipant(localParticipant)
        }

        Component.onDestruction: {
            removeParticipant(localParticipant)
        }

        function addParticipant(participant) {
            if (null !== participant) {
                participants.append({data:participant})
            }
        }

        function removeParticipant(participant) {
            if (null !== participant) {
                for (var i = 0; i < participants.count; i++) {
                    if (participants.get(i).data === participant) {
                        participants.remove(i)
                        break
                    }
                }
            }
        }
    }

    ListModel {
        id: participants
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Pane {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ElementsGrid {
                    anchors.fill: parent
                    model: participants
                    delegate: ParticipantView {
                        anchors.fill: parent
                        property var modelData
                        participant: modelData
                        showIdentity: participants.count > 1
                    }
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

    function connect(url, token, autoSubscribe, adaptiveStream, e2e, iceTransportPolicy, passPhrase = "") {
        if (!session.connectToSfu(url, token, autoSubscribe, adaptiveStream,
                                  e2e, iceTransportPolicy, passPhrase)) {
            error(qsTr("Failed connect to SFU"), "")
        }
    }

    function disconnect() {
        session.disconnectFromSfu()
    }
}
