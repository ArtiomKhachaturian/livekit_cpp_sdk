import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import LiveKitClient 1.0

Item {
    id: root

    property bool showIdentity: true
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
                        Menu {
                           id: contextMenu
                           closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
                           Menu {
                               title: qsTr("Content hint")
                               ActionGroup { id: contentHintGroup }
                               Action {
                                   text: qsTr("None")
                                   checkable: true
                                   checked: VideoTrack.None === renderer.source.contentHint
                                   ActionGroup.group: contentHintGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.contentHint = VideoTrack.None
                                       }
                                   }
                               }
                               Action {
                                   text: qsTr("Fluid")
                                   checkable: true
                                   checked: VideoTrack.Fluid === renderer.source.contentHint
                                   ActionGroup.group: contentHintGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.contentHint = VideoTrack.Fluid
                                       }
                                   }
                               }
                               Action {
                                   text: qsTr("Detailed")
                                   checkable: true
                                   checked: VideoTrack.Detailed === renderer.source.contentHint
                                   ActionGroup.group: contentHintGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.contentHint = VideoTrack.Detailed
                                       }
                                   }
                               }
                               Action {
                                   text: qsTr("Text")
                                   checkable: true
                                   checked: VideoTrack.Text === renderer.source.contentHint
                                   ActionGroup.group: contentHintGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.contentHint = VideoTrack.Text
                                       }
                                   }
                               }
                           }
                           Menu {
                               title: qsTr("Network priority")
                               ActionGroup { id: networkPriorityGroup }
                               Action {
                                   text: qsTr("Very low")
                                   checkable: true
                                   checked: VideoTrack.VeryLow === renderer.source.networkPriority
                                   ActionGroup.group: networkPriorityGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.networkPriority = VideoTrack.VeryLow
                                       }
                                   }
                               }
                               Action {
                                   text: qsTr("Low")
                                   checkable: true
                                   checked: VideoTrack.Low === renderer.source.networkPriority
                                   ActionGroup.group: networkPriorityGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.networkPriority = VideoTrack.Low
                                       }
                                   }
                               }
                               Action {
                                   text: qsTr("Medium")
                                   checkable: true
                                   checked: VideoTrack.Medium === renderer.source.networkPriority
                                   ActionGroup.group: networkPriorityGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.networkPriority = VideoTrack.Medium
                                       }
                                   }
                               }
                               Action {
                                   text: qsTr("High")
                                   checkable: true
                                   checked: VideoTrack.High === renderer.source.networkPriority
                                   ActionGroup.group: networkPriorityGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.networkPriority = VideoTrack.High
                                       }
                                   }
                               }
                           }
                           Menu {
                               title: qsTr("Degradation preference")
                               ActionGroup { id: degradationGroup }
                               Action {
                                   text: qsTr("Default")
                                   checkable: true
                                   checked: VideoTrack.Default === renderer.source.degradationPreference
                                   ActionGroup.group: degradationGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.degradationPreference = VideoTrack.Default
                                       }
                                   }
                               }
                               Action {
                                   text: qsTr("Disabled")
                                   checkable: true
                                   checked: VideoTrack.Disabled === renderer.source.degradationPreference
                                   ActionGroup.group: degradationGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.degradationPreference = VideoTrack.Disabled
                                       }
                                   }
                               }
                               Action {
                                   text: qsTr("Maintain framerate")
                                   checkable: true
                                   checked: VideoTrack.MaintainFramerate === renderer.source.degradationPreference
                                   ActionGroup.group: degradationGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.degradationPreference = VideoTrack.MaintainFramerate
                                       }
                                   }
                               }
                               Action {
                                   text: qsTr("Maintain resolution")
                                   checkable: true
                                   checked: VideoTrack.MaintainResolution === renderer.source.degradationPreference
                                   ActionGroup.group: degradationGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.degradationPreference = VideoTrack.MaintainResolution
                                       }
                                   }
                               }
                               Action {
                                   text: qsTr("Balanced")
                                   checkable: true
                                   checked: VideoTrack.Balanced === renderer.source.degradationPreference
                                   ActionGroup.group: degradationGroup
                                   onCheckedChanged: (checked) => {
                                       if (checked) {
                                           renderer.source.degradationPreference = VideoTrack.Balanced
                                       }
                                   }
                               }
                           }
                       }
                    }
                }
            }
            TextPanel {
                Layout.fillWidth: true
                visible: showIdentity
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
