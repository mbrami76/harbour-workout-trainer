# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Michele Brami

TARGET = tst_sounds

include(../testcase.pri)

DEFINES += SOUNDS_DIR=\\\"$$PWD/../../data/sounds\\\"
SOURCES += tst_sounds.cpp
