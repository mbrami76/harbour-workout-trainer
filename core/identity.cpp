// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "identity.h"

#include <QRegularExpression>

#include "workoutlimits.h"

QString Identity::nameKey(const QString &name)
{
    return name.trimmed().toLower();
}

namespace {

// Sort key of a name: the copy suffix becomes a fixed-width number, so that the text order of the keys is the
// numeric order of the copies.
QString sortKey(const QString &name)
{
    static const QRegularExpression suffix(QStringLiteral("^(.*) \\(copy(?: ([0-9]{1,9}))?\\)$"),
                                           QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m = suffix.match(name);
    if (!m.hasMatch())
        return name.toLower();
    const int number = m.captured(2).isEmpty() ? 1 : m.captured(2).toInt();
    return m.captured(1).toLower() + QStringLiteral(" (copy ") + QString::number(number).rightJustified(10, QLatin1Char('0'))
           + QLatin1Char(')');
}

}

int Identity::compareNames(const QString &a, const QString &b)
{
    return QString::compare(sortKey(a), sortKey(b));
}

bool Identity::isSame(const QString &nameA, const QString &iconA, const QString &colourA,
                      const QString &nameB, const QString &iconB, const QString &colourB)
{
    return nameKey(nameA) == nameKey(nameB) && iconA == iconB && colourA == colourB;
}

bool Identity::isValidIconKey(const QString &icon)
{
    if (icon.isEmpty())
        return true;
    if (icon.length() > Limits::maxIconKeyChars)
        return false;
    for (const QChar c : icon) {
        const ushort u = c.unicode();
        const bool ok = (u >= 'a' && u <= 'z') || (u >= '0' && u <= '9') || u == '-';
        if (!ok)
            return false;
    }
    return true;
}

bool Identity::isTaken(Database *database, Table table, const QString &name, const QString &icon,
                       const QString &colour, int excludeId, bool *taken)
{
    const char *tableName = table == Table::Workout ? "workout" : (table == Table::Metronome ? "metronome" : "challenge");
    // A rhythm has no icon and no colour: both read as the empty key, so its name alone decides.
    const bool plain = table == Table::Metronome;
    const QString sql = QStringLiteral("SELECT id, name, %2, %3 FROM %1")
                            .arg(QLatin1String(tableName), plain ? QStringLiteral("''") : QStringLiteral("icon"),
                                 plain ? QStringLiteral("''") : QStringLiteral("colour"));
    QSqlQuery q;
    if (!database->run(sql, QVariantList(), &q))
        return false;
    *taken = false;
    while (q.next()) {
        if (q.value(0).toInt() == excludeId)
            continue;
        if (isSame(name, icon, colour, q.value(1).toString(), q.value(2).toString(), q.value(3).toString())) {
            *taken = true;
            break;
        }
    }
    return true;
}

QString Identity::freeName(Database *database, Table table, const QString &name, const QString &icon,
                           const QString &colour, const QString &word, int maxChars, int excludeId)
{
    for (int n = 0;; ++n) {
        QString candidate = name;
        if (n > 0) {
            const QString suffix = n == 1 ? QStringLiteral(" (%1)").arg(word)
                                          : QStringLiteral(" (%1 %2)").arg(word).arg(n);
            candidate = name.left(maxChars - suffix.length()) + suffix;
        }
        bool taken = false;
        if (!isTaken(database, table, candidate, icon, colour, excludeId, &taken) || !taken)
            return candidate;
    }
}
