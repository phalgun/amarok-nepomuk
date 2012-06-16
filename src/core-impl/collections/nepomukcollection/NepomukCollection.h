/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#include <QString>
#include <QStringList>
#include <QHash>

#include <KIcon>

class NepomukCollection : public Collections::Collection
{

public:
    NepomukCollection();
    virtual ~NepomukCollection();

    virtual Collections::QueryMaker* queryMaker();

    virtual bool isDirInCollection(const QString &path) { Q_UNUSED( path ); return false; }

    virtual QString uidUrlProtocol() const;

    // unsure if this is really needed.
    virtual QString collectionId() const = 0;

    virtual QString prettyName() const = 0;

    virtual KIcon icon() const = 0;

    virtual bool isWritable() const;

};

#endif // NEPOMUKCOLLECTION_H
