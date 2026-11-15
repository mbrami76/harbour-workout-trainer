// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "tempo.h"

#include <QTextBoundaryFinder>
#include <QVector>

#include "workoutlimits.h"

Tempo::Tempo()
{
}

Tempo Tempo::repsAndTotal(int reps, qint64 totalMs)
{
    Tempo t;
    t.form_ = TempoForm::RepsAndTotal;
    t.reps_ = reps;
    t.valueMs_ = totalMs;
    t.derive();
    return t;
}

Tempo Tempo::timePerRep(qint64 repMs, int reps)
{
    Tempo t;
    t.form_ = TempoForm::TimePerRep;
    t.reps_ = reps;
    t.valueMs_ = repMs;
    t.derive();
    return t;
}

Tempo Tempo::phases(const QList<int> &phaseUnits, int reps, const QStringList &names)
{
    Tempo t;
    t.form_ = TempoForm::Phases;
    t.reps_ = reps;
    t.phases_ = phaseUnits;
    for (int i = 0; i < phaseUnits.size(); ++i)
        t.names_.append(i < names.size() ? names.at(i) : QString());
    t.derive();
    return t;
}

Tempo Tempo::forReps(int reps) const
{
    if (!isValid())
        return Tempo();
    switch (form_) {
    case TempoForm::RepsAndTotal:
        return repsAndTotal(reps, (valueMs_ * reps + reps_ / 2) / reps_);
    case TempoForm::TimePerRep:
        return timePerRep(valueMs_, reps);
    case TempoForm::Phases:
        return phases(phases_, reps, names_);
    }
    return Tempo();
}

int Tempo::nameLength(const QString &name)
{
    // The grapheme finder of Qt 5.6 follows an older Unicode rule: it splits an emoji from its
    // skin-tone modifier and from the next part of a sequence joined with a zero width joiner, and
    // it puts a run of regional indicators (flags) into one cluster. Those cases are corrected
    // here, so that an emoji with a modifier, a joined sequence and a flag each count once.
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, name);
    int count = 0;
    int start = 0;
    bool previousJoins = false;
    for (int end = finder.toNextBoundary(); end != -1; start = end, end = finder.toNextBoundary()) {
        const QVector<uint> code = name.mid(start, end - start).toUcs4();
        if (code.isEmpty())
            continue;
        const uint first = code.first();
        const bool modifier = first >= 0x1F3FB && first <= 0x1F3FF;
        bool allIndicators = true;
        for (uint c : code)
            allIndicators = allIndicators && c >= 0x1F1E6 && c <= 0x1F1FF;
        if (allIndicators)
            count += (code.size() + 1) / 2;
        else if (!(count > 0 && (modifier || previousJoins)))
            ++count;
        previousJoins = code.last() == 0x200D;
    }
    return count;
}

QString Tempo::phaseName(int number) const
{
    return (number >= 1 && number <= names_.size()) ? names_.at(number - 1) : QString();
}

int Tempo::phaseUnits(int number) const
{
    return (number >= 1 && number <= phases_.size()) ? phases_.at(number - 1) : 0;
}

qint64 Tempo::phaseSumMs() const
{
    qint64 units = 0;
    for (int unit : phases_)
        units += unit;
    return units * Limits::halfSecondMs;
}

void Tempo::derive()
{
    beatOffsets_.clear();
    beatPhases_.clear();
    problem_ = TempoProblem::None;

    if (reps_ < Limits::minReps || reps_ > Limits::maxReps) {
        problem_ = TempoProblem::RepsOutOfRange;
        return;
    }
    switch (form_) {
    case TempoForm::RepsAndTotal:
        // The shortest repetition is the mean one: total / reps against the limits, without division.
        if (valueMs_ < qint64(Limits::minRepMs) * reps_)
            problem_ = TempoProblem::RepTooShort;
        else if (valueMs_ > qint64(Limits::maxRepMs) * reps_)
            problem_ = TempoProblem::RepTooLong;
        break;
    case TempoForm::TimePerRep:
        if (valueMs_ < Limits::minRepMs)
            problem_ = TempoProblem::RepTooShort;
        else if (valueMs_ > Limits::maxRepMs)
            problem_ = TempoProblem::RepTooLong;
        break;
    case TempoForm::Phases:
        if (phases_.size() < Limits::minPhases || phases_.size() > Limits::maxPhases) {
            problem_ = TempoProblem::PhaseCountOutOfRange;
            return;
        }
        for (int unit : phases_) {
            if (unit < Limits::minPhaseUnits || unit > Limits::maxPhaseUnits) {
                problem_ = TempoProblem::PhaseOutOfRange;
                return;
            }
        }
        for (const QString &name : names_) {
            if (nameLength(name) > Limits::maxPhaseNameChars) {
                problem_ = TempoProblem::PhaseNameTooLong;
                return;
            }
        }
        // Eight long phases can exceed the longest repetition.
        if (phaseSumMs() > Limits::maxRepMs)
            problem_ = TempoProblem::RepTooLong;
        break;
    }
    if (problem_ != TempoProblem::None)
        return;

    if (form_ == TempoForm::Phases) {
        qint64 offset = 0;
        for (int i = 0; i < phases_.size(); ++i) {
            beatOffsets_.append(offset);
            beatPhases_.append(i + 1);
            offset += qint64(phases_.at(i)) * Limits::halfSecondMs;
        }
    } else {
        beatOffsets_.append(0);
        beatPhases_.append(0);
    }
}

qint64 Tempo::setDurationMs() const
{
    return repStartMs(reps_);
}

qint64 Tempo::repStartMs(int index) const
{
    if (!isValid())
        return 0;
    const qint64 i = qBound(0, index, reps_);
    switch (form_) {
    case TempoForm::RepsAndTotal:
        // round(i * total / reps), half up, in integers.
        return (2 * i * valueMs_ + reps_) / (2 * qint64(reps_));
    case TempoForm::TimePerRep:
        return i * valueMs_;
    case TempoForm::Phases:
        return i * phaseSumMs();
    }
    return 0;
}

qint64 Tempo::repDurationMs(int index) const
{
    if (!isValid() || index < 0 || index >= reps_)
        return 0;
    return repStartMs(index + 1) - repStartMs(index);
}

int Tempo::repIndexAt(qint64 setOffsetMs) const
{
    if (!isValid())
        return 0;
    if (setOffsetMs <= 0)
        return 0;
    if (setOffsetMs >= setDurationMs())
        return reps_ - 1;
    // Estimate from the mean length, then correct by the rounding of at most one repetition.
    int index = int(setOffsetMs * reps_ / setDurationMs());
    index = qBound(0, index, reps_ - 1);
    while (index > 0 && repStartMs(index) > setOffsetMs)
        --index;
    while (index < reps_ - 1 && repStartMs(index + 1) <= setOffsetMs)
        ++index;
    return index;
}

qint64 Tempo::beatOffsetMs(int beat) const
{
    return (beat >= 0 && beat < beatOffsets_.size()) ? beatOffsets_.at(beat) : 0;
}

BeatKind Tempo::beatKind(int beat) const
{
    if (beat <= 0)
        return BeatKind::First;
    return beat >= beatOffsets_.size() - 1 ? BeatKind::Last : BeatKind::Middle;
}

int Tempo::beatPhase(int beat) const
{
    return (beat >= 0 && beat < beatPhases_.size()) ? beatPhases_.at(beat) : 0;
}

int Tempo::beatIndexAt(qint64 repOffsetMs) const
{
    int found = 0;
    for (int i = 0; i < beatOffsets_.size(); ++i) {
        if (beatOffsets_.at(i) <= repOffsetMs)
            found = i;
    }
    return found;
}

int Tempo::phaseAt(qint64 repOffsetMs) const
{
    return beatPhase(beatIndexAt(repOffsetMs));
}

qint64 Tempo::beatSpanMs(int beat, int repIndex) const
{
    if (!isValid() || beat < 0 || beat >= beatOffsets_.size())
        return 0;
    const qint64 end = beat + 1 < beatOffsets_.size() ? beatOffsets_.at(beat + 1) : repDurationMs(repIndex);
    return end - beatOffsets_.at(beat);
}

QString Tempo::toText() const
{
    switch (form_) {
    case TempoForm::RepsAndTotal:
        return QStringLiteral("a;%1;%2").arg(reps_).arg(valueMs_);
    case TempoForm::TimePerRep:
        return QStringLiteral("b;%1;%2").arg(valueMs_).arg(reps_);
    case TempoForm::Phases:
    {
        bool named = false;
        for (const QString &name : names_)
            named = named || !name.isEmpty();
        QString text = named ? QStringLiteral("d") : QStringLiteral("c");
        for (int unit : phases_)
            text += QLatin1Char(';') + QString::number(unit);
        text += QLatin1Char(';') + QString::number(reps_);
        if (named) {
            for (const QString &name : names_)
                text += QLatin1Char(';') + QString::fromLatin1(name.toUtf8().toPercentEncoding());
        }
        return text;
    }
    }
    return QString();
}

bool Tempo::fromText(const QString &text, Tempo *tempo)
{
    const QStringList parts = text.split(QLatin1Char(';'));
    if (parts.isEmpty() || parts.first().size() != 1)
        return false;
    const bool isPhases = parts.first() == QLatin1String("c");
    const bool isNamed = parts.first() == QLatin1String("d");
    // "c" has the phases and then the repetitions: 1 to 8 phases make 3 to 10 fields. "d" has the
    // same and then one name field per phase: 4 to 18 fields, an even number.
    if (isPhases ? (parts.size() < 3 || parts.size() > Limits::maxPhases + 2)
        : isNamed ? (parts.size() < 4 || parts.size() > 2 * Limits::maxPhases + 2 || parts.size() % 2 != 0)
                  : parts.size() != 3)
        return false;
    const int numberFields = isNamed ? parts.size() / 2 + 1 : parts.size();
    QStringList names;
    if (isNamed) {
        bool anyName = false;
        for (int i = numberFields; i < parts.size(); ++i) {
            const QByteArray raw = parts.at(i).toLatin1();
            if (QString::fromLatin1(raw) != parts.at(i))
                return false;
            const QByteArray decoded = QByteArray::fromPercentEncoding(raw);
            const QString name = QString::fromUtf8(decoded);
            // Only the exact form that toText() writes: valid UTF-8 and canonical escapes.
            if (name.toUtf8() != decoded || name.toUtf8().toPercentEncoding() != raw)
                return false;
            anyName = anyName || !name.isEmpty();
            names.append(name);
        }
        if (!anyName)
            return false;
    }
    QList<qint64> n;
    for (int i = 1; i < numberFields; ++i) {
        bool ok = false;
        const qint64 value = parts.at(i).toLongLong(&ok);
        // Only the plain decimal form that toText() writes.
        if (!ok || QString::number(value) != parts.at(i))
            return false;
        n.append(value);
    }
    Tempo result;
    if (parts.first() == QLatin1String("a")) {
        if (n[0] < Limits::minReps || n[0] > Limits::maxReps)
            return false;
        result = repsAndTotal(int(n[0]), n[1]);
    } else if (parts.first() == QLatin1String("b")) {
        if (n[1] < Limits::minReps || n[1] > Limits::maxReps)
            return false;
        result = timePerRep(n[0], int(n[1]));
    } else if (isPhases || isNamed) {
        QList<int> units;
        for (int i = 0; i < n.size() - 1; ++i) {
            if (n[i] < Limits::minPhaseUnits || n[i] > Limits::maxPhaseUnits)
                return false;
            units.append(int(n[i]));
        }
        if (n.last() < Limits::minReps || n.last() > Limits::maxReps)
            return false;
        result = phases(units, int(n.last()), names);
    } else {
        return false;
    }
    if (!result.isValid())
        return false;
    *tempo = result;
    return true;
}

bool Tempo::operator==(const Tempo &other) const
{
    if (form_ != other.form_ || reps_ != other.reps_ || valueMs_ != other.valueMs_ || problem_ != other.problem_)
        return false;
    return phases_ == other.phases_ && names_ == other.names_;
}
