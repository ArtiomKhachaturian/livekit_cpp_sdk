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

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            TabBar {
                id: tabBar
                currentIndex: clients.count - 1
                Layout.fillWidth: true
                enabled: app.valid
                Repeater {
                    model: clients
                    TabButton {
                        text: modelData
                        width: 100
                    }
                }
            }

            ToolButton {
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
            Layout.fillHeight: true

            Repeater {
                model: clients
                enabled: app.valid
                Client {
                    objectName: modelData
                    urlText: lastUrl
                    enabled: app.valid

                    Component.onCompleted: {
                        app.registerClient(objectName, true)
                        closable = clients.usersCount > 1
                    }
                    Component.onDestruction: {
                        app.registerClient(objectName, false)
                    }
                    onWantsToBeClosed: name => {
                        removeClient(name)
                    }
                }
            }
        }
    }

    ListModel {
        id: clients
        property int usersCount: 0
    }

    MessageDialog {
        id: errorMessageBox
        buttons: MessageDialog.Ok
    }

    Connections {
        target: app
        function onShowErrorMessage(message){
            errorMessageBox.text = message
            errorMessageBox.open()
        }
    }

    Component.onCompleted: {
        addNewClient()
    }

    function addNewClient() {
        ++clients.usersCount
        clients.append({text:"client #" + clients.usersCount})
    }

    function removeClient(name) {
        for (var i = 0; i < clients.count; i++) {
            if (clients.get(i).text === name) {
                clients.remove(i)
                break
            }
        }
    }
}
