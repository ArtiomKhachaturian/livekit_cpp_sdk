import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    visibility: Qt.WindowMaximized
    width: 1024
    height: 768
    visible: true
    title: qsTr("LiveKit client demo")

    property string lastUrl: "ws://localhost:7880"

    header: RowLayout {
        TabBar {
            id: tabBar
            currentIndex: clients.count - 1
            enabled: app.valid
            visible: count > 0
            Repeater {
                model: clients
                TabButton {
                    text: modelData
                    width: 100
                }
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("New client")
            enabled: app.valid
            onClicked: {
                addNewClient()
            }
        }
    }

    StackLayout {
        id: clientsView
        currentIndex: tabBar.currentIndex
        anchors.fill: parent
        Repeater {
            model: clients
            enabled: app.valid
            Client {
                objectName: modelData
                urlText: lastUrl
                tokenText: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDQwNDkzNzgsImlzcyI6ImRldmtleSIsIm5iZiI6MTc0Mzk2Mjk3OCwic3ViIjoidXNlcjEiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0.kc8Ob_d8V8NMNkWZNYGEoITeT11t9vEvag5bPhUxzJE"
                enabled: app.valid
                Component.onCompleted: {
                    closable = clients.usersCount > 1
                }
                onWantsToBeClosed: name => {
                    removeClient(name)
                }
                onError: (desc, details) => {
                    showErrorMessage(desc, details, objectName)
                }
            }
        }
    }

    footer: Frame {
        RowLayout {
            anchors.fill: parent
            AudioServiceControl {
                recording: true
                Layout.fillWidth: true
            }
            AudioServiceControl {
                recording: false
                Layout.fillWidth: true
            }
        }
    }

    ListModel {
        id: clients
        property int usersCount: 0
    }

    MessageDialog {
        id: errorMessageBox
        buttons: MessageDialog.Close
        title: qsTr("Fatal error")
    }

    Component.onCompleted: {
        addNewClient()
    }

    function addNewClient() {
        var clientId = "client #" + (++clients.usersCount)
        clients.append({text:clientId})
    }

    function removeClient(name) {
        for (var i = 0; i < clients.count; i++) {
            if (clients.get(i).text === name) {
                clients.remove(i)
                break
            }
        }
    }

    function showErrorMessage(message, details, clientId){
        errorMessageBox.text = message
        errorMessageBox.informativeText = details
        errorMessageBox.title = clientId
        errorMessageBox.open()
    }
}
