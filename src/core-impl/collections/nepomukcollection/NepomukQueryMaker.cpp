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

#define DEBUG_PREFIX "NepomukCollection"

#include "NepomukQueryMaker.h"

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
NepomukQueryMaker::run()
{

}

QueryMaker*
NepomukQueryMaker::setQueryType(QueryType type)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::addReturnValue(qint64 value)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::addReturnFunction(ReturnFunction function, qint64 value)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::orderBy(qint64 value, bool descending)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch(const Meta::TrackPtr &track)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch(const Meta::ArtistPtr &artist)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch(const Meta::AlbumPtr &album)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch(const Meta::ComposerPtr &composer)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch(const Meta::GenrePtr &genre)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch(const Meta::YearPtr &year)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch(const Meta::LabelPtr &label)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::addFilter(qint64 value, const QString &filter, bool matchBegin, bool matchEnd)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::excludeFilter(qint64 value, const QString &filter, bool matchBegin, bool matchEnd)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::excludeNumberFilter(qint64 value, qint64 filter, NumberComparison compare)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::limitMaxResultSize(int size)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::setAlbumQueryMode(AlbumQueryMode mode)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::setArtistQueryMode(ArtistQueryMode mode)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::setLabelQueryMode(LabelQueryMode mode)
{
    return this;
}

QueryMaker*
NepomukQueryMaker::beginAnd()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::beginOr()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::endAndOr()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::setAutoDelete(bool autoDelete)
{
    return this;
}

int
NepomukQueryMaker::validFilterMask()
{
    return 0;
}

void
NepomukQueryMaker::newResultReady(Meta::TrackList)
{

}

void
NepomukQueryMaker::newResultReady(Meta::ArtistList)
{

}

void
NepomukQueryMaker::newResultReady(Meta::AlbumList)
{

}

void
NepomukQueryMaker::newResultReady(Meta::GenreList)
{

}

void
NepomukQueryMaker::newResultReady(Meta::ComposerList)
{

}

void
NepomukQueryMaker::newResultReady(Meta::YearList)
{

}

void
NepomukQueryMaker::newResultReady(QStringList)
{

}

void
NepomukQueryMaker::newResultReady(Meta::LabelList)
{

}

void
NepomukQueryMaker::newResultReady(Meta::DataList)
{

}

void
NepomukQueryMaker::queryDone()
{

}

