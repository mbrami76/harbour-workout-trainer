// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0
import Sailfish.Silica 1.0

// How the three beat kinds of the metronome look (R-MET-4): light blue for the first beat of a
// repetition, green for the beats between, yellow for the last one. A kind is always shown with the
// name or the number of the phase as well, never by colour alone (R-UI-3). The flash is drawn over
// the page background with a limited opacity, so that the text on top keeps a contrast of at
// least 4.5:1 with it at the peak of the flash, in the dark and in the light ambience.
QtObject {
    readonly property bool darkAmbience: Theme.colorScheme === Theme.LightOnDark

    // The kind of a phase of a tempo with `count` phases: the first, the last, or one between.
    function kindOfPhase(phase, count) {
        if (phase <= 1)
            return "first"
        return phase >= count ? "last" : "middle"
    }

    function color(kind) {
        if (darkAmbience) {
            switch (kind) {
            case "first": return "#6ec6ff"
            case "middle": return "#7bdc6e"
            case "last": return "#ffe14d"
            }
        } else {
            switch (kind) {
            case "first": return "#2f9be8"
            case "middle": return "#3fb83c"
            case "last": return "#e6b800"
            }
        }
        return "#ffffff"
    }

    // Relative luminance of a colour (WCAG), and the contrast ratio of two colours.
    function luminance(c) {
        function channel(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
        return 0.2126 * channel(c.r) + 0.7152 * channel(c.g) + 0.0722 * channel(c.b)
    }

    function contrast(a, b) {
        var la = luminance(a)
        var lb = luminance(b)
        return (Math.max(la, lb) + 0.05) / (Math.min(la, lb) + 0.05)
    }

    // Black or white, whichever keeps the better contrast with the fill. The better of the two
    // is never below 4.58:1, so text in it always meets 4.5:1.
    function inkOn(fill) {
        return contrast(fill, Qt.rgba(0, 0, 0, 1)) >= contrast(fill, Qt.rgba(1, 1, 1, 1)) ? "#000000" : "#ffffff"
    }

    // The fill of a phase diamond whose phase has not started: a neutral grey, dark on a dark
    // ambience and light on a light one.
    readonly property color unreachedFill: darkAmbience ? "#505050" : "#d8d8d8"

    // Opacity of the flash at its peak.
    readonly property real peak: darkAmbience ? 0.42 : 0.65
}
