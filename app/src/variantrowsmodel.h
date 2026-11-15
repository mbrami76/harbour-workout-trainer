// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef VARIANTROWSMODEL_H
#define VARIANTROWSMODEL_H

#include <QAbstractListModel>
#include <QVariantList>

// A list of maps for a ListView. Each row is the map of the role "row". setRows() compares the new
// list with the old one and reports only the rows that differ in the middle, so a view keeps its
// scroll position when one row changes or a few rows are added or removed.
class VariantRowsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit VariantRowsModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int count() const { return rows_.size(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(const QVariantList &rows);

signals:
    void countChanged();

private:
    QVariantList rows_;
};

#endif
