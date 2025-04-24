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
            anchors.fill: parent
            Label {
                text: screensPreview.count > 1 ? qsTr("Screens:") : qsTr("Primary screen:")
                Layout.alignment: Qt.AlignLeft
                visible: screensPreview.visible
            }

            PreviewGrid {
                id: screensPreview
                Layout.alignment: Qt.AlignLeft
                Layout.fillHeight: 0 === windowsPreview.count
                visible: count > 0
                objectName: "screens"
                enumerationMode: SharingsVideoModel.Screens
                interactive: false
                onCurrentIndexChanged: {
                    if (currentIndex !== -1) {
                        windowsPreview.currentIndex = -1
                    }
                }
                Component.onCompleted: {
                    if (isEmptyDeviceInfo() && count > 0) {
                        root.deviceInfo = model.deviceInfo(0)
                        currentIndex = 0
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
                visible: count > 0
                showDiagnostics: count < 3
                enumerationMode: SharingsVideoModel.Windows
                objectName: "windows"
                cellMargin: 5
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

    component PreviewGrid : Frame {
        property alias showDiagnostics: grid.showDiagnostics
        property alias enumerationMode: grid.enumerationMode
        property alias currentIndex: grid.currentIndex
        property alias interactive: grid.interactive
        property alias count: grid.count
        property alias cellMargin: grid.cellMargin
        GridView {
            id: grid
            keyNavigationWraps: true
            anchors.fill: parent
            cellWidth: root.cellWidth
            cellHeight: root.cellHeight
            implicitWidth: cellWidth - cellMargin
            implicitHeight: cellHeight - cellMargin
            property int enumerationMode: SharingsVideoModel.Inactive
            property bool showDiagnostics: true
            property bool showSourceName: true
            property int cellMargin: 0
            model: SharingsVideoModel {
                id: sourcesModel
                mode: grid.enumerationMode
                options: root.options
            }
            delegate: VideoRenderer {
                width: grid.cellWidth - grid.cellMargin
                height: grid.cellHeight - grid.cellMargin
                showDiagnostics: grid.showDiagnostics
                showSourceName: grid.showSourceName
                source: sourcesModel.sourceAt(index)
                property var deviceInfo: source ? source.deviceInfo : app.emptyDeviceInfo()
                readonly property bool isCurrentItem: grid.currentIndex === index
                Rectangle {
                    anchors.fill: parent
                    border.width: 2
                    border.color: parent.isCurrentItem ? parent.palette.highlight : parent.palette.mid
                    color: "transparent"
                    z: 1
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked:  grid.currentIndex = index
                }
            }
            // handle clicks on empty area within the grid.
            // this adds an element below the grid items but on the grid's flickable surface
            // (so it won't have mouse events stolen by the grid)
            flickableChildren: MouseArea {
                anchors.fill: parent
                onClicked: grid.currentIndex = -1
            }
            Component.onCompleted: {
                if (!isEmptyDeviceInfo()) {
                    for (var i = 0; i < count; ++i) {
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

            //Keys.onRightPressed: grid.selectedIndex = Math.min(grid.selectedIndex + 1, thumbnailGrid.elementsCount - 1)
            //Keys.onLeftPressed: grid.selectedIndex = Math.max(grid.selectedIndex - 1, 0)
            //Keys.onDownPressed: thumbnailGrid.selectedIndex = Math.min(thumbnailGrid.selectedIndex + 3, thumbnailGrid.count - 1)
            //Keys.onUpPressed: thumbnailGrid.selectedIndex = Math.max(thumbnailGrid.selectedIndex - 3, 0)
        }
    }

    Component.onCompleted: {
        deviceInfo = app.emptyDeviceInfo()
        options = app.emptyVideoOptions()
    }

    onVisibleChanged: {
        contentComponentLoader.active = visible
    }

    function isEmptyDeviceInfo() {
        return root.deviceInfo === app.emptyDeviceInfo()
    }
}
