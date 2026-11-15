// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import QtFeedback 5.0
import "."

// The progress bar under the digits of a timer with a set time (R-UI-7). It shows the elapsed
// fraction of the period: in count-down mode it empties, in direct mode it fills. Its colour
// shifts with the elapsed time from teal at the start to red at the end, with a lightness chosen
// so that it keeps at least 3:1 contrast with the page background in the light and the dark
// ambience. A border in the text colour keeps it visible against any background. The caller draws
// the digits over its upper edge.
//
// Two small diamonds sit on the bar: one at the middle of a period of 45 s or more and one 10 s
// before its end (for a period longer than 10 s). A halo pulses once at the middle of a period of 45 s or more, and at 10,
// 5, 3, 2, 1 and 0 s before the end, larger and with a vibration (when the user has switched
// vibration on and the platform supports it, R-ALR-4); the pulse at 0 is the largest and strongest.
// Only a running timer pulses. A plain rectangle: the scene graph moves it, nothing is repainted
// by script.
Item {
    id: strip

    // Elapsed share of the period, 0 to 1.
    property real fraction: 0
    // "countdown" or "direct".
    property string mode: "countdown"
    // For the pulses: the time left and the length of the period, in milliseconds, and whether the
    // timer runs.
    property real remainingMs: 0
    property real periodMs: 0
    property bool running: false
    // A fixed colour for the halo; transparent for the colour that follows the bar.
    property color haloOverride: "transparent"
    // False for a bar that is not a period of time with cues (the bar of one metronome
    // repetition): no halo, no vibration, no diamonds of its own.
    property bool cues: true
    // The colour of the outline and of the diamonds: the text colour of the page.
    property color markColor: Theme.primaryColor
    // True for a bar whose cues also sound: the last-seconds tick and the end sound, with the
    // same sounds as the other timers (R-ALR-1). The other bars sound from the events of the core.
    property bool sounds: false
    // A fixed colour for the fill; transparent for the colour that follows the elapsed share.
    property color fillOverride: "transparent"

    // The page background and the contrast the bar must keep with it.
    readonly property color background: Qt.rgba(Theme.overlayBackgroundColor.r, Theme.overlayBackgroundColor.g,
                                                Theme.overlayBackgroundColor.b, 1)
    readonly property real minimumContrast: 3
    // Hue of teal and of red, as shares of the colour circle; the hue falls from teal through green,
    // yellow and orange to red.
    readonly property real tealHue: 175 / 360
    readonly property real borderWidth: Math.max(1, Metrics.mm(0.5))
    readonly property color barColor: fillOverride.a > 0 ? fillOverride : fit(Math.max(0, Math.min(1, fraction)))
    readonly property color trackColor: Theme.rgba(barColor, 0.3)
    readonly property real filled: Math.max(0, Math.min(1, mode === "direct" ? fraction : 1 - fraction))

    // The halo is bright on a dark background and the bar hue at full saturation and medium lightness on a light one.
    readonly property bool darkBackground: luminance(background) < 0.3
    readonly property color haloColor: haloOverride.a > 0 ? haloOverride : Qt.hsla(tealHue * (1 - Math.max(0, Math.min(1, fraction))), 1, darkBackground ? 0.85 : 0.58, 1)

    function luminance(c) {
        function channel(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
        return 0.2126 * channel(c.r) + 0.7152 * channel(c.g) + 0.0722 * channel(c.b)
    }

    function contrast(a, b) {
        var la = luminance(a)
        var lb = luminance(b)
        return (Math.max(la, lb) + 0.05) / (Math.min(la, lb) + 0.05)
    }

    // The colour at the elapsed share f. On a dark background the lightness starts at 0.6 and rises,
    // on a light one it starts at 0.4 and falls, until the contrast is enough.
    function fit(f) {
        var hue = tealHue * (1 - f)
        var dark = darkBackground
        var c = Qt.hsla(hue, 0.85, dark ? 0.6 : 0.4, 1)
        for (var i = 0; i <= 12; ++i) {
            c = Qt.hsla(hue, 0.85, dark ? 0.6 + i * 0.03 : 0.4 - i * 0.03, 1)
            if (contrast(c, background) >= minimumContrast)
                return c
        }
        return c
    }

    // How far the halo grows, in units of the thickness of the bar.
    readonly property real smallPulse: 0.8
    readonly property real mediumPulse: 1.6
    readonly property real bigPulse: 2.6
    property real lastRemaining: -1
    readonly property var cueSeconds: [10, 5, 3, 2, 1]

    function pulse(size, duration) {
        halo.maxGrow = height * size
        haloAnimation.duration = duration
        haloAnimation.restart()
    }

    function sound(name) {
        if (sounds)
            Sounds.play(name, 0)
    }

    function vibrate(effect) {
        if (!Settings.vibration || !weak.supported)
            return
        effect.play()
    }

    onRemainingMsChanged: {
        if (!cues)
            return
        if (!running) {
            lastRemaining = remainingMs
            return
        }
        if (lastRemaining >= 0) {
            var before = Math.ceil(lastRemaining / 1000)
            var now = Math.ceil(remainingMs / 1000)
            var half = periodMs / 2
            if (remainingMs > lastRemaining + 500) {
                // A new period has started: the previous one reached zero.
                pulse(bigPulse, 1000)
                vibrate(strong)
                sound("timer-end")
            } else if (remainingMs <= 0 && lastRemaining > 0) {
                pulse(bigPulse, 1000)
                vibrate(strong)
                sound("timer-end")
            } else if (now < before && cueSeconds.indexOf(now) >= 0) {
                pulse(mediumPulse, 700)
                vibrate(now <= 3 ? normal : weak)
                sound("tick")
            } else if (periodMs >= 45000 && lastRemaining > half && remainingMs <= half) {
                pulse(smallPulse, 700)
            }
        }
        lastRemaining = remainingMs
    }
    onRunningChanged: lastRemaining = remainingMs

    ThemeEffect { id: weak; effect: ThemeEffect.PressWeak }
    ThemeEffect { id: normal; effect: ThemeEffect.Press }
    ThemeEffect { id: strong; effect: ThemeEffect.PressStrong }

    // The halo: the shape of the bar grown on all sides, fading out.
    Rectangle {
        id: halo
        property real grow: 0
        property real maxGrow: 0
        anchors.centerIn: parent
        width: parent.width + 2 * grow
        height: parent.height + 2 * grow
        radius: height / 2
        color: strip.haloColor
        opacity: 0
    }
    ParallelAnimation {
        id: haloAnimation
        property int duration: 700
        NumberAnimation { target: halo; property: "grow"; from: 0; to: halo.maxGrow
                          duration: haloAnimation.duration; easing.type: Easing.OutQuad }
        NumberAnimation { target: halo; property: "opacity"; from: 0.8; to: 0
                          duration: haloAnimation.duration }
    }

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: strip.trackColor
        border.width: strip.borderWidth
        border.color: strip.markColor
    }

    Rectangle {
        // The share that is filled, from the left edge, inside the border.
        x: strip.borderWidth
        y: strip.borderWidth
        // Never narrower than it is high, so the rounded ends keep their shape until it is empty.
        width: strip.filled > 0 ? Math.max(height, (parent.width - 2 * strip.borderWidth) * strip.filled) : 0
        height: parent.height - 2 * strip.borderWidth
        radius: height / 2
        color: strip.barColor
    }

    // The diamonds, as shares of the bar from the left: the part that is left is drawn from the left
    // in count-down mode, the part that has passed in direct mode.
    Repeater {
        model: [
            { at: 0.5, show: strip.cues && strip.periodMs >= 45000 },
            { at: strip.periodMs > 0 ? 10000 / strip.periodMs : 0, show: strip.cues && strip.periodMs > 10000 }
        ]
        delegate: Rectangle {
            readonly property real share: strip.mode === "direct" ? 1 - modelData.at : modelData.at
            readonly property real side: strip.height * 0.5
            visible: modelData.show
            width: side
            height: side
            x: strip.borderWidth + (strip.width - 2 * strip.borderWidth) * share - width / 2
            y: (strip.height - height) / 2
            rotation: 45
            color: strip.markColor
            border.width: Math.max(1, strip.borderWidth * 0.6)
            border.color: strip.background
        }
    }
}
