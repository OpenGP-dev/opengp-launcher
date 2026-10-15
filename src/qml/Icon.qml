import QtQuick

// The few icons the launcher needs, drawn (no icon font): plus, gear,
// chevron (down) and cross.
Canvas {
    id: icon
    property string name: "plus"
    property color color: Theme.chalk
    implicitWidth: 18
    implicitHeight: 18
    onColorChanged: requestPaint()
    onNameChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        const w = width, h = height, cx = w / 2, cy = h / 2
        ctx.reset()
        ctx.strokeStyle = color
        ctx.fillStyle = color
        ctx.lineCap = "round"
        ctx.lineJoin = "round"
        ctx.lineWidth = Math.max(2, w / 9)
        ctx.beginPath()
        if (name === "plus") {
            ctx.moveTo(cx, h * 0.15); ctx.lineTo(cx, h * 0.85)
            ctx.moveTo(w * 0.15, cy); ctx.lineTo(w * 0.85, cy)
            ctx.stroke()
        } else if (name === "cross") {
            ctx.moveTo(w * 0.22, h * 0.22); ctx.lineTo(w * 0.78, h * 0.78)
            ctx.moveTo(w * 0.78, h * 0.22); ctx.lineTo(w * 0.22, h * 0.78)
            ctx.stroke()
        } else if (name === "chevron") {
            ctx.moveTo(w * 0.22, h * 0.38); ctx.lineTo(cx, h * 0.64); ctx.lineTo(w * 0.78, h * 0.38)
            ctx.stroke()
        } else if (name === "gear") {
            // Eight teeth around a ring.
            const outer = w * 0.48, inner = w * 0.34, teeth = 8
            for (let i = 0; i < teeth * 2; ++i) {
                const a0 = (i / (teeth * 2)) * Math.PI * 2, a1 = ((i + 1) / (teeth * 2)) * Math.PI * 2
                const r = i % 2 ? inner : outer
                if (i === 0) ctx.moveTo(cx + r * Math.cos(a0), cy + r * Math.sin(a0))
                ctx.lineTo(cx + r * Math.cos(a0), cy + r * Math.sin(a0))
                ctx.lineTo(cx + r * Math.cos(a1), cy + r * Math.sin(a1))
            }
            ctx.closePath()
            ctx.fill()
            ctx.globalCompositeOperation = "destination-out"
            ctx.beginPath()
            ctx.arc(cx, cy, w * 0.15, 0, Math.PI * 2)
            ctx.fill()
            ctx.globalCompositeOperation = "source-over"
        }
    }
}
