import QtQuick

Rectangle {
    id: root
    property int fps: 0
    property size frameSize: Qt.size(0, 0)
    readonly property string frameSizeText: {
        if (frameSize.width >= 0 && frameSize.height >=0) {
            return frameSize.width + "x" + frameSize.height
        }
        return ""
    }
    readonly property string fpsText : qsTr("%3 fps").arg(fps)
    width: fpsText.implicitWidth + 12
    height: fpsText.implicitHeight + 6
    border.width: parent.activeFocus ? 2 : 1
    color: parent.palette.base
    border.color: parent.activeFocus ? parent.palette.highlight : parent.palette.mid
    radius: 2
    opacity: 0.5
    Text {
        id: fpsText
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignJustify
        color: "white"
        styleColor: "black"
        style: Text.Outline
        text: {
            var diagnostics = root.frameSizeText
            if (diagnostics !== "") {
                diagnostics += " - " + root.fpsText
            }
            return diagnostics
        }
    }
}
