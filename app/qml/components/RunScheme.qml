// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// The colours of the workout session screen (R-UI-16). They follow the ambience of the system:
// text, symbols, buttons and the neutral bar come from the Silica theme. When the theme gives no
// colours (`useAmbience` is off, or the theme reports no colour scheme) one of two built-in schemes
// is used, a light one or a dark one, all defined here. Work and rest keep their blue and green
// with a lightness adjusted to the scheme.
QtObject {
    id: scheme

    // False forces the built-in schemes.
    property bool useAmbience: true
    // The theme gives colours when it reports one of its two colour schemes.
    readonly property bool themed: useAmbience && (Theme.colorScheme === Theme.LightOnDark
                                                   || Theme.colorScheme === Theme.DarkOnLight)
    // The scheme in use: the ambience tells, and the built-in dark one stands in without it.
    property bool darkOverride: true
    readonly property bool dark: themed ? Theme.colorScheme === Theme.LightOnDark : darkOverride

    // The built-in schemes.
    readonly property var light: ({
        background: ["#fbeef0", "#e6eefc", "#c9b8fb", "#c7dcfb"],
        text: "#1d2b4a", secondary: "#475066", symbol: "#000000",
        buttonFill: "#b9b3e6", buttonBorder: "#4b4f80",
        barFill: "#3f4a5e", barTrack: "#9aa3bb",
        work: "#214d9c", rest: "#31590b", outline: "#1b1f2a", digits: "#000000", digitsOutline: "#ffffff"
    })
    readonly property var darkScheme: ({
        background: ["#151722", "#1c2036", "#2b2552", "#1b3350"],
        text: "#eef1f8", secondary: "#a9b3cc", symbol: "#ffffff",
        buttonFill: "#3a3768", buttonBorder: "#8a8fb5",
        barFill: "#d5dbea", barTrack: "#3b4358",
        work: "#679af1", rest: "#7ccf3a", outline: "#05060a", digits: "#ffffff", digitsOutline: "#000000"
    })
    readonly property var builtIn: dark ? darkScheme : light

    // Work and rest, with the lightness of the scheme, also when the theme gives the rest.
    readonly property color work: dark ? darkScheme.work : light.work
    readonly property color rest: dark ? darkScheme.rest : light.rest

    // A gradient for the background, drawn only without the ambience.
    readonly property bool drawsBackground: !themed

    readonly property color text: themed ? Theme.primaryColor : builtIn.text
    readonly property color secondary: themed ? Theme.secondaryColor : builtIn.secondary
    readonly property color symbol: themed ? Theme.primaryColor : builtIn.symbol
    readonly property color buttonFill: themed ? Theme.rgba(Theme.highlightColor, 0.25) : builtIn.buttonFill
    readonly property color buttonBorder: themed ? Theme.rgba(Theme.highlightColor, 0.7) : builtIn.buttonBorder
    readonly property color barFill: themed ? Theme.secondaryColor : builtIn.barFill
    readonly property color barTrack: themed ? Theme.rgba(Theme.primaryColor, 0.25) : builtIn.barTrack
    readonly property color outline: themed ? (dark ? "#05060a" : "#1b1f2a") : builtIn.outline
    // The digits are black with a white outline on a light scheme and the reverse on a dark one (R-UI-11).
    readonly property color digits: themed ? Theme.primaryColor : builtIn.digits
    readonly property color digitsOutline: dark ? "#000000" : "#ffffff"
    // The colour of a kind of step: work blue, rest green, the others as the theme has them.
    function stepColor(kind) {
        switch (kind) {
        case "work": return work
        case "rest": return rest
        case "warmup": return themed ? Theme.secondaryHighlightColor : (dark ? "#8fb2f5" : "#1f4b99")
        case "cooldown": return themed ? Theme.highlightDimmerColor : (dark ? "#8fb2f5" : "#1f4b99")
        case "leadin": return text
        }
        return text
    }
}
