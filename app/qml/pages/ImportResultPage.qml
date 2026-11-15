// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The result of an import (R-IMP-2): what was added, reused, kept, replaced and renamed.
Page {
    id: page

    // The result map of the exchange bridge.
    property var result: ({})
    readonly property bool done: result.status === "ok"
    readonly property var c: done ? result.counts : ({})

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: page.done ? qsTr("Imported") : qsTr("Not imported") }

            Label {
                visible: !page.done
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: qsTr("Nothing was changed. Try the file again.")
            }

            CountLine { n: page.c.workoutsAdded === undefined ? 0 : page.c.workoutsAdded
                   text: qsTr("%n workout(s) added.", "", n) }
            CountLine { n: page.c.workoutsRenamed === undefined ? 0 : page.c.workoutsRenamed
                   color: Theme.secondaryColor
                   text: qsTr("%n workout(s) already had that name, icon and colour; they got the suffix \"(imported)\".", "", n) }
            CountLine { n: page.c.exercisesAdded === undefined ? 0 : page.c.exercisesAdded
                   text: qsTr("%n exercise(s) added.", "", n) }
            CountLine { n: page.c.exercisesReused === undefined ? 0 : page.c.exercisesReused
                   text: qsTr("%n exercise(s) were already in the library and were used as they are.", "", n) }
            CountLine { n: page.c.exercisesKept === undefined ? 0 : page.c.exercisesKept
                   text: qsTr("%n exercise(s) kept as they were.", "", n) }
            CountLine { n: page.c.exercisesReplaced === undefined ? 0 : page.c.exercisesReplaced
                   text: qsTr("%n exercise(s) replaced by the version in the file.", "", n) }
            CountLine { n: page.c.exercisesImportedAsNew === undefined ? 0 : page.c.exercisesImportedAsNew
                   text: qsTr("%n exercise(s) added as new, with the suffix \"(imported)\".", "", n) }
            CountLine { n: page.c.metronomesAdded === undefined ? 0 : page.c.metronomesAdded
                   text: qsTr("%n rhythm(s) added.", "", n) }
            CountLine { n: page.c.metronomesReused === undefined ? 0 : page.c.metronomesReused
                   text: qsTr("%n rhythm(s) were already in the library and were used as they are.", "", n) }
            CountLine { n: page.c.metronomesRenamed === undefined ? 0 : page.c.metronomesRenamed
                   color: Theme.secondaryColor
                   text: qsTr("%n rhythm(s) got the suffix \"(imported)\" because of a clash on the name.", "", n) }

            Repeater {
                model: page.done ? page.result.notices : []
                delegate: Label {
                    x: Theme.horizontalPageMargin
                    width: column.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.Wrap
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    visible: text !== ""
                    text: ExchangeText.notice(modelData)
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                preferredWidth: Theme.buttonWidthLarge
                height: Math.max(implicitHeight, Metrics.mm(9))
                text: qsTr("Close")
                onClicked: pageStack.pop()
            }
        }

        VerticalScrollDecorator { }
    }
}
