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

#include "NepomukQueryMaker.h"
#include "core/collections/Collection.h"
#include "core/meta/Meta.h"

// Nepomuk includes
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Vocabulary/NMM>

// Soprano includes
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>

NepomukQueryMaker::NepomukQueryMaker(NepomukCollection *collection)
    : QueryMaker()
    , m_collection()
{

}

NepomukQueryMaker::~NepomukQueryMaker()
{

}

void
NepomukQueryMaker::abortQuery()
{

}

void
NepomukQueryMaker::run()
{

}

Collections::QueryMaker*
NepomukQueryMaker::setQueryType(QueryType type)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addReturnValue(qint64 value)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addReturnFunction(ReturnFunction function, qint64 value)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::orderBy(qint64 value, bool descending)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addMatch(const Meta::TrackPtr &track)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addMatch(const Meta::ArtistPtr &artist)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addMatch(const Meta::AlbumPtr &album)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addMatch(const Meta::ComposerPtr &composer)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addMatch(const Meta::GenrePtr &genre)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addMatch(const Meta::YearPtr &year)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addMatch(const Meta::LabelPtr &label)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addFilter(qint64 value, const QString &filter, bool matchBegin, bool matchEnd)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::excludeFilter(qint64 value, const QString &filter, bool matchBegin, bool matchEnd)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::addNumberFilter(qint64 value, qint64 filter, NumberComparison compare)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::excludeNumberFilter(qint64 value, qint64 filter, NumberComparison compare)
{
    return this;
}


Collections::QueryMaker*
NepomukQueryMaker::limitMaxResultSize(int size)
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::beginAnd()
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::beginOr()
{
    return this;
}

Collections::QueryMaker*
NepomukQueryMaker::endAndOr()
{
    return this;
}

