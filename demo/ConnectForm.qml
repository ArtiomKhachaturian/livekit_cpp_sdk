import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    property alias urlText: url.text
    property alias tokenText: token.text
    property bool autoSubscribe: true
    property bool adaptiveStream: true
    property int iceTransportPolicy: 0 // all

    signal connectClicked
    ColumnLayout {
        id: elementsLayout
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width

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
