import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LiveKitClient 1.0

Pane {
    id: root
    readonly property alias cameraDeviceInfo: cameraModelComboBox.deviceInfo
    readonly property alias cameraOptions: cameraOptionsComboBox.options
    readonly property alias microphoneOptions: micGroup.options
    readonly property string videoFilter: {
        if (videoFilterChx.checked) {
            return videoFilterCombo.currentText
        }
        return ""
    }

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

    signal accepted

    function notifyAboutAccepted(force = false) {
        if (force || liveChangesChx.checked) {
            Qt.callLater(root.accepted)
        }
    }

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
                spacing: 0
                RowLayout {
                    CameraModelComboBox {
                        id: cameraModelComboBox
                        Layout.fillWidth: true
                        onCurrentIndexChanged: notifyAboutAccepted()
                    }
                    ToolButton {
                        Layout.alignment: Qt.AlignRight
                        //icon.width: 16
                        //icon.height: icon.width
                        icon.name: "camera-video"
                        onClicked: {
                            app.displayCameraSettingsDialogBox(cameraModelComboBox.deviceInfo,
                                                               qsTr("Camera settings"), 0, 0)
                        }
                    }
                }
                RowLayout {
                    CameraOptionsComboBox {
                        id: cameraOptionsComboBox
                        deviceInfo: cameraModelComboBox.deviceInfo
                        Layout.fillWidth: true
                        onCurrentIndexChanged: notifyAboutAccepted()
                    }
                    CheckBox {
                        id: interlacedChx
                        text: qsTr("Interlaced")
                    }
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
                    onCheckedChanged: notifyAboutAccepted()
                }
                RowLayout {
                    Layout.preferredWidth: parent.implicitWidth
                    ValueControl {
                        id: sharingResWidth
                        from: 128
                        to: 8912
                        stepSize: 64
                        value: 1920
                        enabled: !sharingOrigResChx.checked
                        LayoutMirroring.enabled: true
                        onValueChanged: {
                            if (enabled) {
                                notifyAboutAccepted()
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
                            if (enabled) {
                                notifyAboutAccepted()
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.preferredWidth: parent.implicitWidth
                    CheckBox {
                        id: defaultFpsChx
                        text: qsTr("Default framerate")
                        checked: true
                        onCheckedChanged: notifyAboutAccepted()
                    }
                    ValueControl {
                        id: sharingFps
                        from: 1
                        to: 60
                        stepSize: 5
                        value: 30
                        enabled: !defaultFpsChx.checked
                        onValueChanged: {
                            if (enabled) {
                                notifyAboutAccepted()
                            }
                        }
                    }
                }
            }
        }

        GroupBox {
            id: micGroup
            Layout.fillWidth: true
            title: qsTr("Microphone")
            property var options: undefined
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                CheckBox {
                    id: echoCancellationChx
                    text: qsTr("Echo cancellation")
                    tristate: true
                    onCheckStateChanged: {
                        if (micGroup.options !== undefined) {
                            micGroup.options.setEchoCancellation(checkState)
                            notifyAboutAccepted()
                        }
                    }
                }
                CheckBox {
                    id: autoGainControlChx
                    text: qsTr("Auto gain control")
                    tristate: true
                    onCheckStateChanged: {
                        if (micGroup.options !== undefined) {
                            micGroup.options.setAutoGainControl(checkState)
                            notifyAboutAccepted()
                        }
                    }
                }
                CheckBox {
                    id: noiseSuppressionChx
                    text: qsTr("Noise suppression")
                    tristate: true
                    onCheckStateChanged: {
                        if (micGroup.options !== undefined) {
                            micGroup.options.setNoiseSuppression(checkState)
                            notifyAboutAccepted()
                        }
                    }
                }
                CheckBox {
                    id: highpassFilterChx
                    text: qsTr("Highpass filter")
                    tristate: true
                    onCheckStateChanged: {
                        if (micGroup.options !== undefined) {
                            micGroup.options.setHighpassFilter(checkState)
                            notifyAboutAccepted()
                        }
                    }
                }
                CheckBox {
                    id: stereoSwappingChx
                    text: qsTr("Stereo swapping")
                    tristate: true
                    onCheckStateChanged: {
                        if (micGroup.options !== undefined) {
                            micGroup.options.setStereoSwapping(checkState)
                            notifyAboutAccepted()
                        }
                    }
                }
            }
            Component.onCompleted: {
                var audioOptions = app.emptyAudioRecordingOptions()
                echoCancellationChx.checkState = audioOptions.echoCancellation()
                autoGainControlChx.checkState = audioOptions.autoGainControl()
                noiseSuppressionChx.checkState = audioOptions.noiseSuppression()
                highpassFilterChx.checkState = audioOptions.highpassFilter()
                stereoSwappingChx.checkState = audioOptions.stereoSwapping()
                options = audioOptions
            }
        }

        RowLayout {
            CheckBox {
                id: videoFilterChx
                Layout.fillWidth: true
                text: qsTr("Video filter:")
                checked: false
                onCheckedChanged: notifyAboutAccepted()
            }
            ComboBox {
                id: videoFilterCombo
                enabled: videoFilterChx.checked
                model: app.availableFilters()
                currentIndex: -1
                Layout.fillWidth: true
                onCurrentIndexChanged: notifyAboutAccepted()
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
                notifyAboutAccepted(true)
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
            }
        }
    }
}
