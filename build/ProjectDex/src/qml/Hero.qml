import QtQuick
import QtMultimedia
import ProjectDex as Dex

Item {
    id: hero

    property var games: gameLibrary.heroGames
    property int currentIndex: 0
    readonly property var currentGame: games.length > 0 ? games[currentIndex] : null

    // Pick a random hero game the first time the list has real content,
    // rather than always opening on the same (most-recently-played) game --
    // keeps the "front door" feeling different every time the app starts.
    // Only fires once per run; later heroGames updates (e.g. a re-sync)
    // just fall through to the existing bounds-check below.
    property bool heroStartRandomized: false

    // Whether the hero banner currently holds keyboard focus -- set from
    // outside (Main.qml) the same way GameRow.isActiveRow is, so Down
    // hands off to the top row and the rows hand Up back here. This is the
    // console-style "start at the top" entry point: the app opens with the
    // hero focused rather than a row.
    property bool focused: false
    onFocusedChanged: if (focused) forceActiveFocus()

    // Fired when Down is pressed while the hero is focused -- Main.qml
    // hands focus to the first row.
    signal navigateDown()

    Keys.onPressed: (event) => {
        Dex.InputState.reportKeyboardInput()
        if (event.key === Qt.Key_Down) {
            hero.navigateDown()
            event.accepted = true
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
            hero.launchOrInstall()
            event.accepted = true
        }
    }

    // Reclaim focus once the expanded detail view closes, same as each
    // row does for itself -- otherwise closing a game opened from the hero
    // leaves keyboard input going nowhere.
    Connections {
        target: Dex.UiState
        function onExpandedAppIdChanged() {
            if (Dex.UiState.expandedAppId < 0 && hero.focused)
                Qt.callLater(hero.forceActiveFocus)
        }
    }

    // Store details (description/requirements/trailer) for whichever game is
    // currently showing.
    property var storeDetails: ({})
    readonly property string trailerUrl: (storeDetails && storeDetails.available === true)
        ? (storeDetails.trailerUrl || "") : ""

    // The next game to rotate to, and its details, pre-selected and
    // pre-fetched a few seconds ahead of the actual rotation so its
    // trailer has a head start buffering in the background player before
    // it's ever shown -- this is what lets rotation cut trailer-to-trailer
    // instead of falling back to the box art each time.
    property int primedIndex: -1
    readonly property var primedGame: (primedIndex >= 0 && primedIndex < games.length) ? games[primedIndex] : null
    property var primedStoreDetails: ({})
    readonly property string primedTrailerUrl: (primedStoreDetails && primedStoreDetails.available === true)
        ? (primedStoreDetails.trailerUrl || "") : ""

    // Which of the two players is currently the "on screen" one.
    property bool playerAActive: true

    // The detail view covers the hero completely, so there's no reason to
    // keep decoding up to two hero trailers underneath it while it's open
    // -- pause both and resume whichever is active when it closes.
    readonly property bool suspended: Dex.UiState.expandedAppId >= 0
    onSuspendedChanged: {
        if (suspended) {
            playerA.pause()
            playerB.pause()
        } else {
            if (playerA.source.toString().length > 0)
                playerA.play()
            if (playerB.source.toString().length > 0)
                playerB.play()
        }
    }

    // A single 6s-tick timer instead of two independently-repeating
    // timers, so the 6s prime-ahead lead time before each 30s rotation
    // stays fixed instead of drifting cycle to cycle.
    property int tickCount: 0
    readonly property int ticksPerRotation: 5 // 5 * 6s = 30s between rotations

    Timer {
        interval: 6000
        running: hero.games.length > 1
        repeat: true
        onTriggered: {
            hero.tickCount = (hero.tickCount + 1) % hero.ticksPerRotation
            if (hero.tickCount === hero.ticksPerRotation - 1)
                hero.primeNext()
            else if (hero.tickCount === 0)
                hero.rotate()
        }
    }

    function pickDifferentIndex(excluding) {
        if (games.length <= 1)
            return excluding
        let next = excluding
        while (next === excluding)
            next = Math.floor(Math.random() * games.length)
        return next
    }

    function primeNext() {
        if (games.length <= 1)
            return
        primedIndex = pickDifferentIndex(currentIndex)
        const g = primedGame
        primedStoreDetails = g ? gameLibrary.storeDetailsFor(g.appId) : {}
        if (g)
            gameLibrary.requestStoreDetails(g.appId)
    }

    function rotate() {
        if (games.length <= 1)
            return
        if (primedIndex < 0 || primedIndex >= games.length)
            primeNext() // wasn't primed in time -- fall back to loading fresh
        currentIndex = primedIndex
        storeDetails = primedStoreDetails
        playerAActive = !playerAActive
        primedIndex = -1
        primedStoreDetails = {}
    }

    function loadCurrentDetails() {
        storeDetails = hero.currentGame ? gameLibrary.storeDetailsFor(hero.currentGame.appId) : {}
        if (hero.currentGame)
            gameLibrary.requestStoreDetails(hero.currentGame.appId)
    }

    // Reset to a valid index if the hero list shrinks/reorders under us --
    // and on the very first population, jump straight to a random game
    // instead of settling on index 0.
    onGamesChanged: {
        if (!heroStartRandomized && games.length > 0) {
            heroStartRandomized = true
            currentIndex = Math.floor(Math.random() * games.length)
        } else if (currentIndex >= games.length) {
            currentIndex = 0
        }
        loadCurrentDetails()
    }

    Component.onCompleted: loadCurrentDetails()

    Connections {
        target: gameLibrary
        function onStoreDetailsReady(appId) {
            if (hero.currentGame && hero.currentGame.appId === appId)
                hero.storeDetails = gameLibrary.storeDetailsFor(appId)
            if (hero.primedGame && hero.primedGame.appId === appId)
                hero.primedStoreDetails = gameLibrary.storeDetailsFor(appId)
        }
    }

    function launchOrInstall() {
        if (!currentGame)
            return
        if (currentGame.installed) {
            // Prefer Steam's -applaunch CLI over the steam:// URL scheme --
            // skips the extra hand-off hop the URL scheme goes through.
            // Falls back to the URL scheme if `steam` isn't on PATH.
            if (!gameLibrary.launchGameViaSteamCli(currentGame.appId))
                Qt.openUrlExternally("steam://rungameid/" + currentGame.appId)
        } else {
            Qt.openUrlExternally("steam://install/" + currentGame.appId)
        }
    }

    // Static box-art backdrop -- the true fallback for whenever neither
    // player is actively playing (no trailer available yet, or the primed
    // one didn't finish buffering in time).
    Image {
        id: backdrop
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        visible: playerA.playbackState !== MediaPlayer.PlayingState
                 && playerB.playbackState !== MediaPlayer.PlayingState
        source: hero.currentGame && hero.currentGame.boxArtPath.length > 0
                ? ("file://" + hero.currentGame.boxArtPath)
                : ""

        Rectangle {
            anchors.fill: parent
            visible: backdrop.status !== Image.Ready
            color: Dex.Theme.surface
        }
    }

    // Two players, alternating as the "on screen" one. Whichever is
    // inactive is the one being primed with the next game's trailer, so at
    // rotation time we just flip which one is visible instead of loading a
    // fresh source and waiting.
    MediaPlayer {
        id: playerA
        source: hero.playerAActive
                ? (hero.trailerUrl.length > 0 ? hero.trailerUrl : "")
                : (hero.primedTrailerUrl.length > 0 ? hero.primedTrailerUrl : "")
        loops: MediaPlayer.Infinite
        audioOutput: AudioOutput { volume: 0 }
        videoOutput: outputA
        property bool skippedIntro: false
        onSourceChanged: {
            skippedIntro = false
            if (source.toString().length === 0)
                stop()
            else if (!hero.suspended)
                play()
        }
        onMediaStatusChanged: {
            // Most game trailers open with a slow logo/studio-card intro --
            // jump into the actual footage instead of showing that every
            // time. If a trailer's too short for this to make sense, skip
            // it.
            if (mediaStatus === MediaPlayer.Loaded && !skippedIntro) {
                skippedIntro = true
                const skipTo = Math.min(12000, Math.max(0, duration - 6000))
                if (skipTo > 0)
                    position = skipTo
            }
        }
    }

    VideoOutput {
        id: outputA
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        visible: hero.playerAActive && playerA.playbackState === MediaPlayer.PlayingState
    }

    MediaPlayer {
        id: playerB
        source: !hero.playerAActive
                ? (hero.trailerUrl.length > 0 ? hero.trailerUrl : "")
                : (hero.primedTrailerUrl.length > 0 ? hero.primedTrailerUrl : "")
        loops: MediaPlayer.Infinite
        audioOutput: AudioOutput { volume: 0 }
        videoOutput: outputB
        property bool skippedIntro: false
        onSourceChanged: {
            skippedIntro = false
            if (source.toString().length === 0)
                stop()
            else if (!hero.suspended)
                play()
        }
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.Loaded && !skippedIntro) {
                skippedIntro = true
                const skipTo = Math.min(12000, Math.max(0, duration - 6000))
                if (skipTo > 0)
                    position = skipTo
            }
        }
    }

    VideoOutput {
        id: outputB
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        visible: !hero.playerAActive && playerB.playbackState === MediaPlayer.PlayingState
    }

    // Gradient scrim so title/button text stays legible over any artwork
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "#00000000" }
            GradientStop { position: 0.55; color: "#66000000" }
            GradientStop { position: 1.0; color: Dex.Theme.background }
        }
    }

    Column {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: Dex.Theme.spacingLarge
        spacing: Dex.Theme.spacingMedium

        Text {
            text: hero.currentGame ? hero.currentGame.name : ""
            color: Dex.Theme.textPrimary
            font.pixelSize: Dex.Theme.fontSizeLarge
        }

        Rectangle {
            id: playButton
            width: playRow.implicitWidth + 48
            height: 44
            radius: Dex.Theme.radiusPill
            color: Dex.Theme.accent
            border.color: Dex.Theme.accentGlow
            border.width: hero.focused ? Dex.Theme.glowRingWidth : 0
            Behavior on border.width { NumberAnimation { duration: 150 } }

            Row {
                id: playRow
                anchors.centerIn: parent
                spacing: 8

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    kind: hero.currentGame && hero.currentGame.installed ? "play" : "install"
                    color: "black"
                    width: 14
                    height: 14
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: hero.currentGame && hero.currentGame.installed ? "Play" : "Install"
                    color: "black"
                    font.pixelSize: Dex.Theme.fontSizeMedium
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: hero.launchOrInstall()
            }
        }
    }
}
