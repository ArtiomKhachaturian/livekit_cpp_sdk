import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import LiveKitClient 1.0

Frame {
    id: root

    property alias activeCamera: session.activeCamera
    property alias activeMicrophone: session.activeMicrophone
    property alias activeSharing: session.activeSharing

    property alias cameraDeviceInfo: session.cameraDeviceInfo
    property alias cameraOptions: session.cameraOptions
    property alias cameraMuted: session.cameraMuted

    property alias sharingDeviceInfo: session.sharingDeviceInfo
    property alias sharingOptions: session.sharingOptions
    property alias sharingMuted: session.sharingMuted

    property string localVideoFilter
    property string prefferedVideoEncoder
    property string prefferedAudioEncoder

    property alias microphoneMuted: session.microphoneMuted
    property alias microphoneOptions: session.microphoneOptions


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
            addParticipant(localParticipant, false)
        }

        onLocalMediaTrackAddFailure: (audio, id, details) => {
            if (audio) {
                error(qsTr("Failed to add audio track to session"), details)
            }
            else {
                error(qsTr("Failed to add video track to session"), details)
            }
        }

        Component.onDestruction: {
            removeParticipant(localParticipant, false)
        }

        function addParticipant(participant, playSound = true) {
            if (null !== participant) {
                participants.append({data:participant})
                if (playSound) {
                    soundEffect.join = true
                    soundEffect.play()
                }
            }
        }

        function removeParticipant(participant, playSound = true) {
            if (null !== participant) {
                for (var i = 0; i < participants.count; i++) {
                    if (participants.get(i).data === participant) {
                        participants.remove(i)
                        if (playSound) {
                            soundEffect.join = false
                            soundEffect.play()
                        }
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

    SoundEffect {
        id: soundEffect
        property bool join: true
        source: join ? "qrc:/resources/sounds/Join.wav" : "qrc:/resources/sounds/Left.wav"
    }

    Timer {
        id: statsTimer
        interval: 1000
        repeat: true
        running: session.connected || session.connected
        onTriggered: {
            for (var i = 0; i < participants.count; i++) {
                participants.get(i).data.queryVideoStats()
            }
        }
    }

    onLocalVideoFilterChanged: {
        session.localParticipant.videoFilter = localVideoFilter
    }

    onPrefferedVideoEncoderChanged: {
        session.setPrefferedVideoEncoder(prefferedVideoEncoder)
    }

    onPrefferedAudioEncoderChanged: {
        session.setPrefferedAudioEncoder(prefferedAudioEncoder)
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
