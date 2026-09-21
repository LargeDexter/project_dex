import QtQuick
import ProjectDex as Dex

// Single home for everything settings-related, opened from the toolbar's
// gear icon: refreshing the library, the controller calibration wizard,
// the account/API-key wizard, and a quick read-only, masked view of the
// three saved credentials (with a per-field reveal toggle) so it's
// possible to sanity-check what's actually saved without having to walk
// through the whole Account Setup wizard again just to look.
//
// Doesn't own ControllerSetupScreen/SetupWizardScreen itself -- those stay
// instantiated once in Main.qml (see the comment there), and this just
// asks for them via openControllerSetup()/openAccountSetup() so there's
// still only ever one instance of each. This screen deliberately stays
// visible (not closed) underneath while one of those is open, so
// Main.qml's reclaimFocus() can hand focus straight back here rather than
// dropping all the way back to game browsing -- see that function's
// comment.
Item {
    id: root
    anchors.fill: parent
    visible: false
    z: 25

    signal closed()
    signal openControllerSetup()
    signal openAccountSetup()

    // Rows 0-3: actions. Row 4: the fullscreen toggle. Rows 5-7: read-only
    // credential rows (reveal toggle only -- editing goes through Account
    // Setup, which already has proper validation and the on-screen
    // keyboard). Every index after actionLabels is derived from its length
    // below, so adding/reordering an action here doesn't need touching
    // those.
    readonly property var actionLabels: ["Refresh Library Now", "Controller Setup", "Account Setup (Edit Keys)", "Quit"]
    readonly property var credentialLabels: ["Steam API Key", "SteamID64", "SteamGridDB Key"]
    readonly property int fullscreenCursor: actionLabels.length
    readonly property int credentialCursorStart: fullscreenCursor + 1
    readonly property int maxCursor: credentialCursorStart + credentialLabels.length - 1

    property int cursor: 0
    property var revealed: [false, false, false]

    function open() {
        cursor = 0
        visible = true
    }

    function close() {
        visible = false
        root.closed()
    }

    // Qt.callLater, not a direct call -- see SetupWizardScreen.qml's
    // identical comment: this screen now toggles fullscreen itself (the
    // Fullscreen row above), and a forceActiveFocus() that lands mid
    // window-state transition doesn't stick.
    onVisibleChanged: if (visible) Qt.callLater(forceActiveFocus)

    function credentialValue(index) {
        if (index === 0)
            return Dex.SettingsManager.steamApiKey
        if (index === 1)
            return Dex.SettingsManager.steamId
        return Dex.SettingsManager.steamGridDbKey
    }

    function maskedDisplay(index) {
        const value = credentialValue(index)
        if (value.length === 0)
            return "(not set)"
        return root.revealed[index] ? value : "••••••••••••••••"
    }

    function toggleRevealed(index) {
        const updated = root.revealed.slice()
        updated[index] = !updated[index]
        root.revealed = updated
    }

    function activateCursor() {
        if (root.cursor === 0)
            gameLibrary.refresh()
        else if (root.cursor === 1)
            root.openControllerSetup()
        else if (root.cursor === 2)
            root.openAccountSetup()
        else if (root.cursor === 3)
            Qt.quit()
        else if (root.cursor === root.fullscreenCursor)
            Dex.SettingsManager.fullscreen = !Dex.SettingsManager.fullscreen
        else
            root.toggleRevealed(root.cursor - root.credentialCursorStart)
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape) {
            root.close()
            event.accepted = true
        } else if (event.key === Qt.Key_Up) {
            root.cursor = Math.max(0, root.cursor - 1)
            event.accepted = true
        } else if (event.key === Qt.Key_Down) {
            root.cursor = Math.min(root.maxCursor, root.cursor + 1)
            event.accepted = true
        } else if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.activateCursor()
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
        width: 620
        // Sized from the actual content rather than a guessed fixed value
        // -- this card holds a variable-length list (4 actions + the
        // fullscreen toggle + 3 credential rows + spacers), easy to get
        // wrong by hand and silently overflow the card's bottom edge
        // (Rectangle doesn't clip children by default).
        height: settingsColumn.implicitHeight + Dex.Theme.spacingLarge * 2
        radius: Dex.Theme.radiusMedium
        color: Dex.Theme.surface
        border.color: Dex.Theme.accent
        border.width: 1

        Column {
            id: settingsColumn
            anchors.fill: parent
            anchors.margins: Dex.Theme.spacingLarge
            spacing: Dex.Theme.spacingMedium

            Text {
                text: "Settings"
                color: Dex.Theme.textPrimary
                font.pixelSize: Dex.Theme.fontSizeLarge
                font.bold: true
            }

            Repeater {
                model: root.actionLabels.length

                delegate: Rectangle {
                    property int rowIndex: index
                    width: card.width - Dex.Theme.spacingLarge * 2
                    height: 52
                    radius: Dex.Theme.radiusSmall
                    color: Dex.Theme.surfaceHighlight
                    border.color: Dex.Theme.accentGlow
                    border.width: (root.cursor === rowIndex) ? Dex.Theme.glowRingWidth : 0

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: Dex.Theme.spacingMedium
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.actionLabels[rowIndex]
                        // Quit gets the same "danger" red used elsewhere in
                        // this app for a borked/incompatible verdict (see
                        // ExpandedGameView.qml's protonDbTierColor) rather
                        // than a new Theme token for one row -- it's the
                        // one action here with real consequences (loses
                        // your place), so it shouldn't blend in with
                        // Refresh/Controller Setup/Account Setup.
                        color: root.actionLabels[rowIndex] === "Quit" ? "#E05555" : Dex.Theme.textPrimary
                        font.pixelSize: Dex.Theme.fontSizeMedium
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.cursor = rowIndex
                            root.activateCursor()
                        }
                    }
                }
            }

            Item { width: 1; height: Dex.Theme.spacingSmall }

            Rectangle {
                width: card.width - Dex.Theme.spacingLarge * 2
                height: 52
                radius: Dex.Theme.radiusSmall
                color: Dex.Theme.surfaceHighlight
                border.color: Dex.Theme.accentGlow
                border.width: (root.cursor === root.fullscreenCursor) ? Dex.Theme.glowRingWidth : 0

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: Dex.Theme.spacingMedium
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Fullscreen"
                    color: Dex.Theme.textPrimary
                    font.pixelSize: Dex.Theme.fontSizeMedium
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: Dex.Theme.spacingMedium
                    anchors.verticalCenter: parent.verticalCenter
                    text: Dex.SettingsManager.fullscreen ? "On" : "Off"
                    color: Dex.SettingsManager.fullscreen ? Dex.Theme.accent : Dex.Theme.textSecondary
                    font.pixelSize: Dex.Theme.fontSizeMedium
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.cursor = root.fullscreenCursor
                        Dex.SettingsManager.fullscreen = !Dex.SettingsManager.fullscreen
                    }
                }
            }

            Item { width: 1; height: Dex.Theme.spacingSmall }

            Text {
                text: "Saved Credentials"
                color: Dex.Theme.textSecondary
                font.pixelSize: Dex.Theme.fontSizeSmall
            }

            Repeater {
                model: root.credentialLabels.length

                delegate: Rectangle {
                    property int rowIndex: index
                    property int cursorSlot: rowIndex + root.credentialCursorStart
                    width: card.width - Dex.Theme.spacingLarge * 2
                    height: 56
                    radius: Dex.Theme.radiusSmall
                    color: Dex.Theme.surfaceHighlight
                    border.color: Dex.Theme.accentGlow
                    border.width: (root.cursor === cursorSlot) ? Dex.Theme.glowRingWidth : 0

                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: Dex.Theme.spacingMedium
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        Text {
                            text: root.credentialLabels[rowIndex]
                            color: Dex.Theme.textSecondary
                            font.pixelSize: Dex.Theme.fontSizeSmall
                        }
                        Text {
                            text: root.maskedDisplay(rowIndex)
                            color: Dex.Theme.textPrimary
                            font.pixelSize: Dex.Theme.fontSizeMedium
                            font.family: "monospace"
                        }
                    }

                    Row {
                        anchors.right: parent.right
                        anchors.rightMargin: Dex.Theme.spacingMedium
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Dex.Theme.spacingSmall

                        Icon {
                            width: 18
                            height: 18
                            anchors.verticalCenter: parent.verticalCenter
                            kind: root.revealed[rowIndex] ? "eyeOff" : "eye"
                            color: Dex.Theme.textSecondary
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: root.revealed[rowIndex] ? "Hide" : "Show"
                            color: Dex.Theme.textSecondary
                            font.pixelSize: Dex.Theme.fontSizeSmall
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.cursor = cursorSlot
                            root.toggleRevealed(rowIndex)
                        }
                    }
                }
            }

            Item { width: 1; height: Dex.Theme.spacingSmall }

            Text {
                text: "D-Pad/Stick to move • A/Enter to select • B/Esc to close"
                color: Dex.Theme.textSecondary
                font.pixelSize: Dex.Theme.fontSizeSmall
            }
        }
    }
}
