/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2013  David Edmundson <davidedmundson@kde.org>
 * Copyright 2013  Martin Klapetek <mklapetek@kde.org>
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

#include "personmanager.h"

// #include "personmanager.moc"
#include <QVariant>
#include <QDebug>
#include <QSqlError>
#include <QDBusConnection>
#include <QDBusMessage>
#include <KStandardDirs>

class Transaction
{
public:
    Transaction(const QSqlDatabase &db);
    void cancel();
    ~Transaction();
private:
    QSqlDatabase m_db;
    bool m_cancelled;
};

Transaction::Transaction(const QSqlDatabase& db) :
    m_db(db),
    m_cancelled(false)
{
    m_db.exec("BEGIN TRANSACTION");
}

void Transaction::cancel()
{
    m_db.exec("ROLLBACK");
    m_cancelled = true;
}

Transaction::~Transaction()
{
    if (!m_cancelled) {
        m_db.exec("END TRANSACTION");
    }
}

PersonManager::PersonManager(QObject* parent):
    QObject(parent),
    m_db(QSqlDatabase::addDatabase("QSQLITE3"))
{
    const QString personDbFilePath = KGlobal::dirs()->locateLocal("data","kpeople/persondb");
    m_db.setDatabaseName(personDbFilePath);
    m_db.open();
    m_db.exec("CREATE TABLE IF NOT EXISTS persons (contactID VARCHAR UNIQUE NOT NULL, personID INT NOT NULL)");
    m_db.exec("CREATE INDEX IF NOT EXISTS contactIdIndex ON persons (contactId)");
    m_db.exec("CREATE INDEX IF NOT EXISTS personIdIndex ON persons (personId)");

    QDBusConnection::sessionBus().connect(QString(), QString("/KPeople"), "org.kde.KPeople", "ContactAddedToPerson", this, SIGNAL(contactAddedToPerson(const QString&, const QString&)));
    QDBusConnection::sessionBus().connect(QString(), QString("/KPeople"), "org.kde.KPeople", "ContactRemovedFromPerson", this, SIGNAL(contactRemovedFromPerson(const QString&)));
}

PersonManager::~PersonManager()
{

}

QMultiHash< QString, QString > PersonManager::allPersons() const
{
    QMultiHash<QString /*PersonID*/, QString /*ContactID*/> contactMapping;

    QSqlQuery query = m_db.exec("SELECT personID, contactID FROM persons");
    while (query.next()) {
        const QString personId = "kpeople://" + query.value(0).toString(); // we store as ints internally, convert it to a string here
        const QString contactID = query.value(1).toString();
        contactMapping.insertMulti(personId, contactID);
    }
    return contactMapping;
}

QStringList PersonManager::contactsForPersonId(const QString& personId) const
{
    if (!personId.startsWith("kpeople://")) {
        return QStringList();
    }

    QStringList contactIds;
    //TODO port to the proper qsql method for args
    QSqlQuery query(m_db);
    query.prepare("SELECT contactID FROM persons WHERE personId = ?");
    query.bindValue(0, personId.mid(strlen("kpeople://")));
    query.exec();

    while (query.next()) {
        contactIds << query.value(0).toString();
    }
    return contactIds;
}

QString PersonManager::personIdForContact(const QString& contactId) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT personId FROM persons WHERE contactId = ?");
    query.bindValue(0, contactId);
    query.exec();
    if (query.next()) {
        return "kpeople://"+query.value(0).toString();
    }
    return contactId;
}


QString PersonManager::mergeContacts(const QStringList& ids)
{
    //TODO, ask mck182 about the logic here
    //for now assume all input is unmerged contacts

    int personId = 0;
    
    QStringList metacontacts;
    QStringList contacts;

    //find personID to use
    QSqlQuery query = m_db.exec(QString("SELECT MAX(personID) FROM persons"));
    if (query.next()) {
        personId = query.value(0).toInt();
    }

    personId++;

    Q_FOREACH (const QString &id, ids) {
        if (id.startsWith(QLatin1String("kpeople://"))) {
            metacontacts << id;
        } else {
            contacts << id;
        }
    }

    if (contacts.count() + metacontacts.count() < 2) {
        return QString();
    }

    QString personIdString;
    personIdString = metacontacts.count() == 0 ? "kpeople://" + QString::number(personId)
                                               : metacontacts.first();

    Transaction t(m_db);

    if (metacontacts.count() > 1 && contacts.count() == 0) {

        Q_FOREACH (const QString &id, metacontacts) {
            if (id != personIdString) {
                contacts << contactsForPersonId(id);
            }
        }

        Q_FOREACH (const QString &id, contacts) {
            QSqlQuery updateQuery(m_db);
            updateQuery.prepare("UPDATE persons SET personID = ? WHERE contactID = ?");
            updateQuery.bindValue(0, personIdString.mid(strlen("kpeople://")));
            updateQuery.bindValue(1, id);
            updateQuery.exec();

            QDBusMessage message = QDBusMessage::createSignal(QLatin1String("/KPeople"),
                                                              QLatin1String("org.kde.KPeople"),
                                                              QLatin1String("ContactRemovedFromPerson"));

            message.setArguments(QVariantList() << id);
            QDBusConnection::sessionBus().send(message);

            message = QDBusMessage::createSignal(QLatin1String("/KPeople"),
                                                              QLatin1String("org.kde.KPeople"),
                                                              QLatin1String("ContactAddedToPerson"));

            message.setArguments(QVariantList() << id << personIdString);
            QDBusConnection::sessionBus().send(message);
        }

        return personIdString;
    }

    Q_FOREACH (const QString &id, contacts) {
        QSqlQuery insertQuery(m_db);
        insertQuery.prepare("INSERT INTO persons VALUES (?, ?)");
        insertQuery.bindValue(0, id);
        insertQuery.bindValue(1, personIdString.mid(strlen("kpeople://"))); //strip kpeople://
        insertQuery.exec();

        //FUTURE OPTIMISATION - this would be best as one signal, but arguments become complex
        QDBusMessage message = QDBusMessage::createSignal(QLatin1String("/KPeople"),
                                                    QLatin1String("org.kde.KPeople"),
                                                    QLatin1String("ContactAddedToPerson"));

        message.setArguments(QVariantList() << id << personIdString);
        QDBusConnection::sessionBus().send(message);
    }

    return personIdString;
}

bool PersonManager::unmergeContact(const QString &id)
{
    //remove rows from DB
    if (id.startsWith("kpeople://")) {
        QSqlQuery query(m_db);

        const QStringList contactIds = contactsForPersonId(id);
        query.prepare("DELETE FROM persons WHERE personId = ?");
        query.bindValue(0, id.mid(strlen("kpeople://")));
        query.exec();

        Q_FOREACH(const QString &contactId, contactIds) {
            //FUTURE OPTIMISATION - this would be best as one signal, but arguments become complex
            QDBusMessage message = QDBusMessage::createSignal(QLatin1String("/KPeople"),
                                                      QLatin1String("org.kde.KPeople"),
                                                      QLatin1String("ContactRemovedFromPerson"));

            message.setArguments(QVariantList() << contactId);
            QDBusConnection::sessionBus().send(message);
        }
    } else {
        QSqlQuery query(m_db);
        query.prepare("DELETE FROM persons WHERE contactId = ?");
        query.bindValue(0, id);
        query.exec();
        //emit signal(dbus)
        Q_EMIT contactRemovedFromPerson(id);
    }

    //TODO return if removing rows worked
    return true;
}

PersonManager* PersonManager::instance()
{
    static PersonManager* s_instance = 0;
    if (!s_instance) {
        s_instance = new PersonManager();
    }
    return s_instance;
}
