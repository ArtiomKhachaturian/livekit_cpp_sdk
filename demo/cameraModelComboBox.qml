import QtQuick
import QtQuick.Controls

ComboBox {
    model: app.camerasModel
    textRole: "display"
    indicator.visible: count > 1
    flat: count <= 1
    enabled: indicator.visible
    readonly property var deviceInfo: {
        return deviceInfoAt(currentIndex)
    }
    function deviceInfoAt(index) {
        return model.itemAt(index)
    }
}
