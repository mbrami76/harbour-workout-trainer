// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0

// The texts about workouts and their steps (R-WRK-1 to R-WRK-13) shared by the list, the detail
// page, the builder and its pages. The codes come from the core; the rules and the limits are never
// repeated here: a limit named in a text is read from workoutBuilder.limits.
QtObject {
    function duration(seconds) {
        return timeFormatter.elapsed(seconds * 1000)
    }

    // The name of a workout icon (R-WRK-12), for its accessible name and the import preview. The
    // default and an unknown key read as the default.
    function iconName(key) {
        switch (key) {
        case "barbell": return qsTr("Barbell")
        case "dumbbells": return qsTr("Dumbbells")
        case "kettlebell": return qsTr("Kettlebell")
        case "medicine-ball": return qsTr("Medicine ball")
        case "cable-machine": return qsTr("Cable machine")
        case "bench": return qsTr("Bench")
        case "bicycle": return qsTr("Bicycle")
        case "treadmill": return qsTr("Treadmill")
        case "jump-rope": return qsTr("Jump rope")
        case "shoe": return qsTr("Running shoe")
        case "stopwatch": return qsTr("Stopwatch")
        case "lungs": return qsTr("Lungs")
        case "heart-pulse": return qsTr("Heartbeat")
        case "yoga": return qsTr("Yoga")
        case "glove": return qsTr("Boxing glove")
        case "abs": return qsTr("Abs")
        case "tiger": return qsTr("Tiger")
        case "bear": return qsTr("Bear")
        case "gorilla": return qsTr("Gorilla")
        case "wave": return qsTr("Wave")
        case "volcano": return qsTr("Volcano")
        case "fire": return qsTr("Fire")
        case "lightning": return qsTr("Lightning")
        case "rocket": return qsTr("Rocket")
        case "anvil": return qsTr("Anvil")
        case "target": return qsTr("Target")
        case "balance": return qsTr("Scales")
        case "summit": return qsTr("Summit")
        case "speed": return qsTr("Speed")
        case "flexibility": return qsTr("Flexibility")
        case "progress": return qsTr("Progress")
        case "recovery": return qsTr("Recovery")
        case "shock": return qsTr("Shock absorber")
        case "spring": return qsTr("Spring")
        case "scale": return qsTr("Body scale")
        }
        return qsTr("Default icon")
    }

    // One line about a workout of the list: blocks, steps and the planned time (R-WRK-9).
    function listSummary(w) {
        if (w.stepCount === 0)
            return qsTr("No steps yet")
        var total = w.hasRepSteps
            ? qsTr("%1, repetition steps that end on a tap not counted").arg(duration(w.plannedSeconds))
            : qsTr("%1 in total").arg(duration(w.plannedSeconds))
        return qsTr("%n block(s)", "", w.blockCount) + ", " + qsTr("%n step(s)", "", w.stepCount) + ", " + total
    }

    // How a step ends: its duration or its repetitions.
    function ending(row) {
        return row.timed ? duration(row.durationSec) : qsTr("%n repetition(s)", "", row.reps)
    }

    // The title of a step: its type (typeLabel, from StepStyle) and, for the types with one, its
    // exercise.
    function stepTitle(row, typeLabel) {
        var type = typeLabel
        if (row.type === "rest")
            return type
        return row.exerciseName !== "" ? type + ": " + row.exerciseName : type + ": " + qsTr("no exercise chosen")
    }

    // The rhythm of a repetition step with its name and the time of one repetition (R-WRK-10).
    function tempo(row) {
        return qsTr("Rhythm (%1): %2 s per repetition").arg(row.tempoName).arg((row.tempoRepMs / 1000).toFixed(1))
    }

    // How the repetitions of a step run, in plain words (R-WRK-14): kind is "rhythm", "total" (in a
    // total time), "per-rep" (a time per repetition) or "none" (a tap on "done"); repMs is the time of
    // one repetition.
    function paceWords(kind, reps, repMs) {
        var seconds = (repMs / 1000).toFixed(1)
        switch (kind) {
        case "rhythm": return qsTr("%n repetition(s) at the rhythm, %1 s each", "", reps).arg(seconds)
        case "total": return qsTr("%n repetition(s) in %1", "", reps).arg(duration(Math.round(reps * repMs / 1000)))
        case "per-rep": return qsTr("%n repetition(s), %1 s each", "", reps).arg(seconds)
        }
        return qsTr("%n repetition(s), tap on done", "", reps)
    }

    // The same for a step of the session (sessionBridge.current and next): empty for a step with a duration.
    function sessionPace(step) {
        if (step === undefined || step.ending === "duration" || step.type === "rest" || step.reps === undefined)
            return ""
        return paceWords(step.pace, step.reps, step.repMs)
    }

    // The lines under a step title: how it ends and how its repetitions are paced, its start mode and
    // a repetition step that the total leaves out.
    function stepDetails(row) {
        var parts = []
        if (row.timed) {
            parts.push(ending(row))
        } else {
            var kind = row.rhythmUsed ? "rhythm" : (row.paceUsed ? row.pace : "none")
            parts.push(paceWords(kind, row.reps, row.repMs))
        }
        if (row.rhythmUsed && row.tempoName !== "")
            parts.push(row.tempoName)
        if (row.notCounted)
            parts.push(qsTr("not counted in the total"))
        if (row.startOnTap)
            parts.push(qsTr("waits for a tap"))
        return parts.join(", ")
    }

    function blockTitle(row) {
        return qsTr("Block %1").arg(row.block + 1)
    }

    // The one line of a collapsed block: its rounds, its steps and its planned time.
    function blockLine(row) {
        var time = row.plannedOpen ? qsTr("%1 and more").arg(duration(row.plannedSeconds)) : duration(row.plannedSeconds)
        return qsTr("%n round(s)", "", row.rounds) + ", " + qsTr("%n step(s)", "", row.stepCount) + ", " + time
    }

    function blockDetails(row) {
        var parts = [qsTr("%n round(s)", "", row.rounds), qsTr("%n step(s)", "", row.stepCount)]
        if (row.rounds > 1 && row.restBetweenRoundsSec > 0)
            parts.push(qsTr("rest between rounds %1").arg(duration(row.restBetweenRoundsSec)))
        return parts.join(", ")
    }

    // The total planned time and the steps it leaves out (R-WRK-9); total is workoutBuilder.total or
    // the total of a detail.
    function totalText(total) {
        return qsTr("Total planned time %1").arg(duration(total.seconds))
    }

    function uncountedText(total) {
        if (total.uncounted === 0)
            return ""
        var names = total.uncountedNames.join(", ")
        return names !== ""
            ? qsTr("%n repetition step(s) are not counted because they end on a tap: %1.", "", total.uncounted).arg(names)
            : qsTr("%n repetition step(s) are not counted because they end on a tap.", "", total.uncounted)
    }

    // The reason for a code of the core; limits is workoutBuilder.limits.
    function problem(code, limits) {
        switch (code) {
        case "no-blocks": return qsTr("Add at least one block.")
        case "too-many-blocks": return qsTr("A workout can have at most %n block(s).", "", limits.maxBlocksPerWorkout)
        case "rest-between-blocks": return qsTr("The rest between blocks must be %1 to %2 seconds.").arg(limits.minRestSec).arg(limits.maxRestSec)
        case "empty-block": return qsTr("A block needs at least one step.")
        case "too-many-steps": return qsTr("A block can have at most %n step(s).", "", limits.maxStepsPerBlock)
        case "rounds": return qsTr("The rounds must be %1 to %2.").arg(limits.minRounds).arg(limits.maxRounds)
        case "rest-between-rounds": return qsTr("The rest between rounds must be %1 to %2 seconds.").arg(limits.minRestSec).arg(limits.maxRestSec)
        case "exercise-missing": return qsTr("Choose an exercise for the step.")
        case "rest-has-exercise": return qsTr("A rest step has no exercise.")
        case "rest-not-timed": return qsTr("A rest step must end by duration.")
        case "ending-missing": return qsTr("Give the step a duration or a number of repetitions.")
        case "ending-both": return qsTr("A step ends by duration or by repetitions, not both.")
        case "duration": return qsTr("The duration must be %1 to %2 seconds.").arg(limits.minDurationSec).arg(limits.maxDurationSec)
        case "reps": return qsTr("The repetitions must be %1 to %2.").arg(limits.minReps).arg(limits.maxReps)
        case "name-empty": return qsTr("Give the workout a name.")
        case "name-too-long": return qsTr("The name is too long: use at most %1 characters.").arg(limits.maxNameChars)
        case "icon": return qsTr("This icon cannot be used.")
        case "colour": return qsTr("This colour cannot be used.")
        case "identity": return qsTr("Another workout already has this name, icon and colour. Change one of them.")
        case "exercise-unknown": return qsTr("The exercise of the step no longer exists. Choose another.")
        case "pace-with-duration": return qsTr("A pace belongs to repetitions: a step that ends by duration has none.")
        case "pace-short": return qsTr("One repetition would last less than 0.5 s. Use fewer repetitions or more time.")
        case "pace-long": return qsTr("One repetition would last more than 60 s. Use more repetitions or less time.")
        case "duration-with-rhythm": return qsTr("The exercise has a rhythm: the step ends by repetitions.")
        case "failed": return qsTr("The workouts could not be read.")
        }
        return ""
    }

    // Where a finding is: "Block 2, step 3".
    function place(problem) {
        if (problem.block < 0)
            return ""
        if (problem.step < 0)
            return qsTr("Block %1").arg(problem.block + 1)
        return qsTr("Block %1, step %2").arg(problem.block + 1).arg(problem.step + 1)
    }

    // The first finding as one sentence with its place, or "" for none.
    function firstProblem(problems, limits) {
        if (problems.length === 0)
            return ""
        var text = problem(problems[0].code, limits)
        var where = place(problems[0])
        return where !== "" ? where + ": " + text : text
    }
}
