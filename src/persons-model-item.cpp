/*
    Persons Model Item
    Represents one person in the model
    Copyright (C) 2012  Martin Klapetek <martin.klapetek@gmail.com>

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


#include "persons-model-item.h"
#include "persons-model-contact-item.h"

#include <Nepomuk2/Vocabulary/PIMO>
#include <Nepomuk2/Vocabulary/NCO>
#include <Nepomuk2/Resource>
#include <Nepomuk2/Variant>
#include <Soprano/Vocabulary/NAO>
#include <KDebug>

PersonsModelItem::PersonsModelItem(const QUrl &personUri)
{
    setData(personUri, PersonsModel::UriRole);
}

PersonsModelItem::PersonsModelItem(const Nepomuk2::Resource &person)
{
    setData(person.uri(), PersonsModel::UriRole);
    setContacts(person.property(Nepomuk2::Vocabulary::PIMO::groundingOccurrence()).toUrlList());
    kDebug() << "new person" << text() << rowCount();
}

QVariant PersonsModelItem::queryChildrenForRole(int role) const
{
    for (int i = 0; i < rowCount(); i++) {
        QVariant value = child(i)->data(role);
        if (!value.isNull()) {
            return value;
        }
    }
    return QVariant();
}

QVariantList PersonsModelItem::queryChildrenForRoleList(int role) const
{
    QVariantList ret;
    for (int i = 0; i < rowCount(); i++) {
        QVariant value = child(i)->data(role);
        if (!value.isNull()) {
            ret += value;
        }
    }
    return ret;
}

QVariant PersonsModelItem::data(int role) const
{
    switch(role) {
        case PersonsModel::NameRole:
        case Qt::DisplayRole: {
            QVariant value = queryChildrenForRole(Qt::DisplayRole);
            if (value.isNull()) {
                value = queryChildrenForRole(PersonsModel::ContactIdRole);
            }
            if (value.isNull()) {
                return QString("PIMO:Person - %1").arg(data(PersonsModel::UriRole).toString());
            } else {
                return value;
            }
        }
        case PersonsModel::StatusRole: //TODO: use a better algorithm for finding the actual status
        case PersonsModel::NickRole:
            return queryChildrenForRole(role);
        case PersonsModel::LabelRole:
        case PersonsModel::IMRole:
        case PersonsModel::PhoneRole:
        case PersonsModel::EmailRole:
        case PersonsModel::PhotoRole:
        case PersonsModel::ContactIdRole:
        case PersonsModel::ContactTypeRole:
            return queryChildrenForRoleList(role);
        case PersonsModel::ContactsCountRole:
            return rowCount();
        case PersonsModel::ResourceTypeRole:
            return PersonsModel::Person;
    }

    return QStandardItem::data(role);
}

void PersonsModelItem::removeContacts(const QList<QUrl> &contacts)
{
    kDebug() << "remove contacts" << contacts;
    for (int i = 0; i < rowCount(); ) {
        QStandardItem* item = child(i);
        if (contacts.contains(item->data(PersonsModel::UriRole).toUrl())) {
            removeRow(i);
        } else {
            ++i;
        }
    }
    emitDataChanged();
}

void PersonsModelItem::addContacts(const QList<QUrl> &_contacts)
{
    QList<QUrl> contacts(_contacts);
    //get existing child-contacts and remove them from the new ones
    QVariantList uris = queryChildrenForRoleList(PersonsModel::UriRole);
    foreach (const QVariant &uri, uris) {
        contacts.removeOne(uri.toUrl());
    }

    //query the model for the contacts, if they are present, then need to be just moved
    QList<QStandardItem*> toplevelContacts;
    foreach (const QUrl &uri, contacts) {
        QModelIndex contactIndex = qobject_cast<PersonsModel*>(model())->indexForUri(uri);
        if (contactIndex.isValid()) {
             toplevelContacts.append(qobject_cast<PersonsModel*>(model())->takeRow(contactIndex.row()));
        }
    }

    //append the moved contacts to this person and remove them from 'contacts'
    //so they are not added twice
    foreach (QStandardItem *contactItem, toplevelContacts) {
        PersonsModelContactItem *contact = dynamic_cast<PersonsModelContactItem*>(contactItem);
        appendRow(contact);
        contacts.removeOne(contact->uri());
    }

    kDebug() << "add contacts" << contacts;
    QList<PersonsModelContactItem*> rows;
    foreach (const QUrl &uri, contacts) {
        appendRow(new PersonsModelContactItem(Nepomuk2::Resource(uri)));
    }
    emitDataChanged();
}

void PersonsModelItem::setContacts(const QList<QUrl> &contacts)
{
    kDebug() << "set contacts" << contacts;
    if (contacts.isEmpty()) {
        //nothing to do here
        return;
    }

    if (hasChildren()) {
        QList<QUrl> toRemove;
        QVariantList uris = queryChildrenForRoleList(PersonsModel::UriRole);
        foreach (const QVariant &contact, uris) {
            if (!contacts.contains(contact.toUrl()))
                toRemove += contact.toUrl();
        }
        removeContacts(toRemove);
    }

    QList<QUrl> toAdd;
    foreach (const QUrl &contact, contacts) {
        toAdd += contact;
    }
    addContacts(toAdd);
    Q_ASSERT(hasChildren());
}
