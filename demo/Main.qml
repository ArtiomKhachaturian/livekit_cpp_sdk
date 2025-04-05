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
                enabled: app.valid

                Component.onCompleted: {
                    closable = clients.usersCount > 1
                }
                Component.onDestruction: {
                    app.unregisterClient(objectName)
                }
                onWantsToBeClosed: name => {
                    removeClient(name)
                }
                onWantsToBeConnected: (url, token) => {
                    app.connect(objectName, url, token)
                }
            }
        }
    }

    footer: Frame {
        RowLayout {
            anchors.fill: parent
            GroupBox {
                title: qsTr("Microphone")
                Layout.fillWidth: true
                RowLayout {
                    anchors.fill: parent
                    ToolButton {
                        icon.name: "audio-input-microphone"
                        checkable: true
                        checked: app.audioRecordingEnabled
                        display: ToolButton.IconOnly
                        onCheckedChanged: {
                            app.audioRecordingEnabled = checked
                        }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: app.recordingAudioDevicesModel
                        textRole: "display"
                        onActivated: index => {
                            app.recordingAudioDevice = model.infoAt(index)
                        }
                        Component.onCompleted: {
                            currentIndex = model.indexOf(app.recordingAudioDevice)
                        }
                    }
                    Label {
                        text: qsTr("Volume:")
                    }
                    Slider {
                        from: 0
                        to: 100
                        value: app.audioRecordingVolume
                        onValueChanged: {
                            app.audioRecordingVolume = value
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Speakers")
                Layout.fillWidth: true
                RowLayout {
                    anchors.fill: parent
                    ToolButton {
                        icon.name: "audio-card"
                        checkable: true
                        checked: app.audioPlayoutEnabled
                        display: ToolButton.IconOnly
                        onCheckedChanged: {
                            app.audioPlayoutEnabled = checked
                        }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: app.playoutAudioDevicesModel
                        textRole: "display"
                        onActivated: index => {
                            app.playoutAudioDevice = model.infoAt(index)
                        }
                        Component.onCompleted: {
                            currentIndex = model.indexOf(app.playoutAudioDevice)
                        }
                    }
                    Label {
                        text: qsTr("Volume:")
                    }
                    Slider {
                        from: 0
                        to: 100
                        value: app.audioPlayoutVolume
                        onValueChanged: {
                            app.audioPlayoutVolume = value
                        }
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
        buttons: MessageDialog.Close
        title: qsTr("Fatal error")
    }

    Connections {
        target: app
        function onShowErrorMessage(message, details, clientId){
            errorMessageBox.text = message
            errorMessageBox.informativeText = details
            errorMessageBox.title = clientId
            errorMessageBox.open()
        }
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
}
