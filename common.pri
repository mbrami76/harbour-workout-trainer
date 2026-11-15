# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Michele Brami

CONFIG += c++11
CONFIG += no_testcase_installs

# CONFIG+=hostbuild builds the pure-logic library and its tests without the application.
hostbuild {
    DEFINES += HOSTBUILD
}
