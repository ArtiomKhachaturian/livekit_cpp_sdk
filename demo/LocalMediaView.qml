import QtQuick

Item {
    id: root

    property SessionWrapper session: null
    property bool cameraAdded: false
    property bool microphoneAdded: false
    property bool microphoneMuted: false
    property alias cameraDeviceInfo: localCameraView.deviceInfo
    property alias cameraOptions: localCameraView.options
    property alias cameraMuted: localCameraView.muted
    readonly property bool hasMicrophoneTrack: cachedObjects.micTrack !== null
    readonly property bool hasCameraTrack: localCameraView.track !== null

    CameraRenderer {
        id: localCameraView
        anchors.fill: parent
    }

    QtObject {
        id: cachedObjects
        property AudioTrackWrapper micTrack: null
        property SessionWrapper cachedSession: null
        onMicTrackChanged: {
            if (micTrack !== null) {
                micTrack.muted = root.microphoneMuted
            }
        }
    }

    onSessionChanged: {
        if (cachedObjects.cachedSession !== null) {
            cachedObjects.cachedSession.removeMicrophoneTrack(cachedObjects.micTrack)
            cachedObjects.cachedSession.removeCameraTrack(localCameraView.takeTrack())
            cachedObjects.micTrack = null
            cachedObjects.cachedSession = null
        }
        if (session !== null) {
            if (microphoneAdded) {
                cachedObjects.micTrack = session.addMicrophoneTrack()
                if (cachedObjects.micTrack === null) {
                    microphoneAdded = false
                }
            }
            if (cameraAdded) {
                localCameraView.track = addCameraTrack()
                if (localCameraView.track === null) {
                    cameraAdded = false
                }
            }
            cachedObjects.cachedSession = session
        }
    }

    onCameraAddedChanged: {
        if (session !== null) {
            if (cameraAdded) {
                localCameraView.track = addCameraTrack()
                if (localCameraView.track === null) {
                    cameraAdded = false
                }
            }
            else {
                if (localCameraView.track !== null) {
                    session.removeCameraTrack(localCameraView.takeTrack())
                }
            }
        }
    }

    onMicrophoneAddedChanged: {
        if (session !== null) {
            if (microphoneAdded) {
                cachedObjects.micTrack = session.addMicrophoneTrack()
                if (cachedObjects.micTrack === null) {
                    microphoneAdded = false
                }
            }
            else {
                if (cachedObjects.micTrack !== null) {
                    session.removeMicrophoneTrack(cachedObjects.micTrack)
                    cachedObjects.micTrack = null
                }
            }
        }
    }

    onMicrophoneMutedChanged: {
        if (cachedObjects.micTrack !== null) {
            cachedObjects.micTrack.muted = microphoneMuted
        }
    }

    function addCameraTrack() {
        if (session !== null) {
            return session.addCameraTrack(localCameraView.deviceInfo, localCameraView.options)
        }
        return null
    }
}
