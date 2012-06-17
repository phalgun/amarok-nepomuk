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

#include "NepomukCollection.h"
#include "NepomukQueryMaker.h"
#include "core/collections/QueryMaker.h"

#include <Nepomuk/ResourceManager>
#include <KIcon>
#include <QString>

NepomukCollection::NepomukCollection()
{

}

NepomukCollection::~NepomukCollection()
{

}

Collections::QueryMaker * NepomukCollection::queryMaker()
{
    return new NepomukQueryMaker(this);
}

QString NepomukCollection::uidUrlProtocol() const
{
    static const QString uid("amarok-nepomuk");
    return uid;
}

QString NepomukCollection::collectionId() const
{
    return QString("%1://").arg(uidUrlProtocol());
}

QString NepomukCollection::prettyName() const
{
    return QString("nepomuk");
}

KIcon NepomukCollection::icon() const
{
    return KIcon("nepomuk");
}

bool NepomukCollection::isWritable() const
{
    // Nepomuk if initialized is always writable
    // A check for nepomuk initialized will suffice

    return Nepomuk::ResourceManager::instance()->initialized();
}
