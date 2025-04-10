import QtQuick
import QtMultimedia
import LiveKitClient 1.0

Item {
    id: root
    property VideoSource source: null
    property bool muted: false

    VideoOutput {
        id: renderer
        anchors.fill: parent
        property VideoSource source: null
        // TODO: replace to https://doc.qt.io/qt-6/qml-qtmultimedia-videooutput.html#clearOutput-method in QT >= 6.9.x
        visible: !muted && source !== null && source.active
        VideoDiagnosticsView {
            id: fpsArea
            x: parent.contentRect.right - 4 - width
            y: parent.contentRect.top + 4
            visible: false
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
                if (!muted && source !== null) {
                    var size = source.frameSize
                    var fps = source.fps
                    return (size.width > 0 && size.height > 0) || fps > 0
                }
                return false
            })
            renderer.source = source
        }
    }
}
