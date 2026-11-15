# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Michele Brami

TARGET = tst_licences

include(../testcase.pri)

DEFINES += SOURCE_ROOT=\\\"$$PWD/../..\\\"
SOURCES += tst_licences.cpp
