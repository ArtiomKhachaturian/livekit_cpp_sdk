import QtQuick
import QtQuick.Layouts
import LiveKitClient 1.0

Item {
    id: root

    property Participant participant
    property bool microphoneAdded: false
    property bool microphoneMuted: false

    ListModel {
        id : videoTracks
    }

    StackLayout {
        id: videoViews
        anchors.fill: parent
        Repeater {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: videoTracks
            VideoRenderer {
                anchors.fill: parent
                source: model.track
            }
        }
    }

    Connections {
        target: participant
        function onVideoTrackAdded(id) {
            var track = participant.videoTrack(id)
            if (null !== track) {
                videoTracks.append({id:id, track:track})
            }
        }
        function onVideoTrackRemoved(id) {
            for (var i = 0; i < videoTracks.count; i++) {
                if (videoTracks.get(i).id === id) {
                    videoTracks.remove(i)
                    break
                }
            }
        }
    }
}
