import QtQuick

Item {
    property var options: app.defaultCameraOptions
    property var deviceInfo: undefined
    property bool muted: false
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

    onMutedChanged: {
        if (track !== null) {
            track.muted = muted
        }
    }
}
