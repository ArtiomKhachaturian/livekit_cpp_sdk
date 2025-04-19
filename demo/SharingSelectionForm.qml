import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LiveKitClient 1.0

Dialog {
    modal: true
    focus: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
    property var deviceInfo: undefined

    contentItem: Loader {
        id: contentComponentLoader
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
            }
            ElementsGrid {
                id: screensPreview
                autoLayout: noWindows
                implicitHeight: cellHeight
                Layout.fillWidth: true
                Layout.fillHeight: noWindows
                model: SharingsVideoModel {
                    id: screensModel
                    mode: SharingsVideoModel.Screens
                }
                delegate: VideoRenderer {
                    anchors.fill: parent
                    showSourceName: true
                    showDiagnostics: noWindows
                    source: screensModel.sourceAt(index)
                }
                //focus: true
            }
            Label {
                text: qsTr("Windows:")
                Layout.alignment: Qt.AlignLeft
                visible: windowsPreview.visible
            }
            ElementsGrid {
                id: windowsPreview
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: !noWindows
                model: SharingsVideoModel {
                    id: windowsModel
                    mode: SharingsVideoModel.Windows
                }
                delegate: VideoRenderer {
                    anchors.fill: parent
                    showSourceName: true
                    showDiagnostics: false
                    source: windowsModel.sourceAt(index)
                }
                onCellHeightChanged: {
                    screensPreview.cellHeight = cellHeight
                }
                onCellWidthChanged: {
                    screensPreview.cellWidth = cellWidth
                }
            }
        }
    }

    onVisibleChanged: {
        contentComponentLoader.active = visible
    }
}
