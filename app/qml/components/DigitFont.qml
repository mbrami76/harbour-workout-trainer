// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0
import Sailfish.Silica 1.0
import "."

// The font of the digits of a time (R-UI-10). The seven-segment font is loaded once from the data
// folder of the package: this file is installed in /usr/share/harbour-workout-trainer/qml/components,
// the font in /usr/share/harbour-workout-trainer/fonts, so the path from here is two folders up. The
// system choice is the generic family "monospace", which the platform resolves.
Item {
    readonly property string segmentFamily: loader.name
    readonly property string systemFamily: "monospace"
    // Strength of the unlit segments as a share of the text colour: 25 % on a dark ambience and
    // 10 % on a light one, where dark segments on a bright page look heavy. Every line of digits
    // takes it from here.
    readonly property real unlitStrength: Theme.colorScheme === Theme.LightOnDark ? 0.25 : 0.10
    readonly property string family: Settings.digitFont === "system" ? systemFamily : segmentFamily
    // True when the bundled font could not be loaded; the digits then use the system family.
    readonly property bool failed: loader.status === FontLoader.Error

    FontLoader {
        id: loader
        source: Qt.resolvedUrl("../../fonts/DSEG7Classic-Regular.ttf")
    }
}
