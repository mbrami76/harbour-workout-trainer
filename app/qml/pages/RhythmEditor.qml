// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The editor of a rhythm (R-MET-1, R-MET-2, R-MET-7): its name, and 1 to 8 phases,
// each 0.5 s to 10 s with an optional name of at most 12 characters. A rhythm has no repetitions, sets
// or rest. Two uses: a new rhythm (creating) and a saved one (savedId). Save and Save as new write it;
// Try runs the values shown, saved or not, for a number of repetitions entered here, after the
// lead-in, and writes nothing. The rules come from the core: the codes of metronomeLibrary.problems()
// decide what is refused and why, and the bridge's problem() decides whether a try can start.
SilicaFlickable {
    id: setup

    property bool portrait: true
    // The saved rhythm being edited, 0 for none.
    property int savedId: 0
    // A new rhythm that is not saved yet.
    property bool creating: false
    property string metronomeName: ""
    property var phases: [2, 2, 2]
    property var phaseNames: []
    // The repetitions of a try (1 to 999).
    property int tryReps: 10
    // Said after a save, until the next change.
    property string notice: ""
    // Scroll to the try section when the page opens (the "Try" action of the list).
    property bool focusTry: false
    // Counts saves, so that the checks against the other rhythms are asked again.
    property int revision: 0

    // What is stored for a rhythm: the phases with the identity.
    readonly property var metronome: ({
        phases: phases, phaseNames: phaseNames, name: metronomeName
    })
    // What a try runs: the phases for the repetitions, one set and no rest.
    readonly property var config: ({
        form: "phases", reps: tryReps, totalSec: 60, repHalfSec: 3, phases: phases, phaseNames: phaseNames,
        sets: 1, restSec: 0, leadInSec: Settings.leadInSec, tickSeconds: Settings.tickSeconds,
        name: metronomeName.trim()
    })
    readonly property string problem: metronomeBridge.problem(config)
    // One repetition: the sum of the phases, in half seconds.
    readonly property int repetitionUnits: {
        var sum = 0
        for (var i = 0; i < phases.length; ++i)
            sum += phases[i]
        return sum
    }
    // The codes that refuse Save, and Save as new, now (empty when allowed).
    readonly property var saveProblems: revision >= 0 ? metronomeLibrary.problems(savedId, metronome) : []
    readonly property var newProblems: (savedId > 0 && revision >= 0) ? metronomeLibrary.problems(0, metronome) : []
    // The first problem about the name, as text.
    readonly property string identityText: {
        for (var i = 0; i < saveProblems.length; ++i) {
            var text = MetronomeText.identityProblem(saveProblems[i])
            if (text !== "")
                return text
        }
        return ""
    }
    // The first problem about the phases, as text.
    readonly property string phaseText: {
        for (var i = 0; i < saveProblems.length; ++i) {
            var text = problemText(saveProblems[i])
            if (text !== "")
                return text
        }
        return ""
    }
    // Save as new needs another name when it only clashes on it.
    readonly property bool newNeedsChange: savedId > 0 && saveProblems.indexOf("identity") < 0 && newProblems.indexOf("identity") >= 0
    readonly property bool canSave: saveProblems.length === 0
    readonly property bool canSaveNew: savedId > 0 && newProblems.length === 0

    // Loads a saved rhythm into the page.
    function load(id) {
        var m = metronomeLibrary.get(id)
        if (m.id === undefined)
            return false
        savedId = m.id
        creating = false
        metronomeName = m.name
        phases = m.phases
        phaseNames = m.phaseNames
        return true
    }

    function afterSave(id, text) {
        savedId = id
        creating = false
        revision = revision + 1
        notice = text
    }

    // Saves over the saved rhythm, or creates the new one; exercises that use it are asked about first.
    function save() {
        notice = ""
        if (savedId === 0) {
            var r = metronomeLibrary.create(metronome)
            if (r.status === "ok")
                afterSave(r.id, qsTr("Saved."))
            else
                revision = revision + 1
            return
        }
        var users = metronomeLibrary.exercisesUsing(savedId)
        if (users.length === 0) {
            applyUpdate()
            return
        }
        var dialog = pageStack.push(Qt.resolvedUrl("MetronomeImpactPage.qml"), { metronomeName: metronomeName, users: users })
        dialog.choice.connect(function (what) {
            if (what === "copy")
                saveCopy()
            else if (what === "anyway")
                applyUpdate()
        })
    }

    function applyUpdate() {
        var r = metronomeLibrary.update(savedId, metronome)
        if (r.status === "ok")
            afterSave(savedId, r.affected.length > 0
                      ? qsTr("Saved. %n exercise(s) now use the new rhythm.", "", r.affected.length) : qsTr("Saved."))
        else
            revision = revision + 1
    }

    // Saves the shown values as a new rhythm with a free name and leaves the original alone.
    function saveCopy() {
        var r = metronomeLibrary.saveAsCopy(metronome)
        if (r.status === "ok") {
            metronomeName = r.name
            afterSave(r.id, qsTr("Saved as \"%1\". The original and its exercises are unchanged.").arg(r.name))
        } else {
            revision = revision + 1
        }
    }

    // Save as new: refused, with the reason, while the name equals another's.
    function saveAsNew() {
        notice = ""
        var r = metronomeLibrary.create(metronome)
        if (r.status === "ok")
            afterSave(r.id, qsTr("Saved as a new rhythm."))
        else
            revision = revision + 1
    }

    Component.onCompleted: {
        if (savedId > 0)
            load(savedId)
        if (focusTry)
            scrollToTry.start()
    }

    Timer {
        id: scrollToTry
        interval: 50
        onTriggered: setup.contentY = Math.max(0, Math.min(tryHeader.y - Theme.paddingLarge,
                                                           setup.contentHeight - setup.height))
    }

    function halfSeconds(units) {
        return qsTr("%1 s").arg((units / 2).toFixed(1))
    }

    // The reasons about the phases; the others are about the identity.
    function problemText(code) {
        switch (code) {
        case "rep-long": return qsTr("One repetition would last more than 60 s. Shorten a phase or use fewer phases.")
        case "phase-count": return qsTr("The number of phases must be from 1 to 8.")
        case "phase-length": return qsTr("Each phase must last from 0.5 s to 10 s.")
        case "phase-name": return qsTr("A phase name has at most 12 characters.")
        case "lead-in": return qsTr("The get-ready time must be from 0 to 10 s.")
        case "reps": return qsTr("The repetitions must be from 1 to 999.")
        }
        return ""
    }

    function setPhaseCount(count) {
        phaseNames = phaseNames.slice(0, count)
        var list = phases.slice(0, count)
        while (list.length < count)
            list.push(list.length > 0 ? list[list.length - 1] : 2)
        phases = list
        notice = ""
    }

    function setPhaseName(index, name) {
        var list = phaseNames.slice()
        while (list.length <= index)
            list.push("")
        list[index] = name
        phaseNames = list
        notice = ""
    }

    function setPhase(index, units) {
        var list = phases.slice()
        list[index] = units
        phases = list
        notice = ""
    }

    contentHeight: column.height + Theme.paddingLarge

    Column {
        id: column
        width: parent.width
        spacing: Theme.paddingLarge

        PageHeader {
            title: setup.creating ? qsTr("New rhythm") : qsTr("Edit rhythm")
        }

        // Name of a saved metronome (R-MET-7).
        Column {
            width: parent.width
                        spacing: Theme.paddingMedium

            TextField {
                width: parent.width
                label: qsTr("Name")
                placeholderText: qsTr("Name")
                text: setup.metronomeName
                color: setup.saveProblems.indexOf("name-too-long") >= 0 ? Theme.errorColor : Theme.primaryColor
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
                onTextChanged: {
                    if (activeFocus) {
                        setup.metronomeName = text
                        setup.notice = ""
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: setup.identityText !== ""
                wrapMode: Text.Wrap
                color: Theme.errorColor
                text: setup.identityText
            }
        }

        NumberStepper {
            width: parent.width
            label: qsTr("Number of phases")
            minimum: 1
            maximum: 8
            steps: [-1, 1]
            value: setup.phases.length
            valueText: String(value)
            onEdited: setup.setPhaseCount(value)
        }

        // The phases, one stepper and one name each.
        Grid {
            width: parent.width
            columns: setup.portrait ? 1 : 2
            spacing: Theme.paddingLarge

            Repeater {
                model: setup.phases.length
                delegate: Column {
                    width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns

                    NumberStepper {
                        label: qsTr("Phase %1").arg(index + 1)
                        minimum: 1
                        maximum: 20
                        steps: [-2, -1, 1, 2]
                        value: setup.phases[index]
                        valueText: setup.halfSeconds(value)
                        onEdited: setup.setPhase(index, value)
                    }

                    // The name of the phase: up to 12 characters as the user sees them. The counter
                    // turns red over the limit, and Start stays off with the reason.
                    TextField {
                        width: parent.width
                        label: qsTr("Name (%1 of 12)").arg(metronomeBridge.nameLength(text))
                        placeholderText: qsTr("Phase %1").arg(index + 1)
                        text: index < setup.phaseNames.length ? setup.phaseNames[index] : ""
                        color: metronomeBridge.nameLength(text) > 12 ? Theme.errorColor : Theme.primaryColor
                        EnterKey.iconSource: "image://theme/icon-m-enter-close"
                        EnterKey.onClicked: focus = false
                        onTextChanged: {
                            if (activeFocus)
                                setup.setPhaseName(index, text)
                        }
                    }
                }
            }
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            color: Theme.secondaryHighlightColor
            text: qsTr("One repetition: %1").arg(setup.halfSeconds(setup.repetitionUnits))
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            visible: setup.phaseText !== ""
            wrapMode: Text.Wrap
            color: Theme.errorColor
            text: setup.phaseText
        }

        // Save and Save as new (R-MET-7).
        Row {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingMedium

            BigButton {
                width: setup.savedId > 0 ? (parent.width - parent.spacing) / 2 : parent.width
                height: Metrics.buttonSize
                minimumSize: Metrics.buttonSize
                enabled: setup.canSave
                opacity: enabled ? 1 : 0.4
                text: qsTr("Save")
                onActivated: setup.save()
            }

            BigButton {
                width: (parent.width - parent.spacing) / 2
                height: Metrics.buttonSize
                minimumSize: Metrics.buttonSize
                visible: setup.savedId > 0
                enabled: setup.canSaveNew
                opacity: enabled ? 1 : 0.4
                text: qsTr("Save as new")
                onActivated: setup.saveAsNew()
            }
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            visible: setup.newNeedsChange
            wrapMode: Text.Wrap
            color: Theme.secondaryColor
            text: qsTr("Save as new needs another name.")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            visible: setup.notice !== ""
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            text: setup.notice
        }

        SectionHeader { id: tryHeader; text: qsTr("Try") }

        // A try runs what is shown, saved or not, for the repetitions set here (R-MET-1).
        NumberStepper {
            width: parent.width
            label: qsTr("Repetitions")
            minimum: 1
            maximum: 999
            steps: [-10, -1, 1, 10]
            value: setup.tryReps
            valueText: String(value)
            onEdited: setup.tryReps = value
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            color: Theme.secondaryColor
            text: qsTr("Get ready: %n second(s). Change it in the settings. Nothing is saved by a try.", "", Settings.leadInSec)
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            visible: setup.problem !== ""
            wrapMode: Text.Wrap
            color: Theme.errorColor
            text: setup.problemText(setup.problem)
        }

        BigButton {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            height: Math.max(implicitHeight, Theme.itemSizeExtraLarge)
            enabled: setup.problem === ""
            opacity: enabled ? 1 : 0.4
            text: qsTr("Try")
            onActivated: metronomeBridge.start(setup.config)
        }
    }

    VerticalScrollDecorator { }
}
