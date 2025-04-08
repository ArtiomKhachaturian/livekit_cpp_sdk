import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {

    id: root
    property alias urlText: url.text
    property alias tokenText: token.text
    property var cameraDeviceInfo: undefined
    //property bool autoSubscribe: true
    //property bool adaptiveStream: true
    //property int iceTransportPolicy: 0 // all

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

                Button {
                    id: connect
                    Layout.fillWidth: true
                    text: qsTr("Connect")
                    enabled: url.text !== "" && token.text !== ""
                    onClicked: {
                        connectClicked()
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Options")
                    onClicked: {
                        connectClicked()
                    }
                }
            }
        }
    }

    onCameraDeviceInfoChanged: {
        if (cameraDeviceInfo === undefined) {
            cameraPreview.source = null
        }
        else if (visible) {
            cameraPreview.source = app.createCamera(cameraDeviceInfo)
        }
    }

    onVisibleChanged: {
        if (visible) {
            if (cameraDeviceInfo !== undefined) {
                cameraPreview.source = app.createCamera(cameraDeviceInfo)
            }
        }
        else {
            cameraPreview.source = null
        }
    }
}
