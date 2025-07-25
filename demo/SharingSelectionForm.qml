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
        property var storedDeviceInfo: undefined
        onItemChanged: {
            if (item) {
                if (storedDeviceInfo.isEmpty()) {
                    selectDefault()
                }
                else {
                    root.deviceInfo = storedDeviceInfo
                }
            }
        }
        function selectDefault() {
            if (item) {
                if (item.screensPreviewItem.count > 0) {
                    item.screensPreviewItem.currentIndex = 0
                }
                else if (item.windowsPreviewItem.count > 0) {
                    item.windowsPreviewItem.currentIndex = 0
                }
            }
        }
    }

    Component {
        id: contentComponent
        ColumnLayout {
            anchors.fill: parent
            readonly property Item screensPreviewItem: screensPreview
            readonly property Item windowsPreviewItem: windowsPreview
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
                pairedView: windowsPreview
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
                pairedView: screensPreview
            }

            DialogButtonBox {
                Layout.fillWidth: true
                standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
                onAccepted: {
                    contentComponentLoader.storedDeviceInfo = root.deviceInfo
                    root.visible = false
                    root.accepted()
                }
                onRejected: {
                    root.visible = false
                    root.rejected()
                }
            }

            Timer {
                id: activator
                interval: 10
                repeat: false
                onTriggered: {
                    var index = lookupDeviceInfo(screensPreview.model, storedDeviceInfo)
                    if (-1 !== index) {
                        screensPreview.currentIndex = index
                        console.log("found screen: " + index)
                    }
                    else {
                        index = lookupDeviceInfo(windowsPreview.model, storedDeviceInfo)
                        if (-1 !== index) {
                            windowsPreview.currentIndex = index
                        }
                        else {
                            contentComponentLoader.selectDefault()
                        }
                    }
                }
            }

            Component.onCompleted: activator.start()
        }
    }

    component PreviewGrid : Frame {
        property alias showDiagnostics: grid.showDiagnostics
        property alias enumerationMode: grid.enumerationMode
        property alias currentIndex: grid.currentIndex
        property alias interactive: grid.interactive
        property alias count: grid.count
        property alias cellMargin: grid.cellMargin
        property alias pairedView: grid.pairedView
        readonly property alias model: grid.model
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
            property Item pairedView: null
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
                readonly property bool isCurrentItem: grid.currentIndex === index
                Rectangle {
                    anchors.fill: parent
                    border.width: 2
                    border.color: isCurrentItem ? parent.palette.highlight : parent.palette.mid
                    color: "transparent"
                    z: 1
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: grid.currentIndex = index
                }
            }
            // handle clicks on empty area within the grid.
            // this adds an element below the grid items but on the grid's flickable surface
            // (so it won't have mouse events stolen by the grid)
            flickableChildren: MouseArea {
                anchors.fill: parent
                onClicked: grid.currentIndex = -1
            }

            onCurrentIndexChanged: {
                focus = -1 !== currentIndex
                if (focus && pairedView && pairedView !== this) {
                    pairedView.currentIndex = -1
                    pairedView.focus = false
                }
                if (focus) {
                    root.deviceInfo = model.deviceInfo(currentIndex)
                }
                else if (-1 !== lookupDeviceInfo(model, root.deviceInfo)) {
                    root.deviceInfo = app.emptyDeviceInfo()
                }
            }

            //Keys.onRightPressed: grid.selectedIndex = Math.min(grid.selectedIndex + 1, thumbnailGrid.elementsCount - 1)
            //Keys.onLeftPressed: grid.selectedIndex = Math.max(grid.selectedIndex - 1, 0)
            //Keys.onDownPressed: thumbnailGrid.selectedIndex = Math.min(thumbnailGrid.selectedIndex + 3, thumbnailGrid.count - 1)
            //Keys.onUpPressed: thumbnailGrid.selectedIndex = Math.max(thumbnailGrid.selectedIndex - 3, 0)
        }
    }

    function lookupDeviceInfo(model, info) {
        if (model && !info.isEmpty()) {
            for (var i = 0; i < model.rowCount(); ++i) {
                if (info === model.deviceInfo(i)) {
                    return i;
                }
            }
        }
        return -1
    }

    Component.onCompleted: {
        contentComponentLoader.storedDeviceInfo = deviceInfo = app.emptyDeviceInfo()
        options = app.emptyVideoOptions()
    }

    onVisibleChanged: {
        contentComponentLoader.active = visible
    }
}
