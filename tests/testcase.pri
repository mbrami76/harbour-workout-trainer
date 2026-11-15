# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Michele Brami

# Shared settings of the test programs: each one links the core library and includes its headers.
CONFIG += testcase
QT += testlib sql
QT -= gui

include($$PWD/../common.pri)

INCLUDEPATH += $$PWD/../core $$PWD/common
LIBS += -L$$OUT_PWD/../../core -lworkouttrainercore
PRE_TARGETDEPS += $$OUT_PWD/../../core/libworkouttrainercore.a
