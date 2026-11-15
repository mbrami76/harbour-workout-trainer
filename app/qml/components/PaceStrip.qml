// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "."

// One period of a metronome drawn like the flash symbol of a lighthouse chart (R-MET-4): a thin line
// for the period and one thick pill per phase, its length proportional to the length of the phase,
// in the colour of the diamond of that phase (light blue first, green between, yellow last). The
// number of the phase is written inside when the pill is wide enough. A pace without phases is one
// light-blue pill. A pill never gets narrower than a minimum, taken from the others.
Item {
    id: strip

    // Length of each phase in any unit; empty or a single entry draws one pill.
    property var units: []
    readonly property real gap: Metrics.mm(0.8)
    readonly property real pillHeight: Metrics.mm(5)
    readonly property real minPill: Metrics.mm(1.6)
    // A number needs this much room (3.5 mm high, about 2 mm wide, with margins).
    readonly property real numberRoom: Metrics.mm(3.6)
    readonly property var list: units.length > 0 ? units : [1]
    readonly property int count: list.length

    // The widths: proportional, none below the minimum, the rest shared in proportion.
    readonly property var widths: {
        var n = list.length
        var room = Math.max(1, width - (n - 1) * gap)
        var total = 0
        for (var i = 0; i < n; ++i)
            total += list[i]
        var w = [], fixed = 0, free = 0, small = []
        for (var j = 0; j < n; ++j) {
            var share = room * list[j] / total
            small.push(share < minPill)
            if (share < minPill)
                fixed += minPill
            else
                free += list[j]
        }
        for (var k = 0; k < n; ++k)
            w.push(small[k] ? minPill : (room - fixed) * list[k] / free)
        return w
    }

    implicitHeight: pillHeight
    height: pillHeight

    Accessible.role: Accessible.Graphic
    Accessible.name: count > 1 ? qsTr("%n phase(s)", "", count) : qsTr("One beat per repetition")

    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: Math.max(1, Metrics.mm(0.3))
        color: Theme.primaryColor
        opacity: 0.45
    }

    Row {
        spacing: strip.gap

        Repeater {
            model: strip.count
            delegate: Rectangle {
                readonly property string kind: strip.count === 1 ? "first" : BeatStyle.kindOfPhase(index + 1, strip.count)
                width: strip.widths[index]
                height: strip.pillHeight
                radius: height / 2
                color: BeatStyle.color(kind)
                border.width: Math.max(1, Metrics.mm(0.3))
                border.color: Theme.primaryColor
                antialiasing: true

                Text {
                    anchors.centerIn: parent
                    visible: strip.count > 1 && parent.width >= strip.numberRoom
                    font.family: Theme.fontFamily
                    font.pixelSize: Metrics.labelSize * 0.9
                    font.bold: true
                    color: BeatStyle.inkOn(parent.color)
                    text: String(index + 1)
                }
            }
        }
    }
}
