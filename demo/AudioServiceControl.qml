import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    property bool recording: true
    title: recording ? qsTr("Microphone") : qsTr("Speakers")
    RowLayout {
        anchors.fill: parent
        ToolButton {
            icon.source: {
                if (recording) {
                    if (checked) {
                        return "qrc:/resources/images/mic-on.png"
                    }
                    return "qrc:/resources/images/mic-off.png"
                }
                if (checked) {
                    return "qrc:/resources/images/volume_on.png"
                }
                return "qrc:/resources/images/volume-off.png"
            }
            checkable: true
            checked: recording ? app.audioRecordingEnabled : app.audioPlayoutEnabled
            display: ToolButton.IconOnly
            onCheckedChanged: {
                if (recording) {
                    app.audioRecordingEnabled = checked
                }
                else {
                    app.audioPlayoutEnabled = checked
                }
            }
        }
        ToolButton {
            icon.source: "qrc:/resources/images/effect.png"
            checkable: true
            checked: recording ? app.audioRecordingProcessingEnabled : app.audioPlayoutProcessingEnabled
            display: ToolButton.IconOnly
            onCheckedChanged: {
                if (recording) {
                    app.audioRecordingProcessingEnabled = checked
                }
                else {
                    app.audioPlayoutProcessingEnabled = checked
                }
            }
        }

        ComboBox {
            Layout.fillWidth: true
            model: recording ? app.recordingAudioDevicesModel : app.playoutAudioDevicesModel
            textRole: "display"
            onActivated: index => {
                var device = model.itemAt(index)
                if (recording) {
                    app.recordingAudioDevice = device
                }
                else {
                    app.playoutAudioDevice = device
                }
            }
            Component.onCompleted: {
                var device = recording ? app.recordingAudioDevice : app.playoutAudioDevice
                currentIndex = model.indexOf(device)
            }
        }
        Label {
            text: qsTr("Volume:")
        }
        Slider {
            from: 0
            to: 100
            value: recording ? app.audioRecordingVolume : app.audioPlayoutVolume
            onMoved: {
                if (recording) {
                    app.audioRecordingVolume = value
                }
                else {
                    app.audioPlayoutVolume = value
                }
            }
        }
    }
}
