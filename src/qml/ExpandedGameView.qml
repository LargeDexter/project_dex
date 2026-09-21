import QtQuick
import QtMultimedia
import ProjectDex as Dex

Item {
    id: root

    readonly property bool active: Dex.UiState.expandedAppId >= 0
    readonly property var game: active ? gameLibrary.gameByAppId(Dex.UiState.expandedAppId) : null
    property var storeDetails: ({})
    readonly property string trailerUrl: (storeDetails && storeDetails.available === true)
        ? (storeDetails.trailerUrl || "") : ""

    // ProtonDB's crowd-sourced Linux/Proton compatibility rating -- fetched
    // lazily the same way store details are, and cached in GameLibraryModel
    // so re-opening the same game later doesn't re-hit the network.
    property var protonDbRating: ({})

    // After sitting on this card for a while with no interaction, collapse
    // the text down to just the title + description and let the trailer
    // (if this game has one) take over the cover-art slot.
    property bool trailerMode: false

    // Manual override for trailer mode's collapse -- "Show more" sets this
    // to bring the compat box/specs back while staying in trailer mode
    // (trailer keeps playing above), "Show less" clears it again.
    property bool detailsExpanded: false

    Timer {
        id: dwellTimer
        interval: 10000
        running: root.active
        repeat: false
        onTriggered: root.trailerMode = true
    }

    visible: opacity > 0
    opacity: active ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 200 } }

    onGameChanged: {
        storeDetails = game ? gameLibrary.storeDetailsFor(game.appId) : {}
        if (game)
            gameLibrary.requestStoreDetails(game.appId)
        protonDbRating = game ? gameLibrary.protonDbRatingFor(game.appId) : {}
        if (game)
            gameLibrary.requestProtonDbRating(game.appId)
        root.trailerMode = false
        root.detailsExpanded = false
        dwellTimer.restart()
    }
    onActiveChanged: {
        if (active) {
            Qt.callLater(root.forceActiveFocus)
            root.trailerMode = false
            root.detailsExpanded = false
            dwellTimer.restart()
        } else {
            dwellTimer.stop()
            root.trailerMode = false
            root.detailsExpanded = false
        }
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Space) {
            Dex.UiState.collapse()
            event.accepted = true
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            // Play/Install was only wired to the pill button's MouseArea --
            // nothing here answered Return, so the gamepad's Confirm button
            // (dispatched as Qt::Key_Return, see gamepad_input.cpp) did
            // nothing while this screen was open. Same action as clicking
            // the pill button or the cover art.
            root.launchOrInstall()
            event.accepted = true
        }
    }

    Connections {
        target: gameLibrary
        function onStoreDetailsReady(appId) {
            if (root.game && root.game.appId === appId)
                root.storeDetails = gameLibrary.storeDetailsFor(appId)
        }
        function onProtonDbRatingReady(appId) {
            if (root.game && root.game.appId === appId)
                root.protonDbRating = gameLibrary.protonDbRatingFor(appId)
        }
    }

    // ProtonDB tier -> display label / badge color. Tier strings come
    // straight from the API (lowercase); colors roughly follow ProtonDB's
    // own badge scheme, adapted for this dark UI.
    function protonDbTierLabel(tier) {
        switch (tier) {
        case "native": return "Native"
        case "platinum": return "Platinum"
        case "gold": return "Gold"
        case "silver": return "Silver"
        case "bronze": return "Bronze"
        case "borked": return "Borked"
        case "pending": return "Pending"
        default: return tier
        }
    }

    function protonDbTierColor(tier) {
        switch (tier) {
        case "native": return "#4CAF50"
        case "platinum": return "#8ecae6"
        case "gold": return "#E0B400"
        case "silver": return "#C0C0C0"
        case "bronze": return "#CD7F32"
        case "borked": return "#E05555"
        default: return "#888888"
        }
    }

    function launchOrInstall() {
        if (!game)
            return
        if (game.installed) {
            // Prefer Steam's -applaunch CLI over the steam:// URL scheme --
            // skips the extra hand-off hop the URL scheme goes through.
            // Falls back to the URL scheme if `steam` isn't on PATH.
            if (!gameLibrary.launchGameViaSteamCli(game.appId))
                Qt.openUrlExternally("steam://rungameid/" + game.appId)
        } else {
            Qt.openUrlExternally("steam://install/" + game.appId)
        }
    }

    function formatPlaytime(minutes) {
        if (!minutes || minutes <= 0)
            return "Never played"
        return (minutes / 60).toFixed(1) + " hrs played"
    }

    function formatLastPlayed(timestamp) {
        if (!timestamp || timestamp <= 0)
            return "Last played: never"
        const d = new Date(timestamp * 1000)
        return "Last played: " + d.toLocaleDateString()
    }

    // Turns Steam's raw requirements HTML (a <ul> of <li><strong>Label:</strong> value</li>)
    // into a plain list of {label, value} rows we can lay out ourselves with
    // a compatibility icon per line.
    function parseRequirementLines(html) {
        if (!html || html.length === 0)
            return []

        const items = []
        const liRegex = /<li>([\s\S]*?)<\/li>/g
        let match
        while ((match = liRegex.exec(html)) !== null) {
            const chunk = match[1]
            let label = ""
            let value = chunk

            const strongMatch = /<strong>([\s\S]*?)<\/strong>/.exec(chunk)
            if (strongMatch) {
                label = strongMatch[1].replace(/<[^>]*>/g, "").replace(/:\s*$/, "").trim()
                value = chunk.slice(strongMatch.index + strongMatch[0].length)
            }

            value = value
                .replace(/<br\s*\/?>/gi, "")
                .replace(/<[^>]*>/g, "")
                .replace(/&nbsp;/gi, " ")
                .replace(/&amp;/gi, "&")
                .trim()

            if (label.length > 0 || value.length > 0)
                items.push({ label: label, value: value })
        }
        return items
    }

    // Green/yellow/red verdict for a single parsed spec line, compared
    // against this machine's scanned hardware (see SystemSpecs). Returns ""
    // when there's nothing to compare (e.g. OS/DirectX/Storage lines, or a
    // model string the bundled reference table doesn't recognize).
    function matchLevelFor(label, value) {
        return Dex.SystemSpecs.matchLevel(label, value)
    }

    // One overall verdict per category (CPU/GPU/RAM) for the summary box at
    // the top of the card -- prefers the Recommended-tier line's verdict,
    // falling back to Minimum when Recommended has no line or no verdict.
    // labelKeys is an array of lowercase substrings to match against a
    // parsed line's label (e.g. ["memory", "ram"]).
    function categoryVerdict(labelKeys) {
        function findLine(items) {
            for (let i = 0; i < items.length; i++) {
                const lbl = items[i].label.toLowerCase()
                for (let k = 0; k < labelKeys.length; k++) {
                    if (lbl.indexOf(labelKeys[k]) >= 0)
                        return items[i]
                }
            }
            return null
        }

        let line = findLine(rightPane.recommendedItems)
        let verdict = line ? root.matchLevelFor(line.label, line.value) : ""
        if (verdict.length > 0)
            return verdict

        line = findLine(rightPane.minimumItems)
        return line ? root.matchLevelFor(line.label, line.value) : ""
    }

    // Dimmed backdrop -- click anywhere on it to close.
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.8

        MouseArea {
            anchors.fill: parent
            onClicked: Dex.UiState.collapse()
        }
    }

    // Left pane: box art + play button. Glides in from off-screen left.
    // Sized down from its original 0.32 so the details card has more room.
    Column {
        id: leftPane
        width: parent.width * 0.22
        spacing: Dex.Theme.spacingLarge
        anchors.verticalCenter: parent.verticalCenter
        x: root.active ? parent.width * 0.05 : -width
        Behavior on x { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }

        // Cover art stays cover art -- the trailer plays inside the
        // details card on the right instead (see rightContent below).
        Image {
            id: coverArt
            width: parent.width
            height: width * 1.5
            source: root.game && root.game.boxArtPath.length > 0 ? ("file://" + root.game.boxArtPath) : ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true

            // Clicking the cover art does the same thing as the pill button
            // below it (play if installed, install if not).
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.launchOrInstall()
            }
        }

        Rectangle {
            width: playRow.implicitWidth + 48
            height: 44
            radius: Dex.Theme.radiusPill
            color: Dex.Theme.accent

            Row {
                id: playRow
                anchors.centerIn: parent
                spacing: 8

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    kind: root.game && root.game.installed ? "play" : "install"
                    color: "black"
                    width: 14
                    height: 14
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.game && root.game.installed ? "Play" : "Install"
                    color: "black"
                    font.pixelSize: Dex.Theme.fontSizeMedium
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.launchOrInstall()
            }
        }
    }

    // Right pane: name, status, description, requirements. Glides in from
    // off-screen right. Styled as a solid card, same look as the brief
    // corner panel (surface fill, accent border) rather than floating text.
    Rectangle {
        id: rightPane
        width: parent.width * 0.56
        height: rightContent.implicitHeight + 32
        radius: Dex.Theme.radiusMedium
        color: Dex.Theme.surface
        border.color: Dex.Theme.accent
        border.width: 1
        anchors.verticalCenter: parent.verticalCenter
        x: root.active ? parent.width * 0.34 : parent.width
        Behavior on x { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }

        readonly property bool detailsLoading: root.game && Object.keys(root.storeDetails).length === 0
        readonly property bool detailsUnavailable: root.storeDetails && root.storeDetails.available === false
        readonly property bool detailsAvailable: root.storeDetails && root.storeDetails.available === true

        readonly property var minimumItems: detailsAvailable
            ? root.parseRequirementLines(root.storeDetails.minimumRequirements) : []
        readonly property var recommendedItems: detailsAvailable
            ? root.parseRequirementLines(root.storeDetails.recommendedRequirements) : []

        Column {
            id: rightContent
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 16
            spacing: 8

            Row {
                width: parent.width
                spacing: 16

                Column {
                    // Leaves room for the right-side badge stack only once
                    // something's actually showing there, so the title gets
                    // full width while loading.
                    width: (compatBox.visible || protonDbBadge.visible)
                           ? parent.width - rightBadgeColumn.width - parent.spacing : parent.width
                    spacing: 8

                    Text {
                        width: parent.width
                        text: root.game ? root.game.name : ""
                        color: Dex.Theme.textPrimary
                        font.pixelSize: Dex.Theme.fontSizeLarge
                        wrapMode: Text.WordWrap
                    }

                    // Status lines drop out once the card has shrunk to
                    // trailer mode -- unless expanded back open via "Show
                    // more" below.
                    Text {
                        visible: !root.trailerMode || root.detailsExpanded
                        text: root.game ? (root.game.installed ? "Installed" : "Not installed") : ""
                        color: root.game && root.game.installed ? Dex.Theme.accent : Dex.Theme.textSecondary
                        font.pixelSize: Dex.Theme.fontSizeSmall
                    }

                    Text {
                        visible: !root.trailerMode || root.detailsExpanded
                        text: root.game ? root.formatPlaytime(root.game.playtime) : ""
                        color: Dex.Theme.textSecondary
                        font.pixelSize: Dex.Theme.fontSizeSmall
                    }

                    Text {
                        visible: !root.trailerMode || root.detailsExpanded
                        text: root.game ? root.formatLastPlayed(root.game.lastPlayed) : ""
                        color: Dex.Theme.textSecondary
                        font.pixelSize: Dex.Theme.fontSizeSmall
                    }
                }

                // Right-side badge stack: the CPU/GPU/RAM compat summary,
                // and the ProtonDB rating underneath it once one's
                // available. Both hidden in trailer mode (unless expanded
                // back open) along with the rest of the spec detail.
                Column {
                    id: rightBadgeColumn
                    spacing: 8

                // At-a-glance compatibility summary -- always shown for now
                // (a natural candidate for a settings toggle later). One
                // verdict per category, stacked in a compact corner box
                // rather than a full-width bar.
                Rectangle {
                    id: compatBox
                    visible: rightPane.detailsAvailable && (!root.trailerMode || root.detailsExpanded)
                    width: 96
                    height: compatColumn.implicitHeight + 16
                    radius: Dex.Theme.radiusSmall
                    color: Qt.darker(Dex.Theme.surface, 1.3)
                    border.color: Dex.Theme.accent
                    border.width: 1

                    Column {
                        id: compatColumn
                        anchors.centerIn: parent
                        spacing: 6

                        Repeater {
                            model: [
                                { label: "CPU", keys: ["processor", "cpu"] },
                                { label: "GPU", keys: ["graphics", "video"] },
                                { label: "RAM", keys: ["memory", "ram"] }
                            ]

                            delegate: Row {
                                spacing: 6
                                readonly property string verdict: root.categoryVerdict(modelData.keys)

                                Text {
                                    text: modelData.label + ":"
                                    color: Dex.Theme.textPrimary
                                    font.pixelSize: Dex.Theme.fontSizeSmall
                                    font.bold: true
                                }

                                Icon {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: parent.verdict.length > 0
                                    kind: parent.verdict.length > 0 ? parent.verdict : "check"
                                    color: parent.verdict === "check" ? "#4CAF50"
                                           : (parent.verdict === "warn" ? "#E0B400" : "#E05555")
                                    width: 14
                                    height: 14
                                }

                                Text {
                                    visible: parent.verdict.length === 0
                                    text: "–"
                                    color: Dex.Theme.textSecondary
                                    font.pixelSize: Dex.Theme.fontSizeSmall
                                }
                            }
                        }
                    }
                }

                // ProtonDB badge -- how well this game runs on Linux via
                // Proton, per the community's own reports. Only shown once
                // a rating actually comes back (native Linux games and
                // very obscure ones often have none).
                Rectangle {
                    id: protonDbBadge
                    visible: (!root.trailerMode || root.detailsExpanded) && root.protonDbRating.available === true
                    width: protonDbLabel.implicitWidth + 16
                    height: protonDbLabel.implicitHeight + 6
                    radius: height / 2
                    color: root.protonDbTierColor(root.protonDbRating.tier)

                    Text {
                        id: protonDbLabel
                        anchors.centerIn: parent
                        text: "ProtonDB: " + root.protonDbTierLabel(root.protonDbRating.tier)
                        color: "black"
                        font.pixelSize: Dex.Theme.fontSizeSmall
                        font.bold: true
                    }
                }
                }
            }

            // Trailer, once trailerMode kicks in -- plays inside the card
            // itself, full card width at a proper 16:9 height, above the
            // (now-minimal) description text, rather than in the cover art
            // slot. Click to toggle sound (it plays muted by default, like
            // the hero banner).
            Item {
                id: trailerBox
                visible: root.trailerMode && root.trailerUrl.length > 0
                width: parent.width
                height: visible ? Math.round(width * 9 / 16) : 0

                MediaPlayer {
                    id: detailTrailerPlayer
                    source: trailerBox.visible ? root.trailerUrl : ""
                    loops: MediaPlayer.Infinite
                    audioOutput: AudioOutput {
                        id: detailTrailerAudio
                        volume: 1
                        muted: true
                    }
                    videoOutput: detailTrailerOutput
                    onSourceChanged: source.toString().length > 0 ? play() : stop()
                }

                VideoOutput {
                    id: detailTrailerOutput
                    anchors.fill: parent
                    fillMode: VideoOutput.PreserveAspectFit
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: detailTrailerAudio.muted = !detailTrailerAudio.muted
                }

                // Small mute/unmute indicator, bottom-right corner.
                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: "#B0000000"
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10

                    Icon {
                        anchors.centerIn: parent
                        kind: detailTrailerAudio.muted ? "mute" : "volume"
                        color: "white"
                        width: 16
                        height: 16
                    }
                }
            }

            Text {
                visible: rightPane.detailsLoading
                text: "Loading details..."
                color: Dex.Theme.textSecondary
                font.pixelSize: 12
                topPadding: 12
            }

            Text {
                visible: rightPane.detailsUnavailable
                width: parent.width
                text: "No additional details available from Steam for this app."
                color: Dex.Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                topPadding: 12
            }

            Column {
                visible: rightPane.detailsAvailable
                width: parent.width
                spacing: 4
                topPadding: 12

                Text {
                    text: "Description"
                    color: Dex.Theme.textPrimary
                    font.pixelSize: Dex.Theme.fontSizeSmall
                }

                Text {
                    width: parent.width
                    text: rightPane.detailsAvailable ? root.storeDetails.shortDescription : ""
                    color: Dex.Theme.textSecondary
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                // In trailer mode, this is how you get the compat box and
                // specs back without leaving trailer mode (or waiting for
                // it to time out) -- everything gated on trailerMode below
                // also checks detailsExpanded now.
                Text {
                    visible: root.trailerMode
                    text: root.detailsExpanded ? "Show less" : "Show more"
                    color: Dex.Theme.accent
                    font.pixelSize: 12
                    font.underline: true
                    topPadding: 4

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.detailsExpanded = !root.detailsExpanded
                    }
                }

                // ---- Minimum / Recommended, side by side ----
                // All of this drops out in trailer mode (unless expanded
                // back open via "Show more" above), leaving just the title
                // + description.
                Item {
                    visible: (!root.trailerMode || root.detailsExpanded) && (rightPane.minimumItems.length > 0 || rightPane.recommendedItems.length > 0)
                    width: 1
                    height: visible ? 10 : 0
                }

                Rectangle {
                    visible: (!root.trailerMode || root.detailsExpanded) && (rightPane.minimumItems.length > 0 || rightPane.recommendedItems.length > 0)
                    width: parent.width
                    height: 1
                    color: Dex.Theme.accent
                    opacity: 0.25
                }

                Text {
                    visible: (!root.trailerMode || root.detailsExpanded) && (rightPane.minimumItems.length > 0 || rightPane.recommendedItems.length > 0)
                    text: "Estimated match to your hardware"
                    color: Dex.Theme.textSecondary
                    font.pixelSize: 10
                    font.italic: true
                    opacity: 0.7
                }

                Item {
                    visible: (!root.trailerMode || root.detailsExpanded) && (rightPane.minimumItems.length > 0 || rightPane.recommendedItems.length > 0)
                    width: 1
                    height: visible ? 6 : 0
                }

                Row {
                    visible: !root.trailerMode || root.detailsExpanded
                    width: rightContent.width
                    spacing: 20

                    // Minimum column
                    Column {
                        width: (parent.width - parent.spacing) / 2
                        spacing: 4
                        visible: rightPane.minimumItems.length > 0

                        Row {
                            spacing: 8
                            Text {
                                text: "SPECS"
                                color: Dex.Theme.textPrimary
                                font.pixelSize: Dex.Theme.fontSizeSmall
                                font.bold: true
                                font.letterSpacing: 1
                            }
                            Text {
                                text: "· Minimum"
                                color: Dex.Theme.textSecondary
                                font.pixelSize: Dex.Theme.fontSizeSmall
                            }
                        }

                        Repeater {
                            model: rightPane.minimumItems

                            delegate: Row {
                                width: parent ? parent.width : 0
                                spacing: 6

                                Text {
                                    id: minLabel
                                    width: parent.width - minIcon.width - parent.spacing
                                    textFormat: Text.RichText
                                    text: (modelData.label.length > 0 ? ("<b>" + modelData.label + ":</b> ") : "")
                                          + modelData.value
                                    color: Dex.Theme.textSecondary
                                    font.pixelSize: 12
                                    wrapMode: Text.WordWrap
                                }

                                Icon {
                                    id: minIcon
                                    anchors.top: minLabel.top
                                    anchors.topMargin: 2
                                    kind: root.matchLevelFor(modelData.label, modelData.value)
                                    visible: kind.length > 0
                                    color: kind === "check" ? "#4CAF50" : (kind === "warn" ? "#E0B400" : "#E05555")
                                    width: 12
                                    height: 12
                                }
                            }
                        }
                    }

                    // Recommended column
                    Column {
                        width: (parent.width - parent.spacing) / 2
                        spacing: 4
                        visible: rightPane.recommendedItems.length > 0

                        Row {
                            spacing: 8
                            Text {
                                text: "SPECS"
                                color: Dex.Theme.textPrimary
                                font.pixelSize: Dex.Theme.fontSizeSmall
                                font.bold: true
                                font.letterSpacing: 1
                            }
                            Text {
                                text: "· Rec"
                                color: Dex.Theme.accent
                                font.pixelSize: Dex.Theme.fontSizeSmall
                            }
                        }

                        Repeater {
                            model: rightPane.recommendedItems

                            delegate: Row {
                                width: parent ? parent.width : 0
                                spacing: 6

                                Text {
                                    id: recLabel
                                    width: parent.width - recIcon.width - parent.spacing
                                    textFormat: Text.RichText
                                    text: (modelData.label.length > 0 ? ("<b>" + modelData.label + ":</b> ") : "")
                                          + modelData.value
                                    color: Dex.Theme.textSecondary
                                    font.pixelSize: 12
                                    wrapMode: Text.WordWrap
                                }

                                Icon {
                                    id: recIcon
                                    anchors.top: recLabel.top
                                    anchors.topMargin: 2
                                    kind: root.matchLevelFor(modelData.label, modelData.value)
                                    visible: kind.length > 0
                                    color: kind === "check" ? "#4CAF50" : (kind === "warn" ? "#E0B400" : "#E05555")
                                    width: 12
                                    height: 12
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Close button, top-right.
    Rectangle {
        width: 36
        height: 36
        radius: 18
        color: Dex.Theme.surface
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Dex.Theme.spacingLarge

        Icon {
            anchors.centerIn: parent
            kind: "close"
            color: Dex.Theme.textPrimary
            width: 14
            height: 14
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: Dex.UiState.collapse()
        }
    }
}
