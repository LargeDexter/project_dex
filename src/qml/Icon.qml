import QtQuick

// Small hand-drawn icon set, rendered on a Canvas rather than shipped as
// image assets -- keeps everything self-contained in QML with no extra
// CMake resource wiring. Add new "kind" values here as the icon set grows
// (e.g. the green check / yellow warn / red x used for the hardware
// compatibility indicators).
Canvas {
    id: root

    property string kind: "play"   // play | install | close | check | warn | x | volume | mute | gear | eye | eyeOff
    property color color: "white"

    width: 16
    height: 16

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        ctx.strokeStyle = color
        ctx.fillStyle = color
        ctx.lineWidth = Math.max(1.5, width * 0.11)
        ctx.lineCap = "round"
        ctx.lineJoin = "round"

        const w = width
        const h = height

        switch (kind) {
        case "play":
            ctx.beginPath()
            ctx.moveTo(w * 0.28, h * 0.18)
            ctx.lineTo(w * 0.28, h * 0.82)
            ctx.lineTo(w * 0.82, h * 0.5)
            ctx.closePath()
            ctx.fill()
            break

        case "install":
            ctx.beginPath()
            ctx.moveTo(w * 0.5, h * 0.12)
            ctx.lineTo(w * 0.5, h * 0.58)
            ctx.moveTo(w * 0.28, h * 0.38)
            ctx.lineTo(w * 0.5, h * 0.6)
            ctx.lineTo(w * 0.72, h * 0.38)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(w * 0.18, h * 0.82)
            ctx.lineTo(w * 0.82, h * 0.82)
            ctx.stroke()
            break

        case "close":
            ctx.beginPath()
            ctx.moveTo(w * 0.24, h * 0.24)
            ctx.lineTo(w * 0.76, h * 0.76)
            ctx.moveTo(w * 0.76, h * 0.24)
            ctx.lineTo(w * 0.24, h * 0.76)
            ctx.stroke()
            break

        case "check":
            ctx.beginPath()
            ctx.moveTo(w * 0.16, h * 0.52)
            ctx.lineTo(w * 0.4, h * 0.76)
            ctx.lineTo(w * 0.86, h * 0.24)
            ctx.stroke()
            break

        case "warn":
            ctx.beginPath()
            ctx.moveTo(w * 0.5, h * 0.1)
            ctx.lineTo(w * 0.92, h * 0.84)
            ctx.lineTo(w * 0.08, h * 0.84)
            ctx.closePath()
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(w * 0.5, h * 0.38)
            ctx.lineTo(w * 0.5, h * 0.62)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(w * 0.5, h * 0.72, w * 0.04, 0, 2 * Math.PI)
            ctx.fill()
            break

        case "x":
            ctx.beginPath()
            ctx.arc(w * 0.5, h * 0.5, w * 0.42, 0, 2 * Math.PI)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(w * 0.34, h * 0.34)
            ctx.lineTo(w * 0.66, h * 0.66)
            ctx.moveTo(w * 0.66, h * 0.34)
            ctx.lineTo(w * 0.34, h * 0.66)
            ctx.stroke()
            break

        case "gear":
            // Approximate cog: a ring, 8 short radial teeth, and a filled
            // hub dot -- reads fine as a settings icon at toolbar sizes
            // without needing a precise involute gear profile.
            ctx.beginPath()
            ctx.arc(w * 0.5, h * 0.5, w * 0.26, 0, 2 * Math.PI)
            ctx.stroke()
            for (let i = 0; i < 8; i++) {
                const angle = (i / 8) * Math.PI * 2
                const x1 = w * 0.5 + Math.cos(angle) * w * 0.3
                const y1 = h * 0.5 + Math.sin(angle) * w * 0.3
                const x2 = w * 0.5 + Math.cos(angle) * w * 0.46
                const y2 = h * 0.5 + Math.sin(angle) * w * 0.46
                ctx.beginPath()
                ctx.moveTo(x1, y1)
                ctx.lineTo(x2, y2)
                ctx.stroke()
            }
            ctx.beginPath()
            ctx.arc(w * 0.5, h * 0.5, w * 0.09, 0, 2 * Math.PI)
            ctx.fill()
            break

        // Open eye ("reveal"), shared outline with eyeOff below.
        case "eye":
        case "eyeOff":
            ctx.beginPath()
            ctx.moveTo(w * 0.08, h * 0.5)
            ctx.quadraticCurveTo(w * 0.5, h * 0.18, w * 0.92, h * 0.5)
            ctx.quadraticCurveTo(w * 0.5, h * 0.82, w * 0.08, h * 0.5)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(w * 0.5, h * 0.5, w * 0.14, 0, 2 * Math.PI)
            ctx.fill()
            if (kind === "eyeOff") {
                // A slash through the whole eye shape to read as hidden.
                ctx.beginPath()
                ctx.moveTo(w * 0.16, h * 0.18)
                ctx.lineTo(w * 0.84, h * 0.82)
                ctx.stroke()
            }
            break

        // Speaker body, shared by volume/mute.
        case "volume":
        case "mute":
            ctx.beginPath()
            ctx.moveTo(w * 0.14, h * 0.38)
            ctx.lineTo(w * 0.32, h * 0.38)
            ctx.lineTo(w * 0.52, h * 0.2)
            ctx.lineTo(w * 0.52, h * 0.8)
            ctx.lineTo(w * 0.32, h * 0.62)
            ctx.lineTo(w * 0.14, h * 0.62)
            ctx.closePath()
            ctx.fill()

            if (kind === "volume") {
                // Sound waves fanning out.
                ctx.beginPath()
                ctx.arc(w * 0.52, h * 0.5, w * 0.16, -0.6, 0.6)
                ctx.stroke()
                ctx.beginPath()
                ctx.arc(w * 0.52, h * 0.5, w * 0.28, -0.75, 0.75)
                ctx.stroke()
            } else {
                // A slash through the speaker to read as muted.
                ctx.beginPath()
                ctx.moveTo(w * 0.6, h * 0.22)
                ctx.lineTo(w * 0.92, h * 0.78)
                ctx.stroke()
            }
            break
        }
    }

    onKindChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    Component.onCompleted: requestPaint()
}
