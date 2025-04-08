import QtQuick
import QtMultimedia

Item {
    id: root
    property VideoSinkWrapper track: null
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
        property VideoSinkWrapper cachedTrack: null
        VideoDiagnosticsView {
            id: fpsArea
            x: parent.contentRect.right - 4 - width
            y: parent.contentRect.top + 4
            visible: false
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
            fpsArea.fps = Qt.binding(function() {
                if (track !== null) {
                    return track.fps
                }
                return 0
            })
            fpsArea.frameSize = Qt.binding(function() {
                if (track !== null) {
                    return track.frameSize
                }
                return Qt.size(0, 0)
            })
            fpsArea.visible = Qt.binding(function() {
                if (track !== null) {
                    if (!track.muted) {
                        var size = track.frameSize
                        var fps = track.fps
                        return (size.width > 0 && size.height > 0) || fps > 0
                    }
                }
                return false
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
