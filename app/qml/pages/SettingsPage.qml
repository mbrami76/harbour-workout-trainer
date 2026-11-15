// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import QtFeedback 5.0
import "../components"

Page {
    id: page
    allowedOrientations: Orientation.All

    // The languages that have a voice, by their own names.
    readonly property string voiceNames: {
        var names = []
        for (var i = 0; i < speech.voiceLanguages.length; ++i)
            names.push(LanguageNames.name(speech.voiceLanguages[i]))
        return names.join(", ")
    }

    // Asked only for `supported`: the option is offered where the platform can vibrate (R-ALR-4).
    ThemeEffect {
        id: vibrationProbe
        effect: ThemeEffect.PressWeak
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Settings") }

            ComboBox {
                label: qsTr("Language")
                currentIndex: Math.max(0, languageBridge.languages.indexOf(languageBridge.language))
                menu: ContextMenu {
                    Repeater {
                        model: languageBridge.languages
                        MenuItem {
                            text: LanguageNames.name(modelData)
                            onClicked: {
                                Settings.setLanguage(modelData)
                                languageBridge.switchTo(modelData)
                            }
                        }
                    }
                }
            }

            Slider {
                width: parent.width
                label: qsTr("Get-ready time before a step")
                minimumValue: Settings.minLeadInSec
                maximumValue: Settings.maxLeadInSec
                stepSize: 1
                value: Settings.leadInSec
                valueText: qsTr("%n second(s)", "", Math.round(value))
                onReleased: Settings.setLeadInSec(value)
            }

            ComboBox {
                label: qsTr("Time display")
                currentIndex: Settings.displayMode === "direct" ? 1 : 0
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Count-down (time left)")
                        onClicked: Settings.setDisplayMode("countdown")
                    }
                    MenuItem {
                        text: qsTr("Direct (time elapsed)")
                        onClicked: Settings.setDisplayMode("direct")
                    }
                }
            }

            ComboBox {
                label: qsTr("Digit font")
                currentIndex: Settings.digitFont === "system" ? 1 : 0
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Seven-segment")
                        onClicked: Settings.setDigitFont("segment")
                    }
                    MenuItem {
                        text: qsTr("System monospace")
                        onClicked: Settings.setDigitFont("system")
                    }
                }
            }

            SectionHeader { text: qsTr("Sounds") }

            TextSwitch {
                automaticCheck: false
                checked: Settings.soundMetronome
                text: qsTr("Metronome clicks")
                description: qsTr("One click for every beat")
                onClicked: Settings.setSoundMetronome(!checked)
            }
            TextSwitch {
                automaticCheck: false
                checked: Settings.soundWorkStart
                text: qsTr("Work starts")
                onClicked: Settings.setSoundWorkStart(!checked)
            }
            TextSwitch {
                automaticCheck: false
                checked: Settings.soundRestStart
                text: qsTr("Rest starts")
                onClicked: Settings.setSoundRestStart(!checked)
            }
            TextSwitch {
                automaticCheck: false
                checked: Settings.soundTicks
                text: qsTr("Last seconds")
                onClicked: Settings.setSoundTicks(!checked)
            }
            TextSwitch {
                automaticCheck: false
                checked: Settings.soundRoundChange
                text: qsTr("New round")
                onClicked: Settings.setSoundRoundChange(!checked)
            }
            TextSwitch {
                automaticCheck: false
                checked: Settings.soundWaitForTap
                text: qsTr("Step waits for a tap")
                onClicked: Settings.setSoundWaitForTap(!checked)
            }
            TextSwitch {
                automaticCheck: false
                checked: Settings.soundWorkoutComplete
                text: qsTr("Workout or timer complete")
                onClicked: Settings.setSoundWorkoutComplete(!checked)
            }

            SectionHeader { text: qsTr("Voice") }
            TextSwitch {
                automaticCheck: false
                enabled: speech.available
                checked: speech.available && Settings.spokenAnnouncements
                text: qsTr("Spoken announcements")
                description: {
                    switch (speech.status) {
                    case "ok": return qsTr("Voices: %1").arg(page.voiceNames)
                    case "no-voice-for-app-language": return qsTr("No voice for the language of the app. Voices: %1").arg(page.voiceNames)
                    case "no-voices": return qsTr("No voice is installed")
                    default: return qsTr("Speech is not available on this device")
                    }
                }
                onClicked: Settings.setSpokenAnnouncements(!Settings.spokenAnnouncements)
            }
            TextSwitch {
                automaticCheck: false
                enabled: speech.available && Settings.spokenAnnouncements
                checked: Settings.spokenCues
                text: qsTr("Spoken cues")
                description: qsTr("Half time, ten seconds left and the repetitions left")
                onClicked: Settings.setSpokenCues(!Settings.spokenCues)
            }
            ComboBox {
                enabled: speech.available && Settings.spokenAnnouncements
                label: qsTr("Spoken phase names")
                currentIndex: Settings.spokenPhaseNames
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("None")
                        onClicked: Settings.setSpokenPhaseNames(0)
                    }
                    MenuItem {
                        text: qsTr("First repetition")
                        onClicked: Settings.setSpokenPhaseNames(1)
                    }
                    MenuItem {
                        text: qsTr("First 3 repetitions")
                        onClicked: Settings.setSpokenPhaseNames(2)
                    }
                    MenuItem {
                        text: qsTr("All repetitions")
                        onClicked: Settings.setSpokenPhaseNames(3)
                    }
                }
            }

            SectionHeader {
                text: qsTr("Vibration")
                visible: vibrationProbe.supported
            }
            TextSwitch {
                visible: vibrationProbe.supported
                automaticCheck: false
                checked: Settings.vibration
                text: qsTr("Vibrate with the beats and step changes")
                onClicked: Settings.setVibration(!checked)
            }
        }

        VerticalScrollDecorator { }
    }
}
