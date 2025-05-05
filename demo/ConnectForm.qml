import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LiveKitClient 1.0

Item {

    id: root
    readonly property alias passPhrase: passPhrase.text
    property string videoFilter
    property string prefferedVideoEncoder
    property string prefferedAudioEncoder
    property alias urlText: url.text
    property alias tokenText: token.text
    property alias activePreview: previewTracksModel.active
    property alias activeCamera: previewTracksModel.activeCamera
    property alias activeSharing: previewTracksModel.activeSharing
    property alias cameraDeviceInfo: previewTracksModel.cameraDeviceInfo
    property alias cameraOptions: previewTracksModel.cameraOptions
    property alias sharingDeviceInfo: previewTracksModel.sharingDeviceInfo
    property alias sharingOptions: previewTracksModel.sharingOptions
    property alias autoSubscribe: autoSubscribeChx.checked
    property alias adaptiveStream: adaptiveStreamChx.checked
    property alias e2e: e2eChx.checked
    property alias iceTransportPolicy: iceTransportPoliciesCombo.currentText

    signal connectClicked

    ColumnLayout {
        //width: parent.width - 10
        //anchors.centerIn: parent
        anchors.fill: parent
        anchors.margins: 10
        spacing: -1

        Frame {
            //Layout.preferredHeight: 300
            Layout.fillHeight: true
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Repeater {
                    model: ConnectionFormVideoModel {
                        id: previewTracksModel
                        filter: root.videoFilter
                    }
                    delegate: VideoRenderer {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        source: previewTracksModel.sourceAt(index)
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            ColumnLayout {
                id: elementsLayout
                anchors.fill: parent

                Label {
                    text: qsTr("Token")
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                }

                TextMultiLine {
                    id: token
                    placeholderText: qsTr("Token")
                    Layout.fillWidth: true
                    Layout.minimumHeight: 60
                    Layout.maximumHeight: Layout.minimumHeight
                }

                RowLayout {
                    Label {
                        text: qsTr("Server URL")
                        Layout.alignment: Qt.AlignHCenter
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                    }
                    TextField {
                        id: url
                        placeholderText: qsTr("LiveKit Server URL: wss://*.livekit.cloud")
                        Layout.fillWidth: true
                        horizontalAlignment: TextField.AlignHCenter
                        verticalAlignment: TextField.AlignVCenter
                    }

                    Button {
                        id: connect
                        highlighted: true
                        text: qsTr("Connect")
                        enabled: url.text !== "" && token.text !== ""
                        onClicked: {
                            connectClicked()
                        }
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Switch {
                        id: autoSubscribeChx
                        text: qsTr("Auto subscribe")
                        checked: true
                    }
                    Switch {
                        id: adaptiveStreamChx
                        text: qsTr("Adaptive stream")
                        checked: true
                    }
                    Switch {
                        id: videoEncoderChx
                        text: qsTr("Video encoder:")
                        checked: true
                        onCheckedChanged: {
                            if (checked) {
                                root.prefferedVideoEncoder = videoEncoderCb.currentText
                            }
                            else {
                                root.prefferedVideoEncoder = ""
                            }
                        }
                    }
                    ComboBox {
                        id: videoEncoderCb
                        enabled: videoEncoderChx.checked
                        Layout.preferredWidth: 70
                        model: app.videoEncoders()
                        onCurrentTextChanged: {
                            if (enabled) {
                                root.prefferedVideoEncoder = currentText
                            }
                        }
                        onModelChanged: {
                            if (model) {
                                currentIndex = 3
                            }
                        }
                    }
                    Switch {
                        id: audioEncoderChx
                        text: qsTr("Audio encoder:")
                        checked: false
                        onCheckedChanged: {
                            if (checked) {
                                root.prefferedAudioEncoder = audioEncoderCb.currentText
                            }
                            else {
                                root.prefferedAudioEncoder = ""
                            }
                        }
                    }

                    ComboBox {
                        id: audioEncoderCb
                        enabled: audioEncoderChx.checked
                        Layout.preferredWidth: 70
                        model: app.audioEncoders()
                        onCurrentTextChanged: {
                            if (enabled) {
                                root.prefferedAudioEncoder = currentText
                            }
                        }
                    }
                    Label {
                        text: qsTr("ICE transport policy:")
                    }
                    ComboBox {
                        id: iceTransportPoliciesCombo
                        model: app.iceTransportPolicies
                        currentIndex: app.defaultIceTransportPolicyIndex
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Switch {
                        id: e2eChx
                        text: qsTr("E2E security")
                        checked: false
                    }
                    Label {
                        text: qsTr("Passphrase:")
                        enabled: e2eChx.checked
                    }
                    TextField {
                        id: passPhrase
                        enabled: e2eChx.checked
                        echoMode: TextInput.Password
                        Layout.fillWidth: true
                        horizontalAlignment: TextField.AlignHCenter
                        verticalAlignment: TextField.AlignVCenter
                        text: "2ze2xwut7f06k5zei50zjwmwncsp2exsseu0u79hcq97qduhpy3dwki2xz6rvrfl"
                        onPressed: {
                            echoMode = TextInput.Normal
                        }
                        onReleased: {
                            echoMode = TextInput.Password
                        }
                    }
                }
            }
        }
    }
}
