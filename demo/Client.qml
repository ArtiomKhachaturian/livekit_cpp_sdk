import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root
    property bool closable: true
    property alias urlText: connection.urlText
    property alias tokenText: connection.tokenText
    signal wantsToBeClosed(string name)
    signal wantsToBeConnected(string url, string token)

    ToolButton {
        visible: closable
        anchors.right: parent.right
        anchors.top: parent.top
        icon.name: "window-close"
        onClicked: {
            wantsToBeClosed(root.objectName)
        }
    }

    StackView {
        anchors.fill: parent
        ConnectForm {
            id: connection
            width: parent.width - 200
            anchors.centerIn: parent
            onConnectClicked: {
                wantsToBeConnected(connection.urlText, connection.tokenText)
            }
        }
        SessionForm {

        }
    }
}
