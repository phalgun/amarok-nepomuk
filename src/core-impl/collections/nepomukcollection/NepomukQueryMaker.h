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

#include "core/collections/QueryMaker.h"

class NepomukCollection;

namespace Collections {

class NepomukQueryMaker : public QueryMaker
{
    Q_OBJECT

public:
    NepomukQueryMaker(NepomukCollection *collection);
    virtual ~NepomukQueryMaker();

    virtual void abortQuery();

    virtual void run();

    virtual QueryMaker* setQueryType( QueryType type );

    virtual QueryMaker* addReturnValue( qint64 value );
    virtual QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );

    virtual QueryMaker* orderBy( qint64 value, bool descending = false );

    virtual QueryMaker* addMatch( const Meta::TrackPtr &track );
    virtual QueryMaker* addMatch( const Meta::ArtistPtr &artist );
    virtual QueryMaker* addMatch( const Meta::AlbumPtr &album );
    virtual QueryMaker* addMatch( const Meta::ComposerPtr &composer );
    virtual QueryMaker* addMatch( const Meta::GenrePtr &genre );
    virtual QueryMaker* addMatch( const Meta::YearPtr &year );
    virtual QueryMaker* addMatch( const Meta::LabelPtr &label );

    virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );
    virtual QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );

    virtual QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare );
    virtual QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare );

    virtual QueryMaker* limitMaxResultSize( int size );

    virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );
    virtual QueryMaker* setArtistQueryMode( ArtistQueryMode mode );
    virtual QueryMaker* setLabelQueryMode( LabelQueryMode mode );

    virtual QueryMaker* beginAnd();
    virtual QueryMaker* beginOr();
    virtual QueryMaker* endAndOr();

    QueryMaker* setAutoDelete( bool autoDelete );
    virtual int validFilterMask();

signals:

    void newResultReady( Meta::TrackList );
    void newResultReady( Meta::ArtistList );
    void newResultReady( Meta::AlbumList );
    void newResultReady( Meta::GenreList );
    void newResultReady( Meta::ComposerList );
    void newResultReady( Meta::YearList );
    void newResultReady( QStringList );
    void newResultReady( Meta::LabelList );
    void newResultReady( Meta::DataList );

    void queryDone();

private:
    NepomukCollection *m_collection;

};

} // namespace
#endif // NEPOMUKQUERYMAKER_H
