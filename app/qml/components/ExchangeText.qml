// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0

// The texts of the export and import screens (R-IMP-1, R-IMP-2). The core and the bridge give fixed
// codes; this turns them into translatable sentences. A place in a file is shown as it is, because
// it names a position and never repeats text the user wrote.
QtObject {
    // Why a file was refused, for a code of the bridge or of a problem.
    function problem(code) {
        switch (code) {
        case "unreadable": return qsTr("The file cannot be read.")
        case "too-large": return qsTr("The file is too large. A workout file is at most 4 MiB.")
        case "not-utf8": return qsTr("The file is not text in UTF-8.")
        case "not-json": return qsTr("The file is not in the JSON format.")
        case "wrong-type": return qsTr("A value has the wrong type.")
        case "wrong-format": return qsTr("The file is not a Workout Trainer workout file.")
        case "newer-version": return qsTr("The file was made by a newer version of the app.")
        case "unsupported-version": return qsTr("The file was made by a version of the app that is no longer supported.")
        case "missing": return qsTr("A required value is missing.")
        case "out-of-range": return qsTr("A value is outside its allowed range.")
        case "too-long": return qsTr("A text is too long.")
        case "too-many": return qsTr("The file has too many entries.")
        case "not-allowed": return qsTr("A value is not allowed here.")
        case "key-invalid": return qsTr("A key is not valid.")
        case "key-prefix": return qsTr("A key does not start as required.")
        case "duplicate-key": return qsTr("Two entries have the same key.")
        case "name-empty": return qsTr("A name is empty.")
        case "muscles-none": return qsTr("An exercise has no muscle group this app knows.")
        case "icon": return qsTr("An icon is not valid.")
        case "language": return qsTr("A language is not valid.")
        case "tempo": return qsTr("A pace is not valid.")
        case "unknown-exercise": return qsTr("A step uses an exercise that is neither in the file nor in the library.")
        case "unknown-metronome": return qsTr("An exercise uses a rhythm that is neither in the file nor in the library.")
        }
        return qsTr("The file is not a valid workout file.")
    }

    // A note about something in an accepted file.
    function notice(item) {
        switch (item.code) {
        case "unknown-colour": return qsTr("A colour is not known; the default colour is used.")
        case "unknown-group": return qsTr("A muscle group is not known and was left out.")
        case "exercise-skipped": return qsTr("An exercise has no muscle group this app knows and was left out.")
        case "workout-skipped": return qsTr("A workout uses an exercise that was left out, so it was not imported.")
        case "same-name": return qsTr("%n exercise(s) have the name of an exercise you already have. They are added as separate exercises.", "", item.count)
        case "nothing-to-import": return qsTr("The file holds nothing to import.")
        }
        return ""
    }

    // The result of an export that did not work.
    function exportFailure(status) {
        switch (status) {
        case "not-found": return qsTr("The workout no longer exists.")
        case "empty": return qsTr("There are no workouts to export.")
        }
        return qsTr("The file could not be written. Check the free space and try again.")
    }

    function field(name) {
        switch (name) {
        case "name": return qsTr("Name")
        case "muscles": return qsTr("Muscles")
        case "instructions": return qsTr("Instructions")
        case "tips": return qsTr("Tips")
        case "icon": return qsTr("Icon")
        case "colour": return qsTr("Colour")
        case "language": return qsTr("Language")
        case "metronome": return qsTr("Rhythm")
        }
        return name
    }
}
