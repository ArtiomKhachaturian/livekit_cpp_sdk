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
        z: 1
        onClicked: {
            sessionForm.disconnect()
            wantsToBeClosed(root.objectName)
        }
    }

    StackLayout {
        id: stackView
        anchors.fill: parent
        Item {
            ConnectForm {
                id: connection
                width: parent.width - 200
                anchors.centerIn: parent
                enabled: !sessionForm.connecting
                onConnectClicked: {
                    wantsToBeConnected(root.objectName)
                    sessionForm.connect(urlText, tokenText)
                }
            }
        }
        SessionForm {
            id: sessionForm
            Layout.fillHeight: true
            Layout.fillWidth: true
            onError: (desc, details) => {
                root.error(desc, details)
            }
            onStateChanged: {
                switch (state) {
                    case SessionWrapper.RtcConnecting:
                    case SessionWrapper.RtcConnected:
                    case SessionWrapper.RtcDisconnected:
                        stackView.currentIndex = 1
                        break
                    default:
                        stackView.currentIndex = 0
                        break
                }
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: sessionForm.connecting
    }

}
