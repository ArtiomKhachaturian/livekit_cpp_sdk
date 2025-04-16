import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {

    id: root
    property alias urlText: url.text
    property alias tokenText: token.text
    property bool activeCamera: false
    property var cameraDeviceInfo: undefined
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
            Layout.minimumHeight: 400
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.verticalStretchFactor: 2
            VideoRenderer {
                id: cameraPreview
                anchors.fill: parent
            }
        }

        Frame {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.verticalStretchFactor: 1
            ColumnLayout {
                id: elementsLayout
                anchors.fill: parent

                Label {
                    text: qsTr("Server URL")
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                }
                TextField {
                    id: url
                    Layout.fillWidth: true
                    horizontalAlignment: TextField.AlignHCenter
                    verticalAlignment: TextField.AlignVCenter
                }

                Label {
                    text: qsTr("Token")
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                }
                TextField {
                    id: token
                    Layout.fillWidth: true
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
                        id: e2eChx
                        text: qsTr("E2E security")
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

                Button {
                    id: connect
                    Layout.fillWidth: true
                    text: qsTr("Connect")
                    enabled: url.text !== "" && token.text !== ""
                    onClicked: {
                        connectClicked()
                    }
                }
            }
        }
    }

    Component.onDestruction: {
        enableCameraPreview(undefined)
    }

    onActiveCameraChanged: {
        if (activeCamera && visible) {
            enableCameraPreview(cameraDeviceInfo)
        }
        else {
            enableCameraPreview(undefined)
        }
    }

    onCameraDeviceInfoChanged: {
        if (activeCamera && visible) {
            enableCameraPreview(cameraDeviceInfo)
        }
    }

    onVisibleChanged: {
        if (activeCamera && visible) {
            enableCameraPreview(cameraDeviceInfo)
        }
        else {
            enableCameraPreview(undefined)
        }
    }

    function enableCameraPreview(deviceInfo) {
        if (deviceInfo === undefined) {
            app.destroyCamera(cameraPreview.source)
            cameraPreview.source = null
        }
        else {
            if (cameraPreview.source === null) {
                cameraPreview.source = app.createCamera(deviceInfo)
            }
            else {
                cameraPreview.source.setDeviceInfo(deviceInfo)
            }
        }
    }
}
