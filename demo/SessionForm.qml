import QtQuick

Item {
    id: root

    property SessionWrapper session: null
    readonly property bool connecting: session != null && session.connecting

    signal error(string desc, string details)

    function connect(url, token) {
        session = app.createSession(this)
        if (session != null) {
            session.connectToSfu(url, token)
        }
        else {
            error(qsTr("Failed to create session"), "")
        }
    }

    function disconnect() {
        if (session != null) {
            session.disconnectFromSfu()
            session = null
        }
    }

    Connections {
        target: session
        function onError(desc, details) {
            root.error(desc, details)
        }
    }
}
