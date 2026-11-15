# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Michele Brami

TEMPLATE = subdirs

core.subdir = core
app.subdir = app
app.depends = core
tests.subdir = tests
tests.depends = core

SUBDIRS += core

# Host tests build only the library and the tests; the package build only the library and the app.
hostbuild {
    SUBDIRS += tests
} else {
    SUBDIRS += app
}

OTHER_FILES += rpm/harbour-workout-trainer.spec
