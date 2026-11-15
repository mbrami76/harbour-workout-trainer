// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "musclegroups.h"

namespace {

struct Entry {
    MuscleGroup group;
    const char *key;
    // For a retired group: the groups that replace it.
    const MuscleGroup *replacements;
    int replacementCount;

    bool retired() const { return replacementCount > 0; }
};

const MuscleGroup chestReplacements[] = { MuscleGroup::UpperChest, MuscleGroup::LowerChest };
const MuscleGroup backReplacements[] = { MuscleGroup::Trapezius, MuscleGroup::Lats, MuscleGroup::MidBack };
const MuscleGroup armsReplacements[] = { MuscleGroup::Biceps, MuscleGroup::Triceps, MuscleGroup::Forearms };

#define ACTIVE(group, key) { MuscleGroup::group, key, nullptr, 0 }
#define RETIRED(group, key, list) { MuscleGroup::group, key, list, int(sizeof(list) / sizeof(list[0])) }

// The table in position order. Positions are stored and never change; a new group is appended.
const Entry table[] = {
    RETIRED(Chest, "chest", chestReplacements),
    RETIRED(Back, "back", backReplacements),
    ACTIVE(Shoulders, "shoulders"),
    RETIRED(Arms, "arms", armsReplacements),
    ACTIVE(Core, "core"),
    ACTIVE(Glutes, "glutes"),
    ACTIVE(Quadriceps, "quadriceps"),
    ACTIVE(Hamstrings, "hamstrings"),
    ACTIVE(Calves, "calves"),
    ACTIVE(LowerAbdomen, "lower-abdomen"),
    ACTIVE(LowerBack, "lower-back"),
    ACTIVE(UpperChest, "upper-chest"),
    ACTIVE(LowerChest, "lower-chest"),
    ACTIVE(Obliques, "obliques"),
    ACTIVE(Trapezius, "trapezius"),
    ACTIVE(Lats, "lats"),
    ACTIVE(MidBack, "mid-back"),
    ACTIVE(Biceps, "biceps"),
    ACTIVE(Triceps, "triceps"),
    ACTIVE(Forearms, "forearms"),
    ACTIVE(Shins, "shins")
};

#undef ACTIVE
#undef RETIRED

const int tableSize = int(sizeof(table) / sizeof(table[0]));

// The stored integer is signed 64-bit: positions above 62 cannot be used.
static_assert(sizeof(table) / sizeof(table[0]) <= 63, "too many muscle groups for the stored set");

const Entry *find(MuscleGroup group)
{
    for (int i = 0; i < tableSize; ++i) {
        if (table[i].group == group)
            return &table[i];
    }
    return nullptr;
}

const Entry *find(const QString &key)
{
    for (int i = 0; i < tableSize; ++i) {
        if (key == QLatin1String(table[i].key))
            return &table[i];
    }
    return nullptr;
}

}

int muscleGroupCount()
{
    return tableSize;
}

MuscleSet allMuscles()
{
    MuscleSet all = 0;
    for (int i = 0; i < tableSize; ++i)
        all |= muscleBit(table[i].group);
    return all;
}

MuscleSet activeMuscles()
{
    MuscleSet active = 0;
    for (int i = 0; i < tableSize; ++i) {
        if (!table[i].retired())
            active |= muscleBit(table[i].group);
    }
    return active;
}

QStringList MuscleKeys::keys()
{
    QStringList list;
    for (int i = 0; i < tableSize; ++i) {
        if (!table[i].retired())
            list.append(QString::fromLatin1(table[i].key));
    }
    return list;
}

QString MuscleKeys::keyOf(MuscleGroup group)
{
    const Entry *entry = find(group);
    return entry ? QString::fromLatin1(entry->key) : QString();
}

bool MuscleKeys::fromKey(const QString &key, MuscleGroup *group)
{
    const Entry *entry = find(key);
    if (!entry)
        return false;
    *group = entry->group;
    return true;
}

bool MuscleKeys::isRetired(MuscleGroup group)
{
    const Entry *entry = find(group);
    return entry && entry->retired();
}

QStringList MuscleKeys::keysOf(MuscleSet set)
{
    QStringList list;
    for (int i = 0; i < tableSize; ++i) {
        if (!table[i].retired() && (set & muscleBit(table[i].group)))
            list.append(QString::fromLatin1(table[i].key));
    }
    return list;
}

MuscleSet MuscleKeys::convertRetired(MuscleSet set)
{
    for (int i = 0; i < tableSize; ++i) {
        if (!table[i].retired() || !(set & muscleBit(table[i].group)))
            continue;
        set &= ~muscleBit(table[i].group);
        for (int r = 0; r < table[i].replacementCount; ++r)
            set |= muscleBit(table[i].replacements[r]);
    }
    return set;
}

QStringList MuscleKeys::replacementKeys(const QString &key)
{
    QStringList list;
    const Entry *entry = find(key);
    if (!entry)
        return list;
    if (!entry->retired()) {
        list.append(key);
        return list;
    }
    for (int r = 0; r < entry->replacementCount; ++r)
        list.append(keyOf(entry->replacements[r]));
    return list;
}
