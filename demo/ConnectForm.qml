import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LiveKitClient 1.0

Item {

    id: root
    readonly property alias passPhrase: passPhrase.text
    property alias urlText: url.text
    property alias tokenText: token.text
    property alias activePreview: previewTracksModel.active
    property alias activeCamera: previewTracksModel.activeCamera
    property alias activeSharing: previewTracksModel.activeSharing
    property alias cameraDeviceInfo: previewTracksModel.cameraDeviceInfo
    property alias sharingDeviceInfo: previewTracksModel.sharingDeviceInfo
    property alias autoSubscribe: autoSubscribeChx.checked
    property alias adaptiveStream: adaptiveStreamChx.checked
    property alias e2e: e2eChx.checked
    property alias iceTransportPolicy: iceTransportPoliciesCombo.currentText

    signal connectClicked

    ColumnLayout {
        width: parent.width - 200
        anchors.centerIn: parent
        spacing: -1

        Frame {
            Layout.preferredHeight: 300
            Layout.fillHeight: true
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Repeater {
                    model: ConnectionFormVideoModel {
                        id: previewTracksModel
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
