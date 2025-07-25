import QtQuick
import QtQuick.Controls
import LiveKitClient 1.0

Menu {
    id: root
    property VideoTrack source: null
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
    //popupType: Popup.Native
    Menu {
        title: qsTr("Content hint")
        enabled: source !== null
        ActionGroup { id: contentHintGroup }
        ContentHintAction { hint: VideoTrack.None; ActionGroup.group: contentHintGroup }
        ContentHintAction {
            hint: VideoTrack.Motion
            enabled: isCamera()
            ActionGroup.group: contentHintGroup
        }
        ContentHintAction {
            hint: VideoTrack.Detailed
            enabled: isScreencast()
            ActionGroup.group: contentHintGroup
        }
        ContentHintAction {
            hint: VideoTrack.Text
            enabled: isScreencast()
            ActionGroup.group: contentHintGroup
        }
    }
    Menu {
        title: qsTr("Network priority")
        ActionGroup { id: networkPriorityGroup }
        enabled: isLocalSource()
        NetworkPriorityAction{ priority: VideoTrack.VeryLow; ActionGroup.group: networkPriorityGroup }
        NetworkPriorityAction{ priority: VideoTrack.Low; ActionGroup.group: networkPriorityGroup }
        NetworkPriorityAction{ priority: VideoTrack.Medium; ActionGroup.group: networkPriorityGroup }
        NetworkPriorityAction{ priority: VideoTrack.High; ActionGroup.group: networkPriorityGroup }
    }
    Menu {
        title: qsTr("Degradation preference")
        enabled: isLocalSource()
        ActionGroup { id: degradationGroup }
        DegradationPreferenceAction { preference: VideoTrack.Default; ActionGroup.group: degradationGroup }
        DegradationPreferenceAction { preference: VideoTrack.Disabled; ActionGroup.group: degradationGroup }
        DegradationPreferenceAction { preference: VideoTrack.MaintainFramerate; ActionGroup.group: degradationGroup }
        DegradationPreferenceAction { preference: VideoTrack.MaintainResolution; ActionGroup.group: degradationGroup }
        DegradationPreferenceAction { preference: VideoTrack.Balanced; ActionGroup.group: degradationGroup }
    }

    Menu {
        title: qsTr("Scalability")
        enabled: isLocalSource()
        ActionGroup { id: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.Auto; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L1T1; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L1T2; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L1T3; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L2T1; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L2T2; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L2T3; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T1; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T2; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T3; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L2T1h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L2T2h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L2T3h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T1h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T2h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T3h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S2T1; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S2T2; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S2T3; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S2T1h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S2T2h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S2T3h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S3T1; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S3T2; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S3T3; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S3T1h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S3T2h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.S3T3h; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L2T2Key; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L2T2KeyShift; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L2T3Key; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L2T3KeyShift; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T1Key; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T2Key; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T2KeyShift; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T3Key; ActionGroup.group: scalabilityGroup }
        ScalabilityModeAction { mode: VideoTrack.L3T3KeyShift; ActionGroup.group: scalabilityGroup }
    }

    component ContentHintAction : Action {
        property int hint: VideoTrack.None
        text: contentHintName(hint)
        checkable: true
        checked: hasContentHint(hint)
        onCheckedChanged: (checked) => {
            if (checked) {
                setContentHint(hint)
            }
       }
    }

    component NetworkPriorityAction : Action {
        property int priority: VideoTrack.VeryLow
        text: networkPriorityName(priority)
        checkable: true
        checked: hasNetworkPriority(priority)
        onCheckedChanged: (checked) => {
            if (checked) {
                setNetworkPriority(priority)
            }
       }
    }

    component DegradationPreferenceAction : Action {
        property int preference: VideoTrack.Default
        text: degradationPreferenceName(preference)
        checkable: true
        checked: hasDegradationPreference(preference)
        onCheckedChanged: (checked) => {
            if (checked) {
                setDegradationPreference(preference)
            }
       }
    }

    component ScalabilityModeAction : Action {
        property int mode: VideoTrack.Auto
        text: scalabilityModeName(mode)
        checkable: true
        checked: hasScalabilityMode(mode)
        onCheckedChanged: (checked) => {
            if (checked) {
                setScalabilityMode((mode))
            }
       }
    }

    function contentHintName(contentHint) {
        switch (contentHint) {
            case VideoTrack.None:
                return qsTr("None")
            case VideoTrack.Motion:
                return qsTr("Motion")
            case VideoTrack.Detailed:
                return qsTr("Detailed")
            case VideoTrack.Text:
                return qsTr("Text")
        }
        return ""
    }

    function networkPriorityName(priority) {
        switch (priority) {
            case VideoTrack.VeryLow:
                return qsTr("Very low")
            case VideoTrack.Low:
                return qsTr("Low")
            case VideoTrack.Medium:
                return qsTr("Medium")
            case VideoTrack.Text:
                return qsTr("High")
        }
        return ""
    }

    function degradationPreferenceName(preference) {
        switch (preference) {
            case VideoTrack.Default:
                return qsTr("Default")
            case VideoTrack.Disabled:
                return qsTr("Disabled")
            case VideoTrack.MaintainFramerate:
                return qsTr("Maintain framerate")
            case VideoTrack.MaintainResolution:
                return qsTr("Maintain resolution")
            case VideoTrack.Balanced:
                return qsTr("Balanced")
        }
        return ""
    }

    function scalabilityModeName(mode) {
        switch (mode) {
            case VideoTrack.Auto:
                return qsTr("Auto")
            case VideoTrack.L1T1:
                return qsTr("L1T1 - 1 spatial && 1 temporal")
            case VideoTrack.L1T2:
                return qsTr("L1T2 - 1 spatial && 2 temporals")
            case VideoTrack.L1T3:
                return qsTr("L1T3 - 1 spatial && 3 temporals")
            case VideoTrack.L2T1:
                return qsTr("L2T1 - 2 spatials && 1 temporal")
            case VideoTrack.L2T2:
                return qsTr("L2T2 - 2 spatials && 2 temporals")
            case VideoTrack.L2T3:
                return qsTr("L2T3 - 2 spatials && 3 temporals")
            case VideoTrack.L3T1:
                return qsTr("L3T1 - 3 spatials && 1 temporal")
            case VideoTrack.L3T2:
                return qsTr("L3T2 - 3 spatials && 2 temporals")
            case VideoTrack.L3T3:
                return qsTr("L3T3 - 3 spatials && 3 temporals")
            case VideoTrack.L2T1h:
                return qsTr("L2T1h - 2 spatials && 1 temporal, resolution ratio 1.5:1")
            case VideoTrack.L2T2h:
                return qsTr("L2T2h - 2 spatials && 2 temporals, resolution ratio 1.5:1")
            case VideoTrack.L2T3h:
                return qsTr("L2T3h - 2 spatials && 3 temporals, resolution ratio 1.5:1")
            case VideoTrack.L3T1h:
                return qsTr("L3T1h - 3 spatials && 1 temporal, resolution ratio 1.5:1")
            case VideoTrack.L3T2h:
                return qsTr("L3T2h - 3 spatials && 2 temporals, resolution ratio 1.5:1")
            case VideoTrack.L3T3h:
                return qsTr("L3T3h - 3 spatials && 3 temporals, resolution ratio 1.5:1")
            case VideoTrack.S2T1:
                return qsTr("S2T1 - 2 spatials && 1 temporal, no inter-layer dependency")
            case VideoTrack.S2T2:
                return qsTr("S2T2 - 2 spatials && 2 temporals, no inter-layer dependency")
            case VideoTrack.S2T3:
                return qsTr("S2T3 - 2 spatials && 3 temporals, no inter-layer dependency")
            case VideoTrack.S2T1h:
                return qsTr("S2T1h - 2 spatials && 1 temporal, resolution ratio 1.5:1, no inter-layer dependency")
            case VideoTrack.S2T2h:
                return qsTr("S2T2h - 2 spatials && 2 temporals, resolution ratio 1.5:1, no inter-layer dependency")
            case VideoTrack.S2T3h:
                return qsTr("S2T3h - 2 spatials && 3 temporals, resolution ratio 1.5:1, no inter-layer dependency")
            case VideoTrack.S3T1:
                return qsTr("S3T1 - 3 spatials && 1 temporal, no inter-layer dependency")
            case VideoTrack.S3T2:
                return qsTr("S3T2 - 3 spatials && 2 temporals, no inter-layer dependency")
            case VideoTrack.S3T3:
                return qsTr("S3T3 - 3 spatials && 3 temporals, no inter-layer dependency")
            case VideoTrack.S3T1h:
                return qsTr("S3T1h - 3 spatials && 1 temporal, resolution ratio 1.5:1, no inter-layer dependency")
            case VideoTrack.S3T2h:
                return qsTr("S3T2h - 3 spatials && 2 temporals, resolution ratio 1.5:1, no inter-layer dependency")
            case VideoTrack.S3T3h:
                return qsTr("S3T3h - 3 spatials && 3 temporals, resolution ratio 1.5:1, no inter-layer dependency")
            case VideoTrack.L2T2Key:
                return qsTr("L2T2Key - 2 spatials && 2 temporals")
            case VideoTrack.L2T2KeyShift:
                return qsTr("L2T2KeyShift - 2 spatials && 2 temporals")
            case VideoTrack.L2T3Key:
                return qsTr("L2T3Key - 2 spatials && 3 temporals")
            case VideoTrack.L2T3KeyShift:
                return qsTr("L2T3KeyShift - 2 spatials && 3 temporals")
            case VideoTrack.L3T1Key:
                return qsTr("L3T1Key - 3 spatials && 1 temporal")
            case VideoTrack.L3T2Key:
                return qsTr("L3T2Key - 3 spatials && 2 temporals")
            case VideoTrack.L3T2KeyShift:
                return qsTr("L3T2KeyShift - 3 spatials && 2 temporals")
            case VideoTrack.L3T3Key:
                return qsTr("L3T3Key - 3 spatials && 3 temporals")
            case VideoTrack.L3T3KeyShift:
                return qsTr("L3T3KeyShift - 3 spatials && 3 temporals")
        }
        return ""
    }

    function hasContentHint(contentHint) {
        return root.source && source.contentHint === contentHint
    }

    function setContentHint(contentHint) {
        if (root.source) {
            root.source.contentHint = contentHint
        }
    }

    function hasNetworkPriority(priority) {
        return root.source && source.networkPriority === priority
    }

    function setNetworkPriority(priority) {
        if (root.source) {
            root.source.networkPriority = priority
        }
    }

    function hasDegradationPreference(preference) {
        return root.source && source.degradationPreference === preference
    }

    function setDegradationPreference(preference) {
        if (root.source) {
            root.source.degradationPreference = preference
        }
    }

    function hasScalabilityMode(mode) {
        return root.source && source.scalabilityMode === mode
    }

    function setScalabilityMode(mode) {
        if (root.source) {
            root.source.scalabilityMode = mode
        }
    }

    function isLocalSource() { return root.source && !root.source.remote }
    function isScreencast() { return root.source && root.source.screencast }
    function isCamera() { return root.source && !root.source.screencast }
}
