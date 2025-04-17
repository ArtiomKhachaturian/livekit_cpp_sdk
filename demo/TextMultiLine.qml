import QtQuick
import QtQuick.Controls

TextArea {
    wrapMode: TextArea.WrapAnywhere
    background: Rectangle {
        border.width: parent.activeFocus ? 2 : 1
        color: parent.palette.base
        border.color: parent.activeFocus ? parent.palette.highlight : parent.palette.mid
    }
}
