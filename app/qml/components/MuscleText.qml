// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0

// The names of the muscle groups and the texts about exercises (R-MUS-3, R-EXR-1). The core holds
// the group keys; this maps them to translatable names and to the sections of the checklist. A key
// that is not named here still shows, under "Other" in the checklist and as its key in a line.
QtObject {
    function label(key) {
        switch (key) {
        case "shoulders": return qsTr("Shoulders")
        case "core": return qsTr("Upper abs")
        case "glutes": return qsTr("Glutes")
        case "quadriceps": return qsTr("Quadriceps")
        case "hamstrings": return qsTr("Hamstrings")
        case "calves": return qsTr("Calves")
        case "lower-abdomen": return qsTr("Lower abs")
        case "lower-back": return qsTr("Lower back")
        case "upper-chest": return qsTr("Upper chest")
        case "lower-chest": return qsTr("Lower chest")
        case "obliques": return qsTr("Obliques")
        case "trapezius": return qsTr("Trapezius")
        case "lats": return qsTr("Lats")
        case "mid-back": return qsTr("Mid back")
        case "biceps": return qsTr("Biceps")
        case "triceps": return qsTr("Triceps")
        case "forearms": return qsTr("Forearms")
        case "shins": return qsTr("Shins")
        }
        return key
    }

    // The sections of the checklist, with the keys they hold. Keys of the core that no section
    // names are added under "Other" by sectionsFor().
    function sectionTitle(id) {
        switch (id) {
        case "chest": return qsTr("Chest and shoulders")
        case "back": return qsTr("Back")
        case "arms": return qsTr("Arms")
        case "abdomen": return qsTr("Abdomen")
        case "legs": return qsTr("Hips and legs")
        }
        return qsTr("Other")
    }

    readonly property var sectionKeys: ({
        "chest": ["upper-chest", "lower-chest", "shoulders"],
        "back": ["trapezius", "lats", "mid-back", "lower-back"],
        "arms": ["biceps", "triceps", "forearms"],
        "abdomen": ["core", "lower-abdomen", "obliques"],
        "legs": ["glutes", "quadriceps", "hamstrings", "calves", "shins"]
    })

    // [{title, keys}] for the keys the core offers, in section order.
    function sectionsFor(available) {
        var order = ["chest", "back", "arms", "abdomen", "legs"]
        var placed = {}
        var list = []
        for (var i = 0; i < order.length; ++i) {
            var keys = []
            var wanted = sectionKeys[order[i]]
            for (var j = 0; j < wanted.length; ++j) {
                if (available.indexOf(wanted[j]) >= 0) {
                    keys.push(wanted[j])
                    placed[wanted[j]] = true
                }
            }
            if (keys.length > 0)
                list.push({ title: sectionTitle(order[i]), keys: keys })
        }
        var rest = []
        for (var k = 0; k < available.length; ++k) {
            if (!placed[available[k]])
                rest.push(available[k])
        }
        if (rest.length > 0)
            list.push({ title: sectionTitle(""), keys: rest })
        return list
    }

    // The names of a list of group keys in one line.
    function names(keys) {
        var parts = []
        for (var i = 0; i < keys.length; ++i)
            parts.push(label(keys[i]))
        return parts.join(", ")
    }

    // A text for a place that reads markup (a page header): the characters that start markup are
    // written as entities, so that a name shows as typed.
    function plain(text) {
        return String(text).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    }

    // The reason a file is refused, for a code of the exercise import.
    function importProblem(code) {
        switch (code) {
        case "too-large": return qsTr("The file is too large for an exercise.")
        case "not-utf8": return qsTr("The file is not text.")
        case "not-json": return qsTr("The file is not an exercise file.")
        case "wrong-format": return qsTr("The file is not an exercise file.")
        case "newer-version": return qsTr("The file was made by a newer version of the app.")
        case "unsupported-version": return qsTr("The file was made by a version of the app that is no longer supported.")
        case "name-empty": return qsTr("The exercise in the file has no name.")
        case "muscles-none": return qsTr("The exercise in the file has no muscle group this app knows.")
        case "too-long": return qsTr("A text in the file is too long.")
        case "icon": return qsTr("The icon in the file is not valid.")
        case "language": return qsTr("The language in the file is not valid.")
        case "unreadable": return qsTr("The file cannot be read.")
        }
        return qsTr("The file is not a valid exercise file.")
    }

    // A note about a part of an imported file that was left out.
    function importNotice(code) {
        switch (code) {
        case "unknown-colour": return qsTr("The colour in the file is not known; the default colour is used.")
        case "unknown-group": return qsTr("A muscle group in the file is not known and was left out.")
        }
        return ""
    }

    // The reason for a code of the core; empty for none.
    function problem(code) {
        switch (code) {
        case "name-empty": return qsTr("Give the exercise a name.")
        case "name-too-long": return qsTr("The name is too long: use at most 80 characters.")
        case "instructions-too-long": return qsTr("The instructions are too long: use at most 4000 characters.")
        case "tips-too-long": return qsTr("The tips are too long: use at most 2000 characters.")
        case "muscles-none": return qsTr("Choose at least one muscle group.")
        case "muscles-unknown": return qsTr("A muscle group is not known.")
        case "icon": return qsTr("This icon cannot be used.")
        case "colour": return qsTr("This colour cannot be used.")
        case "language": return qsTr("The language is not valid.")
        case "failed": return qsTr("The exercises could not be read.")
        }
        return ""
    }
}
