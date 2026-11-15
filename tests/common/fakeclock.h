// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef FAKECLOCK_H
#define FAKECLOCK_H

#include "clock.h"

// Clock that only moves when a test says so.
class FakeClock : public Clock
{
public:
    qint64 nowMs() const override { return now_; }
    void set(qint64 ms) { now_ = ms; }
    void advance(qint64 ms) { now_ += ms; }

private:
    qint64 now_ = 0;
};

#endif
