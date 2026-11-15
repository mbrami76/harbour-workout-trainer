// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// Creates or edits an exercise (R-EXR-1, R-EXR-8, R-EXR-10, R-MUS-6): name, icon and colour, the
// muscle groups as a grouped checklist with the figure showing the choice, the instructions, the
// tips and the metronome that gives the tempo. A value the core refuses is explained and Save
// stays off; the rules are the ones of the core (exerciseLibrary.problems()), never copied here.
// Saving writes the metronome link the page shows. Leaving with the back gesture drops unsaved
// changes without asking.
Page {
    id: page

    // The exercise to edit; empty for a new one.
    property string exerciseKey
    readonly property bool creating: exerciseKey === ""

    property string exerciseName
    property var muscles: []
    property string instructions
    property string tips
    property string icon
    property string colour
    // The metronome link the page shows, 0 for none; saved as it is.
    property int metronomeId: 0
    // Language and starter flag of an exercise being edited; they travel with the map.
    property string language
    property bool hasLanguage: false
    property bool starter: false

    allowedOrientations: Orientation.All

    readonly property var shown: {
        var m = {
            name: exerciseName, muscles: muscles, instructions: instructions, tips: tips,
            icon: icon, colour: colour, metronomeId: metronomeId, starter: starter
        }
        if (!creating)
            m.key = exerciseKey
        if (hasLanguage)
            m.language = language
        return m
    }
    readonly property var problemCodes: exerciseLibrary.problems(shown)
    readonly property bool canSave: problemCodes.length === 0

    // The metronome the link points to, as the list of metronomes shows it.
    readonly property var chosenMetronome: {
        var all = metronomeLibrary.metronomes
        for (var i = 0; i < all.length; ++i) {
            if (all[i].id === metronomeId)
                return all[i]
        }
        return ({})
    }

    function toggle(key, on) {
        var list = muscles.slice()
        var at = list.indexOf(key)
        if (on && at < 0)
            list.push(key)
        else if (!on && at >= 0)
            list.splice(at, 1)
        muscles = list
    }

    function firstProblem() {
        for (var i = 0; i < problemCodes.length; ++i) {
            var text = MuscleText.problem(problemCodes[i])
            if (text !== "")
                return text
        }
        return ""
    }

    // Sent after a new exercise was saved and the page closed, with its key, for a page that
    // asked for the exercise (the picker of a workout step).
    signal saved(string key)

    function finish(r) {
        if (r.status === "ok") {
            pageStack.pop()
            if (creating)
                saved(r.key)
        } else
            notice.text = qsTr("The exercise could not be saved.")
    }

    function save() {
        if (!canSave)
            return
        if (creating) {
            finish(exerciseLibrary.create(shown))
            return
        }
        var change = exerciseLibrary.impact(shown)
        if (change.workouts.length > 0 && change.fields.length > 0) {
            var dialog = pageStack.push(Qt.resolvedUrl("ExerciseImpactPage.qml"), {
                exerciseName: exerciseName, workouts: change.workouts
            })
            dialog.choice.connect(function (what) {
                if (what === "copy") {
                    var r = exerciseLibrary.saveAsCopy(shown)
                    if (r.status === "ok")
                        page.finishAfterDialog()
                    else
                        notice.text = r.problems.length > 0 ? MuscleText.problem(r.problems[0]) : qsTr("The exercise could not be saved.")
                } else if (what === "anyway") {
                    var u = exerciseLibrary.update(shown)
                    if (u.status === "ok")
                        page.finishAfterDialog()
                    else
                        notice.text = qsTr("The exercise could not be saved.")
                }
            })
            return
        }
        finish(exerciseLibrary.update(shown))
    }

    // Set when the impact page has been answered with a save: this page leaves when it is shown
    // again, after the impact page has closed itself.
    property bool closeWhenShown: false

    function finishAfterDialog() {
        closeWhenShown = true
    }

    onStatusChanged: {
        if (status === PageStatus.Active && closeWhenShown)
            pageStack.pop()
    }

    Component.onCompleted: {
        metronomeLibrary.refresh()
        if (creating) {
            language = exerciseLibrary.defaultLanguage()
            hasLanguage = true
        } else {
            var e = exerciseLibrary.get(exerciseKey)
            exerciseName = e.name
            muscles = e.muscles
            instructions = e.instructions
            tips = e.tips
            icon = e.icon
            colour = e.colour
            metronomeId = e.metronomeId
            language = e.language
            hasLanguage = true
            starter = e.starter
        }
    }

    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: page.creating ? qsTr("New exercise") : qsTr("Edit exercise") }

            TextField {
                width: parent.width
                label: qsTr("Name")
                placeholderText: qsTr("Name")
                text: page.exerciseName
                color: page.problemCodes.indexOf("name-too-long") >= 0 ? Theme.errorColor : Theme.primaryColor
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
                onTextChanged: {
                    if (activeFocus)
                        page.exerciseName = text
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                text: qsTr("Icon")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }

            Flow {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                Repeater {
                    model: exerciseLibrary.icons
                    delegate: MouseArea {
                        readonly property bool chosen: page.icon === modelData
                        width: Math.max(Metrics.mm(9), Theme.itemSizeSmall)
                        height: width
                        onClicked: page.icon = modelData
                        Accessible.role: Accessible.Button
                        Accessible.name: modelData === "" ? qsTr("Default icon") : qsTr("Icon %1").arg(modelData)
                        Accessible.checked: chosen

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.paddingSmall
                            color: Theme.rgba(Theme.highlightColor, chosen ? 0.35 : 0.1)
                            border.width: chosen ? Math.max(2, Metrics.mm(0.5)) : 1
                            border.color: chosen ? Theme.highlightColor : Theme.secondaryColor
                        }

                        IconBadge {
                            anchors.centerIn: parent
                            width: parent.width * 0.8
                            iconKey: modelData
                            colourKey: page.colour
                        }
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                text: qsTr("Colour: %1").arg(Swatches.label(page.colour))
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }

            Flow {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                Repeater {
                    model: [""].concat(exerciseLibrary.colours)
                    delegate: MouseArea {
                        readonly property bool chosen: page.colour === modelData
                        width: Math.max(Metrics.mm(9), Theme.itemSizeSmall)
                        height: width
                        onClicked: page.colour = modelData
                        Accessible.role: Accessible.Button
                        Accessible.name: Swatches.label(modelData)
                        Accessible.checked: chosen

                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            // The default colour is an empty ring, so it is not taken for a palette colour.
                            color: modelData === "" ? "transparent" : Swatches.color(modelData)
                            border.width: chosen ? Math.max(3, Metrics.mm(0.7)) : 1
                            border.color: Theme.primaryColor
                        }

                        // A mark on the chosen colour, so that the choice is not shown by colour alone.
                        Glyph {
                            anchors.centerIn: parent
                            width: parent.width * 0.5
                            height: width
                            visible: chosen
                            kind: "check"
                            color: modelData === "" ? Theme.primaryColor : BeatStyle.inkOn(Swatches.color(modelData))
                        }
                    }
                }
            }

            SectionHeader { text: qsTr("Muscle groups") }

            // The checklist on the left and the figure on the right. The figure stays in view
            // while the list is scrolled, so that the choice is seen as it is made.
            Item {
                id: groupsBlock
                width: parent.width
                height: checklist.height

                Column {
                    id: checklist
                    width: parent.width * 0.5

                    Repeater {
                        model: MuscleText.sectionsFor(exerciseLibrary.muscleKeys)
                        delegate: Column {
                            width: checklist.width

                            Label {
                                x: Theme.horizontalPageMargin
                                width: parent.width - Theme.horizontalPageMargin
                                text: modelData.title
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.secondaryHighlightColor
                            }

                            Repeater {
                                model: modelData.keys
                                delegate: TextSwitch {
                                    width: checklist.width
                                    height: Math.max(implicitHeight, Metrics.mm(9))
                                    text: MuscleText.label(modelData)
                                    checked: page.muscles.indexOf(modelData) >= 0
                                    automaticCheck: false
                                    onClicked: page.toggle(modelData, !checked)
                                }
                            }
                        }
                    }
                }

                PuppetFigure {
                    id: groupsFigure
                    readonly property real room: groupsBlock.width - checklist.width - Theme.horizontalPageMargin
                    x: checklist.width + room * 0.5 - width * 0.5
                    width: Math.min(room, Screen.height * 0.3 * 128 / 118)
                    height: width * 118 / 128
                    // Follows the scroll inside the block.
                    y: Math.max(0, Math.min(groupsBlock.height - height,
                                            flick.contentY - (column.y + groupsBlock.y) + Theme.paddingLarge))
                    groups: page.muscles
                    highlightColor: Swatches.known(page.colour) ? Swatches.color(page.colour) : Theme.highlightColor
                    description: page.muscles.length > 0 ? qsTr("Muscle groups: %1").arg(MuscleText.names(page.muscles)) : qsTr("No muscle group chosen")
                }
            }

            SectionHeader { text: qsTr("Instructions and tips") }

            TextArea {
                width: parent.width
                label: qsTr("Instructions")
                placeholderText: qsTr("Instructions")
                text: page.instructions
                color: page.problemCodes.indexOf("instructions-too-long") >= 0 ? Theme.errorColor : Theme.primaryColor
                onTextChanged: {
                    if (activeFocus)
                        page.instructions = text
                }
            }

            TextArea {
                width: parent.width
                label: qsTr("Form tips")
                placeholderText: qsTr("Form tips")
                text: page.tips
                color: page.problemCodes.indexOf("tips-too-long") >= 0 ? Theme.errorColor : Theme.primaryColor
                onTextChanged: {
                    if (activeFocus)
                        page.tips = text
                }
            }

            ComboBox {
                id: languageBox
                width: parent.width
                label: qsTr("Language of the texts")
                description: qsTr("Tag of the language the name, instructions and tips are written in")
                readonly property var tags: exerciseLibrary.languages()
                // Set again whenever the language is read, because the box keeps the index it was given.
                Connections {
                    target: page
                    onLanguageChanged: languageBox.currentIndex = Math.max(0, languageBox.tags.indexOf(page.language))
                }
                menu: ContextMenu {
                    Repeater {
                        model: languageBox.tags
                        MenuItem {
                            text: modelData
                            onClicked: {
                                page.language = modelData
                                page.hasLanguage = true
                            }
                        }
                    }
                }
            }

            SectionHeader { text: qsTr("Rhythm") }

            ComboBox {
                id: metronomeBox
                width: parent.width
                label: qsTr("Rhythm")
                // The first entry is "None", then the saved metronomes in the order of the list.
                // The menu entries are made after the page, so the index is set once they exist
                // and again whenever the link or the list changes.
                function sync() {
                    var all = metronomeLibrary.metronomes
                    var index = 0
                    for (var i = 0; i < all.length; ++i) {
                        if (all[i].id === page.metronomeId)
                            index = i + 1
                    }
                    currentIndex = index
                }
                Timer {
                    id: syncTimer
                    interval: 0
                    onTriggered: metronomeBox.sync()
                }
                Connections {
                    target: page
                    onMetronomeIdChanged: syncTimer.restart()
                }
                Connections {
                    target: metronomeLibrary
                    onMetronomesChanged: syncTimer.restart()
                }
                Component.onCompleted: syncTimer.restart()
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("None")
                        onClicked: page.metronomeId = 0
                    }
                    Repeater {
                        model: metronomeLibrary.metronomes
                        delegate: MenuItem {
                            text: modelData.name
                            onClicked: page.metronomeId = modelData.id
                        }
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: metronomeLibrary.metronomes.length === 0
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("There are no saved rhythms. Make one with the Metronome tile to give an exercise a rhythm.")
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.chosenMetronome.id !== undefined
                spacing: Theme.paddingSmall

                PaceStrip {
                    width: parent.width
                    units: page.chosenMetronome.phaseUnits !== undefined ? page.chosenMetronome.phaseUnits : []
                }
                Label {
                    width: parent.width
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    text: page.chosenMetronome.id !== undefined ? MetronomeText.summary(page.chosenMetronome) : ""
                }
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.metronomeId > 0
                text: qsTr("Detach")
                onActivated: page.metronomeId = 0
            }

            // The message of a save that failed stays until the next change.
            Label {
                id: notice
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: text !== ""
                wrapMode: Text.Wrap
                color: Theme.errorColor
                Connections {
                    target: page
                    onShownChanged: notice.text = ""
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: !page.canSave && notice.text === ""
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: page.firstProblem()
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                enabled: page.canSave
                opacity: enabled ? 1 : 0.4
                text: qsTr("Save")
                onActivated: page.save()
            }
        }

        VerticalScrollDecorator { }
    }
}
