// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "icons.h"

QStringList Icons::workoutStyleKeys()
{
    return QStringList() << QString();
}

QStringList Icons::workoutKeys()
{
    static const char *const names[] = {
        "barbell", "dumbbells", "kettlebell", "medicine-ball", "cable-machine", "bench", "bicycle",
        "treadmill", "jump-rope", "shoe", "stopwatch", "lungs", "heart-pulse", "yoga", "glove", "abs",
        "tiger", "bear", "gorilla", "wave", "volcano", "fire", "lightning", "rocket", "anvil",
        "target", "balance", "summit", "speed", "flexibility", "progress", "recovery", "shock",
        "spring", "scale"
    };
    QStringList keys;
    keys << QString();
    for (const char *name : names)
        keys << QString::fromLatin1(name);
    return keys;
}

QStringList Icons::exerciseKeys()
{
    static const char *const names[] = {
        "arm-circles", "bicycle-crunch", "bird-dog", "calf-raise", "child-pose", "crunch", "dead-bug",
        "donkey-kick", "glute-bridge", "good-morning", "hamstring-stretch", "heel-walk", "high-knees",
        "hollow-hold", "jumping-jack", "knee-push-up", "leg-raise", "lunge", "mountain-climber",
        "pistol-squat", "plank", "prone-sweep", "prone-y-raise", "push-up", "russian-twist",
        "self-resisted-curl", "shoulder-tap", "side-leg-raise", "side-lunge", "side-plank",
        "single-leg-hinge", "sit-up", "squat", "standing-quad-stretch", "superman", "v-up", "wall-angel",
        "wall-handstand-hold", "wall-push-up", "wall-sit", "wrist-rock",
        "burpee", "bear-crawl", "pike-push-up", "knee-to-elbow",
        "oblique-crunch", "torso-rotation", "triceps-stretch", "cross-body-stretch",
        "fire-hydrant", "clamshell", "cat-cow", "hip-circles",
        "thoracic-rotation", "worlds-greatest-stretch", "leg-swings", "hip-flexor-stretch",
        "cobra", "figure-four", "seated-forward-fold", "chest-stretch",
        "flutter-kick", "reverse-crunch", "butt-kicks", "neck-rolls",
        "ankle-circles", "frog-jump"
    };
    QStringList keys;
    keys << QString();
    for (const char *name : names)
        keys << QString::fromLatin1(name);
    return keys;
}
