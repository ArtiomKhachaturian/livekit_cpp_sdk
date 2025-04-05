import QtQuick
import QtQuick.Controls
import QtQuick.Layouts 1.2

Frame {
    id: root
    signal textEntered(string text)
    ColumnLayout {
        anchors.fill: parent
        ScrollView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            TextArea {
                id: chatMessages
                anchors.fill: parent
                clip: true
                readOnly: true
                wrapMode: TextEdit.WordWrap
                textFormat: TextEdit.RichText
                background: Rectangle {
                    border.width: chatMessages.activeFocus ? 2 : 1
                    color: chatMessages.palette.base
                    border.color: chatMessages.activeFocus ? chatMessages.palette.highlight : chatMessages.palette.mid
                }
            }
        }
        TextField {
            id: userMessage
            Layout.fillWidth: true
            horizontalAlignment: TextField.AlignLeft
            verticalAlignment: TextField.AlignVCenter
            placeholderText: qsTr("Enter message")
            rightPadding: senderButton.implicitWidth
            ToolButton {
                id: senderButton
                text: ">"
                //icon.name: "mail-send"
                enabled: userMessage.text !== ""
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                onClicked: {
                    textEntered(userMessage.text)
                    userMessage.clear()
                }
            }
        }
    }
    function append(user, text) {
        chatMessages.append("<b>" + user + "</b>: " + text)
    }
}
