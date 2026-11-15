// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0
import Sailfish.Silica 1.0

// The colours of the palette keys of R-EXR-8. The core holds the keys; this maps them to colours
// and to translatable names. The empty key and an unknown key give the default colour, so data from
// a newer version still shows. Colour is never the only cue: a name always goes with it.
QtObject {
    readonly property var colours: ({
        "red": "#e0463c",
        "orange": "#f08a24",
        "yellow": "#e8c92a",
        "green": "#3fb34f",
        "teal": "#20a8a0",
        "blue": "#3d85e0",
        "purple": "#8e5bd0",
        "pink": "#e060a8",
        "brown": "#9a6a3c",
        "grey": "#8a8f98"
    })

    function color(key) {
        return colours.hasOwnProperty(key) ? colours[key] : Theme.highlightColor
    }

    function known(key) {
        return colours.hasOwnProperty(key)
    }

    function label(key) {
        switch (key) {
        case "red": return qsTr("Red")
        case "orange": return qsTr("Orange")
        case "yellow": return qsTr("Yellow")
        case "green": return qsTr("Green")
        case "teal": return qsTr("Teal")
        case "blue": return qsTr("Blue")
        case "purple": return qsTr("Purple")
        case "pink": return qsTr("Pink")
        case "brown": return qsTr("Brown")
        case "grey": return qsTr("Grey")
        }
        return qsTr("Default")
    }
}
