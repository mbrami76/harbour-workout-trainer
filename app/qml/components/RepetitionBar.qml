// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "."

// The bar of one metronome repetition (R-MET-4): the bar of R-UI-7 (same thickness, rounded fill, border)
// fills in a neutral colour as the repetition elapses and starts again at the next one. A thin
// tick on the bar marks the exact start of each phase. Each phase has a diamond with its number
// inside (1 to 8), sized so that the number is 3.5 mm high (R-UI-12). A diamond whose phase has
// not started is an outline with a neutral fill; once the phase has started it is filled with the
// colour of the pulse of its beat. The number is black or white, whichever keeps at least 4.5:1
// with the fill, and the outline in the text colour keeps the diamond itself visible.
//
// The diamonds sit in a row above the bar, so they never hide the fill. When even the exact
// positions of the ticks leave no room for all of them in one row (with a minimum gap), they
// alternate above and below the bar, odd phases above and even phases below. On each side
// diamonds that would still touch are pushed apart to the minimum gap and kept within the ends of the bar;
// each is joined to its tick by a thin line of three segments behind both: vertical from the vertex
// of the diamond that faces the bar, oblique in the room between, vertical again at the tick down to
// the middle of the bar (a straight vertical line when the diamond is directly above or below its tick).
// The bar is half a diamond narrower on each side, so the first diamond is exactly above its start.
// The decision uses only the start shares and the width, so the layout never changes during a run.
Item {
    id: bar

    // Elapsed share of the repetition, 0 to 1.
    property real fraction: 0
    // The start of each phase as a share of the repetition, ascending; [0] for a single beat.
    property var starts: [0]
    // How many phases have started in the running repetition: the first `reached` diamonds are
    // filled; 0 shows none (lead-in, rest).
    property int reached: 0

    readonly property real thickness: Metrics.mm(6.25)
    // The diamond is 6 mm across the diagonal: the number, 3.5 mm high and about 2 mm wide, fits
    // inside with room for the outline.
    readonly property real markWidth: Metrics.mm(6)
    // A square turned by 45 degrees is 1.414 times its side wide.
    readonly property real markSide: markWidth / 1.414
    readonly property real minGap: Metrics.mm(0.8)
    // The room between a row of diamonds and the bar: 2 mm (the gap of R-UI-7), or more when
    // lines join diamonds to ticks, so that the slanted lines can be seen.
    readonly property real gap: hasLines ? Metrics.mm(4) : Metrics.mm(2)
    // The bar is inset by half a diamond on each side, so that a diamond centred on its start or
    // its end stays inside the width of the page and lines up with that end of the bar.
    readonly property real barX: markWidth / 2
    readonly property real barWidth: width - markWidth
    readonly property real inner: barWidth - 2 * strip.borderWidth
    readonly property int count: starts.length
    readonly property real lineWidth: Math.max(1, strip.borderWidth * 0.8)
    // Smallest offset between a diamond and its tick that needs a line.
    readonly property real lineThreshold: Metrics.mm(0.3)

    readonly property real barY: markWidth + gap
    implicitHeight: barY + thickness + (alternating ? gap + markWidth : 0)
    height: implicitHeight

    // The exact position of each tick.
    readonly property var ticks: {
        var x = []
        for (var i = 0; i < count; ++i)
            x.push(barX + strip.borderWidth + inner * Math.max(0, Math.min(1, starts[i])))
        return x
    }

    // Pushes the diamonds of one list of centres apart from the left, and pulls them back from
    // the right so that all stay inside the bar, keeping the gap between neighbours.
    function spread(exact) {
        var step = markWidth + minGap
        var low = barX
        var high = barX + barWidth
        var x = []
        for (var j = 0; j < exact.length; ++j) {
            var v = Math.max(exact[j], low)
            if (j > 0)
                v = Math.max(v, x[j - 1] + step)
            x.push(v)
        }
        for (var k = exact.length - 1; k >= 0; --k) {
            x[k] = Math.min(x[k], high)
            if (k < exact.length - 1)
                x[k] = Math.min(x[k], x[k + 1] - step)
        }
        return x
    }

    // True when a single row cannot hold the diamonds: with the first and the last kept inside
    // the bar, two neighbours would be closer than a diamond plus the gap.
    readonly property bool alternating: {
        if (count < 2)
            return false
        var kept = []
        for (var i = 0; i < count; ++i)
            kept.push(Math.max(barX, Math.min(ticks[i], barX + barWidth)))
        for (var j = 1; j < count; ++j)
            if (kept[j] - kept[j - 1] < markWidth + minGap - 0.5)
                return true
        return false
    }

    // The centre of every diamond. In a single row all diamonds share the layout; when
    // alternating, each side is laid out on its own (odd phases above, even phases below).
    readonly property var centres: {
        if (!alternating)
            return spread(ticks)
        var above = [], below = []
        for (var i = 0; i < count; ++i)
            (i % 2 === 0 ? above : below).push(ticks[i])
        var a = spread(above), b = spread(below)
        var x = []
        for (var j = 0; j < count; ++j)
            x.push(j % 2 === 0 ? a[j / 2] : b[(j - 1) / 2])
        return x
    }

    readonly property bool hasLines: {
        for (var i = 0; i < count; ++i)
            if (Math.abs(centres[i] - ticks[i]) > lineThreshold)
                return true
        return false
    }

    ProgressStrip {
        id: strip
        x: bar.barX
        y: bar.barY
        width: bar.barWidth
        height: bar.thickness
        mode: "direct"
        cues: false
        fillOverride: Theme.secondaryColor
        fraction: bar.fraction
    }

    Repeater {
        model: bar.count
        delegate: Item {
            id: phaseMark

            readonly property string kind: BeatStyle.kindOfPhase(index + 1, bar.count)
            readonly property bool below: bar.alternating && index % 2 === 1
            readonly property bool started: index < bar.reached
            readonly property color fill: started ? BeatStyle.color(kind) : BeatStyle.unreachedFill
            // The callout has three segments, drawn behind the diamond and the bar: vertical from
            // the vertex of the diamond that faces the bar, oblique between two levels in the room
            // between the row and the bar, then vertical again at the tick down to the middle of the
            // bar. A diamond directly above or below its tick gets a straight vertical line.
            readonly property real fromX: bar.centres[index]
            readonly property real toX: bar.ticks[index]
            readonly property real dir: below ? -1 : 1
            readonly property real vertexY: below ? bar.barY + bar.thickness + bar.gap : bar.markWidth
            readonly property real midY: bar.barY + bar.thickness / 2
            readonly property real obliqueTop: vertexY + dir * bar.gap * 0.25
            readonly property real obliqueBottom: vertexY + dir * bar.gap * 0.75
            readonly property real dx: toX - fromX
            readonly property real dy: obliqueBottom - obliqueTop
            readonly property bool straight: Math.abs(dx) <= bar.lineThreshold
            readonly property real centreY: below ? vertexY + bar.markWidth / 2 : bar.markWidth / 2
            z: -1

            // First vertical segment, under the diamond (the whole line when it is straight).
            Rectangle {
                x: (phaseMark.straight ? phaseMark.toX : phaseMark.fromX) - width / 2
                y: Math.min(phaseMark.vertexY, phaseMark.straight ? phaseMark.midY : phaseMark.obliqueTop)
                width: bar.lineWidth
                height: Math.abs((phaseMark.straight ? phaseMark.midY : phaseMark.obliqueTop) - phaseMark.vertexY)
                color: Theme.primaryColor
            }

            // The oblique segment, between the two levels.
            Rectangle {
                visible: !phaseMark.straight
                x: phaseMark.fromX
                y: phaseMark.obliqueTop - height / 2
                width: Math.sqrt(phaseMark.dx * phaseMark.dx + phaseMark.dy * phaseMark.dy)
                height: bar.lineWidth
                transformOrigin: Item.Left
                rotation: Math.atan2(phaseMark.dy, phaseMark.dx) * 180 / Math.PI
                color: Theme.primaryColor
                antialiasing: true
            }

            // Last vertical segment, at the tick, down to the middle of the bar.
            Rectangle {
                visible: !phaseMark.straight
                x: phaseMark.toX - width / 2
                y: Math.min(phaseMark.obliqueBottom, phaseMark.midY)
                width: bar.lineWidth
                height: Math.abs(phaseMark.midY - phaseMark.obliqueBottom)
                color: Theme.primaryColor
            }

            Rectangle {
                x: phaseMark.fromX - width / 2
                y: phaseMark.centreY - height / 2
                width: bar.markSide
                height: bar.markSide
                rotation: 45
                color: phaseMark.fill
                // A thin outline in the text colour keeps the diamond 3:1 against the background.
                border.width: bar.lineWidth
                border.color: Theme.primaryColor
                antialiasing: true
            }

            Text {
                x: phaseMark.fromX - width / 2
                y: phaseMark.centreY - height / 2
                width: bar.markWidth
                height: bar.markWidth
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.family: Theme.fontFamily
                font.pixelSize: Metrics.labelSize
                font.bold: true
                color: BeatStyle.inkOn(phaseMark.fill)
                text: String(index + 1)
            }
        }
    }
}
