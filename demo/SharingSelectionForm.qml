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
        anchors.fill: parent
    }

    Component {
        id: contentComponent
        ColumnLayout {
            Label {
                text: qsTr("Screens:")
                Layout.alignment: Qt.AlignLeft
            }
            ElementsGrid {
                id: screensPreview
                autoLayout: false
                implicitHeight: cellHeight
                Layout.fillWidth: true
                model: SharingsVideoModel {
                    id: screensModel
                    enumerateScreens: true
                }
                delegate: VideoRenderer {
                    anchors.fill: parent
                    showSourceName: true
                    source: screensModel.sourceAt(index)
                }
                focus: true
            }
            Label {
                text: qsTr("Windows:")
                Layout.alignment: Qt.AlignLeft
            }
            ElementsGrid {
                id: windowsPreview
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: SharingsVideoModel {
                    id: windowsModel
                    enumerateScreens: false
                }
                delegate: VideoRenderer {
                    anchors.fill: parent
                    showSourceName: true
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
