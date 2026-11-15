// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0
import Sailfish.Silica 1.0

// How the kinds of step look (R-UI-3). A kind is always shown with its label as well as its
// colour, never by colour alone. The colours come from the Silica theme, so they follow the light
// or dark ambience. The kinds are the stored step type names of the core ("work", "rest",
// "warmup", "cooldown"), "leadin" for the countdown before a step and "recovery" for the recovery
// period of the interval timer.
QtObject {
    function label(kind) {
        switch (kind) {
        case "work": return qsTr("Work")
        case "rest": return qsTr("Rest")
        case "warmup": return qsTr("Warm-up")
        case "cooldown": return qsTr("Cool-down")
        case "leadin": return qsTr("Get ready")
        case "recovery": return qsTr("Recovery")
        }
        return ""
    }

    // Work stands out with the highlight; the rest recedes to the secondary text colour; warm-up
    // and cool-down use the two softer shades of the highlight; the lead-in uses the plain text
    // colour.
    function color(kind) {
        switch (kind) {
        case "work": return Theme.highlightColor
        case "rest": return Theme.secondaryColor
        case "warmup": return Theme.secondaryHighlightColor
        case "cooldown": return Theme.highlightDimmerColor
        case "leadin": return Theme.primaryColor
        case "recovery": return Theme.secondaryHighlightColor
        }
        return Theme.primaryColor
    }
}
