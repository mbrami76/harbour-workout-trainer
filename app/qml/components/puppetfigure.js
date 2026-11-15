// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

.pragma library

// The puppet figure for a 2D canvas context (R-MUS-1). All shapes are in a 128 x 118 unit box that
// is scaled to fit. The tables below give, for every muscle group key, the face of the figure
// that shows it and its region; the keys are those of the core.

var CX = 64, A_T = 6.5

var REG = {
    torso: {
        f: [["upper-chest", 33.1, 38.5, -1, 1], ["lower-chest", 38.5, 43.5, -1, 1], ["core", 43.5, 52.1, -0.6, 0.6],
            ["obliques", 43.5, 52.1, -1, -0.6], ["obliques", 43.5, 52.1, 0.6, 1]],
        b: [["trapezius", 33.1, 41, -1, 1], ["mid-back", 41, 52.1, -0.45, 0.45], ["lats", 41, 52.1, -1, -0.45], ["lats", 41, 52.1, 0.45, 1]],
        sf: [["obliques", 43.5, 52.1]], sb: [["lats", 41, 52.1]]
    },
    pelvis: { f: [["lower-abdomen", 55.8, 62.8]], b: [["lower-back", 55.8, 62.8]] },
    arm: { f: [["biceps", 41, 51]], b: [["triceps", 41, 51]], a: [["shoulders", 30, 41], ["forearms", 51, 76]] },
    leg: { f: [["quadriceps", 64, 94], ["shins", 94, 115]], b: [["glutes", 64, 80], ["hamstrings", 80, 94], ["calves", 94, 115]] }
}
var ARM = [[35.1, 13.7], [51.1, 18.7], [67.1, 21.7]]

function zfAt(y) { var t = (y - 33.1) / 19; return (t < 0.22 ? 5.5 + 2 * t / 0.22 : 7.5 - 2.5 * (t - 0.22) / 0.78) - 1.5 }
function zbAt(y) { var t = (y - 33.1) / 19; return 3.5 + 0.5 * t - 1.5 }

// Visible part of the front half of a cylinder turned by t, as [lo, hi] in units of the radius.
function halfRange(t) {
    t = ((t + Math.PI) % (2 * Math.PI) + 2 * Math.PI) % (2 * Math.PI) - Math.PI
    if (Math.abs(t) >= Math.PI - 1e-6) return null
    var ct = Math.cos(t)
    return t >= 0 ? [-ct, 1] : [-1, ct]
}

function armAxis(y, s, c, sn) {
    var off
    if (y <= ARM[0][0]) off = ARM[0][1]
    else if (y >= ARM[2][0]) off = ARM[2][1]
    else {
        var i = y < ARM[1][0] ? 0 : 1, a = ARM[i], b = ARM[i + 1]
        off = a[1] + (b[1] - a[1]) * (y - a[0]) / (b[0] - a[0])
    }
    return CX + s * off * c - 2 * sn
}

// Closed band between two x functions over a y range.
function band(y0, y1, fx, n) {
    n = n || 8
    var L = [], R = []
    for (var i = 0; i <= n; i++) {
        var y = y0 + (y1 - y0) * i / n, ab = fx(y)
        L.push([Math.min(ab[0], ab[1]), y])
        R.push([Math.max(ab[0], ab[1]), y])
    }
    return L.concat(R.reverse())
}

function polyPath(ctx, pts) {
    ctx.moveTo(pts[0][0], pts[0][1])
    for (var i = 1; i < pts.length; i++) ctx.lineTo(pts[i][0], pts[i][1])
    ctx.closePath()
}

function area(pts) {
    var a = 0
    for (var i = 0; i < pts.length; i++) { var j = (i + 1) % pts.length; a += pts[i][0] * pts[j][1] - pts[j][0] * pts[i][1] }
    return a
}

// Outline of a polyline with round caps and joins, as a union of capsules with one orientation.
function capsulesPath(ctx, pts, w) {
    var r = w / 2
    for (var i = 0; i < pts.length; i++) {
        ctx.moveTo(pts[i][0] + r, pts[i][1])
        ctx.arc(pts[i][0], pts[i][1], r, 0, 2 * Math.PI, false)
        ctx.closePath()
    }
    for (var k = 0; k + 1 < pts.length; k++) {
        var a = pts[k], b = pts[k + 1], dx = b[0] - a[0], dy = b[1] - a[1], d = Math.sqrt(dx * dx + dy * dy)
        var nx = -dy / d * r, ny = dx / d * r
        var q = [[a[0] + nx, a[1] + ny], [b[0] + nx, b[1] + ny], [b[0] - nx, b[1] - ny], [a[0] - nx, a[1] - ny]]
        if (area(q) < 0) q.reverse()
        // arc(..., false) runs clockwise on screen, which is positive area here
        polyPath(ctx, q)
    }
}

function shape(ctx, p, col) {
    ctx.fillStyle = col
    ctx.strokeStyle = col
    ctx.lineJoin = "round"
    ctx.lineCap = "round"
    if (p.k === "p") {
        ctx.lineWidth = 3
        ctx.beginPath(); polyPath(ctx, p.pts); ctx.fill(); ctx.stroke()
    } else if (p.k === "c") {
        ctx.beginPath(); ctx.arc(p.x, p.y, p.r, 0, 2 * Math.PI, false); ctx.fill()
    } else if (p.k === "r") {
        roundRect(ctx, p.x, p.y, p.w, p.h, 2.5); ctx.fill()
    } else {
        ctx.lineWidth = p.w
        ctx.beginPath(); ctx.moveTo(p.pts[0][0], p.pts[0][1])
        for (var i = 1; i < p.pts.length; i++) ctx.lineTo(p.pts[i][0], p.pts[i][1])
        ctx.stroke()
    }
}

function roundRect(ctx, x, y, w, h, r) {
    ctx.beginPath()
    ctx.moveTo(x + r, y)
    ctx.lineTo(x + w - r, y); ctx.arc(x + w - r, y + r, r, -Math.PI / 2, 0, false)
    ctx.lineTo(x + w, y + h - r); ctx.arc(x + w - r, y + h - r, r, 0, Math.PI / 2, false)
    ctx.lineTo(x + r, y + h); ctx.arc(x + r, y + h - r, r, Math.PI / 2, Math.PI, false)
    ctx.lineTo(x, y + r); ctx.arc(x + r, y + r, r, Math.PI, 3 * Math.PI / 2, false)
    ctx.closePath()
}

function accentPoly(ctx, pts, col, edge) {
    ctx.fillStyle = col
    ctx.strokeStyle = edge
    ctx.lineWidth = 0.7
    ctx.lineJoin = "round"
    ctx.beginPath(); polyPath(ctx, pts); ctx.fill(); ctx.stroke()
}

// CSS text of a colour that is either a string or a QML colour value.
function css(c) {
    if (typeof c === "string") return c
    return "rgba(" + Math.round(c.r * 255) + "," + Math.round(c.g * 255) + "," + Math.round(c.b * 255) + "," + c.a + ")"
}

// sel: object with the selected keys as properties. colors: accent, edge, base, line (CSS strings
// or colour values) and erase (true: the gap around a part is erased, so that the background shows).
function drawFigure(ctx, w, h, ang, sel, rawColors) {
    var colors = { accent: css(rawColors.accent), edge: css(rawColors.edge), base: css(rawColors.base),
                   line: css(rawColors.line), bg: css(rawColors.bg || "#000000"), erase: rawColors.erase }
    var c = Math.cos(ang), sn = Math.sin(ang), parts = []
    var scale = Math.min(w / 128, h / 118)
    ctx.save()
    ctx.translate((w - 128 * scale) / 2, (h - 118 * scale) / 2)
    ctx.scale(scale, scale)

    parts.push({ d: 9, k: "c", x: CX + 1.5 * sn, y: 19.1, r: 8.5, t: null })
    var L = [], R = []
    for (var i = 0; i <= 20; i++) {
        var t = i / 20, y = 33.1 + t * 19
        var zf = (t < 0.22 ? 5.5 + 2 * t / 0.22 : 7.5 - 2.5 * (t - 0.22) / 0.78) - 1.5, zb = 3.5 + 0.5 * t - 1.5
        var lo = 1e9, hi = -1e9
        for (var a = 0; a < 2; a++) {
            var xb = a === 0 ? -A_T : A_T
            for (var b = 0; b < 2; b++) {
                var v = xb * c + (b === 0 ? -zb : zf) * sn
                if (v < lo) lo = v
                if (v > hi) hi = v
            }
        }
        L.push([CX + lo, y]); R.push([CX + hi, y])
    }
    parts.push({ d: 0, k: "p", pts: L.concat(R.reverse()), t: "torso" })
    var pw = 2 * Math.sqrt(64 * c * c + 20.25 * sn * sn)
    parts.push({ d: 0, k: "r", x: CX - pw / 2, y: 55.8, w: pw, h: 7, t: "pelvis" })
    for (var si = 0; si < 2; si++) {
        var s = si === 0 ? -1 : 1
        var ap = []
        for (var m = 0; m < 3; m++) ap.push([CX + s * ARM[m][1] * c - 2 * sn, ARM[m][0]])
        parts.push({ d: -s * sn * 13.7, k: "l", w: 7, pts: ap, t: "arm", s: s })
        var lx = CX + s * 6 * c
        parts.push({ d: -s * sn * 6, x0: lx, k: "l", w: 10, pts: [[lx, 70], [lx, 108]], t: "leg" })
    }
    for (var n = 0; n < parts.length; n++) parts[n].i = n
    parts.sort(function (p, q) { return p.d !== q.d ? p.d - q.d : p.i - q.i })

    ctx.strokeStyle = colors.line; ctx.lineWidth = 1.6; ctx.lineCap = "round"
    ctx.beginPath(); ctx.moveTo(14, 116); ctx.lineTo(114, 116); ctx.stroke()

    for (var pi = 0; pi < parts.length; pi++) {
        var p = parts[pi]
        // Gap around the part: erased, so that what is behind shows on any background.
        var gapColor = colors.erase ? "#000000" : colors.bg
        if (colors.erase) ctx.globalCompositeOperation = "destination-out"
        if (p.k === "p") {
            ctx.fillStyle = gapColor; ctx.strokeStyle = gapColor; ctx.lineWidth = 5.4; ctx.lineJoin = "round"
            ctx.beginPath(); polyPath(ctx, p.pts); ctx.fill(); ctx.stroke()
        } else if (p.k === "r") {
            ctx.fillStyle = gapColor
            roundRect(ctx, p.x - 1.2, p.y - 1.2, p.w + 2.4, p.h + 2.4, 3.7); ctx.fill()
        } else if (p.k === "l") {
            ctx.strokeStyle = gapColor; ctx.lineWidth = p.w + 2.4; ctx.lineJoin = "round"; ctx.lineCap = "round"
            ctx.beginPath(); ctx.moveTo(p.pts[0][0], p.pts[0][1])
            for (var q = 1; q < p.pts.length; q++) ctx.lineTo(p.pts[q][0], p.pts[q][1])
            ctx.stroke()
        }
        ctx.globalCompositeOperation = "source-over"
        shape(ctx, p, colors.base)
        if (!p.t) continue
        highlights(ctx, p, c, sn, ang, sel, colors)
    }
    ctx.restore()
}

function highlights(ctx, p, c, sn, ang, sel, colors) {
    var r, i, j, a, f
    if (p.t === "torso") {
        var R = REG.torso
        f = c > 0 ? "f" : "b"
        var list = R[f]
        for (i = 0; i < list.length; i++) {
            r = list[i]
            if (!sel[r[0]]) continue
            accentPoly(ctx, band(r[1], r[2], function (y) { var z = c > 0 ? zfAt(y) : -zbAt(y); return [CX + r[3] * A_T * c + z * sn, CX + r[4] * A_T * c + z * sn] }), colors.accent, colors.edge)
        }
        for (var sd = -1; sd <= 1; sd += 2) {
            if (sd * sn >= 0) continue
            for (j = 0; j < R.sf.length; j++) {
                r = R.sf[j]
                if (sel[r[0]]) accentPoly(ctx, band(r[1], r[2], function (y) { return [CX + sd * A_T * c, CX + sd * A_T * c + zfAt(y) * sn] }), colors.accent, colors.edge)
            }
            for (j = 0; j < R.sb.length; j++) {
                r = R.sb[j]
                if (sel[r[0]]) accentPoly(ctx, band(r[1], r[2], function (y) { return [CX + sd * A_T * c - zbAt(y) * sn, CX + sd * A_T * c] }), colors.accent, colors.edge)
            }
        }
        return
    }
    if (p.t === "pelvis") {
        f = c > 0 ? "f" : "b"
        var z = c > 0 ? 4.5 : -4.5
        var pl = REG.pelvis[f]
        for (i = 0; i < pl.length; i++) {
            r = pl[i]
            if (!sel[r[0]]) continue
            a = -8 * c + z * sn
            var b2 = 8 * c + z * sn
            accentPoly(ctx, band(r[1], r[2], function () { return [CX + a, CX + b2] }, 1), colors.accent, colors.edge)
        }
        return
    }
    var clips = [], rg
    if (p.t === "leg") {
        var fs = ["f", "b"]
        for (i = 0; i < 2; i++) {
            rg = halfRange(ang + (i === 1 ? Math.PI : 0))
            if (!rg) continue
            var ll = REG.leg[fs[i]]
            for (j = 0; j < ll.length; j++) {
                r = ll[j]
                if (sel[r[0]]) clips.push(band(r[1], r[2], function () { return [p.x0 + rg[0] * 5.2, p.x0 + rg[1] * 5.2] }, 1))
            }
        }
    } else {
        var addArm = function (reg, lohi) {
            if (!sel[reg[0]]) return
            clips.push(band(reg[1], reg[2], function (y) { var x = armAxis(y, p.s, c, sn); return [x + lohi[0] * 3.8, x + lohi[1] * 3.8] }, 6))
        }
        var fk = ["f", "b"]
        for (i = 0; i < 2; i++) {
            rg = halfRange(ang + (i === 1 ? Math.PI : 0))
            if (rg) { var al = REG.arm[fk[i]]; for (j = 0; j < al.length; j++) addArm(al[j], rg) }
        }
        for (j = 0; j < REG.arm.a.length; j++) addArm(REG.arm.a[j], [-1.2, 1.2])
    }
    if (!clips.length) return
    ctx.save()
    ctx.beginPath()
    for (i = 0; i < clips.length; i++) polyPath(ctx, clips[i])
    ctx.clip()
    shape(ctx, p, colors.accent)
    ctx.restore()
    ctx.save()
    ctx.beginPath()
    capsulesPath(ctx, p.pts, p.w)
    ctx.clip()
    ctx.strokeStyle = colors.edge; ctx.lineWidth = 1.1; ctx.lineJoin = "round"
    for (i = 0; i < clips.length; i++) { ctx.beginPath(); polyPath(ctx, clips[i]); ctx.stroke() }
    ctx.restore()
}

// Groups that are drawn on the back of the figure only; every other group shows on the front or on
// both faces.
var BACK = { "trapezius": 1, "mid-back": 1, "lats": 1, "lower-back": 1, "triceps": 1, "glutes": 1, "hamstrings": 1, "calves": 1 }

// "back" for a group that only the back shows, "front" for every other.
function faceFor(key) {
    return BACK[key] ? "back" : "front"
}
