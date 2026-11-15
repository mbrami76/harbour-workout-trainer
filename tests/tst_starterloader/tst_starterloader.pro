# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Michele Brami

TARGET = tst_starterloader

include(../testcase.pri)

DEFINES += STARTER_SHIPPED_DIR=\\\"$$PWD/../../data/starter\\\"
SOURCES += tst_starterloader.cpp
