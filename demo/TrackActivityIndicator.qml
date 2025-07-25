import QtQuick // TrackActivityIndicator

Item {
    id: root
    property bool active: false
    // 0 - camera, 1 - mic, 2 - screen
    property int kind: 0
    width: height
    Image {
        id: background
        source: root.active ? "qrc:/resources/images/greenCircle128.png" : "qrc:/resources/images/yellowCircle128.png"
        anchors.fill: parent
    }
    Image {
        id: foreground
        source: {
            switch (root.kind) {
                case 0:
                    return "qrc:/resources/images/camera.png"
            case 1:
                return "qrc:/resources/images/mic.png"
            case 2:
                return "qrc:/resources/images/screen.png"
            }
            return ""
        }
        anchors.fill: parent
        anchors.margins: 2
        z: 1
    }
    OpacityAnimator {
        target: root
        from: 1
        to: 0
        duration: 1000
        running: root.active && root.visible
    }
    onVisibleChanged: {
        if (visible) {
            opacity = 1
        }
    }
}
