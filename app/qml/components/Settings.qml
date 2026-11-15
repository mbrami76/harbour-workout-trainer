// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0
import Nemo.Configuration 1.0

// The only place where the app keeps settings (R-NFR-3). Every key has a typed, sanitised read
// property and a setter function. A stored value that is missing, of the wrong type or out of its
// range reads as the default, so a damaged or foreign value never reaches the rest of the app.
// The values live in the platform's configuration store under /apps/harbour-workout-trainer.
Item {
    id: settings

    readonly property string root: "/apps/harbour-workout-trainer/"

    // Defaults and limits, for the pages that show the keys.
    readonly property int defaultLeadInSec: 3
    readonly property int minLeadInSec: 0
    readonly property int maxLeadInSec: 10
    readonly property string defaultDisplayMode: "countdown"
    readonly property string defaultTickSeconds: "10,5,3,2,1"
    readonly property int defaultIntervalWorkSec: 30
    readonly property int minIntervalWorkSec: 1
    readonly property int maxIntervalWorkSec: 3600
    readonly property int defaultIntervalRecoverySec: 10
    readonly property int minIntervalRecoverySec: 0
    readonly property int maxIntervalRecoverySec: 600

    // Lead-in before a step, whole seconds, 0 to 10 (R-TMR-1).
    readonly property int leadInSec: wholeNumber(leadInValue.value, minLeadInSec, maxLeadInSec, defaultLeadInSec)
    function setLeadInSec(seconds) {
        leadInValue.value = clamp(Math.round(seconds), minLeadInSec, maxLeadInSec)
    }

    // How a timer with a set time shows its digits: "countdown" (time left) or "direct" (time
    // elapsed). Applies to all modes (R-UI-8).
    readonly property string displayMode: displayModeValue.value === "direct" ? "direct" : defaultDisplayMode
    function setDisplayMode(mode) {
        displayModeValue.value = mode === "direct" ? "direct" : "countdown"
    }

    // The language of the interface and of the starter library (R-I18N-5): "en" or "it", empty until
    // the first choice. Any other stored value reads as no choice.
    readonly property string language: (languageValue.value === "en" || languageValue.value === "it") ? languageValue.value : ""
    function setLanguage(tag) {
        if (tag === "en" || tag === "it")
            languageValue.value = tag
    }

    // True once the user has confirmed the safety notice at the first start (R-GEN-5). Missing or
    // damaged reads as not confirmed; the value is independent of the language.
    readonly property bool safetyNoticeRead: flag(safetyNoticeValue.value, false)
    function setSafetyNoticeRead() { safetyNoticeValue.value = true }

    // Remaining whole seconds at which the last-seconds tick sounds, largest first (R-ALR-1).
    // Stored as text, "10,5,3,2,1"; read as a list of 1 to 10 numbers from 1 to 60.
    readonly property var tickSeconds: parseTicks(tickValue.value)
    function setTickSeconds(list) {
        var clean = sanitizeTicks(list)
        if (clean.length > 0)
            tickValue.value = clean.join(",")
    }

    // One switch per sound event (R-ALR-2). All default to on.
    readonly property bool soundWorkStart: flag(workStartValue.value, true)
    readonly property bool soundRestStart: flag(restStartValue.value, true)
    readonly property bool soundTicks: flag(ticksValue.value, true)
    readonly property bool soundRoundChange: flag(roundChangeValue.value, true)
    readonly property bool soundWorkoutComplete: flag(completeValue.value, true)
    readonly property bool soundWaitForTap: flag(waitForTapValue.value, true)
    // The clicks of the metronome beats, in the metronome mode and in paced workout steps.
    readonly property bool soundMetronome: flag(metronomeClickValue.value, true)
    function setSoundMetronome(on) { metronomeClickValue.value = on === true }
    function setSoundWorkStart(on) { workStartValue.value = on === true }
    function setSoundRestStart(on) { restStartValue.value = on === true }
    function setSoundTicks(on) { ticksValue.value = on === true }
    function setSoundRoundChange(on) { roundChangeValue.value = on === true }
    function setSoundWorkoutComplete(on) { completeValue.value = on === true }
    function setSoundWaitForTap(on) { waitForTapValue.value = on === true }

    // Vibration with every beat and step change (R-ALR-4); off by default.
    // Spoken announcements (R-ALR-6): on by default; they are spoken only where a voice exists.
    readonly property bool spokenAnnouncements: flag(spokenValue.value, true)
    function setSpokenAnnouncements(on) { spokenValue.value = on === true }

    // The switch "Spoken cues" (R-ALR-15): half time, ten seconds left and the repetitions left; on by default.
    readonly property bool spokenCues: flag(spokenCuesValue.value, true)
    function setSpokenCues(on) { spokenCuesValue.value = on === true }
    // How many repetitions have the names of their phases spoken (R-ALR-15): 0 none, 1 the first
    // repetition, 2 the first three (the default), 3 all.
    readonly property int defaultSpokenPhaseNames: 2
    readonly property int spokenPhaseNames: wholeNumber(spokenPhaseNamesValue.value, 0, 3, defaultSpokenPhaseNames)
    function setSpokenPhaseNames(choice) { spokenPhaseNamesValue.value = clamp(Math.round(choice), 0, 3) }

    readonly property bool vibration: flag(vibrationValue.value, false)
    function setVibration(on) { vibrationValue.value = on === true }

    // The values of the last interval timer run, offered again next time (R-MOD-2).
    readonly property int intervalWorkSec: wholeNumber(intervalWorkValue.value, minIntervalWorkSec,
                                                       maxIntervalWorkSec, defaultIntervalWorkSec)
    readonly property int intervalRecoverySec: wholeNumber(intervalRecoveryValue.value, minIntervalRecoverySec,
                                                           maxIntervalRecoverySec, defaultIntervalRecoverySec)
    function setIntervalWorkSec(seconds) {
        intervalWorkValue.value = clamp(Math.round(seconds), minIntervalWorkSec, maxIntervalWorkSec)
    }
    function setIntervalRecoverySec(seconds) {
        intervalRecoveryValue.value = clamp(Math.round(seconds), minIntervalRecoverySec, maxIntervalRecoverySec)
    }

    // Font of the digits of a time (R-UI-10): "segment" (seven-segment, bundled) or "system" (the
    // monospace font of the platform).
    readonly property string digitFont: digitFontValue.value === "system" ? "system" : "segment"
    function setDigitFont(font) { digitFontValue.value = font === "system" ? "system" : "segment" }

    // The last duration of the countdown of the timer screen, whole seconds, 1 s to 99 h 59 min 59 s (R-MOD-3).
    readonly property int defaultCountdownSec: 300
    readonly property int minCountdownSec: 1
    readonly property int maxCountdownSec: 359999
    readonly property int countdownSec: wholeNumber(countdownValue.value, minCountdownSec, maxCountdownSec,
                                                    defaultCountdownSec)
    function setCountdownSec(seconds) {
        countdownValue.value = clamp(Math.round(seconds), minCountdownSec, maxCountdownSec)
    }

    // The values of the last Auto-pace or Manual-pace run, offered again next time (R-MOD-7, R-MOD-8).
    readonly property int metronomeReps: wholeNumber(metronomeRepsValue.value, 1, 999, 10)
    readonly property int metronomeTotalSec: wholeNumber(metronomeTotalValue.value, 1, 59940, 30)
    readonly property int metronomeRepHalfSec: wholeNumber(metronomeRepValue.value, 0, 130, 4)
    readonly property int metronomeSets: wholeNumber(metronomeSetsValue.value, 1, 99, 3)
    readonly property int metronomeRestSec: wholeNumber(metronomeRestValue.value, 0, 600, 30)
    function setMetronomeReps(n) { metronomeRepsValue.value = clamp(Math.round(n), 1, 999) }
    function setMetronomeTotalSec(n) { metronomeTotalValue.value = clamp(Math.round(n), 1, 59940) }
    function setMetronomeRepHalfSec(n) { metronomeRepValue.value = clamp(Math.round(n), 0, 130) }
    function setMetronomeSets(n) { metronomeSetsValue.value = clamp(Math.round(n), 1, 99) }
    function setMetronomeRestSec(n) { metronomeRestValue.value = clamp(Math.round(n), 0, 600) }

    function clamp(value, low, high) {
        return Math.max(low, Math.min(high, value))
    }

    // A whole number inside the limits, or the default.
    function wholeNumber(value, low, high, fallback) {
        return (typeof value === "number" && isFinite(value) && Math.floor(value) === value
                && value >= low && value <= high) ? value : fallback
    }

    function flag(value, fallback) {
        return typeof value === "boolean" ? value : fallback
    }

    // The tick list: whole numbers 1 to 60, no repeats, largest first, at most 10.
    function sanitizeTicks(list) {
        var seen = {}
        var clean = []
        for (var i = 0; list && i < list.length && clean.length < 10; ++i) {
            var n = Number(list[i])
            if (isFinite(n) && Math.floor(n) === n && n >= 1 && n <= 60 && !seen[n]) {
                seen[n] = true
                clean.push(n)
            }
        }
        clean.sort(function (a, b) { return b - a })
        return clean
    }

    function parseTicks(text) {
        if (typeof text !== "string")
            return parseTicks(defaultTickSeconds)
        var parts = text.split(",")
        var clean = sanitizeTicks(parts)
        // Text with an invalid entry is a damaged value as a whole.
        return (clean.length > 0 && clean.length === parts.length) ? clean : parseTicks(defaultTickSeconds)
    }

    ConfigurationValue { id: languageValue; key: settings.root + "language"; defaultValue: "" }
    ConfigurationValue { id: safetyNoticeValue; key: settings.root + "safetyNoticeRead"; defaultValue: false }
    ConfigurationValue { id: leadInValue; key: settings.root + "leadInSec"; defaultValue: settings.defaultLeadInSec }
    ConfigurationValue { id: displayModeValue; key: settings.root + "displayMode"; defaultValue: settings.defaultDisplayMode }
    ConfigurationValue { id: tickValue; key: settings.root + "tickSeconds"; defaultValue: settings.defaultTickSeconds }
    ConfigurationValue { id: workStartValue; key: settings.root + "soundWorkStart"; defaultValue: true }
    ConfigurationValue { id: metronomeClickValue; key: settings.root + "soundMetronome"; defaultValue: true }
    ConfigurationValue { id: restStartValue; key: settings.root + "soundRestStart"; defaultValue: true }
    ConfigurationValue { id: ticksValue; key: settings.root + "soundTicks"; defaultValue: true }
    ConfigurationValue { id: roundChangeValue; key: settings.root + "soundRoundChange"; defaultValue: true }
    ConfigurationValue { id: waitForTapValue; key: settings.root + "soundWaitForTap"; defaultValue: true }
    ConfigurationValue { id: completeValue; key: settings.root + "soundWorkoutComplete"; defaultValue: true }
    ConfigurationValue { id: spokenValue; key: settings.root + "spokenAnnouncements"; defaultValue: true }
    ConfigurationValue { id: spokenCuesValue; key: settings.root + "spokenCues"; defaultValue: true }
    ConfigurationValue { id: spokenPhaseNamesValue; key: settings.root + "spokenPhaseNames"; defaultValue: settings.defaultSpokenPhaseNames }
    ConfigurationValue { id: vibrationValue; key: settings.root + "vibration"; defaultValue: false }
    ConfigurationValue { id: digitFontValue; key: settings.root + "digitFont"; defaultValue: "segment" }
    ConfigurationValue { id: countdownValue; key: settings.root + "countdownSec"; defaultValue: settings.defaultCountdownSec }
    ConfigurationValue { id: metronomeRepsValue; key: settings.root + "metronomeReps"; defaultValue: 10 }
    ConfigurationValue { id: metronomeTotalValue; key: settings.root + "metronomeTotalSec"; defaultValue: 30 }
    ConfigurationValue { id: metronomeRepValue; key: settings.root + "metronomeRepHalfSec"; defaultValue: 4 }
    ConfigurationValue { id: metronomeSetsValue; key: settings.root + "metronomeSets"; defaultValue: 3 }
    ConfigurationValue { id: metronomeRestValue; key: settings.root + "metronomeRestSec"; defaultValue: 30 }
    ConfigurationValue { id: intervalWorkValue; key: settings.root + "intervalWorkSec"; defaultValue: settings.defaultIntervalWorkSec }
    ConfigurationValue { id: intervalRecoveryValue; key: settings.root + "intervalRecoverySec"; defaultValue: settings.defaultIntervalRecoverySec }
}
