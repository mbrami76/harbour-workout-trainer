// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// What importing a chosen file would do (R-IMP-2). The page shows counts and notices as plain
// words. A file that is rejected is shown with its reasons and their places, and nothing is
// changed. For each exercise of the file whose key exists with other content, the two versions are
// shown side by side with the workouts that use the existing one, and the user chooses to keep it
// (the default), replace it or add the file's version as a new exercise. Nothing is written until
// Import is pressed; Cancel, or leaving the page, changes nothing.
Page {
    id: page

    // The result of exchangeBridge.previewFile().
    property var preview: ({})
    // The choices by exercise key: "keep", "replace" or "new". A key without an entry is kept.
    property var choices: ({})
    property bool applied: false

    readonly property bool accepted: preview.status === "ok"
    readonly property var counts: accepted ? preview.counts : ({})
    readonly property var conflicts: accepted ? preview.conflicts : []
    readonly property bool canImport: accepted && !preview.nothingToImport
    readonly property int maxProblemsShown: 20
    readonly property real half: (width - 2 * Theme.horizontalPageMargin - Theme.paddingMedium) / 2

    allowedOrientations: Orientation.All

    function choose(key, value) {
        var copy = {}
        for (var k in choices)
            copy[k] = choices[k]
        copy[key] = value
        choices = copy
    }

    function fieldValue(name, value) {
        switch (name) {
        case "muscles": return MuscleText.names(value)
        case "colour": return value === "" ? qsTr("Default") : Swatches.label(value)
        case "icon":
        case "metronome": return value === "" ? qsTr("None") : value
        }
        return value === "" ? qsTr("Empty") : value
    }

    function refusedText() {
        switch (preview.status) {
        case "unreadable": return ExchangeText.problem("unreadable")
        case "too-large": return ExchangeText.problem("too-large")
        }
        return ""
    }

    function apply() {
        var result = exchangeBridge.applyImport(choices)
        applied = true
        if (result.status === "ok") {
            workoutLibrary.refresh()
            exerciseLibrary.refresh()
        }
        pageStack.replace(Qt.resolvedUrl("ImportResultPage.qml"), { result: result })
    }

    function cancel() {
        exchangeBridge.cancelImport()
        pageStack.pop()
    }

    // Leaving by the back gesture also drops the kept file.
    Component.onDestruction: {
        if (!applied)
            exchangeBridge.cancelImport()
    }

    SilicaFlickable {
        id: flick
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: bar.top
        }
        clip: true
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Import workouts") }

            // A file that cannot be used.
            Label {
                visible: !page.accepted
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: qsTr("This file cannot be imported. Nothing was changed.")
            }
            Label {
                visible: !page.accepted && page.refusedText() !== ""
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                text: page.refusedText()
            }
            Repeater {
                model: (!page.accepted && page.preview.problems !== undefined)
                       ? page.preview.problems.slice(0, page.maxProblemsShown) : []
                delegate: Column {
                    x: Theme.horizontalPageMargin
                    width: column.width - 2 * Theme.horizontalPageMargin
                    Label {
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: ExchangeText.problem(modelData.code)
                    }
                    Label {
                        visible: modelData.location !== ""
                        width: parent.width
                        wrapMode: Text.WrapAnywhere
                        font.pixelSize: Theme.fontSizeExtraSmall
                        font.family: "monospace"
                        color: Theme.secondaryColor
                        text: modelData.location
                    }
                }
            }
            Label {
                visible: !page.accepted && page.preview.problems !== undefined
                         && page.preview.problems.length > page.maxProblemsShown
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                text: qsTr("And %n more.", "", page.preview.problems === undefined ? 0
                                                    : page.preview.problems.length - page.maxProblemsShown)
            }

            // A file that can be used.
            Label {
                visible: page.accepted
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: page.preview.nothingToImport ? qsTr("The file holds nothing to import.")
                                                   : qsTr("This is what importing the file would do. Nothing has been changed yet.")
            }
            CountLine { n: page.counts.workouts === undefined ? 0 : page.counts.workouts
                        text: qsTr("%n workout(s) to add.", "", n) }
            Label {
                visible: page.accepted && page.counts.workoutNames !== undefined && page.counts.workoutNames.length > 0
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                maximumLineCount: 6
                elide: Text.ElideRight
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: page.counts.workoutNames === undefined ? "" : page.counts.workoutNames.join(", ")
            }
            CountLine { n: page.counts.exercisesToAdd === undefined ? 0 : page.counts.exercisesToAdd
                        text: qsTr("%n exercise(s) to add.", "", n) }
            CountLine { n: page.counts.exercisesReused === undefined ? 0 : page.counts.exercisesReused
                        text: qsTr("%n exercise(s) are already in the library and will be used as they are.", "", n) }
            CountLine { n: page.counts.metronomesToAdd === undefined ? 0 : page.counts.metronomesToAdd
                        text: qsTr("%n rhythm(s) to add.", "", n) }
            CountLine { n: page.counts.metronomesReused === undefined ? 0 : page.counts.metronomesReused
                        text: qsTr("%n rhythm(s) are already in the library and will be used as they are.", "", n) }
            Repeater {
                model: page.accepted ? page.preview.notices : []
                delegate: Label {
                    x: Theme.horizontalPageMargin
                    width: column.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.Wrap
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    visible: text !== "" && modelData.code !== "nothing-to-import"
                    text: ExchangeText.notice(modelData)
                }
            }

            // The exercises that differ from the library.
            SectionHeader {
                visible: page.conflicts.length > 0
                text: qsTr("Exercises that differ")
            }
            Label {
                visible: page.conflicts.length > 0
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                text: qsTr("%n exercise(s) in the file have the same key as one of yours but other content. Your exercises stay as they are unless you choose otherwise.", "", page.conflicts.length)
            }
            Repeater {
                model: page.conflicts
                delegate: Column {
                    id: conflict
                    property var item: modelData
                    width: column.width
                    spacing: Theme.paddingSmall

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        wrapMode: Text.Wrap
                        font.bold: true
                        color: Theme.highlightColor
                        text: modelData.existing.name
                    }
                    Row {
                        x: Theme.horizontalPageMargin
                        spacing: Theme.paddingMedium
                        Label {
                            width: page.half
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryHighlightColor
                            text: qsTr("In your library")
                        }
                        Label {
                            width: page.half
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryHighlightColor
                            text: qsTr("In the file")
                        }
                    }
                    Repeater {
                        model: modelData.differs
                        delegate: Column {
                            x: Theme.horizontalPageMargin
                            spacing: 0
                            Label {
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                                text: ExchangeText.field(modelData)
                            }
                            Row {
                                spacing: Theme.paddingMedium
                                Label {
                                    width: page.half
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 3
                                    elide: Text.ElideRight
                                    font.pixelSize: Theme.fontSizeSmall
                                    text: page.fieldValue(modelData, conflict.item.existing[modelData])
                                }
                                Label {
                                    width: page.half
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 3
                                    elide: Text.ElideRight
                                    font.pixelSize: Theme.fontSizeSmall
                                    text: page.fieldValue(modelData, conflict.item.incoming[modelData])
                                }
                            }
                        }
                    }
                    Label {
                        visible: modelData.workouts.length > 0
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        text: qsTr("Used by: %1").arg(modelData.workouts.join(", "))
                    }
                    ComboBox {
                        label: qsTr("What to do")
                        currentIndex: {
                            var c = page.choices[modelData.key]
                            return c === "replace" ? 1 : (c === "new" ? 2 : 0)
                        }
                        menu: ContextMenu {
                            MenuItem {
                                text: qsTr("Keep mine")
                                onClicked: page.choose(modelData.key, "keep")
                            }
                            MenuItem {
                                text: qsTr("Replace mine")
                                onClicked: page.choose(modelData.key, "replace")
                            }
                            MenuItem {
                                text: qsTr("Import as new")
                                onClicked: page.choose(modelData.key, "new")
                            }
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }

    // The choices rest at the bottom, where the thumb reaches first (R-UI-13).
    Row {
        id: bar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: Theme.horizontalPageMargin
        }
        spacing: Theme.paddingMedium
        height: Math.max(Metrics.mm(11), Theme.itemSizeSmall) + Theme.paddingMedium

        Button {
            width: page.canImport ? (parent.width - parent.spacing) / 2 : parent.width
            height: Math.max(Metrics.mm(11), implicitHeight)
            text: page.accepted ? qsTr("Cancel") : qsTr("Close")
            onClicked: page.cancel()
        }
        Button {
            visible: page.canImport
            width: (parent.width - parent.spacing) / 2
            height: Math.max(Metrics.mm(11), implicitHeight)
            text: qsTr("Import")
            onClicked: page.apply()
        }
    }
}
