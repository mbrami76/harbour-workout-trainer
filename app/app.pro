# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Michele Brami

TARGET = harbour-workout-trainer
CONFIG += sailfishapp
CONFIG += sailfishapp_i18n

include(../common.pri)

QT += sql multimedia

# The pure-logic library of the app.
INCLUDEPATH += $$PWD/../core
LIBS += -L$$OUT_PWD/../core -lworkouttrainercore
PRE_TARGETDEPS += $$OUT_PWD/../core/libworkouttrainercore.a

HEADERS += src/services.h src/monotonicclock.h src/timeformatter.h src/screenmetrics.h \
    src/polledbridge.h src/intervalbridge.h src/countdownbridge.h src/stopwatchbridge.h src/timeattackbridge.h src/metronomebridge.h src/metronomelibrarybridge.h src/exerciselibrarybridge.h \
    src/variantrowsmodel.h src/workoutview.h src/workoutlibrarybridge.h src/workoutbuilderbridge.h src/sessionbridge.h src/exchangebridge.h src/dashboardbridge.h src/languagebridge.h \
    src/speechbridge.h src/speechplayer.h src/speechsentences.h src/statusbridge.h src/licencesbridge.h
SOURCES += src/main.cpp src/services.cpp src/timeformatter.cpp src/polledbridge.cpp src/intervalbridge.cpp \
    src/countdownbridge.cpp src/stopwatchbridge.cpp src/timeattackbridge.cpp src/metronomebridge.cpp src/metronomelibrarybridge.cpp src/exerciselibrarybridge.cpp \
    src/variantrowsmodel.cpp src/workoutview.cpp src/workoutlibrarybridge.cpp src/workoutbuilderbridge.cpp src/sessionbridge.cpp src/exchangebridge.cpp src/dashboardbridge.cpp src/languagebridge.cpp \
    src/speechbridge.cpp src/speechplayer.cpp src/speechsentences.cpp src/statusbridge.cpp src/licencesbridge.cpp

# The version is the one of the package: the spec file is its only source.
VERSION = $$system(sed -n 's/^Version:[[:space:]]*//p' $$PWD/../rpm/harbour-workout-trainer.spec)
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

DISTFILES += qml/harbour-workout-trainer.qml \
    qml/pages/MainPage.qml \
    qml/pages/PlaceholderPage.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/LanguagePage.qml \
    qml/pages/SafetyNoticePage.qml \
    qml/components/SafetyNotice.qml \
    qml/pages/AboutPage.qml \
    qml/pages/LicencesPage.qml \
    qml/pages/LicenceTextPage.qml \
    qml/pages/IntervalPage.qml \
    qml/pages/IntervalSetup.qml \
    qml/pages/IntervalRun.qml \
    qml/pages/MetronomePage.qml \
    qml/pages/QuickStartPage.qml \
    qml/pages/PaceSetup.qml \
    qml/pages/RhythmEditor.qml \
    qml/pages/MetronomeListPage.qml \
    qml/pages/MetronomeImpactPage.qml \
    qml/pages/DeleteMetronomeDialog.qml \
    qml/pages/ExerciseListPage.qml \
    qml/pages/ExercisePage.qml \
    qml/pages/ExerciseEditPage.qml \
    qml/pages/DeleteExerciseDialog.qml \
    qml/pages/WorkoutListPage.qml \
    qml/pages/DashboardPage.qml \
    qml/pages/WorkoutPage.qml \
    qml/pages/WorkoutBuilderPage.qml \
    qml/pages/WorkoutBlockPage.qml \
    qml/pages/WorkoutStepPage.qml \
    qml/pages/ExercisePickerPage.qml \
    qml/pages/DeleteWorkoutDialog.qml \
    qml/pages/ExerciseImpactPage.qml \
    qml/pages/MetronomeRun.qml \
    qml/pages/SessionPage.qml \
    qml/pages/SessionRun.qml \
    qml/pages/ExerciseOverlayPage.qml \
    qml/pages/TimerPage.qml \
    qml/pages/CountdownSection.qml \
    qml/pages/StopwatchSection.qml \
    qml/pages/TimeAttackListPage.qml \
    qml/pages/NewChallengePage.qml \
    qml/pages/ChallengePage.qml \
    qml/pages/DeleteChallengeDialog.qml \
    qml/pages/TimeAttackRunPage.qml \
    qml/pages/ErrorPage.qml \
    qml/pages/ImportResultPage.qml \
    qml/pages/ImportPreviewPage.qml \
    qml/pages/ExportResultPage.qml \
    qml/components/qmldir \
    qml/components/ModeGrid.qml \
    qml/components/Settings.qml \
    qml/components/LanguageNames.qml \
    qml/components/CountLine.qml \
    qml/components/ExchangeText.qml \
    qml/components/Sounds.qml \
    qml/components/StepStyle.qml \
    qml/components/KeepAwake.qml \
    qml/components/DigitFont.qml \
    qml/components/TimeDigits.qml \
    qml/components/ProgressStrip.qml \
    qml/components/LedText.qml \
    qml/components/Glyph.qml \
    qml/components/HoldButton.qml \
    qml/components/BigButton.qml \
    qml/components/NumberStepper.qml \
    qml/components/Metrics.qml \
    qml/components/BeatStyle.qml \
    qml/components/BeatPulse.qml \
    qml/components/RepetitionBar.qml \
    qml/components/Swatches.qml \
    qml/components/IconBadge.qml \
    qml/components/PaceStrip.qml \
    qml/components/MarqueeLabel.qml \
    qml/components/MetronomeText.qml \
    qml/components/PuppetFigure.qml \
    qml/components/ExerciseFile.qml \
    qml/components/RunScheme.qml \
    qml/components/RunButton.qml \
    qml/components/MuscleText.qml \
    qml/components/WorkoutText.qml \
    qml/components/IconColourPicker.qml \
    qml/components/TempoDisplay.qml \
    qml/components/CaptionedControl.qml \
    qml/components/puppetfigure.js \
    qml/cover/CoverPage.qml \
    harbour-workout-trainer.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

TRANSLATIONS += translations/harbour-workout-trainer.ts translations/harbour-workout-trainer-it.ts

# Exercise icons (the SVG sources are not installed).
exerciseicons.files = $$PWD/../data/exercise-icons/*.png $$PWD/../data/exercise-icons/README.md
exerciseicons.path = /usr/share/$${TARGET}/exercise-icons
INSTALLS += exerciseicons

# Workout icons (the SVG sources are not installed).
workouticons.files = $$PWD/../data/workout-icons/*.png $$PWD/../data/workout-icons/README.md
workouticons.path = /usr/share/$${TARGET}/workout-icons
INSTALLS += workouticons

# Mode icons of the first page (the SVG sources are not installed).
modeicons.files = $$PWD/../data/mode-icons/*.png $$PWD/../data/mode-icons/README.md
modeicons.path = /usr/share/$${TARGET}/mode-icons
INSTALLS += modeicons

# Monochrome application icon, the faint background of the cover (the SVG source is not installed).
appicon.files = $$PWD/../data/app-icon/app-icon-mono.png $$PWD/../data/app-icon/README.md
appicon.path = /usr/share/$${TARGET}/app-icon
INSTALLS += appicon

# Starter exercises (R-EXR-5).
starter.files = $$PWD/../data/starter/*.json $$PWD/../data/starter/README.md
starter.path = /usr/share/$${TARGET}/starter
INSTALLS += starter

# Notice of the muscle-group pictures (the pictures themselves are not installed).
musclegroups.files = $$PWD/../data/muscle-groups/README.md
musclegroups.path = /usr/share/$${TARGET}/muscle-groups
INSTALLS += musclegroups

# Licence of the app (R-GEN-3), shown on the Licences page.
applicence.files = $$PWD/../LICENSE
applicence.path = /usr/share/$${TARGET}
INSTALLS += applicence

# Digit font and its licence (R-UI-10).
fonts.files = $$PWD/fonts/DSEG7Classic-Regular.ttf $$PWD/fonts/DSEG-LICENSE.txt
fonts.path = /usr/share/$${TARGET}/fonts
INSTALLS += fonts

# Sounds of the beats and cues, written by tools/make-sounds.py (R-ALR-1, R-ALR-3).
sounds.files = $$PWD/../data/sounds/*.wav $$PWD/../data/sounds/README.md
sounds.path = /usr/share/$${TARGET}/sounds
INSTALLS += sounds

# Spoken announcements (R-ALR-6, R-ALR-7). The internal engine exists for the phone only: it needs the
# libraries and voices of the speech bundle (tools/fetch-speech-bundle.sh) under the speech root
# (RT_SPEECH_ROOT, by default ../../build/speech-libs next to the source). Without them, or on another
# architecture, the app builds with no engine and the feature hides itself.
SPEECH_ROOT = $$(RT_SPEECH_ROOT)
isEmpty(SPEECH_ROOT): SPEECH_ROOT = $$PWD/../../build/speech-libs
contains(QT_ARCH, arm64)|contains(QT_ARCH, aarch64) {
    exists($$SPEECH_ROOT/aarch64/include/sherpa-onnx/c-api/c-api.h): CONFIG += speechengine
}
contains(QT_ARCH, arm64)|contains(QT_ARCH, aarch64) {
    speechengine: message("Speech engine: ON, speech root $$SPEECH_ROOT")
    else: message("Speech engine: OFF, nothing found in the speech root $$SPEECH_ROOT (spoken announcements are hidden)")
}
speechengine {
    DEFINES += HAVE_SHERPA
    HEADERS += src/sherpaprovider.h
    SOURCES += src/sherpaprovider.cpp
    INCLUDEPATH += $$SPEECH_ROOT/aarch64/include
    LIBS += -L$$SPEECH_ROOT/aarch64/lib -lsherpa-onnx-c-api -lonnxruntime \
        $$SPEECH_ROOT/aarch64/lib/libespeak-ng.a $$SPEECH_ROOT/aarch64/lib/libucd.a
    QMAKE_LFLAGS += -Wl,-rpath,/usr/share/$${TARGET}/lib

    speechlibs.files = $$SPEECH_ROOT/aarch64/lib/libsherpa-onnx-c-api.so $$SPEECH_ROOT/aarch64/lib/libonnxruntime.so
    speechlibs.path = /usr/share/$${TARGET}/lib
    espeakdata.files = $$SPEECH_ROOT/aarch64/espeak-ng-data
    espeakdata.path = /usr/share/$${TARGET}
    # One folder per voice, so that a voice can be dropped from the package without a change of code.
    voices.files = $$SPEECH_ROOT/aarch64/voices
    voices.path = /usr/share/$${TARGET}
    thirdparty.files = $$PWD/../third-party
    thirdparty.path = /usr/share/$${TARGET}
    INSTALLS += speechlibs espeakdata voices thirdparty
}

# Development build with a fake engine that shows the flow without sound: qmake CONFIG+=fakespeech.
# It is not part of the package build.
fakespeech {
    DEFINES += FAKE_SPEECH
}
