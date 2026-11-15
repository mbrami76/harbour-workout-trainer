// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SESSIONCONTROLS_H
#define SESSIONCONTROLS_H

#include "engine.h"
#include "plan.h"

// What the three buttons of the session screen do (R-UI-2, R-TMR-3, R-TMR-4, R-TMR-11, R-WRK-11).
// Pure logic: the state of the engine and the step decide.
namespace SessionControls {

// The play button: it pauses while time runs, resumes while the session is paused and starts a step
// that waits for a tap. None when it has nothing to do.
enum class Play { None, Pause, Resume, Start };

// What a tap on the skip button does. All of them go to the next step in the block; the kinds only
// tell the user what the tap means: NextStep, Done (a repetition step without a tempo ends when the
// user says so) and EndSet (a paced step ends early).
enum class Skip { None, NextStep, Done, EndSet };

struct Controls {
    Play play = Play::None;
    Skip skipTap = Skip::None;
    // A long press on the skip button goes to the next block.
    bool skipHold = false;
    // A long press on the stop button ends the session.
    bool stopHold = false;
};

// The controls in the given state; step is the current step of the plan, null when there is none.
Controls forState(SessionState state, const PlanStep *step);

// The controls before a session has begun, while the voice is prepared: only stop works.
Controls whilePreparing();

}

#endif
