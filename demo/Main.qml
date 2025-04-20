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
                    text: model.text
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
                objectName: model.id
                urlText: lastUrl
                tokenText: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDUyMjUyODgsImlzcyI6ImRldmtleSIsIm5iZiI6MTc0NTEzODg4OCwic3ViIjoidXNlcjEiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0.871SmKfLxcd2n7k4Zb67VhwmQ6jXjwFg06728OgBFls"
                enabled: app.valid
                Component.onCompleted: {
                    closable = clients.usersCount > 1
                }
                onWantsToBeClosed: {
                    removeClient(objectName)
                }
                onError: (desc, details) => {
                    showErrorMessage(desc, details, objectName)
                }
                onIdentityChanged: {
                    updateClientIdentity(objectName, identity)
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
        var clientId = "User #" + (++clients.usersCount)
        clients.append({id:clientId, text:clientId})
    }

    function removeClient(id) {
        for (var i = 0; i < clients.count; i++) {
            if (clients.get(i).id === id) {
                clients.remove(i)
                break
            }
        }
    }

    function updateClientIdentity(id, identity) {
        for (var i = 0; i < clients.count; i++) {
            if (clients.get(i).id === id) {
                clients.setProperty(i, "text", identity)
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
