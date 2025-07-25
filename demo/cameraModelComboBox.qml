import QtQuick
import QtQuick.Controls

ComboBox {
    model: app.camerasModel
    textRole: "display"
    enabled: count > 1
    readonly property var deviceInfo: {
        return deviceInfoAt(currentIndex)
    }
    function deviceInfoAt(index) {
        return model.itemAt(index)
    }
}
