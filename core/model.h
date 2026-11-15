// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef MODEL_H
#define MODEL_H

#include <QHash>
#include <QList>
#include <QString>
#include <QtGlobal>

#include "tempo.h"

// Muscle groups as bit positions of a MuscleSet. The positions are stored and must never change
// or be reused (R-MUS-4). Chest, Back and Arms are retired: stored sets and files that carry them
// are converted to the groups that replace them (see musclegroups.h). Core now means the upper abs.
enum class MuscleGroup : int {
    Chest = 0,
    Back,
    Shoulders,
    Arms,
    Core,
    Glutes,
    Quadriceps,
    Hamstrings,
    Calves,
    LowerAbdomen,
    LowerBack,
    UpperChest,
    LowerChest,
    Obliques,
    Trapezius,
    Lats,
    MidBack,
    Biceps,
    Triceps,
    Forearms,
    Shins
};

// 64 bits wide. Positions 0 to 62 can be used: the stored integer is signed, so bit 63 stays free.
typedef quint64 MuscleSet;

inline MuscleSet muscleBit(MuscleGroup group)
{
    return MuscleSet(1) << static_cast<int>(group);
}

// Number of defined positions (retired ones included) and the set of all of them; both come from
// the table of group keys in musclegroups.h (R-MUS-6). activeMuscles() leaves the retired out.
int muscleGroupCount();
MuscleSet allMuscles();
MuscleSet activeMuscles();

struct Exercise {
    QString key;
    QString name;
    MuscleSet muscles = 0;
    QString instructions;
    QString tips;
    // Optional keys; empty means the default. Unknown keys from a newer version are kept (R-EXR-8).
    QString icon;
    QString colour;
    // Language tag of the texts (R-EXR-12): lower-case letters, digits and hyphens, 2 to 35
    // characters, starting with a letter. An unknown but well-formed tag is kept.
    QString language = QStringLiteral("en");
    // Set for entries that came from the starter library; kept when the user edits the entry.
    bool starter = false;
    // Identifier of the starter library entry this exercise came from (8 lower-case hexadecimal
    // digits), empty for an entry the user made or imported (R-EXR-15).
    QString starterId;
    // True while the texts and the language tag follow the interface language; the first edit of a
    // text clears it for good (R-EXR-15).
    bool followsLanguage = false;
    // Optional saved metronome that gives the exercise its tempo (R-EXR-10): its id, 0 for none.
    // The exercise stores the link, not a copy. An id that does not exist reads as 0.
    int metronomeId = 0;
    // Key of the linked metronome inside an import file. Only the import uses it, before the
    // metronomes are stored and the id is known; the stores ignore it.
    QString metronomeKey;
};

// The rhythm of each exercise that has a metronome, by exercise key: the snapshot of the saved
// rhythms that plans, planned times and engines take. A value is the rhythm with one repetition;
// only its phases are used (Tempo::forReps()).
typedef QHash<QString, Tempo> ExerciseTempos;

enum class StepType { Work, Rest, Warmup, Cooldown };

// How the repetitions of a step are timed when its exercise has no rhythm (R-WRK-14): not at all
// (they end on a tap on "done"), in a total time, or at a time per repetition.
enum class PaceKind { None, Total, PerRep };

// A step ends by duration or by repetitions, never both (R-WRK-4). Rest steps have no exercise (R-WRK-3).
// A repetition step runs at the rhythm of its exercise when the exercise has one (R-WRK-10); the
// step stores nothing about it. A repetition step of an exercise without a rhythm may store a
// pace (R-WRK-14): paceMs is the total time or the time of one repetition, in milliseconds. A
// rhythm wins over a stored pace, which is then kept but ignored. Any step can wait for a tap
// before it starts (R-WRK-11).
struct Step {
    StepType type = StepType::Work;
    QString exerciseKey;
    int durationSec = 0;
    int reps = 0;
    bool startOnTap = false;
    PaceKind pace = PaceKind::None;
    int paceMs = 0;

    bool isTimed() const { return durationSec > 0; }
    // The pace as a tempo of this step's repetitions; invalid or empty without a pace.
    Tempo paceTempo() const;
};

struct Block {
    QList<Step> steps;
    int rounds = 1;
    int restBetweenRoundsSec = 0;
};

struct Workout {
    int id = 0;
    bool favorite = false;
    QString name;
    QList<Block> blocks;
    int restBetweenBlocksSec = 0;
    // Optional keys; empty means the default. Unknown keys from a newer version are kept
    // (R-WRK-12). Together with the name they identify the workout.
    QString icon;
    QString colour;
};

// One validation finding. blockIndex and stepIndex are -1 when the finding is not about them.
struct Problem {
    enum Code {
        NoBlocks,
        TooManyBlocks,
        RestBetweenBlocksOutOfRange,
        EmptyBlock,
        TooManySteps,
        RoundsOutOfRange,
        RestBetweenRoundsOutOfRange,
        MissingExercise,
        RestHasExercise,
        RestNotTimed,
        MissingDurationOrReps,
        BothDurationAndReps,
        DurationOutOfRange,
        RepsOutOfRange,
        NameEmpty,
        NameTooLong,
        IconInvalid,        // the icon key has other characters than a-z, 0-9 and hyphens, or is too long
        ColourUnknown,      // the colour key is not in the palette
        IdentityTaken,      // another workout has the same name, icon and colour (R-WRK-12)
        UnknownExercise,
        PaceWithDuration,   // a pace on a step that ends by duration (R-WRK-14)
        PaceRepTooShort,    // the pace gives a repetition shorter than 0.5 s
        PaceRepTooLong,     // the pace gives a repetition longer than 60 s
        DurationWithRhythm  // a step of an exercise with a rhythm ends by repetitions only (R-WRK-14)
    };

    Code code;
    int blockIndex = -1;
    int stepIndex = -1;
};

// Stored names of the step types.
QString stepTypeName(StepType type);
bool stepTypeFromName(const QString &name, StepType *type);

QString paceKindName(PaceKind kind);
bool paceKindFromName(const QString &name, PaceKind *kind);

// Findings for one step; the indices are copied into each finding. The pace is checked here.
QList<Problem> validateStep(const Step &step, int blockIndex = -1, int stepIndex = -1);

// The same for a step of an exercise that has (hasRhythm) or has not a rhythm (R-WRK-14). With a
// rhythm only repetitions are allowed (a duration is DurationWithRhythm) and a stored pace is not
// checked, because it is ignored.
QList<Problem> validateStepFor(const Step &step, bool hasRhythm, int blockIndex = -1, int stepIndex = -1);

// All findings for a workout (R-WRK-8). An empty list means the workout can run. The rhythms of
// the exercises, by exercise key, tell which stored paces are ignored and not checked; a duration
// step of an exercise with a rhythm is not refused here (use validateStepFor() on screens that
// edit a step).
QList<Problem> validate(const Workout &workout, const ExerciseTempos &rhythms = ExerciseTempos());

// Planned time of a workout without lead-ins (R-WRK-9). A paced repetition step (a rhythm of its
// exercise, or a timed pace of the step) has a known duration (its repetitions times the length of
// one repetition) and is counted in seconds; a repetition step that ends on a tap has none and is
// left out.
// secondsTimedOnly counts timed steps alone, as the requirement reads. The tempos are the
// snapshot by exercise key; see expandPlan().
struct PlannedTotal {
    int seconds = 0;
    int secondsTimedOnly = 0;
    // There are repetition steps without a tempo, which the total leaves out.
    bool hasRepSteps = false;
    // There are repetition steps with a tempo, which the total counts.
    bool hasTempoSteps = false;
};

PlannedTotal totalPlannedSeconds(const Workout &workout, const ExerciseTempos &tempos = ExerciseTempos());

// The first step of the first block starts automatically, whatever its flag says: the tap that
// opens the session is its trigger (R-WRK-11). Clears the flag there; call it wherever a workout
// is saved, read or imported.
void normalizeStartModes(Workout &workout);

// Interval workout (R-WRK-7): one block of one work step repeated for the rounds, with the rest
// between rounds. No rest follows the last round; a rest of 0 creates none.
Workout quickWorkout(const QString &name, const QString &exerciseKey, int workSec, int restSec, int rounds);

#endif
