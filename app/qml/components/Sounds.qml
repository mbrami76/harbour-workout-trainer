// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0
import QtMultimedia 5.0

// The playback service of the app (R-ALR-1, R-ALR-2, R-ALR-12). Every file is loaded once, when the
// app starts, into its own effect, so nothing is allocated or decoded while a run goes. The pages
// call play() from the handler of the core event that also starts the pulse and the vibration; the
// sound is never started by a timer of its own.
//
// Sounds follow the system volume: the app has no volume control. Nothing sounds while the app is
// not active. A sound whose switch in the settings is off is not played; its pulse and vibration
// are not touched.
//
// This file is the only one that knows how a sound is output. To change the output (for example to
// a push-mode audio stream kept open), replace the effects below and keep play(name, atMs).
Item {
    id: sounds

    // The settings singleton, set by the main file when the app starts. This file does not import
    // its own folder: a singleton of a folder that imports the folder never finishes loading.
    property var settings: null

    // Names accepted by play(): "click-first", "click-middle", "click-last", "lead-in-tick",
    // "tick", "work-start", "rest-start", "round-change", "timer-end", "workout-complete", "wait-for-tap".
    // atMs is the scheduled time of the event; it is accepted so that the output can place the
    // sound relative to it.
    function play(name, atMs) {
        if (!Qt.application.active || !allowed(name))
            return
        var effect = effects[name]
        if (effect)
            effect.play()
    }

    // The switch of the settings that a sound belongs to (R-ALR-2).
    function allowed(name) {
        switch (name) {
        case "click-first":
        case "click-middle":
        case "click-last":
            return settings !== null && settings.soundMetronome
        case "lead-in-tick":
        case "tick":
            return settings !== null && settings.soundTicks
        case "work-start":
            return settings !== null && settings.soundWorkStart
        case "rest-start":
            return settings !== null && settings.soundRestStart
        case "wait-for-tap":
            return settings !== null && settings.soundWaitForTap
        case "round-change":
            return settings !== null && settings.soundRoundChange
        case "timer-end":
        case "workout-complete":
            return settings !== null && settings.soundWorkoutComplete
        }
        return false
    }

    // The sound that plays for a beat of the given kind: "first", "middle" or "last".
    function clickOf(kind) {
        return kind === "first" ? "click-first" : (kind === "last" ? "click-last" : "click-middle")
    }

    readonly property var effects: ({
        "click-first": clickFirst,
        "click-middle": clickMiddle,
        "click-last": clickLast,
        "lead-in-tick": leadInTick,
        "tick": tick,
        "work-start": workStart,
        "rest-start": restStart,
        "round-change": roundChange,
        "wait-for-tap": waitForTap,
        "timer-end": timerEnd,
        "workout-complete": workoutComplete
    })

    SoundEffect { id: clickFirst; source: Qt.resolvedUrl("../../sounds/click-first.wav") }
    SoundEffect { id: clickMiddle; source: Qt.resolvedUrl("../../sounds/click-middle.wav") }
    SoundEffect { id: clickLast; source: Qt.resolvedUrl("../../sounds/click-last.wav") }
    SoundEffect { id: leadInTick; source: Qt.resolvedUrl("../../sounds/lead-in-tick.wav") }
    SoundEffect { id: tick; source: Qt.resolvedUrl("../../sounds/tick.wav") }
    SoundEffect { id: workStart; source: Qt.resolvedUrl("../../sounds/work-start.wav") }
    SoundEffect { id: restStart; source: Qt.resolvedUrl("../../sounds/rest-start.wav") }
    SoundEffect { id: roundChange; source: Qt.resolvedUrl("../../sounds/round-change.wav") }
    SoundEffect { id: waitForTap; source: Qt.resolvedUrl("../../sounds/wait-for-tap.wav") }
    SoundEffect { id: timerEnd; source: Qt.resolvedUrl("../../sounds/timer-end.wav") }
    SoundEffect { id: workoutComplete; source: Qt.resolvedUrl("../../sounds/workout-complete.wav") }
}
