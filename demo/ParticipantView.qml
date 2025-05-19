import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import LiveKitClient 1.0

Item {
    id: root

    property Participant participant
    readonly property bool hasVideo: videoTracks.count > 0
    readonly property bool hasAudio: participant && participant.audioTracksCount > 0

    ListModel {
        id : videoTracks
        property bool cameraTracksActive: false
        property bool sharingTracksActive: false
        property int camerasCount: 0
        property int sharingsCount: 0
        function addTrack(track) {
            if (null !== track) {
                append({id:track.id, track:track})
                track.firstFrameSentOrReceived.connect(updateTracksState)
                updateTracksState()
                if (true === track.screencast) {
                    videoViews.currentIndex = count - 1
                    ++sharingsCount
                }
                else {
                    ++camerasCount
                }
                return true
            }
            return false
        }
        function removeTrack(id) {
            for (var i = 0; i < count; i++) {
                if (get(i).id === id) {
                    var track = get(i).track
                    track.firstFrameSentOrReceived.disconnect(updateTracksState)
                    if (i > 0 && i === videoViews.currentIndex) {
                        videoViews.currentIndex = i -1
                    }
                    remove(i)
                    if (true === track.screencast) {
                        --sharingsCount
                    }
                    else {
                        --camerasCount
                    }
                    updateTracksState()
                    return true
                }
            }
            return false
        }
        function updateTracksState() {
            if (count > 0) {
                var camerasActive = 0, sharingsActive = 0
                for (var i = 0; i < count; i++) {
                    var track = get(i).track
                    if (track.firstPacketSentOrReceived) {
                        if (track.screencast) {
                            ++sharingsActive
                        }
                        else {
                            ++camerasActive
                        }
                    }
                }
                cameraTracksActive = camerasCount === camerasActive
                sharingTracksActive = sharingsCount === sharingsActive
            }
            else {
                cameraTracksActive = sharingTracksActive = false
            }
        }
    }

    ListModel {
        id: audioTracks
        property bool tracksActive: false
        function addTrack(track) {
            if (null !== track) {
                append({id:track.id, track:track})
                track.firstFrameSentOrReceived.connect(updateTracksState)
                updateTracksState()
                return true
            }
            return false
        }
        function removeTrack(id) {
            for (var i = 0; i < count; i++) {
                if (get(i).id === id) {
                    var track = get(i).track
                    track.firstFrameSentOrReceived.disconnect(updateTracksState)
                    remove(i)
                    updateTracksState()
                    return true
                }
            }
            return false
        }
        function updateTracksState() {
            if (count > 0) {
                for (var i = 0; i < count; i++) {
                    var track = get(i).track
                    if (!track.firstPacketSentOrReceived) {
                        tracksActive = false
                        return
                    }
                }
                tracksActive = true
            }
            else {
                tracksActive = false
            }
        }
    }

    Rectangle {
        id: mediaRoot
        property bool activeSpeaker: false
        property real audioLevel: 0
        anchors.fill: parent
        border.width: activeSpeaker ? 4 : 0
        border.color: Qt.rgba(0, audioLevel * 0.3, audioLevel, audioLevel)
        Behavior on border.color {
            ColorAnimation { duration: 200 }
        }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: mediaRoot.border.width
            StackLayout {
                id: videoViews
                Layout.fillWidth: true
                Layout.fillHeight: true
                Repeater {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: videoTracks
                    VideoRenderer {
                        id: renderer
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        source: model.track
                        readonly property bool local: {
                            if (source !== null) {
                                return !source.remote
                            }
                            return false
                        }
                        readonly property bool firstPacketSentOrReceived: {
                            return source !== null && source.firstPacketSentOrReceived
                        }
                        Image {
                            id: secure
                            source: "qrc:/resources/images/secure.png"
                            width: 12
                            height: width
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.margins: 2
                            z: 1
                            visible: model.track && model.track.secure
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: (mouse) => {
                                switch (mouse.button) {
                                    case Qt.RightButton:
                                        contextMenu.popup()
                                        break
                                   case Qt.LeftButton:
                                       contextMenu.close()
                                       break
                                }
                            }
                            onPressAndHold: (mouse) => {
                                if (mouse.source === Qt.MouseEventNotSynthesized) {
                                    contextMenu.popup()
                                }
                                else {
                                     contextMenu.close()
                                }
                            }
                        }
                        VideoTrackContextMenu {
                            id: contextMenu
                            source: renderer.source
                        }
                    }
                }
            }
            TextPanel {
                Layout.fillWidth: true
                text: participant ? participant.identity : ""
                RowLayout {
                    anchors.fill: parent
                    TrackActivityIndicator {
                        id: cameraTracksActivity
                        active: videoTracks.cameraTracksActive
                        height: parent.height
                        width: height
                        kind: 0
                        visible: videoTracks.camerasCount > 0
                    }
                    TrackActivityIndicator {
                        id: audioTracksActivity
                        active: audioTracks.tracksActive
                        height: parent.height
                        width: height
                        kind: 1
                        visible: audioTracks.count > 0
                    }
                    TrackActivityIndicator {
                        active: videoTracks.sharingTracksActive
                        height: parent.height
                        width: height
                        kind: 2
                        visible: videoTracks.sharingsCount > 0
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    function addVideoTrack(track) {
        if (null !== track) {
            videoTracks.append({id:track.id, track:track})
            if (track.screencast) {
                videoViews.currentIndex = videoTracks.count - 1
            }
            console.log("video track added, ID: " + track.id)
        }
    }


    Connections {
        target: participant
        function onVideoTrackAdded(id) { videoTracks.addTrack(participant.videoTrack(id)) }
        function onVideoTrackRemoved(id) { videoTracks.removeTrack(id) }
        function onAudioTrackAdded(id) { audioTracks.addTrack(participant.audioTrack(id)) }
        function onAudioTrackRemoved(id) { audioTracks.removeTrack(id) }

        function onSpeakerInfoChanged(level, active) {
            //console.log("level = " + level + ", active = " + active)
            mediaRoot.audioLevel = level
            mediaRoot.activeSpeaker = active
        }
    }

    onParticipantChanged: {
        videoTracks.clear()
        audioTracks.clear()
        if (participant !== null) {
            for (var v = 0; v < participant.videoTracksCount; ++v) {
                videoTracks.addTrack(participant.videoTrack(v))
            }
            for (var a = 0; a < participant.audioTracksCount; ++a) {
                audioTracks.addTrack(participant.audioTrack(a))
            }
        }
    }
}
