/*
 * Copyright 2013  David Edmundson <davidedmundson@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "global.h"

#include "personmanager.h"

#include <KIconLoader>

//these namespace members expose the useful bits of PersonManager
//global.h should be included from every exported header file so namespace members are always visible

QString KPeople::mergeContacts(const QStringList &ids)
{
    return PersonManager::instance()->mergeContacts(ids);
}

bool KPeople::unmergeContact(const QString &id)
{
    return PersonManager::instance()->unmergeContact(id);
}

QPixmap KPeople::iconForPresenceString(const QString &presenceName)
{
    if (presenceName == QLatin1String("available")) {
        return KIconLoader::global()->loadIcon("user-online", KIconLoader::MainToolbar, KIconLoader::SizeSmallMedium);
    }

    if (presenceName == QLatin1String("away")) {
        return KIconLoader::global()->loadIcon("user-away", KIconLoader::MainToolbar, KIconLoader::SizeSmallMedium);
    }

    if (presenceName == QLatin1String("busy") || presenceName == QLatin1String("dnd")) {
        return KIconLoader::global()->loadIcon("user-busy", KIconLoader::MainToolbar, KIconLoader::SizeSmallMedium);
    }

    if (presenceName == QLatin1String("xa")) {
        return KIconLoader::global()->loadIcon("user-away-extended", KIconLoader::MainToolbar, KIconLoader::SizeSmallMedium);
    }

    if (presenceName == QLatin1String("hidden")) {
        return KIconLoader::global()->loadIcon("user-invisible", KIconLoader::MainToolbar, KIconLoader::SizeSmallMedium);
    }

    return KIconLoader::global()->loadIcon("user-offline", KIconLoader::MainToolbar, KIconLoader::SizeSmallMedium);
}
