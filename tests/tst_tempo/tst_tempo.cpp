// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "tempo.h"
#include "workoutlimits.h"

namespace {

Tempo ph(std::initializer_list<int> units, int reps)
{
    return Tempo::phases(QList<int>(units), reps);
}

// Beat times from the start of the set.
QList<qint64> beatTimes(const Tempo &t)
{
    QList<qint64> times;
    for (int r = 0; r < t.reps(); ++r) {
        for (int k = 0; k < t.beatsPerRep(); ++k)
            times.append(t.repStartMs(r) + t.beatOffsetMs(k));
    }
    return times;
}

}

class TstTempo : public QObject
{
    Q_OBJECT

private slots:
    void forRepsKeepsThePaceOfOneRepetition();
    void emptyTempoIsRefused();
    void repetitionLimits();
    void repsAndTotalLimits();
    void timePerRepLimits();
    void phaseLimits();
    void repetitionLengthWithPhases();
    void refusalCodesDoNotDependOnOrderOfFields();
    void timePerRepBeats();
    void repsAndTotalDividingEvenly();
    void repsAndTotalWithRounding();
    void repsAndTotalHasNoDrift();
    void setEndsExactlyAtTheTotal();
    void repIndexAtMatchesRepStarts();
    void phasesBeatOffsets();
    void oneThreeAndEightPhases();
    void onlyTheFirstBeatCarriesTheAccent();
    void phaseAtAnOffset();
    void beatKindsInsideARepetition();
    void beatSpans();
    void invalidTempoHasNoDerivedValues();
    void atMostThreeBeatsPerSecond();
    void textRoundTripForEveryForm();
    void textIsStable();
    void nameLengthCountsCharactersAsSeen();
    void phaseNamesAreKeptAndLimited();
    void namedTextRoundTripWithAwkwardNames();
    void namedTextIsStableAndStrict();
    void namesDoNotTouchTheOtherForms();
    void malformedTextIsRefused();
    void equality();
};

void TstTempo::emptyTempoIsRefused()
{
    const Tempo t;
    QVERIFY(!t.isValid());
    QVERIFY(t.problem() == TempoProblem::RepsOutOfRange);
    QCOMPARE(t.setDurationMs(), qint64(0));
}

void TstTempo::repetitionLimits()
{
    const int bad[] = { 0, -1, 1000, 100000 };
    for (int reps : bad) {
        QVERIFY(Tempo::timePerRep(1000, reps).problem() == TempoProblem::RepsOutOfRange);
        QVERIFY(Tempo::repsAndTotal(reps, 600000).problem() == TempoProblem::RepsOutOfRange);
        QVERIFY(ph({ 2 }, reps).problem() == TempoProblem::RepsOutOfRange);
    }
    QVERIFY(Tempo::timePerRep(1000, 1).isValid());
    QVERIFY(Tempo::timePerRep(1000, 999).isValid());
    QVERIFY(Tempo::repsAndTotal(1, 500).isValid());
    QVERIFY(Tempo::repsAndTotal(999, 999 * 500).isValid());
    QVERIFY(ph({ 1 }, 1).isValid());
    QVERIFY(ph({ 1 }, 999).isValid());
}

void TstTempo::repsAndTotalLimits()
{
    // The mean repetition must be 0.5 s to 60 s.
    QVERIFY(Tempo::repsAndTotal(10, 4999).problem() == TempoProblem::RepTooShort);
    QVERIFY(Tempo::repsAndTotal(10, 5000).isValid());
    QVERIFY(Tempo::repsAndTotal(10, 600000).isValid());
    QVERIFY(Tempo::repsAndTotal(10, 600001).problem() == TempoProblem::RepTooLong);
    QVERIFY(Tempo::repsAndTotal(7, 3499).problem() == TempoProblem::RepTooShort);
    QVERIFY(Tempo::repsAndTotal(7, 3500).isValid());
    QVERIFY(Tempo::repsAndTotal(7, 420000).isValid());
    QVERIFY(Tempo::repsAndTotal(7, 420001).problem() == TempoProblem::RepTooLong);
    QVERIFY(Tempo::repsAndTotal(1, 0).problem() == TempoProblem::RepTooShort);
    QVERIFY(Tempo::repsAndTotal(1, -5).problem() == TempoProblem::RepTooShort);
    QVERIFY(Tempo::repsAndTotal(999, 999 * 60000).isValid());
    QVERIFY(Tempo::repsAndTotal(999, 999 * 60000 + 1).problem() == TempoProblem::RepTooLong);
}

void TstTempo::timePerRepLimits()
{
    QVERIFY(Tempo::timePerRep(499, 5).problem() == TempoProblem::RepTooShort);
    QVERIFY(Tempo::timePerRep(500, 5).isValid());
    QVERIFY(Tempo::timePerRep(60000, 5).isValid());
    QVERIFY(Tempo::timePerRep(60001, 5).problem() == TempoProblem::RepTooLong);
    QVERIFY(Tempo::timePerRep(0, 5).problem() == TempoProblem::RepTooShort);
    QVERIFY(Tempo::timePerRep(-1, 5).problem() == TempoProblem::RepTooShort);
}

void TstTempo::phaseLimits()
{
    // The number of phases is 1 to 8.
    QVERIFY(ph({}, 5).problem() == TempoProblem::PhaseCountOutOfRange);
    QVERIFY(ph({ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 5).problem() == TempoProblem::PhaseCountOutOfRange);
    QVERIFY(ph({ 2 }, 5).isValid());
    QVERIFY(ph({ 2, 1, 3 }, 5).isValid());
    QVERIFY(ph({ 1, 1, 1, 1, 1, 1, 1, 1 }, 5).isValid());
    QCOMPARE(ph({ 2 }, 5).phaseCount(), 1);
    QCOMPARE(ph({ 2, 1, 3 }, 5).phaseCount(), 3);
    QCOMPARE(ph({ 1, 1, 1, 1, 1, 1, 1, 1 }, 5).phaseCount(), 8);
    QCOMPARE(Tempo::timePerRep(1000, 5).phaseCount(), 0);
    // A phase is 1 to 20 half seconds: no phase is 0 any more.
    QVERIFY(ph({ 0 }, 5).problem() == TempoProblem::PhaseOutOfRange);
    QVERIFY(ph({ 2, 0, 3 }, 5).problem() == TempoProblem::PhaseOutOfRange);
    QVERIFY(ph({ 21 }, 5).problem() == TempoProblem::PhaseOutOfRange);
    QVERIFY(ph({ 2, -1, 1 }, 5).problem() == TempoProblem::PhaseOutOfRange);
    QVERIFY(ph({ 1, 1, 21 }, 5).problem() == TempoProblem::PhaseOutOfRange);
    QVERIFY(ph({ 20 }, 5).isValid());
    QVERIFY(ph({ 1 }, 5).isValid());
}

void TstTempo::repetitionLengthWithPhases()
{
    // A repetition lasts 0.5 s to 60 s.
    QCOMPARE(ph({ 1 }, 5).repDurationMs(0), qint64(500));
    QCOMPARE(ph({ 1, 1, 1, 1, 1, 1, 1, 1 }, 5).repDurationMs(0), qint64(4000));
    QCOMPARE(ph({ 20, 20, 20 }, 5).repDurationMs(0), qint64(30000));
    QVERIFY(ph({ 20, 20, 20, 20, 20, 20 }, 2).isValid());
    QCOMPARE(ph({ 20, 20, 20, 20, 20, 20 }, 2).repDurationMs(0), qint64(60000));
    QVERIFY(ph({ 20, 20, 20, 20, 20, 20, 1 }, 2).problem() == TempoProblem::RepTooLong);
    QVERIFY(ph({ 20, 20, 20, 20, 20, 20, 20, 20 }, 2).problem() == TempoProblem::RepTooLong);
    QVERIFY(ph({ 15, 15, 15, 15, 15, 15, 15, 15 }, 2).isValid());
    QVERIFY(ph({ 16, 15, 15, 15, 15, 15, 15, 15 }, 2).problem() == TempoProblem::RepTooLong);
    // The set is the repetitions end to end.
    QCOMPARE(ph({ 1, 2, 3 }, 10).setDurationMs(), qint64(30000));
}

void TstTempo::refusalCodesDoNotDependOnOrderOfFields()
{
    // Repetitions are checked first, then the number of phases, then the lengths.
    QVERIFY(Tempo::timePerRep(10, 0).problem() == TempoProblem::RepsOutOfRange);
    QVERIFY(ph({}, 0).problem() == TempoProblem::RepsOutOfRange);
    QVERIFY(ph({ 30 }, 0).problem() == TempoProblem::RepsOutOfRange);
    QVERIFY(ph({}, 5).problem() == TempoProblem::PhaseCountOutOfRange);
    QVERIFY(ph({ 30, 0 }, 5).problem() == TempoProblem::PhaseOutOfRange);
}

void TstTempo::timePerRepBeats()
{
    const Tempo t = Tempo::timePerRep(1500, 4);
    QVERIFY(t.isValid());
    QVERIFY(t.form() == TempoForm::TimePerRep);
    QCOMPARE(t.setDurationMs(), qint64(6000));
    QCOMPARE(t.beatsPerRep(), 1);
    QCOMPARE(t.beatOffsetMs(0), qint64(0));
    QCOMPARE(t.beatPhase(0), 0);
    QVERIFY(Tempo::isAccent(0));
    QCOMPARE(beatTimes(t), QList<qint64>() << 0 << 1500 << 3000 << 4500);
    QCOMPARE(t.repDurationMs(2), qint64(1500));
    QCOMPARE(t.phaseAt(700), 0);
}

void TstTempo::repsAndTotalDividingEvenly()
{
    const Tempo t = Tempo::repsAndTotal(4, 6000);
    QCOMPARE(beatTimes(t), QList<qint64>() << 0 << 1500 << 3000 << 4500);
    QCOMPARE(t.setDurationMs(), qint64(6000));
}

void TstTempo::repsAndTotalWithRounding()
{
    // 7 repetitions in 20 s: the beats are round(i * 20000 / 7), never a sum of rounded periods.
    const Tempo t = Tempo::repsAndTotal(7, 20000);
    QVERIFY(t.isValid());
    const qint64 expected[] = { 0, 2857, 5714, 8571, 11429, 14286, 17143 };
    const QList<qint64> times = beatTimes(t);
    QCOMPARE(times.size(), 7);
    for (int i = 0; i < 7; ++i)
        QCOMPARE(times.at(i), expected[i]);
    QCOMPARE(t.repStartMs(6), qint64(qRound(6 * 20000.0 / 7)));
    QCOMPARE(t.setDurationMs(), qint64(20000));
    // The repetitions differ by at most 1 ms.
    for (int i = 0; i < 7; ++i) {
        QVERIFY(t.repDurationMs(i) == 2857 || t.repDurationMs(i) == 2858);
    }
    // Halves round up: 3 repetitions in 2000.5 ms is not allowed, so use 2 in 1001 ms.
    QCOMPARE(Tempo::repsAndTotal(2, 1001).repStartMs(1), qint64(501));
}

void TstTempo::repsAndTotalHasNoDrift()
{
    // 60 minutes at the highest repetition count: every beat is the rounded exact value.
    const Tempo t = Tempo::repsAndTotal(999, 3600000);
    QVERIFY(t.isValid());
    for (int i = 0; i <= 999; ++i) {
        const double exact = double(i) * 3600000.0 / 999.0;
        QVERIFY2(qAbs(double(t.repStartMs(i)) - exact) <= 0.5 + 1e-6, qPrintable(QString::number(i)));
    }
    QCOMPARE(t.setDurationMs(), qint64(3600000));
}

void TstTempo::setEndsExactlyAtTheTotal()
{
    const int totals[] = { 3500, 20000, 59999, 123457 };
    for (int total : totals) {
        const Tempo t = Tempo::repsAndTotal(7, total);
        QVERIFY(t.isValid());
        QCOMPARE(t.setDurationMs(), qint64(total));
        qint64 sum = 0;
        for (int i = 0; i < 7; ++i)
            sum += t.repDurationMs(i);
        QCOMPARE(sum, qint64(total));
    }
}

void TstTempo::repIndexAtMatchesRepStarts()
{
    const Tempo tempos[] = { Tempo::repsAndTotal(7, 20000), Tempo::repsAndTotal(999, 3600000),
                             Tempo::timePerRep(1500, 4), ph({ 2, 3, 1 }, 5), ph({ 1, 1, 1, 1, 1, 1, 1, 1 }, 4),
                             Tempo::repsAndTotal(3, 1501) };
    for (const Tempo &t : tempos) {
        QVERIFY(t.isValid());
        for (int r = 0; r < t.reps(); ++r) {
            QCOMPARE(t.repIndexAt(t.repStartMs(r)), r);
            QCOMPARE(t.repIndexAt(t.repStartMs(r + 1) - 1), r);
        }
        QCOMPARE(t.repIndexAt(-5), 0);
        QCOMPARE(t.repIndexAt(t.setDurationMs()), t.reps() - 1);
        QCOMPARE(t.repIndexAt(t.setDurationMs() + 100000), t.reps() - 1);
    }
}

void TstTempo::phasesBeatOffsets()
{
    // 1 s, 0.5 s, 2 s, 1.5 s.
    const Tempo t = ph({ 2, 1, 4, 3 }, 3);
    QVERIFY(t.isValid());
    QVERIFY(t.form() == TempoForm::Phases);
    QCOMPARE(t.beatsPerRep(), 4);
    QCOMPARE(t.beatOffsetsMs(), QList<qint64>() << 0 << 1000 << 1500 << 3500);
    QCOMPARE(t.repDurationMs(0), qint64(5000));
    QCOMPARE(t.setDurationMs(), qint64(15000));
    for (int k = 0; k < 4; ++k)
        QCOMPARE(t.beatPhase(k), k + 1);
    QCOMPARE(t.phaseUnits(1), 2);
    QCOMPARE(t.phaseUnits(4), 3);
    QCOMPARE(t.phaseUnits(0), 0);
    QCOMPARE(t.phaseUnits(5), 0);
    QCOMPARE(t.phaseUnitList(), QList<int>() << 2 << 1 << 4 << 3);
    QCOMPARE(beatTimes(t).at(5), qint64(5000 + 1000));
}

void TstTempo::oneThreeAndEightPhases()
{
    // One phase: one beat per repetition, labelled phase 1.
    const Tempo one = ph({ 3 }, 4);
    QCOMPARE(one.beatsPerRep(), 1);
    QCOMPARE(one.beatPhase(0), 1);
    QCOMPARE(beatTimes(one), QList<qint64>() << 0 << 1500 << 3000 << 4500);
    QCOMPARE(one.phaseAt(1499), 1);

    // Three phases, the default count of the screens.
    QCOMPARE(Limits::defaultPhaseCount, 3);
    const Tempo three = ph({ 4, 2, 2 }, 2);
    QCOMPARE(three.beatOffsetsMs(), QList<qint64>() << 0 << 2000 << 3000);
    QCOMPARE(beatTimes(three), QList<qint64>() << 0 << 2000 << 3000 << 4000 << 6000 << 7000);
    QCOMPARE(three.beatPhase(2), 3);
    QCOMPARE(three.phaseAt(2999), 2);
    QCOMPARE(three.phaseAt(3000), 3);
    QCOMPARE(three.repIndexAt(4000), 1);

    // Eight phases.
    const Tempo eight = ph({ 1, 2, 3, 4, 5, 6, 7, 8 }, 2);
    QVERIFY(eight.isValid());
    QCOMPARE(eight.beatsPerRep(), 8);
    QCOMPARE(eight.beatOffsetsMs(), QList<qint64>() << 0 << 500 << 1500 << 3000 << 5000 << 7500 << 10500 << 14000);
    QCOMPARE(eight.repDurationMs(0), qint64(18000));
    for (int k = 0; k < 8; ++k) {
        QCOMPARE(eight.beatPhase(k), k + 1);
        QCOMPARE(eight.phaseAt(eight.beatOffsetMs(k)), k + 1);
        QCOMPARE(eight.beatSpanMs(k, 0), qint64((k + 1) * 500));
    }
    QCOMPARE(eight.phaseAt(17999), 8);
    QCOMPARE(eight.phaseAt(99999), 8);
    QCOMPARE(beatTimes(eight).at(8), qint64(18000));
}

void TstTempo::onlyTheFirstBeatCarriesTheAccent()
{
    const Tempo t = ph({ 3, 2, 1 }, 2);
    QCOMPARE(t.beatOffsetMs(0), qint64(0));
    QVERIFY(Tempo::isAccent(0));
    QVERIFY(!Tempo::isAccent(1));
    QVERIFY(!Tempo::isAccent(2));
}

void TstTempo::beatKindsInsideARepetition()
{
    // Forms (a) and (b): one beat, only the first.
    const Tempo a = Tempo::repsAndTotal(4, 8000);
    QVERIFY(a.beatKind(0) == BeatKind::First);
    const Tempo b = Tempo::timePerRep(2000, 4);
    QVERIFY(b.beatKind(0) == BeatKind::First);

    // One phase: only the first. Two phases: first and last.
    QVERIFY(ph({ 2 }, 2).beatKind(0) == BeatKind::First);
    const Tempo two = ph({ 2, 2 }, 2);
    QVERIFY(two.beatKind(0) == BeatKind::First);
    QVERIFY(two.beatKind(1) == BeatKind::Last);

    // Three phases: first, middle, last.
    const Tempo three = ph({ 2, 2, 2 }, 2);
    QVERIFY(three.beatKind(0) == BeatKind::First);
    QVERIFY(three.beatKind(1) == BeatKind::Middle);
    QVERIFY(three.beatKind(2) == BeatKind::Last);

    // Eight phases: first, six in the middle, last.
    const Tempo eight = ph({ 1, 1, 1, 1, 1, 1, 1, 1 }, 1);
    QVERIFY(eight.beatKind(0) == BeatKind::First);
    for (int i = 1; i < 7; ++i)
        QVERIFY(eight.beatKind(i) == BeatKind::Middle);
    QVERIFY(eight.beatKind(7) == BeatKind::Last);
}

void TstTempo::phaseAtAnOffset()
{
    const Tempo t = ph({ 2, 1, 4, 3 }, 3);   // beats at 0, 1000, 1500, 3500; rep 5000
    QCOMPARE(t.phaseAt(0), 1);
    QCOMPARE(t.phaseAt(999), 1);
    QCOMPARE(t.phaseAt(1000), 2);
    QCOMPARE(t.phaseAt(1499), 2);
    QCOMPARE(t.phaseAt(1500), 3);
    QCOMPARE(t.phaseAt(3499), 3);
    QCOMPARE(t.phaseAt(3500), 4);
    QCOMPARE(t.phaseAt(4999), 4);
    QCOMPARE(t.phaseAt(99999), 4);
    QCOMPARE(t.phaseAt(-4), 1);
    QCOMPARE(t.beatIndexAt(1000), 1);
}

void TstTempo::beatSpans()
{
    const Tempo t = ph({ 2, 1, 4, 3 }, 3);
    QCOMPARE(t.beatSpanMs(0, 0), qint64(1000));
    QCOMPARE(t.beatSpanMs(1, 0), qint64(500));
    QCOMPARE(t.beatSpanMs(2, 0), qint64(2000));
    QCOMPARE(t.beatSpanMs(3, 2), qint64(1500));
    const Tempo r = Tempo::repsAndTotal(7, 20000);
    QCOMPARE(r.beatSpanMs(0, 0), r.repDurationMs(0));
    QCOMPARE(r.beatSpanMs(0, 3), r.repDurationMs(3));
    QCOMPARE(r.beatSpanMs(1, 0), qint64(0));
}

void TstTempo::invalidTempoHasNoDerivedValues()
{
    const Tempo t = Tempo::timePerRep(100, 5);
    QVERIFY(!t.isValid());
    QCOMPARE(t.setDurationMs(), qint64(0));
    QCOMPARE(t.repStartMs(2), qint64(0));
    QCOMPARE(t.repDurationMs(0), qint64(0));
    QCOMPARE(t.beatsPerRep(), 0);
    QCOMPARE(t.repIndexAt(500), 0);
    QCOMPARE(t.beatSpanMs(0, 0), qint64(0));
}

void TstTempo::atMostThreeBeatsPerSecond()
{
    // Over a grid of valid inputs of every form, any four consecutive beats span at least 1 s.
    int checked = 0;
    auto check = [&](const Tempo &t) {
        QVERIFY2(t.isValid(), qPrintable(t.toText()));
        const QList<qint64> times = beatTimes(t);
        for (int i = 0; i + 3 < times.size(); ++i)
            QVERIFY2(times.at(i + 3) - times.at(i) >= 1000, qPrintable(t.toText()));
        for (int i = 0; i + 1 < times.size(); ++i)
            QVERIFY2(times.at(i + 1) - times.at(i) >= 500, qPrintable(t.toText()));
        ++checked;
    };
    const int repCounts[] = { 1, 2, 3, 5, 7, 12, 100, 999 };
    for (int reps : repCounts) {
        for (qint64 rep = 500; rep <= 2600; rep += 37)
            check(Tempo::timePerRep(rep, reps));
        check(Tempo::timePerRep(60000, reps));
        for (qint64 mean10 = 5000; mean10 <= 20000; mean10 += 613) {
            // Totals just above the lowest allowed, and at several rounding remainders.
            const qint64 total = qint64(reps) * mean10 / 10 + (mean10 % 7);
            check(Tempo::repsAndTotal(reps, total));
        }
        check(Tempo::repsAndTotal(reps, qint64(reps) * 500));
        check(Tempo::repsAndTotal(reps, qint64(reps) * 500 + 1));
        const int units[] = { 1, 2, 3, 10, 20 };
        for (int a : units) {
            check(ph({ a }, reps));
            for (int b : units) {
                check(ph({ a, b }, reps));
                for (int c : units)
                    check(ph({ a, b, c }, reps));
            }
        }
        check(ph({ 1, 1, 1, 1, 1, 1, 1, 1 }, reps));
        check(ph({ 1, 2, 1, 2, 1, 2, 1, 2 }, reps));
    }
    QVERIFY(checked > 1500);
}

void TstTempo::textRoundTripForEveryForm()
{
    const Tempo tempos[] = { Tempo::repsAndTotal(7, 20000), Tempo::repsAndTotal(1, 500),
                             Tempo::timePerRep(1500, 4), Tempo::timePerRep(60000, 999),
                             ph({ 2, 3, 1 }, 8), ph({ 1 }, 1), ph({ 20, 20, 20 }, 999),
                             ph({ 1, 2, 3, 4, 5, 6, 7, 8 }, 3) };
    for (const Tempo &t : tempos) {
        QVERIFY(t.isValid());
        Tempo back;
        QVERIFY2(Tempo::fromText(t.toText(), &back), qPrintable(t.toText()));
        QVERIFY(back == t);
        QCOMPARE(back.toText(), t.toText());
        QCOMPARE(back.setDurationMs(), t.setDurationMs());
        QCOMPARE(back.beatOffsetsMs(), t.beatOffsetsMs());
    }
}

void TstTempo::nameLengthCountsCharactersAsSeen()
{
    QCOMPARE(Tempo::nameLength(QString()), 0);
    QCOMPARE(Tempo::nameLength(QStringLiteral("abc")), 3);
    // A precomposed letter and a base letter with a combining mark are one character each.
    QCOMPARE(Tempo::nameLength(QString::fromUtf8("\xc3\xa9")), 1);
    QCOMPARE(Tempo::nameLength(QString::fromUtf8("e\xcc\x81")), 1);
    // A character outside the BMP is two UTF-16 units and one character.
    const QString emoji = QString::fromUtf8("\xf0\x9f\x98\x80");
    QCOMPARE(emoji.size(), 2);
    QCOMPARE(Tempo::nameLength(emoji), 1);
    QCOMPARE(Tempo::nameLength(emoji + emoji + QStringLiteral("a")), 3);
    // An emoji with a skin tone modifier is one character.
    QCOMPARE(Tempo::nameLength(QString::fromUtf8("\xf0\x9f\x91\x8d\xf0\x9f\x8f\xbd")), 1);
    // A zero width joiner sequence (a family) and a flag are one character each, two flags are two.
    const QString family = QString::fromUtf8("\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa7");
    QCOMPARE(Tempo::nameLength(family), 1);
    const QString flagIt = QString::fromUtf8("\xf0\x9f\x87\xae\xf0\x9f\x87\xb9");
    QCOMPARE(Tempo::nameLength(flagIt), 1);
    QCOMPARE(Tempo::nameLength(flagIt + flagIt), 2);
    // Spaces and separators count like any other character.
    QCOMPARE(Tempo::nameLength(QStringLiteral(" ; % ")), 5);
}

void TstTempo::phaseNamesAreKeptAndLimited()
{
    const Tempo plain = ph({ 2, 3, 1 }, 4);
    QCOMPARE(plain.phaseNameList(), QStringList() << "" << "" << "");
    QCOMPARE(plain.phaseName(2), QString());

    const Tempo named = Tempo::phases({ 2, 3, 1 }, 4, QStringList() << "Down" << "" << "Up");
    QVERIFY(named.isValid());
    QCOMPARE(named.phaseName(1), QStringLiteral("Down"));
    QCOMPARE(named.phaseName(2), QString());
    QCOMPARE(named.phaseName(3), QStringLiteral("Up"));
    QCOMPARE(named.phaseName(0), QString());
    QCOMPARE(named.phaseName(4), QString());
    QVERIFY(named != plain);
    QVERIFY(named == Tempo::phases({ 2, 3, 1 }, 4, QStringList() << "Down" << "" << "Up"));

    // Missing names are empty, surplus names are dropped.
    const Tempo few = Tempo::phases({ 2, 3, 1 }, 4, QStringList() << "A");
    QCOMPARE(few.phaseNameList(), QStringList() << "A" << "" << "");
    const Tempo many = Tempo::phases({ 2 }, 4, QStringList() << "A" << "B");
    QCOMPARE(many.phaseNameList(), QStringList() << "A");

    // Names may repeat; there are no case or trim rules.
    QVERIFY(Tempo::phases({ 2, 2 }, 1, QStringList() << "Up" << "Up").isValid());
    QVERIFY(Tempo::phases({ 2, 2 }, 1, QStringList() << " up " << "UP").isValid());

    // Twelve characters fit, thirteen do not, whatever the bytes.
    const QString twelve(12, QLatin1Char('x'));
    const QString thirteen(13, QLatin1Char('x'));
    QVERIFY(Tempo::phases({ 2 }, 1, QStringList() << twelve).isValid());
    const Tempo tooLong = Tempo::phases({ 2 }, 1, QStringList() << thirteen);
    QVERIFY(tooLong.problem() == TempoProblem::PhaseNameTooLong);
    QCOMPARE(tooLong.setDurationMs(), qint64(0));
    const QString emoji = QString::fromUtf8("\xf0\x9f\x98\x80");
    QString twelveEmoji;
    for (int i = 0; i < 12; ++i)
        twelveEmoji += emoji;
    QVERIFY(Tempo::phases({ 2 }, 1, QStringList() << twelveEmoji).isValid());
    QVERIFY(Tempo::phases({ 2 }, 1, QStringList() << twelveEmoji + emoji).problem() == TempoProblem::PhaseNameTooLong);
    QString accents;
    for (int i = 0; i < 12; ++i)
        accents += QString::fromUtf8("e\xcc\x81");
    QVERIFY(Tempo::phases({ 2 }, 1, QStringList() << accents).isValid());
    // The other problems keep their place: a bad length is reported before a bad name.
    QVERIFY(Tempo::phases({ 21 }, 1, QStringList() << thirteen).problem() == TempoProblem::PhaseOutOfRange);
}

void TstTempo::namedTextRoundTripWithAwkwardNames()
{
    const QString emoji = QString::fromUtf8("\xf0\x9f\x98\x80");
    const QStringList awkward = QStringList()
        << QStringLiteral("a;b") << QStringLiteral("100%") << QStringLiteral("\"quoted\"") << emoji
        << QString::fromUtf8("\xc3\xa9t\xc3\xa9") << QString::fromUtf8("e\xcc\x81") << QStringLiteral(";;;")
        << QStringLiteral("  edge  ");
    for (int count = 1; count <= 8; ++count) {
        QList<int> units;
        QStringList names;
        for (int i = 0; i < count; ++i) {
            units.append(1 + i);
            names.append(awkward.at(i));
        }
        const Tempo t = Tempo::phases(units, 5, names);
        QVERIFY(t.isValid());
        Tempo back;
        QVERIFY2(Tempo::fromText(t.toText(), &back), qPrintable(t.toText()));
        QVERIFY(back == t);
        QCOMPARE(back.phaseNameList(), names);
        QCOMPARE(back.toText(), t.toText());
        QVERIFY(t.toText().startsWith(QLatin1String("d;")));
        // The text has the separators of the fields only: 2n + 2 fields.
        QCOMPARE(t.toText().count(QLatin1Char(';')), 2 * count + 1);
    }
    // Twelve characters, empty names between named ones, and a newline inside a name.
    const Tempo mixed = Tempo::phases({ 2, 2, 2, 2 }, 9, QStringList() << QString(12, QLatin1Char('m')) << "" << "a\nb" << "");
    Tempo back;
    QVERIFY(Tempo::fromText(mixed.toText(), &back));
    QVERIFY(back == mixed);
    QCOMPARE(back.phaseName(3), QStringLiteral("a\nb"));
}

void TstTempo::namedTextIsStableAndStrict()
{
    // Without any name the text is the old one.
    QCOMPARE(Tempo::phases({ 2, 3, 1 }, 8, QStringList() << "" << "" << "").toText(), QStringLiteral("c;2;3;1;8"));
    QCOMPARE(Tempo::phases({ 2, 3, 1 }, 8, QStringList() << "Up" << "" << "").toText(), QStringLiteral("d;2;3;1;8;Up;;"));
    QCOMPARE(Tempo::phases({ 5 }, 2, QStringList() << "a;b%").toText(), QStringLiteral("d;5;2;a%3Bb%25"));

    Tempo t;
    QVERIFY(Tempo::fromText(QStringLiteral("d;2;3;1;8;Up;;"), &t));
    QCOMPARE(t.phaseName(1), QStringLiteral("Up"));
    const char *bad[] = {
        "d;5;2",                         // no names
        "d;5;2;",                        // all names empty: the unnamed form is "c"
        "d;2;3;1;8;;;",                  // all names empty
        "d;2;3;1;8;Up;",                 // one name too few
        "d;2;3;1;8;Up;;;",               // one name too many
        "d;5;2;a;b",                     // too many names for one phase
        "d;5;2;%c3%a9",                  // lower-case escape
        "d;5;2;%C3",                     // broken UTF-8
        "d;5;2;a b",                     // a character that must be escaped
        "d;5;2;%zz",                     // not an escape
        "d;5;2;xxxxxxxxxxxxx",           // 13 characters
        "d;5;0;x",                       // repetitions out of range
        "d;21;2;x",                      // phase out of range
        "d;1;1;1;1;1;1;1;1;1;1;x;x;x;x;x;x;x;x;x;x", // ten phases
        "e;5;2;x"
    };
    for (const char *text : bad)
        QVERIFY2(!Tempo::fromText(QString::fromLatin1(text), &t), text);
    // Twelve characters are accepted.
    QVERIFY(Tempo::fromText(QStringLiteral("d;5;2;xxxxxxxxxxxx"), &t));
    // A text with a non-Latin-1 character is refused, not misread.
    QVERIFY(!Tempo::fromText(QString::fromUtf8("d;5;2;\xc3\xa9"), &t));
}

void TstTempo::namesDoNotTouchTheOtherForms()
{
    QVERIFY(Tempo::repsAndTotal(7, 20000).phaseNameList().isEmpty());
    QVERIFY(Tempo::timePerRep(1500, 4).phaseNameList().isEmpty());
    QCOMPARE(Tempo::repsAndTotal(7, 20000).toText(), QStringLiteral("a;7;20000"));
    QCOMPARE(Tempo::timePerRep(1500, 4).toText(), QStringLiteral("b;1500;4"));
    QVERIFY(Tempo::repsAndTotal(7, 20000) == Tempo::repsAndTotal(7, 20000));
}

void TstTempo::textIsStable()
{
    // The stored text must never change: it is written to the database and to files.
    QCOMPARE(Tempo::repsAndTotal(7, 20000).toText(), QStringLiteral("a;7;20000"));
    QCOMPARE(Tempo::timePerRep(1500, 4).toText(), QStringLiteral("b;1500;4"));
    QCOMPARE(ph({ 2, 3, 1 }, 8).toText(), QStringLiteral("c;2;3;1;8"));
    QCOMPARE(ph({ 5 }, 2).toText(), QStringLiteral("c;5;2"));
    QCOMPARE(ph({ 1, 2, 3, 4, 5, 6, 7, 8 }, 3).toText(), QStringLiteral("c;1;2;3;4;5;6;7;8;3"));
    // The text of a tempo with four phases is the text that the older form wrote.
    QCOMPARE(ph({ 2, 1, 4, 3 }, 3).toText(), QStringLiteral("c;2;1;4;3;3"));
}

void TstTempo::malformedTextIsRefused()
{
    const char *bad[] = {
        "", "a", "a;7", "a;7;20000;1", "x;7;20000", "A;7;20000", "ab;7;20000", ";7;20000",
        "a;0;20000", "a;1000;20000", "a;7;3499", "a;7;420001", "a;7;-1", "a;07;20000", "a;7;020000",
        "a;+7;20000", "a; 7;20000", "a;7;20000 ", "a;7.0;20000", "a;seven;20000",
        "b;499;4", "b;60001;4", "b;1500;0", "b;1500;1000", "b;1500;",
        "c", "c;5", "c;2;0;3;1;8", "c;0;0;0;0;8", "c;21;8", "c;-1;1;8", "c;2;3;1;0",
        "c;2;3;1;1000", "c;2;3;1;x", "c;1;1;1;1;1;1;1;1;1;3", "c;20;20;20;20;20;20;1;2",
        "99999999999999999999;1;1"
    };
    for (const char *text : bad) {
        Tempo t = Tempo::timePerRep(1000, 3);
        QVERIFY2(!Tempo::fromText(QString::fromLatin1(text), &t), text);
        // The target stays as it was.
        QVERIFY(t == Tempo::timePerRep(1000, 3));
    }
}

void TstTempo::equality()
{
    QVERIFY(Tempo::timePerRep(1000, 3) == Tempo::timePerRep(1000, 3));
    QVERIFY(Tempo::timePerRep(1000, 3) != Tempo::timePerRep(1000, 4));
    QVERIFY(Tempo::timePerRep(1000, 3) != Tempo::repsAndTotal(3, 3000));
    QVERIFY(ph({ 1, 2, 3, 4 }, 3) != ph({ 1, 2, 4, 3 }, 3));
    QVERIFY(ph({ 1, 2, 3 }, 3) != ph({ 1, 2, 3, 1 }, 3));
    QVERIFY(Tempo() == Tempo());
}

void TstTempo::forRepsKeepsThePaceOfOneRepetition()
{
    const Tempo total = Tempo::repsAndTotal(7, 20000).forReps(7);
    QVERIFY(total == Tempo::repsAndTotal(7, 20000));
    const Tempo other = Tempo::repsAndTotal(7, 20000).forReps(3);
    QVERIFY(other.form() == TempoForm::RepsAndTotal);
    QCOMPARE(other.reps(), 3);
    QCOMPARE(other.valueMs(), qint64(8571));
    QVERIFY(Tempo::timePerRep(1500, 4).forReps(9) == Tempo::timePerRep(1500, 9));
    const Tempo named = Tempo::phases(QList<int>() << 2 << 3, 4, QStringList() << "up" << "down").forReps(11);
    QVERIFY(named == Tempo::phases(QList<int>() << 2 << 3, 11, QStringList() << "up" << "down"));
    QVERIFY(!Tempo().forReps(3).isValid());
    QVERIFY(!Tempo::timePerRep(1500, 4).forReps(0).isValid());
    QVERIFY(!Tempo::timePerRep(1500, 4).forReps(1000).isValid());
}

QTEST_APPLESS_MAIN(TstTempo)
#include "tst_tempo.moc"
