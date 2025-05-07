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
                        Image {
                           source: "qrc:/resources/images/secure.png"
                           width: 12
                           height: 12
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
        if (participant !== null) {
            for (var i = 0; i < participant.videoTracksCount; ++i) {
                addVideoTrack(participant.videoTrack(i))
            }
        }
    }
}
