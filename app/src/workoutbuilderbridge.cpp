// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "workoutbuilderbridge.h"

#include "colours.h"
#include "icons.h"
#include "services.h"
#include "workoutform.h"
#include "workoutlimits.h"

namespace {

QString statusName(StoreStatus status)
{
    switch (status) {
    case StoreStatus::Ok: return QStringLiteral("ok");
    case StoreStatus::Invalid: return QStringLiteral("invalid");
    case StoreStatus::NotFound: return QStringLiteral("not-found");
    default: return QStringLiteral("failed");
    }
}

}

WorkoutBuilderBridge::WorkoutBuilderBridge(Services *services, QObject *parent)
    : QObject(parent), services_(services)
{
}

QVariantMap WorkoutBuilderBridge::limits() const
{
    QVariantMap map;
    map.insert(QStringLiteral("minDurationSec"), Limits::minDurationSec);
    map.insert(QStringLiteral("maxDurationSec"), Limits::maxDurationSec);
    map.insert(QStringLiteral("minReps"), Limits::minReps);
    map.insert(QStringLiteral("maxReps"), Limits::maxReps);
    map.insert(QStringLiteral("minRounds"), Limits::minRounds);
    map.insert(QStringLiteral("maxRounds"), Limits::maxRounds);
    map.insert(QStringLiteral("minRestSec"), Limits::minRestSec);
    map.insert(QStringLiteral("maxRestSec"), Limits::maxRestSec);
    map.insert(QStringLiteral("maxStepsPerBlock"), Limits::maxStepsPerBlock);
    map.insert(QStringLiteral("maxBlocksPerWorkout"), Limits::maxBlocksPerWorkout);
    map.insert(QStringLiteral("maxNameChars"), Limits::maxNameChars);
    return map;
}

QStringList WorkoutBuilderBridge::colours() const { return Colours::keys(); }
QStringList WorkoutBuilderBridge::icons() const { return Icons::workoutKeys(); }

bool WorkoutBuilderBridge::begin(int id)
{
    ready_ = false;
    workout_ = Workout();
    if (!services_->isReady())
        return false;
    if (id > 0) {
        if (!services_->workouts()->get(id, &workout_))
            return false;
    } else {
        workout_.blocks.append(Block());
    }
    lookups_ = WorkoutView::load(services_);
    collapsed_ = QList<bool>();
    arranging_ = false;
    ready_ = true;
    update();
    return true;
}

void WorkoutBuilderBridge::refreshLookups()
{
    if (!ready_)
        return;
    lookups_ = WorkoutView::load(services_);
    update();
}

void WorkoutBuilderBridge::update()
{
    header_ = WorkoutForm::headerToMap(workout_);
    problems_.clear();
    if (ready_) {
        bool failed = false;
        const QList<Problem> found = services_->workouts()->problemsOf(workout_, workout_.id, &failed);
        problems_ = WorkoutForm::problemList(found);
        if (failed) {
            QVariantMap p;
            p.insert(QStringLiteral("code"), QStringLiteral("failed"));
            p.insert(QStringLiteral("block"), -1);
            p.insert(QStringLiteral("step"), -1);
            problems_.append(p);
        }
    }
    while (collapsed_.size() < workout_.blocks.size())
        collapsed_.append(false);
    while (collapsed_.size() > workout_.blocks.size())
        collapsed_.removeLast();
    WorkoutView::RowOptions options;
    options.addRows = true;
    options.collapsed = collapsed_;
    rows_.setRows(ready_ ? WorkoutView::rows(workout_, lookups_, problems_, options) : QVariantList());
    total_ = WorkoutView::total(workout_, lookups_);
    emit changed();
}

void WorkoutBuilderBridge::setName(const QString &name)
{
    if (!ready_ || name == workout_.name)
        return;
    workout_.name = name;
    update();
}

void WorkoutBuilderBridge::setIcon(const QString &icon)
{
    if (!ready_ || icon == workout_.icon)
        return;
    workout_.icon = icon;
    update();
}

void WorkoutBuilderBridge::setColour(const QString &colour)
{
    if (!ready_ || colour == workout_.colour)
        return;
    workout_.colour = colour;
    update();
}

void WorkoutBuilderBridge::setRestBetweenBlocks(int seconds)
{
    if (!ready_ || seconds == workout_.restBetweenBlocksSec)
        return;
    workout_.restBetweenBlocksSec = seconds;
    update();
}

QVariantMap WorkoutBuilderBridge::block(int block)
{
    if (!ready_ || block < 0 || block >= workout_.blocks.size())
        return QVariantMap();
    QVariantMap map = WorkoutForm::blockToMap(workout_.blocks.at(block));
    QStringList codes;
    for (const QVariant &v : problems_) {
        const QVariantMap p = v.toMap();
        if (p.value(QStringLiteral("block")).toInt() == block && p.value(QStringLiteral("step")).toInt() == -1)
            codes.append(p.value(QStringLiteral("code")).toString());
    }
    map.insert(QStringLiteral("problems"), codes);
    return map;
}

void WorkoutBuilderBridge::setRounds(int block, int rounds)
{
    if (!ready_ || block < 0 || block >= workout_.blocks.size() || workout_.blocks.at(block).rounds == rounds)
        return;
    workout_.blocks[block].rounds = rounds;
    update();
}

void WorkoutBuilderBridge::setRestBetweenRounds(int block, int seconds)
{
    if (!ready_ || block < 0 || block >= workout_.blocks.size()
        || workout_.blocks.at(block).restBetweenRoundsSec == seconds)
        return;
    workout_.blocks[block].restBetweenRoundsSec = seconds;
    update();
}

Step *WorkoutBuilderBridge::stepAt(int block, int step)
{
    if (!ready_ || block < 0 || block >= workout_.blocks.size() || step < 0
        || step >= workout_.blocks.at(block).steps.size())
        return nullptr;
    return &workout_.blocks[block].steps[step];
}

QVariantMap WorkoutBuilderBridge::step(int block, int step)
{
    if (!stepAt(block, step))
        return QVariantMap();
    return WorkoutView::stepRow(workout_, block, step, lookups_, problems_);
}

void WorkoutBuilderBridge::setStepType(int block, int step, const QString &type)
{
    Step *s = stepAt(block, step);
    StepType parsed;
    if (!s || !stepTypeFromName(type, &parsed))
        return;
    WorkoutDraft::setStepType(*s, parsed);
    update();
}

void WorkoutBuilderBridge::setStepTimed(int block, int step, bool timed)
{
    Step *s = stepAt(block, step);
    if (!s)
        return;
    WorkoutDraft::setStepTimed(*s, timed);
    update();
}

void WorkoutBuilderBridge::setStepDuration(int block, int step, int seconds)
{
    Step *s = stepAt(block, step);
    if (!s)
        return;
    WorkoutDraft::setStepDuration(*s, seconds);
    update();
}

void WorkoutBuilderBridge::setStepReps(int block, int step, int reps)
{
    Step *s = stepAt(block, step);
    if (!s)
        return;
    WorkoutDraft::setStepReps(*s, reps);
    update();
}

void WorkoutBuilderBridge::setStepPace(int block, int step, const QString &kind, int paceMs)
{
    Step *s = stepAt(block, step);
    PaceKind parsed = PaceKind::None;
    if (!s || (kind != QLatin1String("none") && !paceKindFromName(kind, &parsed)))
        return;
    WorkoutDraft::setStepPace(*s, parsed, paceMs);
    update();
}

void WorkoutBuilderBridge::setStepStartOnTap(int block, int step, bool onTap)
{
    Step *s = stepAt(block, step);
    if (!s)
        return;
    s->startOnTap = onTap;
    normalizeStartModes(workout_);
    update();
}

void WorkoutBuilderBridge::setStepExercise(int block, int step, const QString &exerciseKey)
{
    Step *s = stepAt(block, step);
    if (!s || s->type == StepType::Rest)
        return;
    s->exerciseKey = exerciseKey;
    // The exercise may be new: its name and rhythm are read now.
    lookups_ = WorkoutView::load(services_);
    // A step of an exercise with a rhythm ends by repetitions only (R-WRK-14).
    if (s->isTimed() && lookups_.hasTempo(exerciseKey))
        WorkoutDraft::setStepTimed(*s, false);
    update();
}

QVariantMap WorkoutBuilderBridge::position(const WorkoutDraft::Position &p) const
{
    QVariantMap map;
    if (p.isValid()) {
        map.insert(QStringLiteral("block"), p.block);
        map.insert(QStringLiteral("step"), p.step);
    }
    return map;
}

QVariantMap WorkoutBuilderBridge::addStep(int block)
{
    if (!ready_)
        return QVariantMap();
    const WorkoutDraft::Position p = WorkoutDraft::addStep(workout_, block);
    update();
    return position(p);
}

QVariantMap WorkoutBuilderBridge::addBlock()
{
    if (!ready_)
        return QVariantMap();
    const WorkoutDraft::Position p = WorkoutDraft::addBlock(workout_);
    update();
    return position(p);
}

bool WorkoutBuilderBridge::removeStep(int block, int step)
{
    const bool done = ready_ && WorkoutDraft::removeStep(workout_, block, step);
    if (done)
        update();
    return done;
}

bool WorkoutBuilderBridge::removeBlock(int block)
{
    const bool done = ready_ && WorkoutDraft::removeBlock(workout_, block);
    if (done) {
        if (block < collapsed_.size())
            collapsed_.removeAt(block);
        update();
    }
    return done;
}

QVariantMap WorkoutBuilderBridge::duplicateStep(int block, int step)
{
    if (!ready_)
        return QVariantMap();
    const WorkoutDraft::Position p = WorkoutDraft::duplicateStep(workout_, block, step);
    update();
    return position(p);
}

QVariantMap WorkoutBuilderBridge::duplicateBlock(int block)
{
    if (!ready_)
        return QVariantMap();
    const bool had = block >= 0 && block < collapsed_.size();
    const bool wasCollapsed = had && collapsed_.at(block);
    const WorkoutDraft::Position p = WorkoutDraft::duplicateBlock(workout_, block);
    if (p.isValid() && had)
        collapsed_.insert(block + 1, wasCollapsed);
    update();
    return position(p);
}

QVariantMap WorkoutBuilderBridge::moveStep(int block, int step, int delta)
{
    if (!ready_)
        return QVariantMap();
    const WorkoutDraft::Position p = WorkoutDraft::moveStep(workout_, block, step, delta);
    update();
    return position(p);
}

QVariantMap WorkoutBuilderBridge::moveBlock(int block, int delta)
{
    if (!ready_)
        return QVariantMap();
    const WorkoutDraft::Position p = WorkoutDraft::moveBlock(workout_, block, delta);
    if (p.isValid() && block >= 0 && block < collapsed_.size() && p.block != block)
        collapsed_.move(block, p.block);
    update();
    return position(p);
}

void WorkoutBuilderBridge::toggleBlockCollapsed(int block)
{
    if (!ready_ || block < 0 || block >= collapsed_.size())
        return;
    collapsed_[block] = !collapsed_.at(block);
    update();
}

void WorkoutBuilderBridge::setArranging(bool arranging)
{
    if (!ready_ || arranging == arranging_)
        return;
    arranging_ = arranging;
    for (int i = 0; i < collapsed_.size(); ++i)
        collapsed_[i] = arranging;
    update();
}

QVariantMap WorkoutBuilderBridge::moveStepAcross(int block, int step, int delta)
{
    if (!ready_)
        return QVariantMap();
    const WorkoutDraft::Position p = WorkoutDraft::moveStepAcross(workout_, block, step, delta);
    update();
    return position(p);
}

QVariantMap WorkoutBuilderBridge::moveStepToBlock(int block, int step, int targetBlock)
{
    if (!ready_)
        return QVariantMap();
    const WorkoutDraft::Position p = WorkoutDraft::moveStepToBlock(workout_, block, step, targetBlock);
    update();
    return position(p);
}

QVariantMap WorkoutBuilderBridge::save()
{
    QVariantMap map;
    if (!ready_ || !services_->isReady()) {
        map.insert(QStringLiteral("status"), QStringLiteral("failed"));
        return map;
    }
    QList<Problem> found;
    int id = workout_.id;
    StoreStatus status;
    if (workout_.id > 0) {
        status = services_->workouts()->update(workout_.id, workout_, &found);
    } else {
        status = services_->workouts()->create(workout_, &id, &found);
        if (status == StoreStatus::Ok)
            workout_.id = id;
    }
    map.insert(QStringLiteral("status"), statusName(status));
    map.insert(QStringLiteral("id"), status == StoreStatus::Ok ? id : 0);
    map.insert(QStringLiteral("problems"), WorkoutForm::problemList(found));
    if (status == StoreStatus::Ok)
        update();
    return map;
}
