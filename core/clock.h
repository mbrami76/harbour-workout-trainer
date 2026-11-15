// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef CLOCK_H
#define CLOCK_H

#include <QtGlobal>

// Monotonic time source in milliseconds. The value never goes backwards and has no relation
// to the wall clock (R-TMR-5).
class Clock
{
public:
    virtual ~Clock() {}
    virtual qint64 nowMs() const = 0;
};

#endif
