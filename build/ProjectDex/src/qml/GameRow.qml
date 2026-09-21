import QtQuick
import ProjectDex as Dex

Column {
    id: root

    property string title: ""
    property alias model: listView.model

    // A stable identifier for this row (e.g. its category tag, or a fixed
    // string for the library row) -- set by Main.qml. Category rows get
    // torn down and rebuilt as a *new* GameRow instance every time fresh
    // store data changes categoryRows, so tracking "the active row" by
    // object reference goes stale the moment that happens; a plain string
    // survives the rebuild, and the isActiveRow binding below just
    // re-matches itself against it once the new instance exists.
    property string rowKey: ""

    // The currently-dwelt-on game's full data (see gameAt() on the model),
    // or null if nothing has been highlighted for 0.4s+ yet. Bind to this
    // from outside to drive a detail panel.
    property var detailGame: null

    // Now that there can be several rows on screen at once, each row's
    // ListView still has its own currentIndex (defaulting to item 0), so
    // without this every row would show its own glowing tile simultaneously
    // the moment the user interacts with *any* of them. Set from outside
    // (see Main.qml) to true only for whichever row the user is actually on
    // right now -- see GameTile.qml's `selected`, which is gated on this.
    property bool isActiveRow: false

    // Fired whenever this row's current tile changes because of real user
    // interaction (mouse hover or keyboard nav within it) -- this is what
    // Main.qml listens for to know which row just became "the" active one.
    signal hovered()

    // Fired when a vertical (non-shift, non-trackpad-horizontal) wheel
    // scroll happens over this row. Main.qml forwards this straight to the
    // page's own scroll position -- see the comment on `interactive` below
    // for why this exists instead of just letting the wheel event through.
    signal verticalWheel(real deltaY)

    // Up/Down move between rows (and the hero banner) rather than doing
    // anything within this row -- a horizontal ListView doesn't use
    // vertical arrows for anything itself, so this row always claims them
    // and asks Main.qml to hand focus to whichever row/hero is next. This
    // is also the seam controller support will hook into later: a D-pad
    // up/down (or left-stick) maps onto these same two signals.
    signal navigateUp()
    signal navigateDown()

    // model can be either the GameLibraryModel itself (has a gameAt(index)
    // slot -- used for the full "Your Library" row) or a plain JS array of
    // game maps (used for the category rows, e.g. {appId, name,
    // boxArtPath, installed}). This normalizes both to "get me row index i".
    // Snaps this row's highlighted tile to whichever one is actually first
    // visible on screen right now, rather than leaving it wherever
    // currentIndex was last left (which could be scrolled off-screen from
    // earlier browsing). Called by Main.qml specifically when this row
    // becomes active via keyboard/controller navigation -- NOT on mouse
    // hover, which already sets currentIndex to the exact tile the mouse
    // is over and shouldn't be second-guessed.
    // Hands keyboard/controller focus to this row's ListView explicitly --
    // for when something outside the normal isActiveRow flow (a closed
    // overlay, e.g. ControllerSetupScreen) needs to hand focus back
    // without isActiveRow itself having changed (onIsActiveRowChanged only
    // fires on an actual value change, so re-setting the same value is a
    // no-op and doesn't reclaim focus by itself).
    function claimFocus() {
        listView.forceActiveFocus()
    }

    function snapToFirstVisible() {
        if (listView.count === 0)
            return
        const idx = listView.indexAt(listView.contentX + 1, listView.height / 2)
        if (idx >= 0)
            listView.currentIndex = idx
    }

    // Jumps a full "page" of tiles forward (direction 1) or back
    // (direction -1), then snaps the newly-current tile to the front
    // (left edge) of the row -- like turning to a new page rather than
    // scrolling one tile at a time. Page size is however many tiles
    // actually fit on screen right now, so it adapts to window size.
    //
    // This turned out to need more than just a Behavior on contentX (see
    // listView.animatedScroll below): a Behavior only intercepts writes
    // to contentX that go through QML's own property system (a plain
    // "target.prop = value", or a binding). Both positionViewAtIndex()
    // and ListView's own automatic highlight-follow (which fires the
    // instant currentIndex changes, to keep the new current item on
    // screen) set contentX straight from C++ instead, bypassing that
    // property-interceptor chain entirely -- so either one always jumped
    // instantly no matter what Behavior was attached, which is why the
    // row never actually looked like it animated.
    //
    // So this computes the destination itself (exactly what
    // positionViewAtIndex(newIndex, ListView.Beginning) would have
    // landed on) and assigns contentX directly from here -- a real QML
    // property write the Behavior *does* see -- while temporarily
    // turning off highlight-follow so it can't jump ahead of that
    // animation. animatedScrollResetTimer turns highlight-follow back on
    // once the animation's had time to finish.
    function pageBy(direction) {
        if (listView.count === 0)
            return
        const tileWidth = (listView.currentItem ? listView.currentItem.width : 180) + listView.spacing
        const itemsPerPage = Math.max(1, Math.floor(listView.width / tileWidth))
        const newIndex = Math.max(0, Math.min(listView.count - 1, listView.currentIndex + direction * itemsPerPage))
        const maxContentX = Math.max(0, listView.contentWidth - listView.width)
        const targetContentX = Math.max(0, Math.min(maxContentX, newIndex * tileWidth))

        listView.highlightFollowsCurrentItem = false
        listView.animatedScroll = true
        listView.currentIndex = newIndex
        listView.contentX = targetContentX
        animatedScrollResetTimer.restart()
    }

    function itemAt(index) {
        if (index < 0 || !listView.model)
            return {}
        if (typeof listView.model.gameAt === "function")
            return listView.model.gameAt(index)
        if (index < listView.model.length)
            return listView.model[index]
        return {}
    }

    spacing: Dex.Theme.spacingSmall

    Text {
        text: root.title
        color: Dex.Theme.textPrimary
        font.pixelSize: Dex.Theme.fontSizeMedium
    }

    ListView {
        id: listView
        orientation: ListView.Horizontal
        width: parent.width
        height: 360
        spacing: Dex.Theme.spacingMedium
        clip: true
        // Only the currently-active row should actually hold keyboard
        // focus -- with several rows on screen, each unconditionally
        // requesting focus: true meant they fought over it and arrow keys
        // stopped reliably reaching any one of them.
        focus: root.isActiveRow
        // Requesting `focus` via a plain binding (above) can silently stop
        // working: Qt Quick imperatively clears an item's `focus` property
        // whenever active focus moves elsewhere for any reason, and that
        // overwrite destroys the binding along with it. forceActiveFocus()
        // here re-asserts it every time this row actually becomes active,
        // so a focus change elsewhere can't permanently break keyboard
        // input into whichever row the user is currently on.
        onIsActiveRowChanged: if (isActiveRow) forceActiveFocus()

        // Stays interactive (drag-to-scroll horizontally still works) --
        // but horizontal ListViews are Flickables too, and Qt Quick by
        // default maps a plain vertical mouse-wheel scroll onto a
        // horizontal Flickable's own scrolling. That meant every row was
        // silently eating the page's scroll wheel input whenever the
        // cursor happened to be over it (most of the screen). The
        // WheelHandler below claims vertical-dominant wheel input first and
        // forwards it to the page instead, while a genuinely horizontal
        // gesture (Shift+wheel, a trackpad swipe) still falls through to
        // scroll the row itself as normal.
        interactive: true
        currentIndex: 0
        highlightMoveDuration: 150
        delegate: GameTile {}
        enabled: Dex.UiState.expandedAppId < 0

        // Off by default -- a Behavior on contentX left on permanently
        // would also try to animate every frame of a manual mouse drag,
        // making dragging feel laggy/mushy. Only pageBy() turns this on,
        // for the duration of one page jump.
        property bool animatedScroll: false
        Behavior on contentX {
            enabled: listView.animatedScroll
            NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
        }
        Timer {
            id: animatedScrollResetTimer
            interval: 320
            onTriggered: {
                listView.animatedScroll = false
                listView.highlightFollowsCurrentItem = true
            }
        }

        WheelHandler {
            target: null
            onWheel: (event) => {
                if (Math.abs(event.angleDelta.y) > Math.abs(event.angleDelta.x)) {
                    root.verticalWheel(event.angleDelta.y)
                    event.accepted = true
                }
            }
        }

        // Forwarded onto the ListView itself (rather than read straight
        // off `root`) so GameTile -- which only has ListView.view, not a
        // reference to this row -- can get at it via ListView.view.isActiveRow.
        property bool isActiveRow: root.isActiveRow

        Keys.onPressed: (event) => {
            Dex.InputState.reportKeyboardInput()
            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                // Enter/Return alongside Space -- Space matches what was
                // already there, Enter/Return is the more natural "confirm"
                // key (and the one a controller's A/X button will map to).
                const game = root.itemAt(listView.currentIndex)
                if (game.appId !== undefined)
                    Dex.UiState.expandGame(game.appId)
                event.accepted = true
            } else if (event.key === Qt.Key_Up) {
                root.navigateUp()
                event.accepted = true
            } else if (event.key === Qt.Key_Down) {
                root.navigateDown()
                event.accepted = true
            } else if (event.key === Qt.Key_Left) {
                // Handled explicitly rather than left to the ListView's own
                // built-in arrow-key behavior -- that only fires when the
                // ListView itself is the literal focused item, and in
                // practice focus can end up sitting on the current
                // delegate (a GameTile) instead. This still gets reached
                // either way, since an unhandled key always bubbles up to
                // this handler regardless of which of the two holds focus.
                if (listView.currentIndex > 0)
                    listView.currentIndex -= 1
                event.accepted = true
            } else if (event.key === Qt.Key_Right) {
                if (listView.currentIndex < listView.count - 1)
                    listView.currentIndex += 1
                event.accepted = true
            } else if (event.key === Qt.Key_PageDown) {
                // Controller right bumper -- see pageBy() below.
                root.pageBy(1)
                event.accepted = true
            } else if (event.key === Qt.Key_PageUp) {
                // Controller left bumper.
                root.pageBy(-1)
                event.accepted = true
            }
        }

        Connections {
            target: Dex.UiState
            function onExpandedAppIdChanged() {
                // Guarded on isActiveRow -- every row listens for this, and
                // without the guard whichever one happened to run last
                // would steal focus back regardless of which row the user
                // actually had open the detail view from.
                if (Dex.UiState.expandedAppId < 0 && root.isActiveRow)
                    Qt.callLater(listView.forceActiveFocus)
            }
        }

        onCurrentIndexChanged: {
            root.detailGame = null
            dwellTimer.restart()
            root.hovered()
        }
    }

    Timer {
        id: dwellTimer
        interval: 400
        repeat: false
        running: Dex.InputState.hasInteracted && listView.count > 0
        onTriggered: root.detailGame = root.itemAt(listView.currentIndex)
    }
}
