// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "sessioncontrols.h"

namespace SessionControls {

Controls forState(SessionState state, const PlanStep *step)
{
    Controls c;
    switch (state) {
    case SessionState::Ready:
    case SessionState::Finished:
    case SessionState::Stopped:
        return c;
    case SessionState::LeadIn:
    case SessionState::WaitingForVoice:
        c.play = Play::Pause;
        c.skipTap = Skip::NextStep;
        break;
    case SessionState::Running:
        c.play = Play::Pause;
        c.skipTap = step && step->hasTempo ? Skip::EndSet : Skip::NextStep;
        break;
    case SessionState::WaitingForReps:
        c.play = Play::Pause;
        c.skipTap = Skip::Done;
        break;
    case SessionState::Paused:
        c.play = Play::Resume;
        c.skipTap = Skip::NextStep;
        break;
    case SessionState::WaitingToStart:
        c.play = Play::Start;
        c.skipTap = Skip::NextStep;
        break;
    }
    c.skipHold = true;
    c.stopHold = true;
    return c;
}

Controls whilePreparing()
{
    Controls c;
    c.stopHold = true;
    return c;
}

}
