import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LiveKitClient 1.0

Pane {
    id: root
    readonly property alias cameraDeviceInfo: cameraModelComboBox.deviceInfo
    readonly property alias cameraOptions: cameraOptionsComboBox.options
    property alias micGroupEnabled : micGroup.enabled
    readonly property var sharingOptions: {
        var options = app.emptyVideoOptions()
        if (sharingOrigResChx.checked) {
            options.width = options.height = 0
        }
        else {
            options.width = sharingResWidth.value
            options.height = sharingResHeight.value
        }
        options.maxFPS = defaultFpsChx.checked ? 0 : sharingFps.value
        return options
    }

    readonly property var microphoneOptions: {
        var options = app.emptyAudioRecordingOptions()
        options.echoCancellation = echoCancellationChx.checkState
        options.autoGainControl = autoGainControlChx.checkState
        options.noiseSuppression = noiseSuppressionChx.checkState
        options.highpassFilter = highpassFilterChx.checkState
        options.stereoSwapping = stereoSwappingChx.checkState
        return options
    }

    signal accepted

    contentItem: ColumnLayout {
        anchors.fill: parent

        CheckBox {
            id: liveChangesChx
            Layout.fillWidth: true
            text: qsTr("Live changes")
            checked: true
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Camera")
            ColumnLayout {
                anchors.fill: parent
                CameraModelComboBox {
                    id: cameraModelComboBox
                    Layout.fillWidth: true
                    onCurrentIndexChanged: {
                        if (liveChangesChx.checked) {
                            root.accepted()
                        }
                    }
                }

                CameraOptionsComboBox {
                    id: cameraOptionsComboBox
                    deviceInfo: cameraModelComboBox.deviceInfo
                    Layout.fillWidth: true
                    onCurrentIndexChanged: {
                        if (liveChangesChx.checked) {
                            root.accepted()
                        }
                    }
                }

                CheckBox {
                    id: interlacedChx
                    text: qsTr("Interlaced frames")
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Sharing")
            ColumnLayout {
                anchors.fill: parent
                CheckBox {
                    id: sharingOrigResChx
                    text: qsTr("Original resolution (width x height)")
                    checked: true
                    onCheckedChanged: {
                        if (liveChangesChx.checked && checked) {
                            root.accepted()
                        }
                    }
                }
                ValueControl {
                    id: sharingResWidth
                    from: 128
                    to: 8912
                    stepSize: 64
                    value: 1920
                    enabled: !sharingOrigResChx.checked
                    onValueChanged: {
                        if (liveChangesChx.checked && enabled) {
                            root.accepted()
                        }
                    }
                }
                ValueControl {
                    id: sharingResHeight
                    from: sharingResWidth.from
                    to: sharingResWidth.to
                    stepSize: sharingResWidth.stepSize
                    value: 1080
                    enabled: sharingResWidth.enabled
                    onValueChanged: {
                        if (liveChangesChx.checked && enabled) {
                            root.accepted()
                        }
                    }
                }
                CheckBox {
                    id: defaultFpsChx
                    text: qsTr("Framerate by default")
                    checked: true
                    onCheckedChanged: {
                        if (liveChangesChx.checked && checked) {
                            root.accepted()
                        }
                    }
                }
                ValueControl {
                    id: sharingFps
                    from: 1
                    to: 60
                    stepSize: 5
                    value: 30
                    enabled: !defaultFpsChx.checked
                    onValueChanged: {
                        if (liveChangesChx.checked && enabled) {
                            root.accepted()
                        }
                    }
                }
            }
        }

        GroupBox {
            id: micGroup
            Layout.fillWidth: true
            title: qsTr("Microphone")
            ColumnLayout {
                anchors.fill: parent
                CheckBox {
                    id: echoCancellationChx
                    text: qsTr("Echo cancellation")
                    tristate: true
                    onCheckStateChanged: {
                        if (liveChangesChx.checked) {
                            root.accepted()
                        }
                    }
                }
                CheckBox {
                    id: autoGainControlChx
                    text: qsTr("Auto gain control")
                    tristate: true
                    onCheckStateChanged: {
                        if (liveChangesChx.checked) {
                            root.accepted()
                        }
                    }
                }
                CheckBox {
                    id: noiseSuppressionChx
                    text: qsTr("Noise suppression")
                    tristate: true
                    onCheckStateChanged: {
                        if (liveChangesChx.checked) {
                            root.accepted()
                        }
                    }
                }
                CheckBox {
                    id: highpassFilterChx
                    text: qsTr("Highpass filter")
                    tristate: true
                    onCheckStateChanged: {
                        if (liveChangesChx.checked) {
                            root.accepted()
                        }
                    }
                }
                CheckBox {
                    id: stereoSwappingChx
                    text: qsTr("Stereo swapping")
                    tristate: true
                    onCheckStateChanged: {
                        if (liveChangesChx.checked) {
                            root.accepted()
                        }
                    }
                }
            }
        }

        Item { // spacer
            Layout.fillHeight: true
        }

        DialogButtonBox {
            Layout.fillWidth: true
            standardButtons: DialogButtonBox.Close
            Button {
                text: qsTr("Apply")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            onAccepted: {
                root.accepted()
            }
            onRejected: {
                root.visible = false
            }
        }

        component ValueControl : RowLayout {
            property alias from: sliderValue.from
            property alias to: sliderValue.to
            property alias stepSize: sliderValue.stepSize
            property alias value: sliderValue.value
            TextField {
                id: textValue
                text: sliderValue.value
                Layout.maximumWidth: 40
                validator: IntValidator {
                    bottom: sliderValue.from
                    top: sliderValue.to
                }
            }
            Slider {
                id: sliderValue
                value: textValue.text
                Layout.fillWidth: true
                Layout.columnSpan: 2
            }
        }
    }
}
