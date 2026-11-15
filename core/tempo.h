// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef TEMPO_H
#define TEMPO_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QtGlobal>

enum class TempoForm {
    RepsAndTotal,  // repetitions and total time
    TimePerRep,    // time per repetition and repetitions
    Phases         // 1 to 8 phases per repetition (in half seconds) and repetitions
};

// Where a beat stands inside its repetition (R-MET-3): the first beat, a beat between, or the last
// beat. A repetition with one beat has only the first; with two beats, the first and the last.
enum class BeatKind { First, Middle, Last };

// Why a tempo is refused (R-MET-2). The app layer turns the code into text.
enum class TempoProblem {
    None,
    RepsOutOfRange,    // repetitions not 1 to 999
    RepTooShort,       // one repetition lasts less than 0.5 s
    RepTooLong,        // one repetition lasts more than 60 s
    PhaseCountOutOfRange, // not 1 to 8 phases
    PhaseOutOfRange,      // a phase is not 1 to 20 half seconds
    PhaseNameTooLong      // a phase name has more than 12 characters
};

// The pace of a set of repetitions (R-MET-2, R-MET-3). A value type: the three forms keep the
// numbers the user entered, and everything else is derived from them. A tempo that breaks a rule
// has a problem() other than None, and its derived values are 0.
//
// Time inside a set is counted in milliseconds from the start of the set. In the form
// "repetitions and total time" the start of repetition i is round(i * total / repetitions),
// computed from the start of the set, so rounding never accumulates; the other forms have
// whole-millisecond repetitions.
class Tempo
{
public:
    // An empty tempo: no repetitions, refused with RepsOutOfRange.
    Tempo();

    static Tempo repsAndTotal(int reps, qint64 totalMs);
    static Tempo timePerRep(qint64 repMs, int reps);
    // Phase lengths in half seconds, 1 to 20 each; 1 to 8 phases. The optional names go with the
    // phases in order (R-MET-2): any text of at most 12 characters, an empty name meaning "no
    // name". Missing names are empty and surplus names are dropped.
    static Tempo phases(const QList<int> &phaseUnits, int reps, const QStringList &names = QStringList());
    // Characters of a name as the user sees them: extended grapheme clusters, so an accented
    // letter written as a base and a combining mark counts once, as does an emoji with its
    // modifier. Non-BMP characters are one character, not two UTF-16 units.
    static int nameLength(const QString &name);

    TempoProblem problem() const { return problem_; }
    bool isValid() const { return problem_ == TempoProblem::None; }

    // The numbers as entered.
    TempoForm form() const { return form_; }
    int reps() const { return reps_; }
    // Total time of the set (RepsAndTotal) or time per repetition (TimePerRep), in milliseconds.
    qint64 valueMs() const { return valueMs_; }
    // Number of phases (Phases); 0 for the other forms.
    int phaseCount() const { return phases_.size(); }
    // Length of phase 1 to phaseCount() in half seconds; 0 for a number outside that range.
    int phaseUnits(int number) const;
    QList<int> phaseUnitList() const { return phases_; }
    // Name of phase 1 to phaseCount(); empty without a name or for a number outside that range.
    QString phaseName(int number) const;
    // One name per phase, empty where there is none.
    QStringList phaseNameList() const { return names_; }

    // The pace of one repetition applied to another number of repetitions (R-EXR-10): the phases
    // with their names stay, the form "time per repetition" keeps its time, and the form
    // "repetitions and total time" gets the total of the new repetitions at the same mean
    // repetition (rounded to whole milliseconds), so that beats stay on the exact grid.
    // An invalid tempo or a number of repetitions outside 1 to 999 gives an invalid tempo.
    Tempo forReps(int reps) const;

    // Length of the set and offset of the start of repetition i (0-based; i = reps() gives the
    // end of the set).
    qint64 setDurationMs() const;
    qint64 repStartMs(int index) const;
    qint64 repDurationMs(int index) const;
    // Repetition that contains the offset from the start of the set; clamped to the set.
    int repIndexAt(qint64 setOffsetMs) const;

    // Beats inside one repetition: one at 0 for RepsAndTotal and TimePerRep, one at the start of
    // every phase for Phases. Offsets are from the start of the repetition.
    int beatsPerRep() const { return beatOffsets_.size(); }
    qint64 beatOffsetMs(int beat) const;
    QList<qint64> beatOffsetsMs() const { return beatOffsets_; }
    // Only the first beat of a repetition is accented.
    static bool isAccent(int beat) { return beat == 0; }
    BeatKind beatKind(int beat) const;
    // Phase number 1 to phaseCount() of a beat, or 0 for the forms without phases.
    int beatPhase(int beat) const;
    // Phase number at an offset inside a repetition (the offset is clamped), 0 without phases.
    int phaseAt(qint64 repOffsetMs) const;
    // Beat that starts the phase (or the repetition) that contains the offset in repetition
    // repIndex, and the length of that phase (or of the repetition).
    int beatIndexAt(qint64 repOffsetMs) const;
    qint64 beatSpanMs(int beat, int repIndex) const;

    // Plain text without words, for storage and files: "a;reps;totalMs", "b;repMs;reps" or
    // "c;p1;...;pn;reps" with 1 to 8 phases; a tempo with at least one phase name is written
    // "d;p1;...;pn;reps;name1;...;namen", each name percent-encoded (UTF-8) so that any text,
    // separators included, survives. fromText() accepts only text that toText() could have produced for a
    // valid tempo.
    QString toText() const;
    static bool fromText(const QString &text, Tempo *tempo);

    bool operator==(const Tempo &other) const;
    bool operator!=(const Tempo &other) const { return !(*this == other); }

private:
    void derive();
    qint64 phaseSumMs() const;

    TempoForm form_ = TempoForm::TimePerRep;
    int reps_ = 0;
    qint64 valueMs_ = 0;
    QList<int> phases_;
    QStringList names_;
    TempoProblem problem_ = TempoProblem::RepsOutOfRange;
    QList<qint64> beatOffsets_;
    QList<int> beatPhases_;
};

#endif
