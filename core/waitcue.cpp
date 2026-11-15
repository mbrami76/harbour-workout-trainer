// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "waitcue.h"

void WaitCue::begin(qint64 nowMs)
{
    active_ = true;
    next_ = nowMs;
}

void WaitCue::end()
{
    active_ = false;
}

bool WaitCue::due(qint64 nowMs)
{
    if (!active_ || nowMs < next_)
        return false;
    next_ += repeatMs;
    while (next_ <= nowMs)
        next_ += repeatMs;
    return true;
}
