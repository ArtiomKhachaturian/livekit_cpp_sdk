import QtQuick

TextPanel {
    property int fps: 0
    property size frameSize: Qt.size(0, 0)
    readonly property string frameSizeText: {
        if (frameSize.width >= 0 && frameSize.height >=0) {
            return frameSize.width + "x" + frameSize.height
        }
        return ""
    }
    readonly property string fpsText : qsTr("%3 fps").arg(fps)
    text: {
        var diagnostics = frameSizeText
        if (diagnostics !== "") {
            diagnostics += " - " + fpsText
        }
        return diagnostics
    }
}
