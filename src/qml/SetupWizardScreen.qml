import QtQuick
import ProjectDex as Dex

// First-run (and reopenable any time via the toolbar's "Account Setup"
// button) profile screen: the 3 credentials the app needs -- Steam Web API
// Key, SteamID64, and an optional SteamGridDB API Key -- each entered via
// OnScreenKeyboard.qml so the whole thing is usable from a controller on
// the couch, no keyboard/mouse required. A QR code links to a page with
// step-by-step instructions + the official pages to get each one, so the
// player can look those up on their phone while typing the result back in
// on the TV -- see docs/setup-keys.md.
//
// Same full-window-overlay-with-dimmed-background shape as
// ControllerSetupScreen.qml, and the same "closing always hands focus back
// explicitly" contract (see this file's closed() signal and Main.qml's
// reclaimFocus()).
Item {
    id: root
    anchors.fill: parent
    visible: false
    z: 1000

    // GitHub renders a .md file straight from the repo, so no separate
    // hosting/Pages setup is needed -- docs/setup-keys.md is committed
    // there directly.
    readonly property string instructionsUrl: "https://github.com/LargeDexter/project_dex/blob/main/docs/setup-keys.md"

    readonly property var fieldDefs: [
        {
            key: "steamApiKey",
            label: "Steam Web API Key",
            hint: "Required to read your owned-games list.",
        },
        {
            key: "steamId",
            label: "SteamID64",
            hint: "Required -- your own 17-digit Steam ID.",
        },
        {
            key: "steamGridDbKey",
            label: "SteamGridDB API Key (optional)",
            hint: "Nicer box art -- leave blank to use Steam's own art instead.",
        },
    ]

    // [apiKey, steamId, gridDbKey], edited via the on-screen keyboard and
    // only actually persisted (via SettingsManager.save()) when Save is
    // pressed -- Skip/Cancel/Escape just drops this array on the floor,
    // leaving whatever was previously saved untouched.
    property var draftValues: ["", "", ""]

    // 0-2 select a field, 3 is the button row below the fields.
    property int cursor: 0
    property int buttonIndex: 0 // 0 = Save, 1 = Skip
    property int editingIndex: -1 // which field the keyboard overlay is currently editing, -1 = none
    property string saveError: ""

    signal closed()
    signal saved()

    function open() {
        draftValues = [Dex.SettingsManager.steamApiKey, Dex.SettingsManager.steamId, Dex.SettingsManager.steamGridDbKey]
        cursor = 0
        buttonIndex = 0
        saveError = ""
        visible = true
    }

    function close() {
        visible = false
        root.closed()
    }

    // Qt.callLater, not a direct call: this screen auto-opens at startup
    // (see Main.qml's Component.onCompleted) right as the main window is
    // doing its first fullscreen transition (now the default -- see
    // Main.qml's visibility binding). Calling forceActiveFocus() before
    // that transition settles doesn't stick -- focus silently falls back
    // to whatever had it before (the game grid), leaving the wizard
    // visible but not controller/keyboard-navigable. Deferring to the next
    // event-loop tick sidesteps the race. Same pattern ExpandedGameView.qml
    // already uses for this reason.
    onVisibleChanged: if (visible) Qt.callLater(forceActiveFocus)

    function validationFor(index) {
        if (index === 0)
            return Dex.SettingsManager.validateSteamApiKey(draftValues[0])
        if (index === 1)
            return Dex.SettingsManager.validateSteamId(draftValues[1])
        return Dex.SettingsManager.validateSteamGridDbKey(draftValues[2])
    }

    function openKeyboardFor(index) {
        root.editingIndex = index
        keyboard.open(root.draftValues[index], root.fieldDefs[index].label)
    }

    function doSave() {
        const error = Dex.SettingsManager.save(root.draftValues[0], root.draftValues[1], root.draftValues[2])
        if (error.length > 0) {
            root.saveError = error
            return
        }
        root.saved()
        root.close()
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape) {
            root.close()
            event.accepted = true
        } else if (event.key === Qt.Key_Up) {
            root.cursor = Math.max(0, root.cursor - 1)
            event.accepted = true
        } else if (event.key === Qt.Key_Down) {
            root.cursor = Math.min(3, root.cursor + 1)
            event.accepted = true
        } else if (event.key === Qt.Key_Left) {
            if (root.cursor === 3)
                root.buttonIndex = Math.max(0, root.buttonIndex - 1)
            event.accepted = true
        } else if (event.key === Qt.Key_Right) {
            if (root.cursor === 3)
                root.buttonIndex = Math.min(1, root.buttonIndex + 1)
            event.accepted = true
        } else if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            if (root.cursor < 3)
                root.openKeyboardFor(root.cursor)
            else if (root.buttonIndex === 0)
                root.doSave()
            else
                root.close()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.75
        MouseArea { anchors.fill: parent }
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: 780
        // Sized from actual content rather than a hand-guessed fixed
        // value -- a validation-error line under any field can appear or
        // disappear, and Rectangle doesn't clip overflowing children by
        // default, so a guess that's even a little short would silently
        // cut the bottom of the card off instead of visibly failing.
        height: wizardColumn.implicitHeight + Dex.Theme.spacingLarge * 2
        radius: Dex.Theme.radiusMedium
        color: Dex.Theme.surface
        border.color: Dex.Theme.accent
        border.width: 1

        Column {
            id: wizardColumn
            anchors.fill: parent
            anchors.margins: Dex.Theme.spacingLarge
            spacing: Dex.Theme.spacingMedium

            Text {
                text: "Account Setup"
                color: Dex.Theme.textPrimary
                font.pixelSize: Dex.Theme.fontSizeLarge
                font.bold: true
            }

            Text {
                width: card.width - Dex.Theme.spacingLarge * 2
                wrapMode: Text.WordWrap
                text: "Project Dex needs a Steam Web API key and your SteamID64 to read your library. "
                      + "Scan the QR code with your phone for step-by-step instructions, then type each "
                      + "result in below."
                color: Dex.Theme.textSecondary
                font.pixelSize: Dex.Theme.fontSizeSmall
            }

            // QR + instructions link, side by side -- falls back to just
            // the URL as selectable-looking text when the build has no
            // libqrencode (see SettingsManager.qrCodeAvailable).
            Row {
                spacing: Dex.Theme.spacingMedium

                Rectangle {
                    visible: Dex.SettingsManager.qrCodeAvailable
                    width: 96
                    height: 96
                    radius: Dex.Theme.radiusSmall
                    color: "white"

                    Image {
                        anchors.fill: parent
                        anchors.margins: 6
                        fillMode: Image.PreserveAspectFit
                        smooth: false // crisp block edges read better at small sizes than a blurred scale
                        source: "image://qr/" + encodeURIComponent(root.instructionsUrl)
                    }
                }

                Column {
                    width: card.width - Dex.Theme.spacingLarge * 2 - 96 - Dex.Theme.spacingMedium
                    spacing: Dex.Theme.spacingSmall
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: Dex.SettingsManager.qrCodeAvailable
                              ? "Scan for setup instructions:"
                              : "Setup instructions:"
                        color: Dex.Theme.textSecondary
                        font.pixelSize: Dex.Theme.fontSizeSmall
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.WrapAnywhere
                        text: root.instructionsUrl
                        color: Dex.Theme.accent
                        font.pixelSize: Dex.Theme.fontSizeSmall
                    }
                }
            }

            Item { width: 1; height: Dex.Theme.spacingSmall }

            // One row per field: label, current value, and an inline
            // valid/invalid indicator -- clicking or pressing Confirm on a
            // highlighted row opens the on-screen keyboard for it.
            Repeater {
                model: root.fieldDefs.length

                delegate: Rectangle {
                    id: fieldRow
                    property int fieldIndex: index
                    readonly property string validationError: root.validationFor(fieldIndex)
                    readonly property bool isFocused: root.cursor === fieldIndex

                    width: card.width - Dex.Theme.spacingLarge * 2
                    height: 74
                    radius: Dex.Theme.radiusSmall
                    color: Dex.Theme.surfaceHighlight
                    border.color: Dex.Theme.accentGlow
                    border.width: isFocused ? Dex.Theme.glowRingWidth : 0

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.cursor = fieldIndex
                            root.openKeyboardFor(fieldIndex)
                        }
                    }

                    Column {
                        anchors.left: parent.left
                        anchors.right: iconArea.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: Dex.Theme.spacingMedium
                        anchors.rightMargin: Dex.Theme.spacingSmall
                        spacing: 2

                        Text {
                            text: root.fieldDefs[fieldIndex].label
                            color: Dex.Theme.textSecondary
                            font.pixelSize: Dex.Theme.fontSizeSmall
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideMiddle
                            text: root.draftValues[fieldIndex].length > 0
                                  ? root.draftValues[fieldIndex]
                                  : ("Not set -- " + root.fieldDefs[fieldIndex].hint)
                            color: root.draftValues[fieldIndex].length > 0 ? Dex.Theme.textPrimary : Dex.Theme.textSecondary
                            font.pixelSize: Dex.Theme.fontSizeMedium
                            font.family: "monospace"
                        }
                        Text {
                            visible: validationError.length > 0
                            text: validationError
                            color: "#e05252"
                            font.pixelSize: Dex.Theme.fontSizeSmall
                        }
                    }

                    Item {
                        id: iconArea
                        anchors.right: parent.right
                        anchors.rightMargin: Dex.Theme.spacingMedium
                        anchors.verticalCenter: parent.verticalCenter
                        width: 20
                        height: 20

                        Icon {
                            anchors.fill: parent
                            visible: root.draftValues[fieldIndex].length > 0
                            kind: fieldRow.validationError.length === 0 ? "check" : "warn"
                            color: fieldRow.validationError.length === 0 ? "#4caf6a" : "#e0a952"
                        }
                    }
                }
            }

            Text {
                visible: root.saveError.length > 0
                text: root.saveError
                color: "#e05252"
                font.pixelSize: Dex.Theme.fontSizeSmall
            }

            Item { width: 1; height: Dex.Theme.spacingSmall }

            Row {
                spacing: Dex.Theme.spacingMedium

                Rectangle {
                    width: saveButtonText.width + Dex.Theme.spacingLarge
                    height: saveButtonText.height + Dex.Theme.spacingMedium
                    radius: Dex.Theme.radiusPill
                    color: Dex.Theme.accent
                    border.color: Dex.Theme.accentGlow
                    border.width: (root.cursor === 3 && root.buttonIndex === 0) ? Dex.Theme.glowRingWidth : 0

                    Text {
                        id: saveButtonText
                        anchors.centerIn: parent
                        text: "Save"
                        color: Dex.Theme.background
                        font.pixelSize: Dex.Theme.fontSizeMedium
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.cursor = 3
                            root.buttonIndex = 0
                            root.doSave()
                        }
                    }
                }

                Rectangle {
                    width: skipButtonText.width + Dex.Theme.spacingLarge
                    height: skipButtonText.height + Dex.Theme.spacingMedium
                    radius: Dex.Theme.radiusPill
                    color: "transparent"
                    border.color: (root.cursor === 3 && root.buttonIndex === 1) ? Dex.Theme.accentGlow : Dex.Theme.textSecondary
                    border.width: (root.cursor === 3 && root.buttonIndex === 1) ? Dex.Theme.glowRingWidth : 1

                    Text {
                        id: skipButtonText
                        anchors.centerIn: parent
                        text: "Skip for Now"
                        color: Dex.Theme.textPrimary
                        font.pixelSize: Dex.Theme.fontSizeMedium
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.cursor = 3
                            root.buttonIndex = 1
                            root.close()
                        }
                    }
                }
            }

            Text {
                text: "D-Pad/Stick to move • A/Enter to open • B/Esc to close"
                color: Dex.Theme.textSecondary
                font.pixelSize: Dex.Theme.fontSizeSmall
            }
        }
    }

    OnScreenKeyboard {
        id: keyboard

        onAccepted: {
            const updated = root.draftValues.slice()
            updated[root.editingIndex] = keyboard.text
            root.draftValues = updated
            root.editingIndex = -1
            keyboard.close()
            root.forceActiveFocus()
        }
        onCancelled: {
            root.editingIndex = -1
            keyboard.close()
            root.forceActiveFocus()
        }
    }
}
