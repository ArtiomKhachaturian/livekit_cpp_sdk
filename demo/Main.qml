import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    //color: "#232323"
    title: qsTr("Hello World")

    /*Client {
        anchors.fill: parent
    }*/
    ConnectForm {
        width: parent.width - 200
        anchors.centerIn: parent
        urlText: "ws://localhost:7880"
        onConnectClicked: {
            console.log("Connect!!!")
        }
    }
}
