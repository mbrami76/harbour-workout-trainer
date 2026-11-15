// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "variantrowsmodel.h"

int VariantRowsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : rows_.size();
}

QVariant VariantRowsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rows_.size() || role != Qt::UserRole)
        return QVariant();
    return rows_.at(index.row());
}

QHash<int, QByteArray> VariantRowsModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names.insert(Qt::UserRole, "row");
    return names;
}

void VariantRowsModel::setRows(const QVariantList &rows)
{
    const int oldCount = rows_.size();
    int prefix = 0;
    while (prefix < oldCount && prefix < rows.size() && rows_.at(prefix) == rows.at(prefix))
        ++prefix;
    int suffix = 0;
    while (suffix < oldCount - prefix && suffix < rows.size() - prefix
           && rows_.at(oldCount - 1 - suffix) == rows.at(rows.size() - 1 - suffix))
        ++suffix;

    const int removed = oldCount - prefix - suffix;
    const int added = rows.size() - prefix - suffix;
    // A change of equal size is reported as changed rows; a different size as a removal and an
    // insertion of the rows in the middle.
    if (removed == added) {
        rows_ = rows;
        if (removed > 0)
            emit dataChanged(index(prefix), index(prefix + removed - 1));
        return;
    }
    if (removed > 0) {
        beginRemoveRows(QModelIndex(), prefix, prefix + removed - 1);
        for (int i = 0; i < removed; ++i)
            rows_.removeAt(prefix);
        endRemoveRows();
    }
    if (added > 0) {
        beginInsertRows(QModelIndex(), prefix, prefix + added - 1);
        for (int i = 0; i < added; ++i)
            rows_.insert(prefix + i, rows.at(prefix + i));
        endInsertRows();
    }
    emit countChanged();
}
