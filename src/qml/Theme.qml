pragma Singleton
import QtQuick

QtObject {
    // Dark background, "in-between" tone (not harsh grid, not pure minimal)
    readonly property color background: "#0e0e12"
    readonly property color surface: "#1a1a20"
    readonly property color surfaceHighlight: "#242430"

    // Signature orange accent -- pill play button, glow-ring selection
    readonly property color accent: "#e8823c"
    readonly property color accentGlow: "#e8823c66" // translucent, for the ring

    readonly property color textPrimary: "#f2f2f0"
    readonly property color textSecondary: "#9a9a96"

    // Rounder edges throughout, not sharp corners
    readonly property int radiusSmall: 8
    readonly property int radiusMedium: 14
    readonly property int radiusPill: 999

    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 32

    readonly property int fontSizeSmall: 13
    readonly property int fontSizeMedium: 18
    readonly property int fontSizeLarge: 28

    readonly property int glowRingWidth: 3
}
