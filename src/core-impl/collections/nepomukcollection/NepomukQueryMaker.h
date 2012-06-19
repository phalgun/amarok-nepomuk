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


#ifndef NEPOMUKQUERYMAKER_H
#define NEPOMUKQUERYMAKER_H

#include "core/collections/Collection.h"
#include "core/meta/Meta.h"
#include "core-impl/collections/support/MemoryFilter.h"

#include <QList>
#include <QStack>

using namespace Meta;

class NepomukCollection;
class NepomukCollectionFactory;

class NepomukQueryMaker : public Collections::QueryMaker
{
    Q_OBJECT

public:
    NepomukQueryMaker(NepomukCollection *collection);
    virtual ~NepomukQueryMaker();

    virtual void abortQuery();

    virtual void run();

    virtual Collections::QueryMaker* setQueryType( QueryType type );

    virtual Collections::QueryMaker* addReturnValue( qint64 value );
    virtual Collections::QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );

    virtual Collections::QueryMaker* orderBy( qint64 value, bool descending = false );

    virtual Collections::QueryMaker* addMatch( const Meta::TrackPtr &track );
    virtual Collections::QueryMaker* addMatch( const Meta::ArtistPtr &artist );
    virtual Collections::QueryMaker* addMatch( const Meta::AlbumPtr &album );
    virtual Collections::QueryMaker* addMatch( const Meta::ComposerPtr &composer );
    virtual Collections::QueryMaker* addMatch( const Meta::GenrePtr &genre );
    virtual Collections::QueryMaker* addMatch( const Meta::YearPtr &year );
    virtual Collections::QueryMaker* addMatch( const Meta::LabelPtr &label );

    virtual Collections::QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );
    virtual Collections::QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );

    virtual Collections::QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare );
    virtual Collections::QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare );

    virtual Collections::QueryMaker* limitMaxResultSize( int size );

    virtual Collections::QueryMaker* beginAnd();
    virtual Collections::QueryMaker* beginOr();
    virtual Collections::QueryMaker* endAndOr();

    Collections::QueryMaker* setAutoDelete( bool autoDelete );

    virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );
    virtual QueryMaker* setArtistQueryMode( ArtistQueryMode mode );
    virtual QueryMaker* setLabelQueryMode( LabelQueryMode mode );


private:
    NepomukCollection *m_collection;

    QueryType m_queryType;
    AlbumQueryMode m_albumQueryMode;
    ArtistQueryMode m_artistQueryMode;
    LabelQueryMode m_labelQueryMode;
    QStack<ContainerMemoryFilter*> m_containerFilters;

    qint64 m_returnValue;
    bool m_usingFilters;
    int m_maxsize;
};


#endif // NEPOMUKQUERYMAKER_H
