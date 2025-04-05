import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    property bool closable: true
    property alias urlText: connection.urlText
    property alias tokenText: connection.tokenText

    signal wantsToBeClosed(string name)
    signal wantsToBeConnected(string name)
    signal error(string desc, string details)

    ToolButton {
        visible: closable
        anchors.right: parent.right
        anchors.top: parent.top
        icon.name: "window-close"
        onClicked: {
            sessionForm.disconnect()
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
                wantsToBeConnected(root.objectName)
                sessionForm.connect(urlText, tokenText)
            }
        }
        SessionForm {
            id: sessionForm
            onError: (desc, details) => {
                root.error(desc, details)
            }
        }
    }

    BusyIndicator {
        anchors.fill: parent
        running: sessionForm.connecting
    }

}
