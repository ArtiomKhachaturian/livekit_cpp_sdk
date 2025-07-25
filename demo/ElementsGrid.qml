import QtQuick
import QtQuick.Controls

Item {
    id: root
    property alias model: repeater.model
    property Component delegate
    readonly property int cellWidth: width / (columns > 0 ? columns : 1)
    readonly property int cellHeight: height / (rows > 0 ? rows : 1)
    readonly property alias columns: grid.columns
    readonly property alias rows: grid.rows
    readonly property alias elementsCount: grid.elementsCount
    Grid {
        id: grid
        anchors.fill: parent
        horizontalItemAlignment: Grid.AlignHCenter
        spacing: 2
        columns: Math.ceil(Math.sqrt(elementsCount))
        rows: Math.ceil(elementsCount / columns)
        readonly property int elementsCount: {
            if (model !== null) {
                // if it's a pure QML model
                if (model.count !== undefined) {
                    return model.count
                }
                return model.rowCount()
            }
            return 0
        }
        property alias cellWidth: root.cellWidth
        property alias cellHeight: root.cellHeight
        Repeater {
            id: repeater
            delegate: Rectangle {
                clip: true
                width: grid.cellWidth
                height: grid.cellHeight
                border.width: 1
                color: root.palette.window.lighter(1.2)
                border.color: root.palette.mid
                property alias item: delegateLoader.item
                Loader {
                    id: delegateLoader
                    anchors.fill: parent
                    sourceComponent: root.delegate
                    onLoaded: {
                        if (delegateLoader.item) {
                            if ("modelData" in delegateLoader.item) {
                                delegateLoader.item.modelData = modelData
                            }
                            if ("index" in item) {
                                delegateLoader.item.index = index
                            }
                        }
                    }
                }
                onFocusChanged: {
                    delegateLoader.item.focus = focus
                }
            }
        }

        add: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutBounce
            }
        }

        move: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutBounce
            }
        }

        populate: Transition {
            NumberAnimation {
                properties: "x,y"
                from: 200
                duration: 100
                easing.type: Easing.OutBounce
            }
        }
    }

    function itemAt(index) {
        var item = repeater.itemAt(index)
        if (item) {
            return item.item
        }
        return null
    }
}
