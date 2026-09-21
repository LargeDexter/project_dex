import QtQuick
import QtQuick.Window
import QtQuick.Controls
import ProjectDex as Dex

Window {
    id: mainWindow
    width: 1280
    height: 720
    visible: true
    // Toggleable from Settings (SettingsScreen.qml) rather than fixed --
    // couch setups on a TV generally want fullscreen, but this also runs
    // fine on a regular desktop monitor windowed. Windowed keeps the
    // width/height above as its size; fullscreen ignores them.
    visibility: Dex.SettingsManager.fullscreen ? Window.FullScreen : Window.Windowed
    title: "Project Dex"
    color: Dex.Theme.background

    // Aggregated across every browsing row below (the category rows and
    // the full-library row) -- whichever row the user is currently
    // dwelling on drives the corner detail panel. Each row tracks whether
    // *it* was the one that last set this, so hovering away from one row
    // doesn't clobber another row's still-active game.
    property var activeDetailGame: null

    // Which row the user is currently on, by its stable rowKey (empty
    // string before any interaction). Deliberately NOT an object reference
    // to the GameRow instance -- category rows get destroyed and rebuilt
    // as fresh instances every time new store data changes categoryRows
    // (several times in the first few seconds after launch), which would
    // instantly invalidate a stored reference. A string survives that.
    property string activeRowKey: ""

    // True when keyboard focus is on the hero banner rather than any row --
    // console-style launchers (Netflix, PS5) open with the banner focused,
    // and Down reveals the rows below it, so that's the default here too.
    property bool heroFocused: false

    // Give keyboard focus somewhere sane at startup, before the user has
    // hovered or clicked anything -- otherwise nothing is focused (no row,
    // no hero) until the first interaction, and arrow keys do nothing.
    //
    // Also offers the setup wizard right away if the required credentials
    // (Steam API Key + SteamID64) haven't actually been saved through it
    // yet -- checked against SettingsManager directly (the wizard's own
    // on-disk settings.json) rather than waiting on a sync failure, so
    // this fires even for someone whose STEAM_API_KEY/STEAM_ID env vars
    // happen to make syncing work anyway (see library_sync_worker.cpp's
    // fallback chain) -- the wizard should still offer to save those
    // properly rather than silently relying on an env var forever.
    Component.onCompleted: {
        focusHero()
        if (!Dex.SettingsManager.hasCredentials) {
            mainWindow.setupWizardAutoOffered = true
            setupWizardScreen.open()
        }
    }

    // Guards the setup wizard's auto-open so it only ever offers itself
    // once per launch -- without this, every failed sync retry (e.g. the
    // user backing out of the wizard without saving, then clicking
    // Refresh) would pop it right back open.
    property bool setupWizardAutoOffered: false

    Connections {
        target: gameLibrary
        function onSyncFailed(error) {
            // "missing-credentials" is LibrarySyncWorker's fixed sentinel
            // for "no Steam API key/SteamID configured yet" (see
            // library_sync_worker.cpp) -- a genuine network/API failure
            // comes through with a different, human-readable message and
            // deliberately isn't handled here, so a real outage doesn't
            // repeatedly yank the wizard open over something it can't fix.
            // Mainly a backstop for the (currently impossible, but cheap
            // to guard against) case where credentials looked present at
            // startup but syncing still reports them missing.
            if (error === "missing-credentials" && !mainWindow.setupWizardAutoOffered) {
                mainWindow.setupWizardAutoOffered = true
                setupWizardScreen.open()
            }
        }
    }

    // Every row on screen, top to bottom, as actual GameRow instances --
    // the category rows (however many have filled in so far) followed by
    // the always-present "Your Library" row. This is the single source of
    // truth Up/Down navigation and scroll-into-view walk through, and the
    // same list a future controller mapping would drive.
    function allRows() {
        const rows = [toolbarRow]
        for (let i = 0; i < categoryRepeater.count; i++) {
            const item = categoryRepeater.itemAt(i)
            if (item)
                rows.push(item)
        }
        rows.push(libraryRow)
        return rows
    }

    function focusHero() {
        activeRowKey = ""
        heroFocused = true
        scrollIntoView(heroItem)
    }

    // Hands keyboard/controller focus back to wherever it belongs (the
    // hero, or the active row) -- for when an overlay that took focus away
    // (ControllerSetupScreen, SetupWizardScreen, SettingsScreen) closes.
    // Re-setting activeRowKey/heroFocused to their own current values
    // wouldn't trigger the onChanged handlers that normally do this, since
    // nothing actually changed, so this calls straight through instead.
    function reclaimFocus() {
        // ControllerSetupScreen/SetupWizardScreen can be opened either
        // directly from the toolbar OR from within SettingsScreen (see
        // its openControllerSetup()/openAccountSetup() signals below) --
        // SettingsScreen deliberately stays visible underneath either one
        // rather than closing itself, so when the nested screen closes
        // and calls this, focus should land back on Settings rather than
        // dropping all the way out to game browsing.
        if (settingsScreen.visible) {
            settingsScreen.forceActiveFocus()
            return
        }
        if (heroFocused) {
            heroItem.forceActiveFocus()
            return
        }
        const rows = allRows()
        const row = rows.find((r) => r.rowKey === activeRowKey)
        if (row && typeof row.claimFocus === "function")
            row.claimFocus()
        else
            focusHero() // Nothing valid to return to -- fall back to a sane default.
    }

    function focusRowIndex(index) {
        const rows = allRows()
        if (rows.length === 0)
            return
        const clamped = Math.max(0, Math.min(rows.length - 1, index))
        heroFocused = false
        const row = rows[clamped]
        if (typeof row.snapToFirstVisible === "function")
            row.snapToFirstVisible()
        activeRowKey = row.rowKey
        scrollIntoView(row)
    }

    function navigateUpFrom(key) {
        const rows = allRows()
        const idx = rows.findIndex((r) => r.rowKey === key)
        if (idx <= 0)
            focusHero()
        else
            focusRowIndex(idx - 1)
    }

    function navigateDownFrom(key) {
        const rows = allRows()
        const idx = rows.findIndex((r) => r.rowKey === key)
        if (idx < 0)
            return
        if (idx < rows.length - 1)
            focusRowIndex(idx + 1)
        // Already at the bottom row -- nothing further down to go to.
    }

    // Smoothly scrolls the page just far enough to bring `item` fully into
    // view (with a small margin), without disturbing the scroll position
    // at all if it's already visible. Used for keyboard/controller-driven
    // navigation only -- mouse wheel and click-drag scrolling stay as
    // direct, unanimated contentY changes so they still feel immediate.
    NumberAnimation {
        id: scrollAnim
        target: pageFlickable
        property: "contentY"
        duration: 250
        easing.type: Easing.OutCubic
    }

    function scrollIntoView(item) {
        if (!item)
            return
        const itemY = item.mapToItem(contentColumn, 0, 0).y
        const itemBottom = itemY + item.height
        const margin = 24
        const maxY = Math.max(0, pageFlickable.contentHeight - pageFlickable.height)
        let target = pageFlickable.contentY
        if (itemY - margin < pageFlickable.contentY)
            target = Math.max(0, itemY - margin)
        else if (itemBottom + margin > pageFlickable.contentY + pageFlickable.height)
            target = Math.min(maxY, itemBottom + margin - pageFlickable.height)
        if (Math.abs(target - pageFlickable.contentY) > 1) {
            scrollAnim.stop()
            scrollAnim.from = pageFlickable.contentY
            scrollAnim.to = target
            scrollAnim.start()
        }
    }

    Flickable {
        id: pageFlickable
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        flickDeceleration: 4000
        maximumFlickVelocity: 4000

        // ScrollView's default wheel step is tiny, and painfully slow on a
        // page this tall (hero + several rows). Handling the wheel
        // ourselves and flicking gives a much snappier, more natural
        // scroll speed.
        WheelHandler {
            target: pageFlickable
            onWheel: (event) => {
                const maxY = Math.max(0, pageFlickable.contentHeight - pageFlickable.height)
                pageFlickable.contentY = Math.max(0, Math.min(maxY, pageFlickable.contentY - event.angleDelta.y * 2.5))
                event.accepted = true
            }
        }

        ScrollBar.vertical: ScrollBar {}

        Column {
            id: contentColumn
            width: pageFlickable.width
            spacing: Dex.Theme.spacingSmall

            Hero {
                id: heroItem
                width: parent.width
                height: 480

                focused: mainWindow.heroFocused
                onNavigateDown: mainWindow.focusRowIndex(0)
            }

            // Part of the same Up/Down/Left/Right navigation chain as the
            // hero and the rows below (see allRows(), which prepends this
            // as the first entry) -- Down from the hero lands here before
            // reaching any row, Left/Right moves between its two buttons,
            // and Enter/A activates whichever one is highlighted. Follows
            // the same isActiveRow/rowKey/claimFocus() contract every
            // GameRow implements, so it slots into the existing navigation
            // functions without those needing to special-case it.
            Row {
                id: toolbarRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Dex.Theme.spacingLarge
                anchors.rightMargin: Dex.Theme.spacingLarge
                spacing: Dex.Theme.spacingMedium

                property string rowKey: "__toolbar__"
                readonly property bool isActiveRow: mainWindow.activeRowKey.length > 0 && mainWindow.activeRowKey === rowKey

                function snapToFirstVisible() { }
                function claimFocus() { toolbarRow.forceActiveFocus() }

                focus: isActiveRow
                onIsActiveRowChanged: if (isActiveRow) forceActiveFocus()

                Keys.onPressed: (event) => {
                    Dex.InputState.reportKeyboardInput()
                    if (event.key === Qt.Key_Up) {
                        mainWindow.navigateUpFrom(toolbarRow.rowKey)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Down) {
                        mainWindow.navigateDownFrom(toolbarRow.rowKey)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        settingsScreen.open()
                        event.accepted = true
                    }
                }

                Text {
                    text: gameLibrary.syncing
                          ? "Syncing..."
                          : ("Library (" + gameLibrary.count + " games)")
                    color: Dex.Theme.textPrimary
                    font.pixelSize: Dex.Theme.fontSizeMedium
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Everything else (Refresh, Controller Setup, Account
                // Setup, and a masked view of the saved credentials) lives
                // behind this one gear icon now -- see SettingsScreen.qml.
                Rectangle {
                    width: 44
                    height: 44
                    radius: Dex.Theme.radiusSmall
                    color: "transparent"
                    border.color: Dex.Theme.accentGlow
                    border.width: toolbarRow.isActiveRow ? Dex.Theme.glowRingWidth : 0

                    Icon {
                        anchors.centerIn: parent
                        width: 22
                        height: 22
                        kind: "gear"
                        // Flags missing required credentials right on the
                        // icon itself, in case the wizard's auto-open was
                        // dismissed/skipped -- otherwise there'd be no
                        // visible sign anywhere that anything needs doing.
                        color: Dex.SettingsManager.hasCredentials ? Dex.Theme.textPrimary : Dex.Theme.accent
                    }

                    MouseArea {
                        id: settingsButton
                        anchors.fill: parent
                        onClicked: settingsScreen.open()
                    }
                }
            }

            // Netflix-style browsing rows: "Recently Played" first, then a
            // randomized handful of genre/category rows (Co-op, RPG,
            // etc.) -- built in GameLibraryModel.categoryRows from Steam
            // store data as it's fetched in the background. Empty until
            // the first row's data is ready, then fills in progressively.
            Repeater {
                id: categoryRepeater
                model: gameLibrary.categoryRows

                delegate: GameRow {
                    id: categoryRowDelegate
                    anchors.left: parent ? parent.left : undefined
                    anchors.right: parent ? parent.right : undefined
                    anchors.leftMargin: Dex.Theme.spacingLarge
                    anchors.rightMargin: Dex.Theme.spacingLarge
                    width: parent ? parent.width : 0
                    title: modelData.title
                    model: modelData.games
                    rowKey: modelData.title

                    isActiveRow: mainWindow.activeRowKey.length > 0 && mainWindow.activeRowKey === rowKey
                    onHovered: {
                        mainWindow.heroFocused = false
                        mainWindow.activeRowKey = rowKey
                    }
                    onNavigateUp: mainWindow.navigateUpFrom(rowKey)
                    onNavigateDown: mainWindow.navigateDownFrom(rowKey)
                    onVerticalWheel: (deltaY) => {
                        const maxY = Math.max(0, pageFlickable.contentHeight - pageFlickable.height)
                        pageFlickable.contentY = Math.max(0, Math.min(maxY, pageFlickable.contentY - deltaY * 2.5))
                    }

                    property bool isActiveSource: false
                    onDetailGameChanged: {
                        if (detailGame) {
                            mainWindow.activeDetailGame = detailGame
                            isActiveSource = true
                        } else if (isActiveSource) {
                            mainWindow.activeDetailGame = null
                            isActiveSource = false
                        }
                    }
                }
            }

            // The full catalog, always last -- everything lives here
            // regardless of genre data, so nothing is ever unreachable
            // while the category rows above are still filling in.
            GameRow {
                id: libraryRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Dex.Theme.spacingLarge
                anchors.rightMargin: Dex.Theme.spacingLarge
                title: "Your Library"
                model: gameLibrary
                rowKey: "__library__"

                isActiveRow: mainWindow.activeRowKey.length > 0 && mainWindow.activeRowKey === rowKey
                onHovered: {
                    mainWindow.heroFocused = false
                    mainWindow.activeRowKey = rowKey
                }
                onNavigateUp: mainWindow.navigateUpFrom(rowKey)
                onNavigateDown: mainWindow.navigateDownFrom(rowKey)
                onVerticalWheel: (deltaY) => {
                    const maxY = Math.max(0, pageFlickable.contentHeight - pageFlickable.height)
                    pageFlickable.contentY = Math.max(0, Math.min(maxY, pageFlickable.contentY - deltaY * 2.5))
                }

                property bool isActiveSource: false
                onDetailGameChanged: {
                    if (detailGame) {
                        mainWindow.activeDetailGame = detailGame
                        isActiveSource = true
                    } else if (isActiveSource) {
                        mainWindow.activeDetailGame = null
                        isActiveSource = false
                    }
                }
            }
        }
    }

    // Floats above everything else -- deliberately outside the scroll
    // content so it doesn't get clipped or scrolled out of view.
    DetailPanel {
        game: Dex.UiState.expandedAppId < 0 ? mainWindow.activeDetailGame : null
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Dex.Theme.spacingLarge
        z: 10
    }

    ExpandedGameView {
        anchors.fill: parent
        z: 20
    }

    ControllerSetupScreen {
        id: controllerSetupScreen
        z: 30
        // Closing it (Done, Cancel, Escape, or finishing the last step)
        // takes active focus with it unless something explicitly claims it
        // back -- this was the "no buttons work after the wizard closes"
        // bug. See Main.qml's reclaimFocus() and GameRow.qml's claimFocus().
        onClosed: mainWindow.reclaimFocus()
    }

    SetupWizardScreen {
        id: setupWizardScreen
        z: 31
        onClosed: mainWindow.reclaimFocus()
        // A successful Save is the one case worth re-syncing for
        // automatically -- Cancel/Skip closes without this firing, since
        // nothing actually changed for a retry to help with.
        onSaved: gameLibrary.refresh()
    }

    SettingsScreen {
        id: settingsScreen
        z: 25
        onClosed: mainWindow.reclaimFocus()
        onOpenControllerSetup: controllerSetupScreen.open()
        onOpenAccountSetup: setupWizardScreen.open()
    }
}
