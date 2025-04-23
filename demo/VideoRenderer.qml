import QtQuick
import QtMultimedia
import LiveKitClient 1.0

Item {
    id: root
    property VideoSource source: null
    property bool showSourceName: false
    property bool showDiagnostics: true

    VideoOutput {
        id: renderer
        anchors.fill: parent
        property VideoSource source: null
        clip: true
        VideoDiagnosticsView {
            id: fpsArea
            anchors.top: parent.top
            anchors.right: parent.right
            z: 1
            visible: false
        }
        TextPanel {
            id: sourceNameText
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            visible: root.showSourceName && source !== null && text !== ""
            text: source ? source.name : ""
            showBorder: false
            elide: Text.ElideMiddle
        }
    }

    onSourceChanged: {
        if (renderer.source !== null) {
            renderer.source.removeOutput(renderer.videoSink)
            renderer.source = null
        }
        if (source !== null) {
            source.addOutput(renderer.videoSink)
            fpsArea.fps = Qt.binding(function() {
                if (source !== null) {
                    return source.fps
                }
                return 0
            })
            fpsArea.frameSize = Qt.binding(function() {
                if (source !== null) {
                    return source.frameSize
                }
                return Qt.size(0, 0)
            })
            fpsArea.frameType = Qt.binding(function() {
                if (source !== null) {
                    return source.frameType
                }
                return ""
            })
            fpsArea.visible = Qt.binding(function() {
                if (source !== null && showDiagnostics) {
                    var size = source.frameSize
                    var fps = source.fps
                    return (size.width > 0 && size.height > 0) || fps > 0
                }
                return false
            })
            renderer.source = source
        }
        else {
            renderer.clearOutput()
        }
    }

    Connections {
        target: renderer.source
        function onActiveChanged() {
            if (!renderer.source.active) {
                renderer.clearOutput()
            }
        }
    }
}
