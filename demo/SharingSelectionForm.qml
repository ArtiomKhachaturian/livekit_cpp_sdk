import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LiveKitClient 1.0

Pane {
    id: root
    property var options: undefined
    property var deviceInfo: undefined
    property var focusedCell: null
    signal accepted
    signal rejected

    contentItem: Loader {
        id: contentComponentLoader
        anchors.fill: parent
        sourceComponent: contentComponent
        active: false
        focus: true
    }

    Component {
        id: contentComponent
        ColumnLayout {
            readonly property bool noWindows: 0 == windowsPreview.elementsCount
            Label {
                text: qsTr("Screens:")
                Layout.alignment: Qt.AlignLeft
                visible: !noWindows && screensPreview.elementsCount > 1
            }

            PreviewGrid {
                id: screensPreview
                autoLayout: noWindows
                cellWidth: 300
                cellHeight: 200
                implicitHeight: cellHeight
                Layout.fillWidth: true
                Layout.fillHeight: noWindows
                visible: elementsCount > 0
                enumerationMode: SharingsVideoModel.Screens
                //showDiagnostics: noWindows
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
                visible: !noWindows
                enumerationMode: SharingsVideoModel.Windows
                showDiagnostics: elementsCount < 3
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

    component PreviewGrid : ElementsGrid {
        id: grid
        property int enumerationMode: SharingsVideoModel.Inactive
        property bool showDiagnostics: true
        property bool showSourceName: true
        property int selectedIndex: -1
        model: SharingsVideoModel {
            id: sourcesModel
            mode: grid.enumerationMode
            options: root.options
        }
        delegate: VideoRenderer {
            anchors.fill: parent
            showDiagnostics: grid.showDiagnostics
            showSourceName: grid.showSourceName
            source: sourcesModel.sourceAt(index)
            property int index
            property var deviceInfo: source ? source.deviceInfo : app.emptyDeviceInfo()
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.selectedIndex = index
                }
            }
            Rectangle {
                anchors.fill: parent
                visible: parent.focus
                border.width: 2
                border.color: parent.palette.highlight
                color: "transparent"
                z: 1
            }
            //Keys.onRightPressed: grid.selectedIndex = Math.min(grid.selectedIndex + 1, thumbnailGrid.elementsCount - 1)
            //Keys.onLeftPressed: grid.selectedIndex = Math.max(grid.selectedIndex - 1, 0)
            //Keys.onDownPressed: thumbnailGrid.selectedIndex = Math.min(thumbnailGrid.selectedIndex + 3, thumbnailGrid.count - 1)
            //Keys.onUpPressed: thumbnailGrid.selectedIndex = Math.max(thumbnailGrid.selectedIndex - 3, 0)
        }
        onSelectedIndexChanged: {
            setFocusedCell(itemAt(selectedIndex))
        }
    }

    function setFocusedCell(cell) {
        if (focusedCell !== cell) {
            if (focusedCell) {
                focusedCell.focus = false
            }
            focusedCell = cell
            if (focusedCell) {
                focusedCell.focus = true
                deviceInfo = focusedCell.deviceInfo
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
