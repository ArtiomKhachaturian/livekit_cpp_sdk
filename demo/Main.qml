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
                    objectName: model.id
                    implicitWidth: 120
                    icon.width: 12
                    icon.height: 12
                    icon.source: {
                        switch (model.state) {
                            case 1:
                                return "qrc:/resources/images/yellowCircle128.png"
                            case 2:
                                return "qrc:/resources/images/greenCircle128.png"
                            default:
                                break
                        }
                        return "qrc:/resources/images/redCircle128.png"
                    }
                    indicator: ToolButton {
                        Image {
                            source: "qrc:/resources/images/close.png"
                            anchors.fill: parent
                            anchors.margins: 2
                        }
                        implicitHeight: parent.height - 4
                        implicitWidth: implicitHeight
                        anchors.right: parent.right
                        anchors.rightMargin: 2
                        anchors.verticalCenter: parent.verticalCenter
                        Component.onCompleted: {
                            visible = clients.usersCount > 1
                        }
                        onClicked: {
                            removeClient(parent.objectName)
                        }
                    }
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
                tokenText: {
                    if (objectName === "User #1") {
                        return "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDY2MTYzODQsImlzcyI6ImRldmtleSIsIm5iZiI6MTc0NjUyOTk4NCwic3ViIjoidXNlcjEiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0.-rK7_xrOwrXgeg-33NN6Zr1OXLbwO3ubRXcXPRamJSM"
                    }
                    return "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDY2MTYzODQsImlzcyI6ImRldmtleSIsIm5iZiI6MTc0NjUyOTk4NCwic3ViIjoidXNlcjIiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0._Q4huzOmE8je_pqK89m2MICffX1S9nye3JIyhs1dT1M"
                }
                //tokenText: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDY0MTM0MDAsImlzcyI6ImRldmtleSIsIm5iZiI6MTc0NjMyNzAwMCwic3ViIjoidXNlcjEiLCJ2aWRlbyI6eyJyb29tIjoibXktZmlyc3Qtcm9vbSIsInJvb21Kb2luIjp0cnVlfX0.NutkhLAXU4Brnq85omImDWO0jcE7uBL16R_iKhLCKJQ"
                enabled: app.valid
                Component.onCompleted: {
                    model.state = 0
                }
                Component.onDestruction: {
                    disconnect()
                }
                onError: (desc, details) => {
                    showErrorMessage(desc, details, objectName)
                }
                onIdentityChanged: {
                    updateClientIdentity(objectName, identity)
                }
                onSessionActiveChanged: {
                    if (sessionConnected) {
                        model.state = 2
                    }
                    else if (sessionConnecting) {
                        model.state = 1
                    }
                    else {
                        model.state = 0
                    }
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
        addNewClient()
    }

    function addNewClient() {
        var clientId = "User #" + (++clients.usersCount)
        clients.append({id:clientId, text:clientId, state:0})
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
