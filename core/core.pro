# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Michele Brami

TEMPLATE = lib
TARGET = workouttrainercore
CONFIG += staticlib
QT -= gui
QT += sql

include(../common.pri)

HEADERS += timeformat.h workoutlimits.h model.h plan.h clock.h engine.h database.h exercisestore.h workoutstore.h sessionlog.h colours.h musclegroups.h exchange.h starterloader.h timeattackstore.h countdown.h stopwatch.h intervaltimer.h tempo.h metronome.h identity.h metronomestore.h icons.h metronomeform.h exerciseform.h exerciseexchange.h workoutdraft.h workoutform.h statistics.h language.h speechaudio.h speechengine.h fakespeech.h speech.h spokencues.h speechprep.h speechtext.h sessionexercise.h waitcue.h sessioncontrols.h statusline.h licences.h startflow.h
SOURCES += timeformat.cpp model.cpp plan.cpp engine.cpp database.cpp exercisestore.cpp workoutstore.cpp sessionlog.cpp colours.cpp musclegroups.cpp exchange.cpp starterloader.cpp timeattackstore.cpp countdown.cpp stopwatch.cpp intervaltimer.cpp tempo.cpp metronome.cpp identity.cpp metronomestore.cpp icons.cpp metronomeform.cpp exerciseform.cpp exerciseexchange.cpp workoutdraft.cpp workoutform.cpp statistics.cpp language.cpp speechaudio.cpp speechengine.cpp fakespeech.cpp speech.cpp spokencues.cpp speechprep.cpp speechtext.cpp sessionexercise.cpp waitcue.cpp sessioncontrols.cpp statusline.cpp licences.cpp startflow.cpp
