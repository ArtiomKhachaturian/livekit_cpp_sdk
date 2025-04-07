import QtQuick

VideoRenderer {

    property var options: app.defaultCameraOptions
    property var deviceInfo: undefined

    onTrackChanged: {
        if (track !== null) {
            track.deviceInfo = deviceInfo
            track.options = options
        }
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
