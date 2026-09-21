import QtQuick
import QtQuick.Controls
import ProjectDex as Dex

// Guided controller-remapping wizard: walks through one logical action at
// a time (Move Up, Confirm, Page Forward, etc.) and captures whatever
// button/stick/trigger the player presses for it, via
// GamepadInput.startCalibration()/calibrationCaptured(). See
// gamepad_input.h for why remapping exists at all (some controllers' SDL
// mapping doesn't line up with their physical labels).
//
// A full-window overlay rather than a separate page -- it needs to sit on
// top of the browsing screen and dim it, and closing it should always
// just drop back to exactly where the player was.
Item {
    id: root
    anchors.fill: parent
    visible: false
    z: 1000

    // Imperative rather than `focus: visible` -- a plain binding on focus
    // can get silently clobbered the moment active focus moves elsewhere
    // for any other reason (see GameRow.qml's onIsActiveRowChanged for the
    // same pattern), so this re-asserts it every time the wizard opens.
    onVisibleChanged: if (visible) forceActiveFocus()

    // Every action GamepadInput knows how to remap, walked through in
    // order. Captured once up front rather than re-queried every step --
    // it's a fixed list, and re-fetching it on every binding change would
    // be pointless churn.
    property var actions: []
    property int stepIndex: 0
    readonly property string currentAction: (stepIndex >= 0 && stepIndex < actions.length) ? actions[stepIndex] : ""

    // Set right after a capture lands, so the step briefly shows what got
    // bound before auto-advancing -- without this the wizard would jump
    // to the next step so fast the player couldn't tell what happened.
    property string justCapturedLabel: ""

    signal closed()

    function open() {
        actions = Dex.GamepadInput.calibrationActions()
        stepIndex = 0
        justCapturedLabel = ""
        visible = true
        beginStep()
    }

    function close() {
        Dex.GamepadInput.cancelCalibration()
        visible = false
        root.closed()
    }

    function beginStep() {
        justCapturedLabel = ""
        if (currentAction.length > 0)
            Dex.GamepadInput.startCalibration(currentAction)
    }

    function advance() {
        if (stepIndex < actions.length - 1) {
            stepIndex += 1
            beginStep()
        } else {
            close()
        }
    }

    function skip() {
        Dex.GamepadInput.cancelCalibration()
        advance()
    }

    function goBack() {
        if (stepIndex > 0) {
            Dex.GamepadInput.cancelCalibration()
            stepIndex -= 1
            beginStep()
        }
    }

    Connections {
        target: Dex.GamepadInput
        function onCalibrationCaptured(action, label) {
            if (action !== root.currentAction)
                return
            root.justCapturedLabel = label
            advanceTimer.restart()
        }
    }

    Timer {
        id: advanceTimer
        interval: 700
        onTriggered: root.advance()
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape) {
            root.close()
            event.accepted = true
        }
    }

    // Dimmed background -- also eats clicks so nothing behind the wizard
    // is reachable while it's open.
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.75
        MouseArea { anchors.fill: parent }
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: 620
        height: 400
        radius: Dex.Theme.radiusMedium
        color: Dex.Theme.surface
        border.color: Dex.Theme.accent
        border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: Dex.Theme.spacingLarge
            spacing: Dex.Theme.spacingMedium

            Text {
                text: "Controller Setup"
                color: Dex.Theme.textPrimary
                font.pixelSize: Dex.Theme.fontSizeLarge
                font.bold: true
            }

            Text {
                text: root.actions.length > 0
                      ? ("Step " + (root.stepIndex + 1) + " of " + root.actions.length)
                      : ""
                color: Dex.Theme.textSecondary
                font.pixelSize: Dex.Theme.fontSizeSmall
            }

            Item { width: 1; height: Dex.Theme.spacingSmall }

            Text {
                text: Dex.GamepadInput.actionLabel(root.currentAction)
                color: Dex.Theme.textPrimary
                font.pixelSize: Dex.Theme.fontSizeLarge
                font.bold: true
            }

            Text {
                width: card.width - Dex.Theme.spacingLarge * 2
                wrapMode: Text.WordWrap
                text: root.justCapturedLabel.length > 0
                      ? ("Captured: " + root.justCapturedLabel)
                      : (Dex.GamepadInput.connected
                         ? "Press the button, stick, or trigger you want for this now..."
                         : "No controller detected -- plug one in to calibrate.")
                color: root.justCapturedLabel.length > 0 ? Dex.Theme.accent : Dex.Theme.textSecondary
                font.pixelSize: Dex.Theme.fontSizeMedium
            }

            Text {
                text: "Currently: " + Dex.GamepadInput.currentBindingLabel(root.currentAction)
                color: Dex.Theme.textSecondary
                font.pixelSize: Dex.Theme.fontSizeSmall
            }

            Item { width: 1; height: Dex.Theme.spacingMedium }

            // Flow instead of Row -- wraps onto a second line rather than
            // squeezing four buttons (one with fairly long text) into a
            // fixed width. Each button gets real padding instead of the
            // bare-minimum default, so it doesn't look cramped either.
            Flow {
                width: card.width - Dex.Theme.spacingLarge * 2
                spacing: Dex.Theme.spacingMedium

                Button {
                    text: root.stepIndex > 0 ? "Back" : "Cancel"
                    padding: Dex.Theme.spacingMedium
                    onClicked: root.stepIndex > 0 ? root.goBack() : root.close()
                }

                Button {
                    text: "Skip"
                    padding: Dex.Theme.spacingMedium
                    onClicked: root.skip()
                }

                Button {
                    text: "Reset All to Defaults"
                    padding: Dex.Theme.spacingMedium
                    onClicked: {
                        Dex.GamepadInput.resetAllBindingsToDefault()
                        root.justCapturedLabel = ""
                    }
                }

                Button {
                    text: "Done"
                    padding: Dex.Theme.spacingMedium
                    onClicked: root.close()
                }
            }
        }
    }
}
