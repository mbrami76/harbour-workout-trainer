// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef MONOTONICCLOCK_H
#define MONOTONICCLOCK_H

#include <time.h>

#include "clock.h"

// The platform's monotonic clock, in milliseconds (R-TMR-5). It stops for nothing the app can
// control and is unrelated to the wall clock.
class MonotonicClock : public Clock
{
public:
    qint64 nowMs() const override
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return qint64(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
    }
};

#endif
