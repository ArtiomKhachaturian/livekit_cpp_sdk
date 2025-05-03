import QtQuick
import QtQuick.Layouts
import QtMultimedia
import LiveKitClient 1.0

Item {
    id: root
    property VideoSource source: null
    property bool showSourceName: false
    property bool showDiagnostics: true
    property bool clearOutputWhenInactive: true

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        VideoOutput {
            id: renderer
            Layout.fillWidth: true
            Layout.fillHeight: true
            property VideoSource source: null
            TextPanel {
                id: stats
                anchors.top: parent.top
                anchors.right: parent.right
                z: 1
                visible: root.showDiagnostics && text !== ""
            }
        }
        TextPanel {
            id: sourceNameText
            Layout.fillWidth: true
            clip: true
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
            stats.text = Qt.binding(function() {
                if (source !== null) {
                    return source.stats
                }
                return ""
            })
            renderer.source = source
        }
        else {
            app.clearVideoOutput(renderer)
        }
    }

    Connections {
        target: renderer.source
        function onActiveChanged() {
            if (clearOutputWhenInactive && !renderer.source.active) {
                app.clearVideoOutput(renderer)
            }
        }
        function onMutedChanged() {
            if (renderer.source.muted) {
                app.clearVideoOutput(renderer)
            }
        }
    }
}
