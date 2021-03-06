/*
    Copyright (C) 2013  Martin Klapetek <mklapetek@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef PERSON_ACTIONS_H
#define PERSON_ACTIONS_H

#include <QAbstractListModel>

#include "kpeople_export.h"

class QAction;

namespace KPeople
{
class PersonActionsPrivate;

class KPEOPLE_EXPORT PersonActionsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY personChanged)
public:
    PersonActionsModel(QObject *parent = 0);
    virtual ~PersonActionsModel();

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    Q_INVOKABLE void setPerson(QAbstractItemModel *model, int row);
    void setPerson(const QPersistentModelIndex &index);
    QList<QAction*> actions() const;

    Q_INVOKABLE void triggerAction(int row) const;

Q_SIGNALS:
    void personChanged();

private:
    Q_DECLARE_PRIVATE(PersonActions);
    PersonActionsPrivate * const d_ptr;
};
}

#endif // PERSON_ACTIONS_H
