// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import "."

// The full-screen flash of a beat (R-MET-4): a short soft rise and a fast fade, never a hard
// on-off flash. The flash starts from the scheduled time of the beat, not from the moment the
// signal reaches the page (R-ALR-12): at every frame its opacity is computed from the distance
// between the clock and that time, so a late signal or a late frame does not shift it, and the
// first frame that shows it is already on the right point of the curve. Each flash lasts
// attackMs + fadeMs, shorter than the 333 ms of three beats per second.
//
// The frames are driven by an animation that runs only while a flash is visible; nothing runs
// between the beats.
Item {
    id: pulse

    property int attackMs: 30
    property int fadeMs: 270
    // Milliseconds on the clock of the bridge (clock.clockMs()).
    property real startMs: 0
    property color color: "transparent"
    property real level: 0
    property real tick: 0
    // The bridge whose clockMs() the beats are scheduled on.
    property var clock: metronomeBridge

    // Starts a flash for a beat of the given kind, scheduled at atMs.
    function fire(kind, atMs) {
        color = BeatStyle.color(kind)
        startMs = atMs
        update()
        frames.restart()
    }

    function clear() {
        frames.stop()
        level = 0
    }

    function update() {
        var d = clock.clockMs() - startMs
        if (d >= attackMs + fadeMs) {
            clear()
            return
        }
        var envelope
        if (d < attackMs) {
            var a = Math.max(0, d) / attackMs
            envelope = a * a * (3 - 2 * a)
        } else {
            var f = 1 - (d - attackMs) / fadeMs
            envelope = f * f
        }
        level = BeatStyle.peak * envelope
    }

    onTickChanged: update()

    NumberAnimation {
        id: frames
        target: pulse
        property: "tick"
        from: 0
        to: 1
        duration: 1000
        loops: Animation.Infinite
    }

    Rectangle {
        anchors.fill: parent
        color: pulse.color
        opacity: pulse.level
    }
}
