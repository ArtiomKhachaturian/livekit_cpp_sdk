import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    property bool recording: true
    title: recording ? qsTr("Microphone") : qsTr("Speakers")
    RowLayout {
        anchors.fill: parent
        ToolButton {
            icon.name: recording ? "audio-input-microphone" : "audio-card"
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
