import QtQuick

Rectangle {
    property alias font: textArea.font
    property alias style : textArea.style
    property alias text: textArea.text
    property alias elide: textArea.elide
    property bool showBorder: true
    width: textArea.implicitWidth + 12
    height: textArea.implicitHeight + 6
    border.width: showBorder ? (parent.activeFocus ? 2 : 1) : 0
    color: "black"
    border.color: parent.activeFocus ? parent.palette.highlight : parent.palette.mid
    radius: 2
    opacity: 0.5
    Text {
        id: textArea
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignJustify
        color: "white"
        styleColor: "black"
        style: Text.Outline
    }
}
