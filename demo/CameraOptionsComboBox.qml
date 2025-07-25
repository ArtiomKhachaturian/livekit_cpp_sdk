import QtQuick
import QtQuick.Controls
import LiveKitClient 1.0

ComboBox {
    textRole: "display"
    model: cameraOptionsModel

    property alias deviceInfo: cameraOptionsModel.deviceInfo
    readonly property var options: {
        return optionsAt(currentIndex)
    }

    function optionsAt(index) {
        return model.itemAt(index)
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

