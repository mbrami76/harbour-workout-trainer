// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "exerciseexchange.h"
#include "icons.h"
#include "musclegroups.h"

namespace {

QByteArray file(const QJsonObject &exercise, int version = 1, const QString &format = ExerciseExchange::format)
{
    QJsonObject root;
    root.insert("format", format);
    root.insert("version", version);
    root.insert("exercise", exercise);
    return QJsonDocument(root).toJson();
}

QJsonObject good()
{
    QJsonObject o;
    o.insert("name", "Wall sit");
    o.insert("muscles", QJsonArray() << "quadriceps" << "glutes");
    o.insert("instructions", "Back on the wall.\nHold.");
    o.insert("tips", "Knees at 90 degrees.");
    o.insert("icon", "wall-sit");
    o.insert("colour", "teal");
    o.insert("language", "en");
    return o;
}

bool hasCode(const ExerciseExchange::Parsed &p, ImportIssue::Code code)
{
    for (const ImportIssue &i : p.problems)
        if (i.code == code)
            return true;
    return false;
}

}

class TstExerciseExchange : public QObject
{
    Q_OBJECT

private slots:
    void exportAndImportRoundTrip();
    void exportCarriesNoKeyNoMetronomeNoStarter();
    void importedExerciseHasNoKeyNoTempo();
    void aFileOfAnotherFormatIsRefused();
    void aWorkoutFileIsRefused();
    void aNewerVersionIsRefused();
    void aLargeFileIsRefusedUnread();
    void notJsonAndNotUtf8AreRefused();
    void wrongShapesAreRefused();
    void anExtraMetronomeOrKeyIsIgnored();
    void textIsKeptAsTextNotInterpreted();
    void controlAndDirectionCharactersAreRemoved();
    void aNameThatIsEmptyAfterCleaningIsRefused();
    void limitsOfTheCoreApply();
    void unknownGroupsAreDroppedButNotAllOfThem();
    void unknownColourGivesANoticeAndTheDefault();
    void iconKeyIsCheckedAsAKey();
    void fileNamesAreSafe();
    void exerciseIconsStartWithTheDefaultAndHaveNoRepeats();
};

void TstExerciseExchange::exportAndImportRoundTrip()
{
    Exercise e;
    e.key = "abc";
    e.name = "Plank";
    e.muscles = muscleBit(MuscleGroup::Core) | muscleBit(MuscleGroup::Obliques);
    e.instructions = "Line up.\nBreathe.";
    e.tips = "Do not sag.";
    e.icon = "plank";
    e.colour = "green";
    e.language = "de";
    const ExerciseExchange::Parsed p = ExerciseExchange::parse(ExerciseExchange::toJson(e));
    QVERIFY(p.valid);
    QCOMPARE(p.exercise.name, e.name);
    QCOMPARE(p.exercise.muscles, e.muscles);
    QCOMPARE(p.exercise.instructions, e.instructions);
    QCOMPARE(p.exercise.tips, e.tips);
    QCOMPARE(p.exercise.icon, e.icon);
    QCOMPARE(p.exercise.colour, e.colour);
    QCOMPARE(p.exercise.language, e.language);
}

void TstExerciseExchange::exportCarriesNoKeyNoMetronomeNoStarter()
{
    Exercise e;
    e.key = "secret-key";
    e.name = "Plank";
    e.muscles = muscleBit(MuscleGroup::Core);
    e.metronomeId = 5;
    e.metronomeKey = "m-key";
    e.starter = true;
    const QByteArray bytes = ExerciseExchange::toJson(e);
    QVERIFY(!bytes.contains("secret-key"));
    QVERIFY(!bytes.contains("m-key"));
    QVERIFY(!bytes.contains("metronome"));
    QVERIFY(!bytes.contains("starter"));
}

void TstExerciseExchange::importedExerciseHasNoKeyNoTempo()
{
    QJsonObject o = good();
    o.insert("key", "x");
    o.insert("metronome", "m1");
    o.insert("starter", true);
    const ExerciseExchange::Parsed p = ExerciseExchange::parse(file(o));
    QVERIFY(p.valid);
    QVERIFY(p.exercise.key.isEmpty());
    QVERIFY(p.exercise.metronomeKey.isEmpty());
    QCOMPARE(p.exercise.metronomeId, 0);
    QVERIFY(!p.exercise.starter);
}

void TstExerciseExchange::aFileOfAnotherFormatIsRefused()
{
    QVERIFY(hasCode(ExerciseExchange::parse(file(good(), 1, "something-else")), ImportIssue::WrongFormat));
}

void TstExerciseExchange::aWorkoutFileIsRefused()
{
    QVERIFY(hasCode(ExerciseExchange::parse(file(good(), 1, Exchange::exportFormat)), ImportIssue::WrongFormat));
}

void TstExerciseExchange::aNewerVersionIsRefused()
{
    QVERIFY(hasCode(ExerciseExchange::parse(file(good(), 2)), ImportIssue::NewerVersion));
    QVERIFY(!ExerciseExchange::parse(file(good(), 0)).valid);
}

void TstExerciseExchange::aLargeFileIsRefusedUnread()
{
    QByteArray big(ExerciseExchange::maxFileBytes + 1, ' ');
    QVERIFY(hasCode(ExerciseExchange::parse(big), ImportIssue::TooLarge));
}

void TstExerciseExchange::notJsonAndNotUtf8AreRefused()
{
    QVERIFY(hasCode(ExerciseExchange::parse("not json"), ImportIssue::NotJson));
    QVERIFY(hasCode(ExerciseExchange::parse("{\"format\": \"\xff\xfe\"}"), ImportIssue::NotUtf8));
    QVERIFY(!ExerciseExchange::parse("[1, 2]").valid);
    QVERIFY(!ExerciseExchange::parse(QByteArray()).valid);
}

void TstExerciseExchange::wrongShapesAreRefused()
{
    {
        QJsonObject root;
        root.insert("format", ExerciseExchange::format);
        root.insert("version", 1);
        root.insert("exercise", QJsonArray() << good());
        QVERIFY(!ExerciseExchange::parse(QJsonDocument(root).toJson()).valid);
    }
    QJsonObject o = good();
    o.insert("name", 5);
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o = good();
    o.insert("muscles", "quadriceps");
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o = good();
    o.insert("muscles", QJsonArray() << 1);
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o = good();
    o.insert("instructions", QJsonArray() << "a");
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o = good();
    o.remove("name");
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o = good();
    o.remove("muscles");
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o = good();
    o.insert("muscles", QJsonArray());
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
}

void TstExerciseExchange::anExtraMetronomeOrKeyIsIgnored()
{
    QJsonObject o = good();
    o.insert("metronome", "../../etc/passwd");
    o.insert("key", "../x");
    o.insert("script", "run()");
    const ExerciseExchange::Parsed p = ExerciseExchange::parse(file(o));
    QVERIFY(p.valid);
}

void TstExerciseExchange::textIsKeptAsTextNotInterpreted()
{
    QJsonObject o = good();
    o.insert("name", "<b>Bold</b> ${x} `cmd` $(cmd) ' OR 1=1; --");
    o.insert("instructions", "<script>alert(1)</script> \\u0000 %s %n {{x}}");
    const ExerciseExchange::Parsed p = ExerciseExchange::parse(file(o));
    QVERIFY(p.valid);
    QCOMPARE(p.exercise.name, QStringLiteral("<b>Bold</b> ${x} `cmd` $(cmd) ' OR 1=1; --"));
    QVERIFY(p.exercise.instructions.contains("<script>"));
}

void TstExerciseExchange::controlAndDirectionCharactersAreRemoved()
{
    QCOMPARE(ExerciseExchange::clean(QStringLiteral("a\x01" "b\x1b" "c\x7f" "d"), true), QStringLiteral("abcd"));
    QCOMPARE(ExerciseExchange::clean(QString("a") + QChar(0x202e) + "b" + QChar(0x2066) + "c", true), QStringLiteral("abc"));
    QCOMPARE(ExerciseExchange::clean(QStringLiteral("a\nb\tc"), true), QStringLiteral("a\nb\tc"));
    QCOMPARE(ExerciseExchange::clean(QStringLiteral("a\nb\tc"), false), QStringLiteral("a b c"));
    QCOMPARE(ExerciseExchange::clean(QStringLiteral("a\r\nb"), true), QStringLiteral("a\nb"));
    QJsonObject o = good();
    o.insert("name", QString("Plank") + QChar(0x202e) + "\n" + QChar(0x01));
    const ExerciseExchange::Parsed p = ExerciseExchange::parse(file(o));
    QVERIFY(p.valid);
    QCOMPARE(p.exercise.name, QStringLiteral("Plank"));
}

void TstExerciseExchange::aNameThatIsEmptyAfterCleaningIsRefused()
{
    QJsonObject o = good();
    o.insert("name", QString(QChar(0x202e)) + QChar(0x01) + " ");
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
}

void TstExerciseExchange::limitsOfTheCoreApply()
{
    QJsonObject o = good();
    o.insert("name", QString(81, 'a'));
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o = good();
    o.insert("instructions", QString(4001, 'a'));
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o = good();
    o.insert("tips", QString(2001, 'a'));
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o = good();
    o.insert("language", "not a tag!");
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
}

void TstExerciseExchange::unknownGroupsAreDroppedButNotAllOfThem()
{
    QJsonObject o = good();
    o.insert("muscles", QJsonArray() << "quadriceps" << "wings");
    ExerciseExchange::Parsed p = ExerciseExchange::parse(file(o));
    QVERIFY(p.valid);
    QCOMPARE(p.exercise.muscles, muscleBit(MuscleGroup::Quadriceps));
    QVERIFY(!p.notices.isEmpty());
    o.insert("muscles", QJsonArray() << "wings");
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
}

void TstExerciseExchange::unknownColourGivesANoticeAndTheDefault()
{
    QJsonObject o = good();
    o.insert("colour", "ultraviolet");
    const ExerciseExchange::Parsed p = ExerciseExchange::parse(file(o));
    QVERIFY(p.valid);
    QVERIFY(p.exercise.colour.isEmpty());
    QVERIFY(!p.notices.isEmpty());
}

void TstExerciseExchange::iconKeyIsCheckedAsAKey()
{
    QJsonObject o = good();
    o.insert("icon", "../../x");
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o.insert("icon", "Squat");
    QVERIFY(!ExerciseExchange::parse(file(o)).valid);
    o.insert("icon", "future-icon");
    QVERIFY(ExerciseExchange::parse(file(o)).valid);
}

void TstExerciseExchange::fileNamesAreSafe()
{
    QCOMPARE(ExerciseExchange::fileBaseName("Wall sit"), QStringLiteral("Wall-sit"));
    QCOMPARE(ExerciseExchange::fileBaseName("../../etc/passwd"), QStringLiteral("etc-passwd"));
    QCOMPARE(ExerciseExchange::fileBaseName(".."), QStringLiteral("exercise"));
    QCOMPARE(ExerciseExchange::fileBaseName(QString()), QStringLiteral("exercise"));
    QCOMPARE(ExerciseExchange::fileBaseName(QString(QChar(0x00dc)) + "ab"), QStringLiteral("ab"));
    QVERIFY(ExerciseExchange::fileBaseName(QString(200, 'a')).length() <= 40);
    const QString odd = ExerciseExchange::fileBaseName("a/b\\c:d*e?f\"g<h>i|j\n");
    QVERIFY(!odd.contains('/') && !odd.contains('\\') && !odd.contains('.'));
}

void TstExerciseExchange::exerciseIconsStartWithTheDefaultAndHaveNoRepeats()
{
    const QStringList keys = Icons::exerciseKeys();
    QVERIFY(keys.size() > 20);
    QVERIFY(keys.first().isEmpty());
    QCOMPARE(QSet<QString>::fromList(keys).size(), keys.size());
    for (int i = 1; i < keys.size(); ++i) {
        QVERIFY(!keys.at(i).isEmpty());
        QVERIFY(keys.at(i) == keys.at(i).toLower());
    }
}

QTEST_APPLESS_MAIN(TstExerciseExchange)
#include "tst_exerciseexchange.moc"
