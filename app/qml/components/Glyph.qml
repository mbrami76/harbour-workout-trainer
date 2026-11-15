// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0

// A symbol drawn from shapes at the size it is shown, so it is sharp at any size and needs no image
// file. `kind` is one of play, pause, stop, refresh, add, remove, check, star, next, previous and info.
Canvas {
    id: glyph

    property string kind: "play"
    property color color: "#ffffff"

    onKindChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        var s = Math.min(width, height)
        ctx.reset()
        ctx.clearRect(0, 0, width, height)
        if (s <= 0)
            return
        ctx.translate((width - s) / 2, (height - s) / 2)
        ctx.fillStyle = glyph.color
        ctx.strokeStyle = glyph.color
        ctx.lineWidth = s * 0.12
        ctx.lineCap = "butt"

        if (kind === "play") {
            ctx.beginPath()
            ctx.moveTo(s * 0.22, s * 0.1)
            ctx.lineTo(s * 0.9, s * 0.5)
            ctx.lineTo(s * 0.22, s * 0.9)
            ctx.closePath()
            ctx.fill()
        } else if (kind === "pause") {
            ctx.fillRect(s * 0.2, s * 0.12, s * 0.2, s * 0.76)
            ctx.fillRect(s * 0.6, s * 0.12, s * 0.2, s * 0.76)
        } else if (kind === "stop") {
            ctx.fillRect(s * 0.18, s * 0.18, s * 0.64, s * 0.64)
        } else if (kind === "add") {
            ctx.fillRect(s * 0.12, s * 0.44, s * 0.76, s * 0.12)
            ctx.fillRect(s * 0.44, s * 0.12, s * 0.12, s * 0.76)
        } else if (kind === "remove") {
            ctx.fillRect(s * 0.12, s * 0.44, s * 0.76, s * 0.12)
        } else if (kind === "next") {
            // A triangle pointing right with a bar after it.
            ctx.beginPath()
            ctx.moveTo(s * 0.14, s * 0.12)
            ctx.lineTo(s * 0.66, s * 0.5)
            ctx.lineTo(s * 0.14, s * 0.88)
            ctx.closePath()
            ctx.fill()
            ctx.fillRect(s * 0.72, s * 0.12, s * 0.14, s * 0.76)
        } else if (kind === "previous") {
            ctx.beginPath()
            ctx.moveTo(s * 0.86, s * 0.12)
            ctx.lineTo(s * 0.34, s * 0.5)
            ctx.lineTo(s * 0.86, s * 0.88)
            ctx.closePath()
            ctx.fill()
            ctx.fillRect(s * 0.14, s * 0.12, s * 0.14, s * 0.76)
        } else if (kind === "star") {
            ctx.beginPath()
            for (var i = 0; i < 10; ++i) {
                var radius = (i % 2 === 0) ? s * 0.46 : s * 0.19
                var angle = -Math.PI / 2 + i * Math.PI / 5
                var px = s / 2 + radius * Math.cos(angle)
                var py = s * 0.53 + radius * Math.sin(angle)
                if (i === 0)
                    ctx.moveTo(px, py)
                else
                    ctx.lineTo(px, py)
            }
            ctx.closePath()
            ctx.fill()
        } else if (kind === "check") {
            ctx.lineJoin = "miter"
            ctx.beginPath()
            ctx.moveTo(s * 0.14, s * 0.54)
            ctx.lineTo(s * 0.4, s * 0.8)
            ctx.lineTo(s * 0.88, s * 0.2)
            ctx.stroke()
        } else if (kind === "refresh") {
            // An open circle with an arrow head at its end.
            var cx = s / 2
            var cy = s / 2
            var r = s * 0.34
            var start = -Math.PI * 0.35
            var end = Math.PI * 1.4
            ctx.beginPath()
            ctx.arc(cx, cy, r, start, end, false)
            ctx.stroke()
            var ex = cx + r * Math.cos(start)
            var ey = cy + r * Math.sin(start)
            ctx.beginPath()
            ctx.moveTo(ex - s * 0.04, ey - s * 0.26)
            ctx.lineTo(ex + s * 0.24, ey + s * 0.1)
            ctx.lineTo(ex - s * 0.2, ey + s * 0.12)
            ctx.closePath()
            ctx.fill()
        } else if (kind === "info") {
            // A circle with a dot over a bar.
            ctx.beginPath()
            ctx.arc(s / 2, s / 2, s * 0.4, 0, Math.PI * 2, false)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(s / 2, s * 0.32, s * 0.07, 0, Math.PI * 2, false)
            ctx.fill()
            ctx.fillRect(s * 0.44, s * 0.44, s * 0.12, s * 0.28)
        }
    }
}
