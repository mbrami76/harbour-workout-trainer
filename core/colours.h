// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef COLOURS_H
#define COLOURS_H

#include <QString>
#include <QStringList>

// Colour keys of the exercise palette (R-EXR-8). The table holds keys only: the application maps
// them to colours. The empty key means the default colour and is not in the table.
namespace Colours {

// All keys in table order.
QStringList keys();

// True for a key of the table; false for the empty key and for unknown keys.
bool isKnown(const QString &key);

}

#endif
