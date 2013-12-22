/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  David Edmundson <davidedmundson@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef FAKECONTACTSOURCE_H
#define FAKECONTACTSOURCE_H

#include <basepersonsdatasource.h>
#include <allcontactsmonitor.h>

class FakeContactSource : public KPeople::BasePersonsDataSource
{
public:
    FakeContactSource(QObject* parent, const QVariantList& args = QVariantList());
    void changeContact1();
protected:
    virtual KPeople::AllContactsMonitor* createAllContactsMonitor();

private:
};

class FakeAllContactsMonitor : public KPeople::AllContactsMonitor
{
    Q_OBJECT
public:
    explicit FakeAllContactsMonitor();
    void changeContact1();
    virtual KABC::Addressee::Map contacts();
};

#endif // FAKECONTACTSOURCE_H
