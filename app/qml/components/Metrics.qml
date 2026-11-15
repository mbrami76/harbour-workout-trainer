// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0
import Sailfish.Silica 1.0

// Physical sizes for the timer pages (R-UI-12, R-NFR-4). Everything is derived from the pixels per
// millimetre that the platform reports, so a page does not depend on the system font size. When
// the platform value is missing or implausible the width of the screen is taken as 65 mm.
QtObject {
    readonly property real shortSide: Math.min(Screen.width, Screen.height)
    readonly property real fallbackPerMm: shortSide / 65
    readonly property real pixelsPerMm: {
        var reported = screenMetrics.pixelsPerMm
        var widthMm = reported > 0 ? shortSide / reported : 0
        return (widthMm >= 45 && widthMm <= 120) ? reported : fallbackPerMm
    }

    // How long a long press lasts (R-TMR-11); the only place the time is written.
    readonly property int holdMs: 2000
    // Smallest size of a label: 3.5 mm.
    readonly property real labelSize: mm(3.5)
    // The sizes of the texts of the run pages, as multiples of labelSize: a title (the step type, the
    // set, the phase) and a line of text. One place, so that the pages agree (R-UI-12, R-UI-14).
    readonly property real runTitleFactor: 1.6
    readonly property real runTextFactor: 1.25
    // The size scale of the session screen, one step 1.25 times the one before, from the smallest
    // label: runSize(0) is 3.5 mm, 1 is 4.4 mm, 2 is 5.5 mm, 3 is 6.8 mm, 4 is 8.5 mm. The upper part
    // of the session screen uses steps 0 to 2 and the type of the step step 1; the name of the exercise
    // step 2.
    readonly property real runScaleRatio: 1.25
    function runSize(step) {
        return labelSize * Math.pow(runScaleRatio, step)
    }
    // A press that ends within this time is a tap; a longer press that is not a hold is cancelled.
    readonly property int tapMaxMs: 500
    // Touch target of the icon buttons of the timer pages.
    readonly property real buttonSize: mm(11)

    function mm(value) {
        return value * pixelsPerMm
    }
}
