// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// The safety notice: title and text in the interface language (R-GEN-5). Shown at the first start
// and on the About page.
Column {
    spacing: Theme.paddingMedium

    Label {
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * Theme.horizontalPageMargin
        wrapMode: Text.Wrap
        font.pixelSize: Theme.fontSizeLarge
        color: Theme.highlightColor
        text: qsTr("Important notice")
    }

    Label {
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * Theme.horizontalPageMargin
        wrapMode: Text.Wrap
        color: Theme.primaryColor
        text: qsTr("The exercises, workouts and loads (durations, repetitions, rests) that come with the app, and any you create, are provided as they are, without any warranty, express or implied. They have not been reviewed by qualified professionals and, depending on the condition of each user and on how the exercise is understood and performed, they may be harmful. If in doubt, ask a physician or a qualified trainer first.")
    }
}
