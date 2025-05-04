import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    width: 1200
    height: 800
    //visibility: Window.Maximized
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
                tokenText: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDY0MTM0MDAsImlzcyI6ImRldmtleSIsIm5iZiI6MTc0NjMyNzAwMCwic3ViIjoidXNlcjEiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0.NutkhLAXU4Brnq85omImDWO0jcE7uBL16R_iKhLCKJQ"
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
