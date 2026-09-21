import QtQuick
import ProjectDex as Dex

// A D-pad/stick-navigable virtual keyboard, for typing API keys/IDs on a
// couch with a controller instead of a real keyboard -- see
// SetupWizardScreen.qml, its only user. Every key is just a cell in a grid;
// Up/Down/Left/Right move a highlighted cursor cell to cell (reusing the
// same synthetic-key-event navigation every other controller-driven part of
// this app already relies on -- see gamepad_input.h), and Confirm/Return
// "presses" whatever's currently highlighted.
//
// A full-window overlay like ControllerSetupScreen, so it can sit on top of
// the setup wizard's own card and dim everything below it -- text is edited
// here, then handed back to the caller via accepted()/cancelled() rather
// than bound live, so backing out with Cancel/Escape can cleanly discard
// whatever was typed without the caller needing to snapshot/restore
// anything itself.
Item {
    id: root
    anchors.fill: parent
    visible: false
    z: 100

    property string text: ""
    property string label: ""
    property int maxLength: 64

    signal accepted()
    signal cancelled()

    function open(initialText, fieldLabel) {
        root.text = initialText
        root.label = fieldLabel
        root.cursorRow = 0
        root.cursorCol = 0
        root.visible = true
    }

    function close() {
        root.visible = false
    }

    onVisibleChanged: if (visible) forceActiveFocus()

    // Row-major key layout. Multi-character entries are control keys
    // handled explicitly in activateCurrentKey() below rather than typed
    // literally -- everything else is inserted as-is, which is why the
    // alphabet/digit rows can just be plain single characters.
    readonly property var rows: [
        ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"],
        ["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"],
        ["A", "S", "D", "F", "G", "H", "J", "K", "L"],
        ["Z", "X", "C", "V", "B", "N", "M", "Backspace"],
        ["Space", "Cancel", "Done"],
    ]

    property int cursorRow: 0
    property int cursorCol: 0

    function moveVertical(delta) {
        const newRow = Math.max(0, Math.min(rows.length - 1, cursorRow + delta))
        cursorRow = newRow
        cursorCol = Math.max(0, Math.min(rows[newRow].length - 1, cursorCol))
    }

    function moveHorizontal(delta) {
        const row = rows[cursorRow]
        cursorCol = Math.max(0, Math.min(row.length - 1, cursorCol + delta))
    }

    function activateKey(key) {
        if (key === "Backspace")
            root.text = root.text.slice(0, -1)
        else if (key === "Space") {
            if (root.text.length < root.maxLength)
                root.text += " "
        } else if (key === "Cancel")
            root.cancelled()
        else if (key === "Done")
            root.accepted()
        else if (root.text.length < root.maxLength)
            root.text += key
    }

    function activateCurrentKey() {
        activateKey(rows[cursorRow][cursorCol])
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Up) {
            moveVertical(-1)
            event.accepted = true
        } else if (event.key === Qt.Key_Down) {
            moveVertical(1)
            event.accepted = true
        } else if (event.key === Qt.Key_Left) {
            moveHorizontal(-1)
            event.accepted = true
        } else if (event.key === Qt.Key_Right) {
            moveHorizontal(1)
            event.accepted = true
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
            activateCurrentKey()
            event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
            root.cancelled()
            event.accepted = true
        } else if (event.key === Qt.Key_Backspace) {
            root.text = root.text.slice(0, -1)
            event.accepted = true
        } else if (event.text.length > 0 && /^[\x20-\x7e]+$/.test(event.text)) {
            // A real, physical keyboard -- not just the on-screen grid --
            // is attached in most dev/desktop setups (and plenty of couch
            // setups have a wireless one too), so typing normally works
            // directly rather than forcing everyone through arrow-key
            // grid navigation one character at a time. This only matches
            // actual printable ASCII text (letters/digits/punctuation);
            // arrow keys, Tab, function keys etc. report an empty
            // event.text and never reach this branch.
            if (root.text.length < root.maxLength)
                root.text += event.text
            event.accepted = true
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.85
        MouseArea { anchors.fill: parent }
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: 700
        height: keyboardColumn.implicitHeight + Dex.Theme.spacingLarge * 2
        radius: Dex.Theme.radiusMedium
        color: Dex.Theme.surface
        border.color: Dex.Theme.accent
        border.width: 1

        Column {
            id: keyboardColumn
            anchors.fill: parent
            anchors.margins: Dex.Theme.spacingLarge
            spacing: Dex.Theme.spacingMedium

            Text {
                text: root.label
                color: Dex.Theme.textSecondary
                font.pixelSize: Dex.Theme.fontSizeSmall
            }

            Rectangle {
                width: parent.width
                height: 48
                radius: Dex.Theme.radiusSmall
                color: Dex.Theme.surfaceHighlight
                border.color: Dex.Theme.accent
                border.width: 1

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Dex.Theme.spacingSmall
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideLeft
                    text: root.text.length > 0 ? root.text : " "
                    color: Dex.Theme.textPrimary
                    font.pixelSize: Dex.Theme.fontSizeMedium
                    font.family: "monospace"
                }
            }

            Item { width: 1; height: Dex.Theme.spacingSmall }

            Column {
                spacing: Dex.Theme.spacingSmall

                Repeater {
                    model: root.rows.length

                    delegate: Row {
                        spacing: Dex.Theme.spacingSmall
                        property int rowIndex: index

                        Repeater {
                            model: root.rows[rowIndex].length

                            delegate: Rectangle {
                                property int colIndex: index
                                property string keyLabel: root.rows[rowIndex][colIndex]
                                readonly property bool isWide: keyLabel.length > 1

                                width: isWide ? 96 : 42
                                height: 42
                                radius: Dex.Theme.radiusSmall
                                color: Dex.Theme.surfaceHighlight
                                border.color: Dex.Theme.accentGlow
                                border.width: (root.cursorRow === rowIndex && root.cursorCol === colIndex)
                                              ? Dex.Theme.glowRingWidth : 0

                                Text {
                                    anchors.centerIn: parent
                                    text: keyLabel
                                    color: Dex.Theme.textPrimary
                                    font.pixelSize: Dex.Theme.fontSizeSmall
                                }

                                // Mouse/touch works too, not just
                                // controller/keyboard -- clicking a key
                                // both moves the cursor there and presses
                                // it, matching how a real on-screen
                                // keyboard behaves under a pointer.
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        root.cursorRow = rowIndex
                                        root.cursorCol = colIndex
                                        root.activateKey(keyLabel)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Text {
                text: "D-Pad/Stick to move • A/Enter to select • B/Esc to cancel"
                color: Dex.Theme.textSecondary
                font.pixelSize: Dex.Theme.fontSizeSmall
            }
        }
    }
}
