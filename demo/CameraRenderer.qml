import QtQuick

Item {
    id: root
    property var options: app.defaultCameraOptions
    property var deviceInfo: undefined
    property alias muted: renderer.muted
    property CameraTrackWrapper track: null

    function takeTrack() {
        var oldTrack = track
        if (oldTrack) {
            oldTrack.videoOutput = null
        }
        renderer.source = null
        track = null
        return oldTrack
    }

    VideoRenderer {
        id: renderer
        anchors.fill: parent
        onMutedChanged: {
            if (root.track !== null) {
                root.track.muted = muted
            }
        }
    }

    onTrackChanged: {
        if (track !== null) {
            track.deviceInfo = deviceInfo
            track.options = options
            track.muted = muted
        }
        renderer.source = track
    }

    onOptionsChanged: {
        if (track !== null) {
            track.options = options
        }
    }

    onDeviceInfoChanged: {
        if (track !== null) {
            track.deviceInfo = deviceInfo
        }
    }
}
