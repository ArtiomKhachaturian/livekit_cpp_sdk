import QtQuick
import QtMultimedia

Item {
    property VideoTrackWrapper track: null
    property bool muted: false

    function takeTrack() {
        var oldTrack = track
        if (oldTrack) {
            oldTrack.videoOutput = null
        }
        renderer.cachedTrack = track = null
        return oldTrack
    }

    VideoOutput {
        id: renderer
        anchors.fill: parent
        property VideoTrackWrapper cachedTrack: null
        property int fps: 0
        Rectangle {
            id: fpsArea
            x: renderer.contentRect.right - width
            y: renderer.contentRect.top
            width: 100
            height: 20
            visible: renderer.contentRect.right - renderer.contentRect.left > 0
            color: "yellow"
            Text {
                anchors.fill: parent
                text: renderer.fps
            }
        }
    }

    onTrackChanged: {
        if (renderer.cachedTrack !== null) {
            renderer.cachedTrack.videoOutput = null
            renderer.cachedTrack = null
        }
        if (track !== null) {
            track.muted = muted
            track.videoOutput = renderer.videoSink
            renderer.fps = Qt.binding(function() { return track.fps })
            renderer.cachedTrack = track
        }
    }

    onMutedChanged: {
        if (track !== null) {
            track.muted = muted
        }
    }
}
