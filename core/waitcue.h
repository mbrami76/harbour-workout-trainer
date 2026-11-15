// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef WAITCUE_H
#define WAITCUE_H

#include <QtGlobal>

// The cue that calls the user while a step waits for a tap (R-WRK-11, R-ALR-1): it is due when
// the wait begins and again every WaitCue::repeatMs until the wait ends. Pure logic: the caller asks
// with the time of its clock, so a pause, a stop or a tap ends the cue by calling end().
class WaitCue
{
public:
    // The time between two cues, in milliseconds.
    static const qint64 repeatMs = 3000;

    // The wait begins at the given time: a cue is due at once.
    void begin(qint64 nowMs);
    // The wait is over (a tap, a pause, a stop).
    void end();
    bool active() const { return active_; }
    // True once for every cue that has come due since the last call. A caller that was late, for
    // example because the app was suspended, gets one cue, not one for every period it missed.
    bool due(qint64 nowMs);

private:
    bool active_ = false;
    qint64 next_ = 0;
};

#endif
