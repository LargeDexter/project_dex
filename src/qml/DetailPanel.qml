import QtQuick
import ProjectDex as Dex

Rectangle {
    id: root

    property var game: null

    visible: opacity > 0
    opacity: game ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 200 } }

    width: 320
    height: content.implicitHeight + 32
    radius: Dex.Theme.radiusMedium
    color: Dex.Theme.surface
    border.color: Dex.Theme.accent
    border.width: 1

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

    Column {
        id: content
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        Text {
            width: parent.width
            text: root.game ? root.game.name : ""
            color: Dex.Theme.textPrimary
            font.pixelSize: Dex.Theme.fontSizeMedium
            wrapMode: Text.WordWrap
        }

        Text {
            text: root.game ? (root.game.installed ? "Installed" : "Not installed") : ""
            color: root.game && root.game.installed ? Dex.Theme.accent : Dex.Theme.textSecondary
            font.pixelSize: Dex.Theme.fontSizeSmall
        }

        Text {
            text: root.game ? root.formatPlaytime(root.game.playtime) : ""
            color: Dex.Theme.textSecondary
            font.pixelSize: Dex.Theme.fontSizeSmall
        }

        Text {
            text: root.game ? root.formatLastPlayed(root.game.lastPlayed) : ""
            color: Dex.Theme.textSecondary
            font.pixelSize: Dex.Theme.fontSizeSmall
        }
    }
}
