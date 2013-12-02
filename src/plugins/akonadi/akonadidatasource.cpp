/*
 * <one line to give the library's name and an idea of what it does.>
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

#include "akonadidatasource.h"

#include <Akonadi/Item>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>

#include <KABC/Addressee>

#include <KPluginFactory>
#include <KPluginLoader>

#include <QDebug>

using namespace Akonadi;

class AkonadiAllContacts : public AllContactsMonitor
{
    Q_OBJECT
public:
    AkonadiAllContacts();
    ~AkonadiAllContacts();
    virtual KABC::Addressee::Map contacts();
private Q_SLOTS:
    void onCollectionsFetched(KJob* job);
    void onItemsFetched(KJob* job);
    void onItemAdded(const Akonadi::Item &item);
    void onItemChanged(const Akonadi::Item &item);
    void onItemRemoved(const Akonadi::Item &item);
private:
    Akonadi::Monitor *m_monitor;
    KABC::Addressee::Map m_contacts;
};

AkonadiAllContacts::AkonadiAllContacts():
    m_monitor(new Akonadi::Monitor(this))
{
    connect(m_monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)), SLOT(onItemAdded(Akonadi::Item)));
    connect(m_monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), SLOT(onItemChanged(Akonadi::Item)));
    connect(m_monitor, SIGNAL(itemRemoved(Akonadi::Item)), SLOT(onItemRemoved(Akonadi::Item)));

    m_monitor->setMimeTypeMonitored("text/directory");
    m_monitor->itemFetchScope().fetchFullPayload();

    CollectionFetchJob *fetchJob = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
    fetchJob->fetchScope().setContentMimeTypes( QStringList() << "text/directory" );
    connect(fetchJob, SIGNAL(finished(KJob*)), SLOT(onCollectionsFetched(KJob*)));
}

AkonadiAllContacts::~AkonadiAllContacts()
{
}

KABC::Addressee::Map AkonadiAllContacts::contacts()
{
    return m_contacts;
}

void AkonadiAllContacts::onItemAdded(const Item& item)
{
    if(!item.hasPayload<KABC::Addressee>()) {
        return;
    }
    const QString id = item.url().prettyUrl();
    const KABC::Addressee contact = item.payload<KABC::Addressee>();
    m_contacts[id] = contact;
    Q_EMIT contactAdded(item.url().prettyUrl(), contact);
}

void AkonadiAllContacts::onItemChanged(const Item& item)
{
    if(!item.hasPayload<KABC::Addressee>()) {
        return;
    }
    const QString id = item.url().prettyUrl();
    const KABC::Addressee contact = item.payload<KABC::Addressee>();
    m_contacts[id] = contact;
    Q_EMIT contactChanged(item.url().prettyUrl(), contact);
}

void AkonadiAllContacts::onItemRemoved(const Item& item)
{
    if(!item.hasPayload<KABC::Addressee>()) {
        return;
    }
    const QString id = item.url().prettyUrl();
    m_contacts.remove(id);
    Q_EMIT contactRemoved(id);
}

//or we could add items as we go along...
void AkonadiAllContacts::onItemsFetched(KJob *job)
{
    ItemFetchJob *itemFetchJob = qobject_cast<ItemFetchJob*>(job);
    foreach (const Item &item, itemFetchJob->items()) {
        onItemAdded(item);
    }
}

void AkonadiAllContacts::onCollectionsFetched(KJob* job)
{
    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>(job);
    QList<Collection> contactCollections;
    foreach (const Collection &collection, fetchJob->collections()) {
        if (collection.contentMimeTypes().contains( KABC::Addressee::mimeType() ) ) {
            ItemFetchJob *itemFetchJob = new ItemFetchJob(collection);
            itemFetchJob->fetchScope().fetchFullPayload();
            connect(itemFetchJob, SIGNAL(finished(KJob*)), SLOT(onItemsFetched(KJob*)));
        }
    }
}




class AkonadiContact: public ContactMonitor
{
    Q_OBJECT
public:
    AkonadiContact(const QString &contactId);
private Q_SLOTS:
    void onContactFetched(KJob*);
    void onContactChanged(const Akonadi::Item &);
private:
    Akonadi::Monitor *m_monitor;
};

AkonadiContact::AkonadiContact(const QString& contactId):
    ContactMonitor(contactId),
    m_monitor(new Akonadi::Monitor(this))
{
    //optimisation, base class could copy across from the model if the model exists
    //then we should check if contact is already set to something and avoid the initial fetch

    //FIXME This is a bug in the sending code. See Fixme in PersonData
    if (contactId.startsWith("akonadi://")) {

        Item item = Item::fromUrl(QUrl(contactId));
        ItemFetchJob* itemFetchJob = new ItemFetchJob(item);
        itemFetchJob->fetchScope().fetchFullPayload();
        connect(itemFetchJob, SIGNAL(finished(KJob*)), SLOT(onContactFetched(KJob*)));

        itemFetchJob->exec();//HACK because something isn't working FIXME FIXME FIXME!!!!

        //monitor here too
        m_monitor->setItemMonitored(item, true);
        connect(m_monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), SLOT(onContactChanged(Akonadi::Item)));
        m_monitor->itemFetchScope().fetchFullPayload();
    }
}

void AkonadiContact::onContactFetched(KJob *job)
{
    ItemFetchJob* fetchJob = qobject_cast<ItemFetchJob*>(job);
    if (fetchJob->items().count() && fetchJob->items().first().hasPayload<KABC::Addressee>()) {
        setContact(fetchJob->items().first().payload<KABC::Addressee>());
    }
}

void AkonadiContact::onContactChanged(const Item &item)
{
    if(!item.hasPayload<KABC::Addressee>()) {
        return;
    }
    setContact(item.payload<KABC::Addressee>());
}


AkonadiDataSource::AkonadiDataSource(QObject *parent, const QVariantList &args):
    BasePersonsDataSource(parent)
{
    Q_UNUSED(args);

}

AkonadiDataSource::~AkonadiDataSource()
{

}

AllContactsMonitor* AkonadiDataSource::createAllContactsMonitor()
{
    return new AkonadiAllContacts();
}

ContactMonitor* AkonadiDataSource::createContactMonitor(const QString& contactId)
{
    return new AkonadiContact(contactId);
}

K_PLUGIN_FACTORY( AkonadiDataSourceFactory, registerPlugin<AkonadiDataSource>(); )
K_EXPORT_PLUGIN( AkonadiDataSourceFactory("akonadi_kpeople_plugin") )

#include "akonadidatasource.moc"