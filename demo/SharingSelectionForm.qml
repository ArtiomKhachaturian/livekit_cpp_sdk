import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LiveKitClient 1.0

Pane {
    id: root
    property var options: undefined
    property var deviceInfo: undefined
    property int cellWidth: 300
    property int cellHeight: 200
    signal accepted
    signal rejected

    contentItem: Loader {
        id: contentComponentLoader
        anchors.fill: parent
        sourceComponent: contentComponent
        active: false
    }

    Component {
        id: contentComponent
        ColumnLayout {

            Label {
                text: screensPreview.count > 1 ? qsTr("Screens:") : qsTr("Primary screen:")
                Layout.alignment: Qt.AlignLeft
                visible: screensPreview.count > 0
            }

            PreviewGrid {
                id: screensPreview
                implicitHeight: cellHeight
                Layout.fillWidth: true
                Layout.fillHeight: 0 === windowsPreview.count
                visible: count > 0
                objectName: "screens"
                enumerationMode: SharingsVideoModel.Screens
                focus: true
                onCurrentIndexChanged: {
                    if (currentIndex !== -1) {
                        windowsPreview.currentIndex = -1
                    }
                }
            }

            Label {
                text: qsTr("Windows:")
                Layout.alignment: Qt.AlignLeft
                visible: windowsPreview.visible
            }

            PreviewGrid {
                id: windowsPreview
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                //cellWidth: 300
                //cellHeight: 200
                visible: count > 0
                showDiagnostics: count < 3
                enumerationMode: SharingsVideoModel.Windows
                objectName: "windows"
                onCurrentIndexChanged: {
                    if (currentIndex !== -1) {
                        screensPreview.currentIndex = -1
                    }
                }
            }

            DialogButtonBox {
                Layout.fillWidth: true
                standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
                onAccepted: {
                    root.visible = false
                    root.accepted()
                }
                onRejected: {
                    root.visible = false
                    root.rejected()
                }
            }
        }
    }

    component PreviewGrid : GridView {
        id: grid
        keyNavigationWraps: true
        cellWidth: root.cellWidth
        cellHeight: root.cellHeight
        property int enumerationMode: SharingsVideoModel.Inactive
        property bool showDiagnostics: true
        property bool showSourceName: true
        model: SharingsVideoModel {
            id: sourcesModel
            mode: grid.enumerationMode
            options: root.options
        }
        delegate: VideoRenderer {
            width: grid.cellWidth - 10
            height: grid.cellHeight - 10
            showDiagnostics: grid.showDiagnostics
            showSourceName: grid.showSourceName
            source: sourcesModel.sourceAt(index)
            property var deviceInfo: source ? source.deviceInfo : app.emptyDeviceInfo()
            Rectangle {
                anchors.fill: parent
                border.width: 2
                border.color: grid.currentIndex === index ? parent.palette.highlight : parent.palette.mid
                color: "transparent"
                z: 1
            }
            MouseArea {
                anchors.fill: parent
                onClicked:  grid.currentIndex = index
            }
            //Keys.onRightPressed: grid.selectedIndex = Math.min(grid.selectedIndex + 1, thumbnailGrid.elementsCount - 1)
            //Keys.onLeftPressed: grid.selectedIndex = Math.max(grid.selectedIndex - 1, 0)
            //Keys.onDownPressed: thumbnailGrid.selectedIndex = Math.min(thumbnailGrid.selectedIndex + 3, thumbnailGrid.count - 1)
            //Keys.onUpPressed: thumbnailGrid.selectedIndex = Math.max(thumbnailGrid.selectedIndex - 3, 0)
        }
        // handle clicks on empty area within the grid.
        // this adds an element below the grid items but on the grid's flickable surface
        // (so it won't have mouse events stolen by the grid)
        flickableChildren: MouseArea {
            anchors.fill: parent
            onClicked: grid.currentIndex = -1
        }
        Component.onCompleted: {
            if (root.deviceInfo !== undefined) {
                for (var i = 0; i < model.rowCount(); ++i) {
                    if (root.deviceInfo === model.deviceInfo(i)) {
                        currentIndex = i
                        return
                    }
                }
            }
            currentIndex = -1
        }

        onCurrentIndexChanged: {
            focus = -1 !== currentIndex
            if (focus) {
                var item = grid.itemAtIndex(currentIndex)
                if (item !== null) {
                    deviceInfo = item.deviceInfo
                }
            }
        }
    }

    Component.onCompleted: {
        deviceInfo = app.emptyDeviceInfo()
        options = app.emptyVideoOptions()
    }

    onVisibleChanged: {
        contentComponentLoader.active = visible
    }
}
