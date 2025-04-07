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
        Rectangle {
            id: fpsArea
            x: renderer.contentRect.right - width
            y: renderer.contentRect.top
            width: 100
            height: 20
            visible: {
                return fpsText.frameSize.width > 0 && fpsText.frameSize.height > 0
            }
            color: "yellow"
            Text {
                id: fpsText
                property int fps: 0
                property size frameSize: Qt.size(0, 0)
                anchors.fill: parent
                text: qsTr("%1x%2 %3 fps").arg(frameSize.width).arg(frameSize.height).arg(fps)
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
            fpsText.fps = Qt.binding(function() {
                if (track !== null) {
                    return track.fps
                }
                return 0
            })
            fpsText.frameSize = Qt.binding(function() {
                if (track !== null) {
                    return track.frameSize
                }
                return Qt.size(0, 0)
            })
            renderer.cachedTrack = track
        }
    }

    onMutedChanged: {
        if (track !== null) {
            track.muted = muted
        }
    }
}
