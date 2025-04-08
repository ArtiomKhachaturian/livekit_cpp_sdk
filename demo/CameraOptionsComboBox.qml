import QtQuick
import QtQuick.Controls

ComboBox {
    textRole: "display"
    model: cameraOptionsModel

    property alias deviceInfo: cameraOptionsModel.deviceInfo
    readonly property var options: {
        return model.itemAt(currentIndex)
    }

    Component.onCompleted: {
        currentIndex = model.defaultOptionsIndex()
    }

    CameraOptionsModel {
        id: cameraOptionsModel
        onDeviceInfoChanged: {
            parent.currentIndex = defaultOptionsIndex()
        }
    }
}

