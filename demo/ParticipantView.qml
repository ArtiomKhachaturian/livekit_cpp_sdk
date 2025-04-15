import QtQuick
import QtQuick.Layouts
import LiveKitClient 1.0

Item {
    id: root

    property bool showIdentity: true
    property Participant participant
    readonly property bool hasVideo: videoTracks.count > 0
    readonly property bool hasAudio: participant.audioTracksCount > 0

    ListModel {
        id : videoTracks
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
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        source: model.track
                    }
                }
            }
            TextPanel {
                Layout.fillWidth: true
                visible: showIdentity
                text: participant.identity
            }
        }
    }

    function addVideoTrack(track) {
        if (null !== track) {
            videoTracks.append({id:track.id, track:track})
            if (track.screencast) {
                videoViews.currentIndex = videoTracks.count - 1
            }
            //console.log("video track added, ID: " + track.id)
        }
    }

    function addVideoTrackById(id) {
        addVideoTrack(participant.videoTrack(id))
    }

    function removeVideoTrackById(id) {
        for (var i = 0; i < videoTracks.count; i++) {
            if (videoTracks.get(i).id === id) {
                if (i > 0 && i === videoViews.currentIndex) {
                    videoViews.currentIndex = i -1
                }
                videoTracks.remove(i)
                //console.log("video track removed, ID: " + id)
                break
            }
        }
    }

    Connections {
        target: participant
        function onVideoTrackAdded(id) { addVideoTrackById(id) }
        function onVideoTrackRemoved(id) { removeVideoTrackById(id) }
        function onSpeakerInfoChanged(level, active) {
            //console.log("level = " + level + ", active = " + active)
            mediaRoot.audioLevel = level
            mediaRoot.activeSpeaker = active
        }
    }

    onParticipantChanged: {
        videoTracks.clear()
        for (var i = 0; i < participant.videoTracksCount; ++i) {
            addVideoTrack(participant.videoTrack(i))
        }
    }
}
