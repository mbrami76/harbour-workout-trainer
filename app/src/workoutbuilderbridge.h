// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef WORKOUTBUILDERBRIDGE_H
#define WORKOUTBUILDERBRIDGE_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "model.h"
#include "variantrowsmodel.h"
#include "workoutdraft.h"
#include "workoutview.h"

class Services;

// The workout that is being built (R-WRK-1 to R-WRK-13) for QML. Exposed as the context property
// "workoutBuilder", created in main() with the Services.
//
// The draft lives here, not in a page: begin() starts it and the pages that edit it (the builder,
// the block and step pages, the exercise picker and the exercise editor above them) only read it
// and call the setters. Pages stacked above the builder therefore change nothing about what the
// builder shows when they close, and a "New exercise" round trip loses no input. Nothing is
// written until save(); starting another draft or leaving the builder drops the unsaved one.
//
// The rules stay in the core: after every change the draft is checked with the validation of the
// core and the store (the limits, the exercises and the identity of R-WRK-12), and `problems`
// holds the findings; Save is possible only without any. The store has the last word on save.
//
// Places are zero-based block and step indices. An operation that moves or adds returns the new
// place as {block, step} (step -1 for a block) or an empty map when the place did not exist.
class WorkoutBuilderBridge : public QObject
{
    Q_OBJECT
    // {id, favorite, name, icon, colour, restBetweenBlocksSec, blockCount, stepCount}; id is 0 for
    // a workout that was never saved.
    Q_PROPERTY(QVariantMap header READ header NOTIFY changed)
    // The blocks and steps as flat rows of a ListView (see WorkoutView::rows()), with a row to add
    // a step at the end of each block.
    Q_PROPERTY(QObject *rows READ rows CONSTANT)
    // [{code, block, step}] in order, the codes of WorkoutForm::problemCode().
    Q_PROPERTY(QVariantList problems READ problems NOTIFY changed)
    Q_PROPERTY(bool canSave READ canSave NOTIFY changed)
    // The blocks are being arranged: they are shown collapsed, one line each, with their move
    // buttons (R-WRK-6). A view only, never stored in the workout.
    Q_PROPERTY(bool arranging READ arranging NOTIFY changed)
    // The planned time: {seconds, hasTempoSteps, uncounted, uncountedNames} (R-WRK-9).
    Q_PROPERTY(QVariantMap total READ total NOTIFY changed)
    // The input limits of the core (R-WRK-8): minDurationSec, maxDurationSec, minReps, maxReps,
    // minRounds, maxRounds, minRestSec, maxRestSec, maxStepsPerBlock, maxBlocksPerWorkout,
    // maxNameChars.
    Q_PROPERTY(QVariantMap limits READ limits CONSTANT)
    Q_PROPERTY(QStringList colours READ colours CONSTANT)
    Q_PROPERTY(QStringList icons READ icons CONSTANT)

public:
    explicit WorkoutBuilderBridge(Services *services, QObject *parent = nullptr);

    QVariantMap header() const { return header_; }
    QObject *rows() { return &rows_; }
    bool arranging() const { return arranging_; }
    QVariantList problems() const { return problems_; }
    bool canSave() const { return problems_.isEmpty() && ready_; }
    QVariantMap total() const { return total_; }
    QVariantMap limits() const;
    QStringList colours() const;
    QStringList icons() const;

    // Starts a draft: the saved workout of the id, or a new one (id 0) with one empty block.
    // Returns false when the workout cannot be read.
    Q_INVOKABLE bool begin(int id);
    // Reads the exercises and their tempos again, after a page changed them.
    Q_INVOKABLE void refreshLookups();

    Q_INVOKABLE void setName(const QString &name);
    Q_INVOKABLE void setIcon(const QString &icon);
    Q_INVOKABLE void setColour(const QString &colour);
    Q_INVOKABLE void setRestBetweenBlocks(int seconds);

    // A block as {rounds, restBetweenRoundsSec, stepCount, problems}; empty if it does not exist.
    Q_INVOKABLE QVariantMap block(int block);
    Q_INVOKABLE void setRounds(int block, int rounds);
    Q_INVOKABLE void setRestBetweenRounds(int block, int seconds);

    // A step as its row (see WorkoutView::rows()); empty if it does not exist.
    Q_INVOKABLE QVariantMap step(int block, int step);
    // type: "work", "rest", "warmup" or "cooldown".
    Q_INVOKABLE void setStepType(int block, int step, const QString &type);
    Q_INVOKABLE void setStepTimed(int block, int step, bool timed);
    Q_INVOKABLE void setStepDuration(int block, int step, int seconds);
    Q_INVOKABLE void setStepReps(int block, int step, int reps);
    // kind is "none", "total" (the repetitions in a total time) or "per-rep" (a time per
    // repetition); paceMs is the value in milliseconds, 0 for the default; it is clamped to the
    // limits of the core (R-WRK-14).
    Q_INVOKABLE void setStepPace(int block, int step, const QString &kind, int paceMs);
    Q_INVOKABLE void setStepStartOnTap(int block, int step, bool onTap);
    Q_INVOKABLE void setStepExercise(int block, int step, const QString &exerciseKey);

    Q_INVOKABLE QVariantMap addStep(int block);
    Q_INVOKABLE QVariantMap addBlock();
    // Collapses or expands one block, or starts and ends the arranging: starting collapses every
    // block, ending expands every block. Views only (R-WRK-6).
    Q_INVOKABLE void toggleBlockCollapsed(int block);
    Q_INVOKABLE void setArranging(bool arranging);
    // One place up (delta -1) or down (+1); at the end of a block the step goes into the next one.
    Q_INVOKABLE QVariantMap moveStepAcross(int block, int step, int delta);
    Q_INVOKABLE bool removeStep(int block, int step);
    Q_INVOKABLE bool removeBlock(int block);
    Q_INVOKABLE QVariantMap duplicateStep(int block, int step);
    Q_INVOKABLE QVariantMap duplicateBlock(int block);
    Q_INVOKABLE QVariantMap moveStep(int block, int step, int delta);
    Q_INVOKABLE QVariantMap moveBlock(int block, int delta);
    Q_INVOKABLE QVariantMap moveStepToBlock(int block, int step, int targetBlock);

    // Saves the draft as a new workout or over the one being edited: {status, id, problems}.
    // status is "ok", "invalid" (see problems), "not-found" or "failed". The draft stays as it is.
    Q_INVOKABLE QVariantMap save();

signals:
    void changed();

private:
    QVariantMap position(const WorkoutDraft::Position &p) const;
    Step *stepAt(int block, int step);
    void update();

    Services *services_;
    Workout workout_;
    WorkoutView::Lookups lookups_;
    bool ready_ = false;
    bool arranging_ = false;
    // Whether each block is shown as one line, by position: a view, kept with the draft for the
    // open editor and never saved.
    QList<bool> collapsed_;
    QVariantMap header_;
    QVariantList problems_;
    QVariantMap total_;
    VariantRowsModel rows_;
};

#endif
