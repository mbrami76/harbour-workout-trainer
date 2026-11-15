// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <QString>
#include <QStringList>

// The languages of the interface and of the starter library (R-I18N-1, R-I18N-4, R-I18N-5).
namespace Language {

// Language tags the app ships, English first. English is the source language of the interface.
QStringList shipped();
bool isShipped(const QString &tag);

// The shipped tag a stored or system value stands for, or an empty string: "it", "it_IT" and
// "IT-ch" all mean "it". Used to read the stored choice, so a damaged value reads as no choice.
QString shippedFrom(const QString &value);

// The language offered first at the first start: the system language when it is shipped,
// otherwise English (R-I18N-5).
QString preselected(const QString &systemTag);

// The tags offered for the language of an exercise: the shipped ones, then the tags already in
// the database that are not shipped, sorted (R-EXR-12).
QStringList exerciseChoices(const QStringList &tagsInUse);

// The tag a new exercise gets: the chosen language, or English before the choice (R-EXR-12).
QString newExerciseTag(const QString &chosen);

// File names of the starter data: the structure (exercises and rhythms) and the workouts do not
// depend on a language; each shipped language has a texts file, for example "texts.it.json".
QString structureFileName();
QString textsFileName(const QString &language);
QString workoutsFileName();

}

#endif
