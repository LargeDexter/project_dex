import QtQuick
import ProjectDex as Dex

Rectangle {
    id: root
    width: 180
    height: 350
    radius: Dex.Theme.radiusMedium
    color: Dex.Theme.surface

    // The row's model is either the C++ GameLibraryModel (exposes appId/
    // name/boxArtPath/installed as named roles directly) or a plain JS
    // array of game maps, as used by the category rows -- for an array
    // model, QML only reliably exposes the whole element as `modelData`,
    // not each of its properties as its own top-level context property.
    // Prefer modelData when it's present (array case) and fall back to the
    // bare role names otherwise (QAbstractListModel case), so this one
    // delegate works correctly for both.
    readonly property var rowGame: (typeof modelData !== "undefined" && modelData) ? modelData : null
    readonly property var tileAppId: rowGame ? rowGame.appId : appId
    readonly property string tileName: rowGame ? (rowGame.name || "") : name
    readonly property string tileBoxArtPath: rowGame ? (rowGame.boxArtPath || "") : boxArtPath
    readonly property bool tileInstalled: rowGame ? (rowGame.installed === true) : installed

    // True when this is the current item AND the user has actually
    // interacted at least once -- keeps the glow from appearing
    // automatically on launch just because something has to be "current"
    // for keyboard navigation to have somewhere to start from. Also
    // requires this tile's row to be the currently "active" one (see
    // GameRow.isActiveRow) -- otherwise, with several rows on screen at
    // once, every row's own current item would glow simultaneously rather
    // than just the one the user is actually on.
    // Defaults to false (not true) when ListView.view isn't set yet --
    // category rows get fully torn down and rebuilt every time new store
    // data streams in during the first several seconds after launch, and
    // freshly-created tiles briefly have no ListView.view while that
    // happens. Defaulting to "active" during that gap made every row's
    // first tile flash as selected simultaneously.
    readonly property bool rowIsActive: ListView.view ? ListView.view.isActiveRow : false
    readonly property bool selected: ListView.isCurrentItem && Dex.InputState.hasInteracted && rowIsActive

    HoverHandler {
        enabled: Dex.UiState.expandedAppId < 0
        onHoveredChanged: {
            if (hovered) {
                Dex.InputState.reportMouseInput()
                if (root.ListView.view)
                    root.ListView.view.currentIndex = index
            }
        }
    }

    TapHandler {
        enabled: Dex.UiState.expandedAppId < 0
        onTapped: Dex.UiState.expandGame(root.tileAppId)
    }

    // Soft outer halo -- extends beyond the tile, low-opacity translucent
    // orange, fades in/out rather than popping.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -8
        radius: root.radius + 8
        color: "transparent"
        border.color: Dex.Theme.accentGlow
        border.width: 8
        opacity: root.selected ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    // Crisp inner ring, sits right on the tile edge.
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "transparent"
        border.color: Dex.Theme.accent
        border.width: Dex.Theme.glowRingWidth
        opacity: root.selected ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        Image {
            id: art
            width: parent.width
            // Steam box art is a 2:3 portrait ratio (600x900) -- match it
            // exactly so PreserveAspectFit never has to letterbox, and
            // nothing gets cropped off the top/bottom like it was before.
            height: width * 1.5
            source: root.tileBoxArtPath.length > 0 ? ("file://" + root.tileBoxArtPath) : ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true

            Rectangle {
                anchors.fill: parent
                visible: art.status !== Image.Ready
                color: Dex.Theme.surfaceHighlight
                radius: Dex.Theme.radiusSmall
            }
        }

        Text {
            width: parent.width
            text: root.tileName
            color: Dex.Theme.textPrimary
            font.pixelSize: Dex.Theme.fontSizeSmall
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
        }

        Text {
            text: root.tileInstalled ? "Installed" : "Not installed"
            color: root.tileInstalled ? Dex.Theme.accent : Dex.Theme.textSecondary
            font.pixelSize: 11
        }
    }
}
