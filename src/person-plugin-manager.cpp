/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  David Edmundson <davidedmundson@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "person-plugin-manager.h"

#include <QAction>
#include <KService>
#include <KServiceTypeTrader>
#include <kdemacros.h>

#include "abstract-person-plugin.h"

#include "plugins/im-plugin.h"
#include "plugins/email-plugin.h"
#include "base-persons-data-source.h"

class PersonPluginManagerPrivate
{
public:
    PersonPluginManagerPrivate();
    ~PersonPluginManagerPrivate();
    QList<AbstractPersonPlugin*> plugins;
    BasePersonsDataSource* presencePlugin;
};

K_GLOBAL_STATIC(PersonPluginManagerPrivate, s_instance);

PersonPluginManagerPrivate::PersonPluginManagerPrivate()
{
    plugins << new IMPlugin(0);
    plugins << new EmailPlugin(0);

    KService::Ptr imService = KServiceTypeTrader::self()->preferredService("KPeople/ModelPlugin");
    if (imService.isNull()) {
        presencePlugin = new BasePersonsDataSource(0);
    } else {
        presencePlugin = imService->createInstance<BasePersonsDataSource>(0);
    }
}

PersonPluginManagerPrivate::~PersonPluginManagerPrivate()
{
    qDeleteAll(plugins);
    presencePlugin->deleteLater();
}

QList<QAction*> PersonPluginManager::actionsForPerson(PersonData* person, QObject* parent)
{
    QList<QAction*> actions;
    Q_FOREACH(AbstractPersonPlugin *plugin, s_instance->plugins) {
        actions << plugin->actionsForPerson(person, parent);
    }
    return actions;
}

BasePersonsDataSource* PersonPluginManager::presencePlugin()
{
    return s_instance->presencePlugin;
}
