// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

#include "colours.h"
#include "exercisestore.h"
#include "icons.h"
#include "metronomestore.h"
#include "musclegroups.h"
#include "starterloader.h"
#include "testdb.h"
#include "workoutstore.h"

// The starter library (R-EXR-5, R-EXR-6, R-EXR-15, R-TPL-2, R-I18N-4): one structure file and one
// texts file per language, entries offered once per identifier, texts that follow the language.
namespace {

QByteArray bytes(const QJsonObject &o)
{
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

// A small library in memory: the structure, the texts of two languages and some workouts.
struct Library {
    struct Exercise {
        QString id;
        QString rhythm;
        QString en;
        QString other;
    };
    struct Rhythm {
        QString id;
        QString en;
        QString other;
    };
    struct Workout {
        QString id;
        QString exercise;
        QString en;
        QString other;
    };
    QList<Exercise> exercises;
    QList<Rhythm> rhythms;
    QList<Workout> workouts;

    static Library basic()
    {
        Library l;
        l.rhythms << Rhythm{"00000001", "2P-U-4 Down-up", "2P-U-4 Giu-su"};
        l.exercises << Exercise{"00000001", "00000001", "Plank", "Plancia"}
                    << Exercise{"00000002", QString(), "Squat", "Accosciata"}
                    << Exercise{"00000003", QString(), "Lunge", "Affondo"};
        return l;
    }

    QJsonObject structure() const
    {
        QJsonArray ex;
        for (const Exercise &e : exercises) {
            QJsonObject o{{"id", e.id}, {"colour", "green"}, {"icon", "squat"}, {"muscles", QJsonArray{"core"}}};
            if (!e.rhythm.isEmpty())
                o.insert("metronome", e.rhythm);
            ex.append(o);
        }
        QJsonArray rh;
        for (const Rhythm &r : rhythms)
            rh.append(QJsonObject{{"id", r.id}, {"phases", QJsonArray{4, 4}}});
        return QJsonObject{{"format", "workout-trainer-starter"}, {"version", 1}, {"exercises", ex}, {"metronomes", rh}};
    }

    QJsonObject workoutFile() const
    {
        QJsonArray list;
        for (const Workout &w : workouts) {
            list.append(QJsonObject{{"id", w.id},
                                    {"blocks", QJsonArray{QJsonObject{
                                        {"rounds", 2},
                                        {"steps", QJsonArray{QJsonObject{{"type", "work"}, {"exercise", w.exercise}, {"durationSec", 20}},
                                                             QJsonObject{{"type", "rest"}, {"durationSec", 10}}}}}}}});
        }
        return QJsonObject{{"format", "workout-trainer-starter-workouts"}, {"version", 1}, {"workouts", list}};
    }

    QJsonObject texts(const QString &language, bool english) const
    {
        QJsonObject ex;
        for (const Exercise &e : exercises) {
            const QString name = english ? e.en : e.other;
            if (!name.isEmpty())
                ex.insert(e.id, QJsonObject{{"name", name}, {"instructions", name + " instructions"}, {"tips", name + " tips"}});
        }
        QJsonObject rh;
        for (const Rhythm &r : rhythms) {
            const QString name = english ? r.en : r.other;
            if (!name.isEmpty())
                rh.insert(r.id, QJsonObject{{"name", name}, {"phaseNames", QJsonArray{english ? "Down" : "Giu", english ? "Up" : "Su"}}});
        }
        QJsonObject wo;
        for (const Workout &w : workouts) {
            const QString name = english ? w.en : w.other;
            if (!name.isEmpty())
                wo.insert(w.id, QJsonObject{{"name", name}, {"blocks", QJsonArray{name + " block"}}});
        }
        return QJsonObject{{"format", "workout-trainer-starter-texts"}, {"version", 1}, {"language", language},
                           {"exercises", ex}, {"metronomes", rh}, {"workouts", wo}};
    }

    StarterLoader::Files files(const QString &language) const
    {
        StarterLoader::Files f;
        f.structure = bytes(structure());
        f.workouts = bytes(workoutFile());
        f.english = bytes(texts("en", true));
        f.texts = language == QLatin1String("en") ? f.english : bytes(texts(language, false));
        return f;
    }
};

StarterLoader::Result load(const Library &l, const QString &language, Database *db)
{
    return StarterLoader::applyBytes(l.files(language), language, db);
}

Exercise starterExercise(Database &db, const QString &id)
{
    Exercise e;
    ExerciseStore(&db).get(StarterLoader::exerciseKey(id), &e);
    return e;
}

QString rhythmName(Database &db, const QString &id)
{
    SavedMetronome m;
    MetronomeStore(&db).getByKey(StarterLoader::rhythmKey(id), &m);
    return m.name;
}

QString codeOf(const QJsonArray &phases)
{
    QList<double> seconds;
    for (const QJsonValue &v : phases)
        seconds << v.toDouble() / 2.0;
    bool uniform = true;
    for (double d : seconds)
        uniform = uniform && d == seconds.first();
    QString code = QStringLiteral("%1P-").arg(seconds.size());
    if (uniform) {
        code += QStringLiteral("U-%1").arg(seconds.first() * seconds.size());
    } else {
        QStringList parts;
        for (double d : seconds)
            parts << QString::number(d);
        code += parts.join(QLatin1Char('-'));
    }
    return code;
}

QJsonObject shipped(const QString &file)
{
    QFile f(QDir(QStringLiteral(STARTER_SHIPPED_DIR)).filePath(file));
    return f.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(f.readAll()).object() : QJsonObject();
}

}

class TstStarterLoader : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void shippedStructureAndTexts();
    void shippedIdsIconsColoursAndGroups();
    void shippedRhythmNamesCarryTheirCode();
    void shippedLibraryLoadsInBothLanguages();
    void shippedWorkoutsAreValidAndInTheirTimeRange();
    void shippedWorkoutTextsFollowTheLanguage();

    void addsEntriesWithTheirIdentifierAndFollowFlag();
    void addsOncePerIdentifier();
    void deletedEntryIsNotAddedAgain();
    void newEntriesOfALaterFileAreAdded();
    void rhythmsComeFirstAndAreLinked();
    void missingRhythmAddsTheExerciseWithoutItAndReports();
    void deletedRhythmLeavesTheExerciseWithoutTempo();

    void languageChangeReplacesTextsAndTag();
    void languageChangeLeavesUserEntriesAlone();
    void languageChangeRetextsRhythmsAndWorkouts();
    void twoChangesInARowEndWhereTheyStarted();
    void textMissingInTheLanguageFallsBackToEnglish();
    void nameClashGetsTheSuffix();
    void changeOfALaterFileIsOneTransaction();

    void editingATextDetaches();
    void editingAnotherFieldKeepsFollowing();
    void detachedEntryKeepsItsTextsAfterAChange();
    void editingARhythmNameOrPhaseNameDetaches();
    void editingAWorkoutNameDetaches();

    void invalidFilesAddNothing();
    void entryWithoutEnglishTextIsAnError();
    void unknownTextIdIsIgnoredAndReported();
    void missingDirectoryOrLanguageAddsNothing();

    void workoutsRefuseUnknownExercisesAndComeBack();
    void workoutStepsResolveStarterIdentifiers();
    void firstStepOfAStarterWorkoutStartsAutomatically();

    void userAndImportedEntriesHaveNoStarterData();

private:
    TestDb t_;
};

void TstStarterLoader::init()
{
    QVERIFY(t_.dir.isValid());
    t_.db.close();
    QDir(t_.dir.path()).removeRecursively();
    QVERIFY(t_.open());
}

void TstStarterLoader::shippedStructureAndTexts()
{
    const QJsonObject structure = shipped("exercises.json");
    const QJsonArray exercises = structure.value("exercises").toArray();
    const QJsonArray rhythms = structure.value("metronomes").toArray();
    QCOMPARE(exercises.size(), 88);
    QCOMPARE(rhythms.size(), 16);
    const QRegularExpression pattern("^[0-9a-f]{8}$");
    QSet<QString> exerciseIds;
    QSet<QString> rhythmIds;
    for (const QJsonValue &v : exercises) {
        const QString id = v.toObject().value("id").toString();
        QVERIFY2(pattern.match(id).hasMatch(), qPrintable(id));
        QVERIFY2(!exerciseIds.contains(id), qPrintable(id));
        exerciseIds.insert(id);
    }
    for (const QJsonValue &v : rhythms) {
        const QString id = v.toObject().value("id").toString();
        QVERIFY2(pattern.match(id).hasMatch(), qPrintable(id));
        QVERIFY2(!rhythmIds.contains(id), qPrintable(id));
        rhythmIds.insert(id);
    }
    QCOMPARE(shipped("workouts.json").value("workouts").toArray().size(), 16);
    for (const QJsonValue &v : exercises) {
        const QString rhythm = v.toObject().value("metronome").toString();
        QVERIFY2(rhythm.isEmpty() || rhythmIds.contains(rhythm), qPrintable(rhythm));
    }

    for (const QString &language : Language::shipped()) {
        const QJsonObject texts = shipped(Language::textsFileName(language));
        QCOMPARE(texts.value("language").toString(), language);
        const QJsonObject ex = texts.value("exercises").toObject();
        const QJsonObject rh = texts.value("metronomes").toObject();
        // Every identifier has a text and no text has an unknown identifier.
        QCOMPARE(ex.size(), exerciseIds.size());
        QCOMPARE(rh.size(), rhythmIds.size());
        for (const QString &id : exerciseIds) {
            QVERIFY2(ex.contains(id), qPrintable(language + " " + id));
            QVERIFY(!ex.value(id).toObject().value("name").toString().isEmpty());
        }
        for (const QString &id : rhythmIds) {
            QVERIFY2(rh.contains(id), qPrintable(language + " " + id));
            QVERIFY(!rh.value(id).toObject().value("name").toString().isEmpty());
        }
        for (const QString &id : ex.keys())
            QVERIFY2(exerciseIds.contains(id), qPrintable(id));
        for (const QString &id : rh.keys())
            QVERIFY2(rhythmIds.contains(id), qPrintable(id));
        // The phase names are as many as the phases.
        for (const QJsonValue &v : rhythms) {
            const QJsonObject r = v.toObject();
            const QJsonArray names = rh.value(r.value("id").toString()).toObject().value("phaseNames").toArray();
            QVERIFY2(names.isEmpty() || names.size() == r.value("phases").toArray().size(), qPrintable(language));
        }
    }
}

void TstStarterLoader::shippedIdsIconsColoursAndGroups()
{
    const QJsonArray exercises = shipped("exercises.json").value("exercises").toArray();
    const QStringList icons = Icons::exerciseKeys();
    const QStringList levels = QStringList() << "green" << "yellow" << "red";
    QSet<QString> covered;
    for (const QJsonValue &v : exercises) {
        const QJsonObject o = v.toObject();
        const QString id = o.value("id").toString();
        QVERIFY2(icons.contains(o.value("icon").toString()), qPrintable(id));
        QVERIFY2(levels.contains(o.value("colour").toString()), qPrintable(id));
        for (const QJsonValue &m : o.value("muscles").toArray())
            covered.insert(m.toString());
    }
    for (const QString &key : MuscleKeys::keys())
        QVERIFY2(covered.contains(key), qPrintable(key));
}

void TstStarterLoader::shippedRhythmNamesCarryTheirCode()
{
    const QJsonArray rhythms = shipped("exercises.json").value("metronomes").toArray();
    for (const QString &language : Language::shipped()) {
        const QJsonObject names = shipped(Language::textsFileName(language)).value("metronomes").toObject();
        for (const QJsonValue &v : rhythms) {
            const QJsonObject r = v.toObject();
            const QString name = names.value(r.value("id").toString()).toObject().value("name").toString();
            const QString code = codeOf(r.value("phases").toArray());
            QVERIFY2(name.startsWith(code + QLatin1Char(' ')), qPrintable(name + " / " + code));
        }
    }
}

void TstStarterLoader::shippedLibraryLoadsInBothLanguages()
{
    for (const QString &language : Language::shipped()) {
        init();
        const StarterLoader::Result r = StarterLoader::apply(QStringLiteral(STARTER_SHIPPED_DIR), language, &t_.db);
        QVERIFY2(r.status == StoreStatus::Ok, qPrintable(language));
        QVERIFY(r.problems.isEmpty());
        QVERIFY(r.notices.isEmpty());
        QCOMPARE(r.exercisesAdded, 88);
        QCOMPARE(r.workoutsAdded, 16);
        QCOMPARE(r.rhythmsAdded, 16);
        QCOMPARE(countRows(t_.db, "exercise"), 88);
        QCOMPARE(countRows(t_.db, "metronome"), 16);
        QCOMPARE(countRows(t_.db, "workout"), 16);
        QCOMPARE(countRows(t_.db, "starter_offered"), 120);
        // A starter rhythm has no icon and no colour, and no two of them have the same name.
        QSqlQuery rhythmIcons;
        QVERIFY(t_.db.run("SELECT COUNT(*) FROM metronome WHERE icon <> ''", QVariantList(), &rhythmIcons) && rhythmIcons.next());
        QCOMPARE(rhythmIcons.value(0).toInt(), 0);
        QSet<QString> identities;
        for (const MetronomeSummary &m : MetronomeStore(&t_.db).list())
            identities.insert(m.name.toLower());
        QCOMPARE(identities.size(), 16);
        for (const Exercise &e : ExerciseStore(&t_.db).list()) {
            QCOMPARE(e.language, language);
            QVERIFY(e.followsLanguage);
            QVERIFY(e.starter);
        }
        QVERIFY(integrityOk(t_.db));
    }
    // Italian and back: every text of the first language is back and nothing was added.
    init();
    QVERIFY(StarterLoader::apply(QStringLiteral(STARTER_SHIPPED_DIR), "en", &t_.db).status == StoreStatus::Ok);
    const QList<Exercise> before = ExerciseStore(&t_.db).list();
    const StarterLoader::Result it = StarterLoader::apply(QStringLiteral(STARTER_SHIPPED_DIR), "it", &t_.db);
    QCOMPARE(it.exercisesAdded, 0);
    QCOMPARE(it.retexted, 120);
    QCOMPARE(countRows(t_.db, "exercise"), 88);
    const StarterLoader::Result en = StarterLoader::apply(QStringLiteral(STARTER_SHIPPED_DIR), "en", &t_.db);
    QCOMPARE(en.retexted, 120);
    const QList<Exercise> after = ExerciseStore(&t_.db).list();
    QCOMPARE(after.size(), before.size());
    for (int i = 0; i < before.size(); ++i) {
        QCOMPARE(after.at(i).name, before.at(i).name);
        QCOMPARE(after.at(i).instructions, before.at(i).instructions);
    }
}

// Planned time in minutes (R-WRK-9) as the app computes it, and with the lead-in of every step that
// has one (a rest has none).
void TstStarterLoader::shippedWorkoutsAreValidAndInTheirTimeRange()
{
    const QJsonObject structure = shipped("exercises.json");
    QSet<QString> rhythmIds;
    for (const QJsonValue &v : structure.value("metronomes").toArray())
        rhythmIds.insert(v.toObject().value("id").toString());
    QHash<QString, bool> hasRhythm;
    for (const QJsonValue &v : structure.value("exercises").toArray()) {
        const QJsonObject o = v.toObject();
        hasRhythm.insert(o.value("id").toString(), rhythmIds.contains(o.value("metronome").toString()));
    }

    const QJsonArray list = shipped("workouts.json").value("workouts").toArray();
    QCOMPARE(list.size(), 16);
    QSet<QString> workoutIds;
    // Beginner, advanced and hero ranges in minutes, by colour.
    const QHash<QString, QPair<double, double>> ranges{{"green", {40, 50}}, {"yellow", {45, 60}}, {"red", {50, 70}}};

    QVERIFY(StarterLoader::apply(QStringLiteral(STARTER_SHIPPED_DIR), "en", &t_.db).status == StoreStatus::Ok);
    WorkoutStore store(&t_.db);
    const ExerciseTempos tempos = ExerciseStore(&t_.db).tempoMap();
    const QList<WorkoutSummary> summaries = store.list();
    QCOMPARE(summaries.size(), 16);
    QHash<QString, WorkoutSummary> byName;
    for (const WorkoutSummary &sm : summaries)
        byName.insert(sm.name, sm);
    const QJsonObject names = shipped("texts.en.json").value("workouts").toObject();

    for (const QJsonValue &wv : list) {
        const QJsonObject wo = wv.toObject();
        const QString id = wo.value("id").toString();
        QVERIFY2(!workoutIds.contains(id), qPrintable(id));
        workoutIds.insert(id);
        const QString name = names.value(id).toObject().value("name").toString();
        QVERIFY2(byName.contains(name), qPrintable(id));
        const WorkoutSummary summary = byName.value(name);
        QVERIFY(!summary.icon.isEmpty());
        QVERIFY(ranges.contains(summary.colour));
        QVERIFY(summary.icon == wo.value("icon").toString());

        Workout w;
        QVERIFY(store.get(summary.id, &w));
        QVERIFY2(validate(w, tempos).isEmpty(), qPrintable(name));
        QVERIFY2(WorkoutStore::validateWorkout(w, tempos).isEmpty(), qPrintable(name));
        QVERIFY(w.restBetweenBlocksSec > 0);
        QCOMPARE(w.blocks.size(), wo.value("blocks").toArray().size());
        // The names file has one block name per block.
        QCOMPARE(names.value(id).toObject().value("blocks").toArray().size(), w.blocks.size());
        QVERIFY(w.blocks.size() >= 3);

        // Every step uses an exercise that exists; a rhythm means repetitions, no rhythm means a duration.
        for (const Block &b : w.blocks) {
            for (const Step &st : b.steps) {
                if (st.type == StepType::Rest) {
                    QVERIFY(st.isTimed());
                    continue;
                }
                Exercise e;
                QVERIFY2(ExerciseStore(&t_.db).get(st.exerciseKey, &e), qPrintable(st.exerciseKey));
                const bool rhythm = tempos.value(st.exerciseKey).isValid();
                QVERIFY2(rhythm ? (!st.isTimed() && st.reps > 0) : (st.isTimed() && st.reps == 0), qPrintable(name + " " + st.exerciseKey));
            }
        }
        // The first block is warm-up steps only, the last block cool-down steps only, the others neither.
        for (int b = 0; b < w.blocks.size(); ++b) {
            QVERIFY(!w.blocks.at(b).steps.isEmpty());
            for (const Step &st : w.blocks.at(b).steps) {
                if (b == 0)
                    QVERIFY2(st.type == StepType::Warmup, qPrintable(name));
                else if (b == w.blocks.size() - 1)
                    QVERIFY2(st.type == StepType::Cooldown, qPrintable(name));
                else
                    QVERIFY2(st.type == StepType::Work || st.type == StepType::Rest, qPrintable(name));
            }
        }

        // The planned time by the app's own rule, and with a lead-in of 3 s on every step but a rest.
        QVERIFY(!summary.hasRepSteps);
        int leadInSteps = 0;
        for (const PlanStep &ps : expandPlan(w, tempos))
            if (ps.type != StepType::Rest)
                ++leadInSteps;
        const double plain = summary.plannedSeconds / 60.0;
        const double withLeadIn = (summary.plannedSeconds + 3 * leadInSteps) / 60.0;
        const QPair<double, double> range = ranges.value(summary.colour);
        qInfo("planned %-34s %-6s %5.1f min, with lead-in %5.1f min", qPrintable(name), qPrintable(summary.colour), plain, withLeadIn);
        QVERIFY2(plain >= range.first && plain <= range.second, qPrintable(name));
        QVERIFY2(withLeadIn >= range.first && withLeadIn <= range.second, qPrintable(name));
    }
}

void TstStarterLoader::shippedWorkoutTextsFollowTheLanguage()
{
    QSet<QString> englishNames;
    for (const QString &language : Language::shipped()) {
        init();
        QVERIFY(StarterLoader::apply(QStringLiteral(STARTER_SHIPPED_DIR), language, &t_.db).status == StoreStatus::Ok);
        QSet<QString> names;
        for (const WorkoutSummary &sm : WorkoutStore(&t_.db).list())
            names.insert(sm.name);
        QCOMPARE(names.size(), 16);
        if (language == QLatin1String("en"))
            englishNames = names;
        else
            QVERIFY(names != englishNames);
    }
    // A change of language replaces the names of the workouts that follow it, and back again.
    init();
    QVERIFY(StarterLoader::apply(QStringLiteral(STARTER_SHIPPED_DIR), "en", &t_.db).status == StoreStatus::Ok);
    QVERIFY(StarterLoader::apply(QStringLiteral(STARTER_SHIPPED_DIR), "it", &t_.db).status == StoreStatus::Ok);
    QSet<QString> afterChange;
    for (const WorkoutSummary &sm : WorkoutStore(&t_.db).list())
        afterChange.insert(sm.name);
    QVERIFY(afterChange != englishNames);
    QVERIFY(StarterLoader::apply(QStringLiteral(STARTER_SHIPPED_DIR), "en", &t_.db).status == StoreStatus::Ok);
    QSet<QString> back;
    for (const WorkoutSummary &sm : WorkoutStore(&t_.db).list())
        back.insert(sm.name);
    QCOMPARE(back, englishNames);
    QCOMPARE(countRows(t_.db, "workout"), 16);
}

void TstStarterLoader::addsEntriesWithTheirIdentifierAndFollowFlag()
{
    const StarterLoader::Result r = load(Library::basic(), "en", &t_.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.exercisesAdded, 3);
    QCOMPARE(r.rhythmsAdded, 1);
    const Exercise e = starterExercise(t_.db, "00000002");
    QCOMPARE(e.name, QStringLiteral("Squat"));
    QCOMPARE(e.starterId, QStringLiteral("00000002"));
    QVERIFY(e.followsLanguage);
    QVERIFY(e.starter);
    QCOMPARE(e.language, QStringLiteral("en"));
    QCOMPARE(e.key, QStringLiteral("starter-00000002"));
    QSqlQuery q;
    QVERIFY(t_.db.run("SELECT starter_id, follows_language FROM metronome", QVariantList(), &q) && q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("00000001"));
    QCOMPARE(q.value(1).toInt(), 1);
}

void TstStarterLoader::addsOncePerIdentifier()
{
    const Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    const StarterLoader::Result again = load(l, "en", &t_.db);
    QVERIFY(again.status == StoreStatus::Ok);
    QVERIFY(!again.changed());
    QCOMPARE(countRows(t_.db, "exercise"), 3);
    QCOMPARE(countRows(t_.db, "starter_offered"), 4);
    // The other language does not add them again either.
    const StarterLoader::Result it = load(l, "it", &t_.db);
    QCOMPARE(it.exercisesAdded, 0);
    QCOMPARE(it.rhythmsAdded, 0);
    QCOMPARE(countRows(t_.db, "exercise"), 3);
    QCOMPARE(countRows(t_.db, "starter_offered"), 4);
}

void TstStarterLoader::deletedEntryIsNotAddedAgain()
{
    const Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    QVERIFY(ExerciseStore(&t_.db).remove("starter-00000003") == StoreStatus::Ok);
    SavedMetronome m;
    QVERIFY(MetronomeStore(&t_.db).getByKey("starter-m-00000001", &m));
    QVERIFY(MetronomeStore(&t_.db).remove(m.id) == StoreStatus::Ok);
    const StarterLoader::Result r = load(l, "it", &t_.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.exercisesAdded, 0);
    QCOMPARE(r.rhythmsAdded, 0);
    QCOMPARE(countRows(t_.db, "exercise"), 2);
    QCOMPARE(countRows(t_.db, "metronome"), 0);
}

void TstStarterLoader::newEntriesOfALaterFileAreAdded()
{
    Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    l.exercises << Library::Exercise{"00000004", QString(), "Bridge", "Ponte"};
    const StarterLoader::Result r = load(l, "it", &t_.db);
    QCOMPARE(r.exercisesAdded, 1);
    const Exercise e = starterExercise(t_.db, "00000004");
    QCOMPARE(e.name, QStringLiteral("Ponte"));
    QCOMPARE(e.language, QStringLiteral("it"));
}

void TstStarterLoader::rhythmsComeFirstAndAreLinked()
{
    QVERIFY(load(Library::basic(), "en", &t_.db).status == StoreStatus::Ok);
    SavedMetronome m;
    QVERIFY(MetronomeStore(&t_.db).getByKey("starter-m-00000001", &m));
    QCOMPARE(starterExercise(t_.db, "00000001").metronomeId, m.id);
    QCOMPARE(starterExercise(t_.db, "00000002").metronomeId, 0);
    QCOMPARE(m.tempo.phaseName(1), QStringLiteral("Down"));
}

void TstStarterLoader::missingRhythmAddsTheExerciseWithoutItAndReports()
{
    Library l = Library::basic();
    l.rhythms.clear();
    const StarterLoader::Result r = load(l, "en", &t_.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.exercisesAdded, 3);
    QCOMPARE(starterExercise(t_.db, "00000001").metronomeId, 0);
    QCOMPARE(r.notices.size(), 1);
    QCOMPARE(int(r.notices.first().code), int(ImportIssue::UnknownMetronome));
}

void TstStarterLoader::deletedRhythmLeavesTheExerciseWithoutTempo()
{
    const Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    SavedMetronome m;
    QVERIFY(MetronomeStore(&t_.db).getByKey("starter-m-00000001", &m));
    QVERIFY(MetronomeStore(&t_.db).remove(m.id) == StoreStatus::Ok);
    QCOMPARE(starterExercise(t_.db, "00000001").metronomeId, 0);
    QVERIFY(load(l, "it", &t_.db).status == StoreStatus::Ok);
    QCOMPARE(starterExercise(t_.db, "00000001").metronomeId, 0);
}

void TstStarterLoader::languageChangeReplacesTextsAndTag()
{
    const Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    const StarterLoader::Result r = load(l, "it", &t_.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.retexted, 4);
    const Exercise e = starterExercise(t_.db, "00000002");
    QCOMPARE(e.name, QStringLiteral("Accosciata"));
    QCOMPARE(e.instructions, QStringLiteral("Accosciata instructions"));
    QCOMPARE(e.tips, QStringLiteral("Accosciata tips"));
    QCOMPARE(e.language, QStringLiteral("it"));
    QVERIFY(e.followsLanguage);
    QCOMPARE(rhythmName(t_.db, "00000001"), QStringLiteral("2P-U-4 Giu-su"));
}

void TstStarterLoader::languageChangeLeavesUserEntriesAlone()
{
    const Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    ExerciseStore store(&t_.db);
    Exercise mine = makeExercise("Plank");
    QVERIFY(store.create(&mine) == StoreStatus::Ok);
    mine.language = "en";
    QVERIFY(store.update(mine) == StoreStatus::Ok);
    const int mineId = addMetronome(t_.db, "My rhythm", Tempo::phases(QList<int>() << 4 << 4, 1));
    QVERIFY(mineId > 0);
    QVERIFY(load(l, "it", &t_.db).status == StoreStatus::Ok);
    Exercise read;
    QVERIFY(store.get(mine.key, &read));
    QCOMPARE(read.name, QStringLiteral("Plank"));
    QCOMPARE(read.language, QStringLiteral("en"));
    QVERIFY(!read.followsLanguage);
    QVERIFY(read.starterId.isEmpty());
    SavedMetronome m;
    QVERIFY(MetronomeStore(&t_.db).get(mineId, &m));
    QCOMPARE(m.name, QStringLiteral("My rhythm"));
    // The starter entry of the same name was retexted next to it.
    QCOMPARE(starterExercise(t_.db, "00000001").name, QStringLiteral("Plancia"));
}

void TstStarterLoader::languageChangeRetextsRhythmsAndWorkouts()
{
    Library l = Library::basic();
    l.workouts << Library::Workout{"00000001", "00000002", "Circuit", "Circuito"};
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    const StarterLoader::Result r = load(l, "it", &t_.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.workoutsAdded, 0);
    const QList<WorkoutSummary> workouts = WorkoutStore(&t_.db).list();
    QCOMPARE(workouts.size(), 1);
    QCOMPARE(workouts.first().name, QStringLiteral("Circuito"));
    SavedMetronome m;
    QVERIFY(MetronomeStore(&t_.db).getByKey("starter-m-00000001", &m));
    QCOMPARE(m.tempo.phaseName(1), QStringLiteral("Giu"));
}

void TstStarterLoader::twoChangesInARowEndWhereTheyStarted()
{
    const Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    QVERIFY(load(l, "it", &t_.db).status == StoreStatus::Ok);
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    const Exercise e = starterExercise(t_.db, "00000003");
    QCOMPARE(e.name, QStringLiteral("Lunge"));
    QCOMPARE(e.language, QStringLiteral("en"));
    QCOMPARE(countRows(t_.db, "exercise"), 3);
    QCOMPARE(countRows(t_.db, "starter_offered"), 4);
}

void TstStarterLoader::textMissingInTheLanguageFallsBackToEnglish()
{
    Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    l.exercises[2].other.clear();
    QVERIFY(load(l, "it", &t_.db).status == StoreStatus::Ok);
    const Exercise lunge = starterExercise(t_.db, "00000003");
    QCOMPARE(lunge.name, QStringLiteral("Lunge"));
    QCOMPARE(lunge.language, QStringLiteral("en"));
    QCOMPARE(starterExercise(t_.db, "00000002").language, QStringLiteral("it"));
    // A first start in that language adds it in English with the tag "en".
    init();
    QVERIFY(load(l, "it", &t_.db).status == StoreStatus::Ok);
    QCOMPARE(starterExercise(t_.db, "00000003").name, QStringLiteral("Lunge"));
    QCOMPARE(starterExercise(t_.db, "00000003").language, QStringLiteral("en"));
    QCOMPARE(starterExercise(t_.db, "00000002").language, QStringLiteral("it"));
}

void TstStarterLoader::nameClashGetsTheSuffix()
{
    const Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    ExerciseStore store(&t_.db);
    Exercise mine = makeExercise("Plancia");
    QVERIFY(store.create(&mine) == StoreStatus::Ok);
    QVERIFY(load(l, "it", &t_.db).status == StoreStatus::Ok);
    QCOMPARE(starterExercise(t_.db, "00000001").name, QStringLiteral("Plancia (starter)"));
    Exercise read;
    QVERIFY(store.get(mine.key, &read));
    QCOMPARE(read.name, QStringLiteral("Plancia"));
    // Changing back gives the plain name, with nothing left over.
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    QCOMPARE(starterExercise(t_.db, "00000001").name, QStringLiteral("Plank"));

    // The same for a rhythm and a workout, which share a name only with another icon or colour.
    Library m = Library::basic();
    init();
    QVERIFY(addMetronome(t_.db, "2P-U-4 Giu-su", Tempo::phases(QList<int>() << 4 << 4, 1)) > 0);
    QVERIFY(load(m, "it", &t_.db).status == StoreStatus::Ok);
    QCOMPARE(rhythmName(t_.db, "00000001"), QStringLiteral("2P-U-4 Giu-su (starter)"));
}

void TstStarterLoader::changeOfALaterFileIsOneTransaction()
{
    const Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    QVERIFY(t_.db.run("CREATE TRIGGER stop_here BEFORE UPDATE ON exercise WHEN NEW.name = 'Accosciata' "
                      "BEGIN SELECT RAISE(ABORT, 'stop'); END"));
    const StarterLoader::Result r = load(l, "it", &t_.db);
    QVERIFY(r.status == StoreStatus::Failed);
    // Nothing of the change was kept, not even what came before the failure.
    QCOMPARE(starterExercise(t_.db, "00000002").name, QStringLiteral("Squat"));
    QCOMPARE(rhythmName(t_.db, "00000001"), QStringLiteral("2P-U-4 Down-up"));
    QCOMPARE(starterExercise(t_.db, "00000001").language, QStringLiteral("en"));
}

void TstStarterLoader::editingATextDetaches()
{
    QVERIFY(load(Library::basic(), "en", &t_.db).status == StoreStatus::Ok);
    ExerciseStore store(&t_.db);
    for (int field = 0; field < 3; ++field) {
        init();
        QVERIFY(load(Library::basic(), "en", &t_.db).status == StoreStatus::Ok);
        Exercise e = starterExercise(t_.db, "00000002");
        QVERIFY(e.followsLanguage);
        if (field == 0)
            e.name = "My squat";
        else if (field == 1)
            e.instructions = "Mine.";
        else
            e.tips = "Mine.";
        QVERIFY(store.update(e) == StoreStatus::Ok);
        const Exercise read = starterExercise(t_.db, "00000002");
        QVERIFY(!read.followsLanguage);
        QCOMPARE(read.starterId, QStringLiteral("00000002"));
        QVERIFY(read.starter);
        // It stays detached when the same text is put back.
        QVERIFY(store.update(e) == StoreStatus::Ok);
        QVERIFY(!starterExercise(t_.db, "00000002").followsLanguage);
    }
}

void TstStarterLoader::editingAnotherFieldKeepsFollowing()
{
    QVERIFY(load(Library::basic(), "en", &t_.db).status == StoreStatus::Ok);
    ExerciseStore store(&t_.db);
    Exercise e = starterExercise(t_.db, "00000002");
    e.muscles = muscleBit(MuscleGroup::Chest);
    e.colour = "red";
    e.icon = "plank";
    e.language = "it";
    e.metronomeId = starterExercise(t_.db, "00000001").metronomeId;
    QVERIFY(store.update(e) == StoreStatus::Ok);
    const Exercise read = starterExercise(t_.db, "00000002");
    QVERIFY(read.followsLanguage);
    QCOMPARE(read.colour, QStringLiteral("red"));
}

void TstStarterLoader::detachedEntryKeepsItsTextsAfterAChange()
{
    const Library l = Library::basic();
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    ExerciseStore store(&t_.db);
    Exercise e = starterExercise(t_.db, "00000002");
    e.name = "My squat";
    QVERIFY(store.update(e) == StoreStatus::Ok);
    QVERIFY(load(l, "it", &t_.db).status == StoreStatus::Ok);
    const Exercise read = starterExercise(t_.db, "00000002");
    QCOMPARE(read.name, QStringLiteral("My squat"));
    QCOMPARE(read.instructions, QStringLiteral("Squat instructions"));
    QCOMPARE(read.language, QStringLiteral("en"));
    // The others of the same file changed.
    QCOMPARE(starterExercise(t_.db, "00000003").name, QStringLiteral("Affondo"));
}

void TstStarterLoader::editingARhythmNameOrPhaseNameDetaches()
{
    for (int field = 0; field < 2; ++field) {
        init();
        QVERIFY(load(Library::basic(), "en", &t_.db).status == StoreStatus::Ok);
        MetronomeStore store(&t_.db);
        SavedMetronome m;
        QVERIFY(store.getByKey("starter-m-00000001", &m));
        // Saving without a change does not detach.
        QVERIFY(store.update(m.id, m) == StoreStatus::Ok);
        QVERIFY(load(Library::basic(), "it", &t_.db).status == StoreStatus::Ok);
        QCOMPARE(rhythmName(t_.db, "00000001"), QStringLiteral("2P-U-4 Giu-su"));
        // A name or a phase name does.
        QVERIFY(store.getByKey("starter-m-00000001", &m));
        if (field == 0)
            m.name = "Mine";
        else
            m.tempo = Tempo::phases(m.tempo.phaseUnitList(), 1, QStringList() << "x" << "y");
        QVERIFY(store.update(m.id, m) == StoreStatus::Ok);
        const QString kept = rhythmName(t_.db, "00000001");
        QVERIFY(load(Library::basic(), "en", &t_.db).status == StoreStatus::Ok);
        QCOMPARE(rhythmName(t_.db, "00000001"), kept);
    }
}

void TstStarterLoader::editingAWorkoutNameDetaches()
{
    Library l = Library::basic();
    l.workouts << Library::Workout{"00000001", "00000002", "Circuit", "Circuito"};
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    WorkoutStore store(&t_.db);
    Workout w;
    const int id = store.list().first().id;
    QVERIFY(store.get(id, &w));
    // A rest between blocks does not detach.
    w.restBetweenBlocksSec = 5;
    QVERIFY(store.update(id, w) == StoreStatus::Ok);
    QVERIFY(load(l, "it", &t_.db).status == StoreStatus::Ok);
    QCOMPARE(store.list().first().name, QStringLiteral("Circuito"));
    QVERIFY(store.get(id, &w));
    w.name = "My circuit";
    QVERIFY(store.update(id, w) == StoreStatus::Ok);
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    QCOMPARE(store.list().first().name, QStringLiteral("My circuit"));
}

void TstStarterLoader::invalidFilesAddNothing()
{
    const Library l = Library::basic();
    for (int which = 0; which < 4; ++which) {
        init();
        StarterLoader::Files f = l.files("it");
        if (which == 0)
            f.structure = "{not json";
        else if (which == 1)
            f.workouts = bytes(QJsonObject{{"format", "other"}, {"version", 1}});
        else if (which == 2)
            f.texts = bytes(QJsonObject{{"format", "workout-trainer-starter-texts"}, {"version", 2}, {"language", "it"}});
        else
            f.english = bytes(QJsonObject{{"format", "workout-trainer-starter-texts"}, {"version", 1}, {"language", "fr"}});
        const StarterLoader::Result r = StarterLoader::applyBytes(f, "it", &t_.db);
        QVERIFY2(r.status == StoreStatus::Invalid, qPrintable(QString::number(which)));
        QVERIFY(!r.problems.isEmpty());
        QCOMPARE(countRows(t_.db, "exercise"), 0);
        QCOMPARE(countRows(t_.db, "metronome"), 0);
        QCOMPARE(countRows(t_.db, "starter_offered"), 0);
    }
    // A bad value inside an entry rejects the whole file, even when the first entries are fine.
    init();
    StarterLoader::Files f = l.files("en");
    QJsonObject structure = QJsonDocument::fromJson(f.structure).object();
    QJsonArray ex = structure.value("exercises").toArray();
    QJsonObject last = ex.last().toObject();
    last.insert("colour", 5);
    ex.replace(ex.size() - 1, last);
    structure.insert("exercises", ex);
    f.structure = bytes(structure);
    QVERIFY(StarterLoader::applyBytes(f, "en", &t_.db).status == StoreStatus::Invalid);
    QCOMPARE(countRows(t_.db, "exercise"), 0);
    // A repeated or malformed identifier is a problem.
    const QStringList badIds = QStringList() << "00000001" << "ABCDEF01" << "1" << "starter-1";
    for (const QString &bad : badIds) {
        init();
        Library m = Library::basic();
        m.exercises[1].id = bad;
        QVERIFY2(load(m, "en", &t_.db).status == StoreStatus::Invalid, qPrintable(bad));
        QCOMPARE(countRows(t_.db, "exercise"), 0);
    }
}

void TstStarterLoader::entryWithoutEnglishTextIsAnError()
{
    Library l = Library::basic();
    l.exercises[1].en.clear();
    const StarterLoader::Result r = load(l, "it", &t_.db);
    QVERIFY(r.status == StoreStatus::Invalid);
    QCOMPARE(countRows(t_.db, "exercise"), 0);
    Library m = Library::basic();
    m.rhythms[0].en.clear();
    QVERIFY(load(m, "en", &t_.db).status == StoreStatus::Invalid);
}

void TstStarterLoader::unknownTextIdIsIgnoredAndReported()
{
    const Library l = Library::basic();
    StarterLoader::Files f = l.files("it");
    QJsonObject texts = QJsonDocument::fromJson(f.texts).object();
    QJsonObject ex = texts.value("exercises").toObject();
    ex.insert("0000ffff", QJsonObject{{"name", "Ghost"}, {"instructions", ""}, {"tips", ""}});
    texts.insert("exercises", ex);
    f.texts = bytes(texts);
    const StarterLoader::Result r = StarterLoader::applyBytes(f, "it", &t_.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.exercisesAdded, 3);
    QCOMPARE(r.notices.size(), 1);
    QCOMPARE(countRows(t_.db, "exercise"), 3);
}

void TstStarterLoader::missingDirectoryOrLanguageAddsNothing()
{
    const StarterLoader::Result gone = StarterLoader::apply(inDir(t_.dir, "nowhere"), "en", &t_.db);
    QVERIFY(gone.status != StoreStatus::Ok);
    QVERIFY(!gone.problems.isEmpty());
    // A language with no texts file and one that is not a tag, nor a path trick.
    const QStringList badLanguages = QStringList() << "fr" << "../en" << "" << "EN";
    for (const QString &language : badLanguages) {
        const StarterLoader::Result r = StarterLoader::apply(QStringLiteral(STARTER_SHIPPED_DIR), language, &t_.db);
        QVERIFY2(r.status != StoreStatus::Ok, qPrintable(language));
        QVERIFY(!r.problems.isEmpty());
    }
    QCOMPARE(countRows(t_.db, "exercise"), 0);
    QCOMPARE(countRows(t_.db, "starter_offered"), 0);
}

void TstStarterLoader::workoutsRefuseUnknownExercisesAndComeBack()
{
    Library l = Library::basic();
    l.workouts << Library::Workout{"00000001", "00000002", "Circuit", "Circuito"}
               << Library::Workout{"00000002", "00000009", "Ghost", "Fantasma"};
    const StarterLoader::Result r = load(l, "en", &t_.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.workoutsAdded, 1);
    QCOMPARE(r.notices.size(), 1);
    QCOMPARE(int(r.notices.first().code), int(ImportIssue::UnknownExercise));
    QCOMPARE(countRows(t_.db, "workout"), 1);
    QCOMPARE(countRows(t_.db, "starter_offered"), 1 + 3 + 1);
    // When the exercise appears in a later file the workout is offered.
    l.exercises << Library::Exercise{"00000009", QString(), "Ghost squat", "Accosciata fantasma"};
    const StarterLoader::Result later = load(l, "en", &t_.db);
    QCOMPARE(later.workoutsAdded, 1);
    QCOMPARE(countRows(t_.db, "workout"), 2);
}

void TstStarterLoader::workoutStepsResolveStarterIdentifiers()
{
    Library l = Library::basic();
    l.workouts << Library::Workout{"00000001", "00000003", "Circuit", "Circuito"};
    QVERIFY(load(l, "en", &t_.db).status == StoreStatus::Ok);
    WorkoutStore store(&t_.db);
    Workout w;
    QVERIFY(store.get(store.list().first().id, &w));
    QCOMPARE(w.blocks.first().steps.first().exerciseKey, StarterLoader::exerciseKey("00000003"));
    QVERIFY(!w.favorite);
}

void TstStarterLoader::firstStepOfAStarterWorkoutStartsAutomatically()
{
    Library l = Library::basic();
    l.workouts << Library::Workout{"00000001", "00000003", "Circuit", "Circuito"};
    StarterLoader::Files f = l.files("en");
    QJsonObject file = QJsonDocument::fromJson(f.workouts).object();
    QJsonArray list = file.value("workouts").toArray();
    QJsonObject w = list.first().toObject();
    QJsonArray blocks = w.value("blocks").toArray();
    QJsonObject block = blocks.first().toObject();
    QJsonArray steps = block.value("steps").toArray();
    QJsonObject first = steps.first().toObject();
    first.insert("startOnTap", true);
    steps.replace(0, first);
    block.insert("steps", steps);
    blocks.replace(0, block);
    w.insert("blocks", blocks);
    list.replace(0, w);
    file.insert("workouts", list);
    f.workouts = bytes(file);
    QVERIFY(StarterLoader::applyBytes(f, "en", &t_.db).status == StoreStatus::Ok);
    WorkoutStore store(&t_.db);
    Workout read;
    QVERIFY(store.get(store.list().first().id, &read));
    QVERIFY(!read.blocks.first().steps.first().startOnTap);
}

void TstStarterLoader::userAndImportedEntriesHaveNoStarterData()
{
    ExerciseStore store(&t_.db);
    Exercise mine = makeExercise("Mine");
    mine.starterId = "00000001";
    mine.followsLanguage = true;
    QVERIFY(store.create(&mine) == StoreStatus::Ok);
    QString copyKey;
    QVERIFY(load(Library::basic(), "en", &t_.db).status == StoreStatus::Ok);
    QVERIFY(store.clone("starter-00000002", &copyKey) == StoreStatus::Ok);
    const QStringList userKeys = QStringList() << mine.key << copyKey;
    for (const QString &key : userKeys) {
        Exercise e;
        QVERIFY(store.get(key, &e));
        QVERIFY(e.starterId.isEmpty());
        QVERIFY(!e.followsLanguage);
        QVERIFY(!e.starter);
    }
    Exercise keyed = makeExercise("Keyed");
    keyed.key = "u-imported";
    keyed.starterId = "00000001";
    keyed.followsLanguage = true;
    QVERIFY(store.createKeyed(keyed) == StoreStatus::Ok);
    Exercise read;
    QVERIFY(store.get("u-imported", &read));
    QVERIFY(read.starterId.isEmpty());
    QVERIFY(!read.followsLanguage);
    // A language change does not touch any of them.
    QVERIFY(load(Library::basic(), "it", &t_.db).status == StoreStatus::Ok);
    QVERIFY(store.get("u-imported", &read));
    QCOMPARE(read.name, QStringLiteral("Keyed"));
    QVERIFY(store.get(copyKey, &read));
    QVERIFY(read.name.startsWith("Squat"));
}

QTEST_APPLESS_MAIN(TstStarterLoader)

#include "tst_starterloader.moc"
