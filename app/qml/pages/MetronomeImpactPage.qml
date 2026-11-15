// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// Asked before a change to a saved metronome that exercises use is saved (R-MET-8): how many and
// which exercises change their tempo, and three ways out. `choice` carries "copy" (save the change
// on a copy, the original and its exercises stay as they are), "anyway" (save the change; it
// reaches all of them) or "cancel". Leaving with the back gesture is a cancel.
Page {
    id: page

    property string metronomeName
    // The exercises that use it: [{key, name}].
    property var users: []

    signal choice(string what)

    allowedOrientations: Orientation.All

    function choose(what) {
        choice(what)
        pageStack.pop()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: qsTr("Exercises use this rhythm") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: qsTr("%n exercise(s) use the rhythm \"%1\" and take its new phases if you save the change:", "", page.users.length).arg(page.metronomeName)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                color: Theme.primaryColor
                text: MetronomeText.names(page.users)
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Save on a copy")
                onActivated: page.choose("copy")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("The original and its exercises stay as they are.")
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Save anyway")
                onActivated: page.choose("anyway")
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Cancel")
                onActivated: page.choose("cancel")
            }
        }

        VerticalScrollDecorator { }
    }
}
