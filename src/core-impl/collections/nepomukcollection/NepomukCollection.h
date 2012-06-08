/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef NEPOMUKCOLLECTION_H
#define NEPOMUKCOLLECTION_H

#include "core/collections/Collection.h"


class NepomukCollectionFactory : public Collections::CollectionFactory
{
    Q_OBJECT

    NepomukCollectionFactory() {}
    virtual ~NepomukCollectionFactory() {}

    virtual void init();

};



class NepomukCollection : public Collections::Collection
{
    Q_OBJECT

    NepomukCollection();
    virtual ~NepomukCollection();

    virtual Collections::QueryMaker * queryMaker();

    virtual bool isDirInCollection(const QString &path) { Q_UNUSED( path ); return false; }

    virtual QString uidUrlProtocol() const;

    // unsure if this is really needed.
    virtual QString collectionId() const = 0;

    virtual QString prettyName() const = 0;

    virtual KIcon icon() const = 0;

    // assuming this is needed when tracks need to moved from one collection to another
    // unsure if this is neeeded
    // detering it to later stages of development

    //virtual Collections::CollectionLocation* location();

    virtual bool isWritable() const;

public slots:
    virtual void collectionUpdated() { emit updated(); }

signals:
    // Nepomuk daemon handles all dynamic changes
    // unsure if we need a specific signal for changes to db

    //void remove();

    //void updated();
};



























#endif // NEPOMUKCOLLECTION_H
