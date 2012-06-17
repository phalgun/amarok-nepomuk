/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "SqlPlaylist.h"

#include "core/collections/support/SqlStorage.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "core-impl/meta/stream/Stream.h"
#include "core-impl/meta/timecode/TimecodeMeta.h"
#include "playlistmanager/PlaylistManager.h"
#include "playlistmanager/sql/SqlPlaylistGroup.h"
#include "playlistmanager/sql/SqlUserPlaylistProvider.h"

#include <typeinfo>

namespace Playlists {

SqlPlaylist::SqlPlaylist( const QString & name, const Meta::TrackList
        &tracks, SqlPlaylistGroupPtr parent, PlaylistProvider *provider,
        const QString &urlId )
    : m_dbId( -1 )
    , m_parent( parent )
    , m_tracks( tracks )
    , m_provider( provider)
    , m_name( name )
    , m_description( i18n( "Playlist in database" ) )
    , m_urlId( urlId )
    , m_tracksLoaded( true )
{
    saveToDb();
}

SqlPlaylist::SqlPlaylist( const QStringList & resultRow,
                                SqlPlaylistGroupPtr parent,
                                PlaylistProvider *provider )
    : m_parent( parent )
    , m_provider( provider)
    , m_tracksLoaded( false )
{
    m_dbId = resultRow[0].toInt();
    m_name = resultRow[2];
    m_description = resultRow[3];
    m_urlId = resultRow[4];
}


SqlPlaylist::~SqlPlaylist()
{
}

KUrl
SqlPlaylist::uidUrl() const
{
    return KUrl( QString( "amarok-sqlplaylistuid://%1").arg( m_dbId ) );
}

QStringList
SqlPlaylist::groups()
{
    QStringList groups;
    if( m_parent && !m_parent->name().isNull() )
        groups << m_parent->name();
    return groups;
}

void
SqlPlaylist::setGroups( const QStringList &groups )
{
    SqlUserPlaylistProvider *userPlaylistProvider =
            dynamic_cast<SqlUserPlaylistProvider *>( m_provider );
    if( !userPlaylistProvider )
    {
        error() << "Provider could not be cast to SqlUserPlaylistProvider";
        return;
    }

    if( groups.isEmpty() )
        m_parent = SqlPlaylistGroupPtr();
    else
        m_parent = userPlaylistProvider->group( groups.first() );

    saveToDb();
}

bool
SqlPlaylist::saveToDb( bool tracks )
{
    int parentId = -1;
    if( m_parent )
        parentId = m_parent->id();

    SqlStorage *sql = CollectionManager::instance()->sqlStorage();

    //figure out if we have a urlId and if this id is already in the db, if so, update it instead of creating a new one.
    if( !m_urlId.isEmpty() )
    {
        debug() << "Checking " << m_urlId << " against db";

        //check if urlId exists
        QString query = "SELECT id from playlists WHERE urlid='%1'";
        query = query.arg( sql->escape( m_urlId ) );
        QStringList result = sql->query( query );

        if( !result.isEmpty() )
        {
            //set this id to the already existing one
            m_dbId =  result.at( 0 ).toInt();
            debug() << "Got existing playlist with id " << m_dbId;
        }
    }

    if( m_dbId != -1 )
    {
        //update existing
        QString query = "UPDATE playlists SET parent_id=%1, name='%2', description='%3' WHERE id=%4;";
        query = query.arg( QString::number( parentId ) )
                .arg( sql->escape( m_name ) )
                .arg( sql->escape( m_description ) )
                .arg( QString::number( m_dbId ) );
        CollectionManager::instance()->sqlStorage()->query( query );

        if( tracks )
        {
            //delete existing tracks and insert all
            query = "DELETE FROM playlist_tracks where playlist_id=%1;";
            query = query.arg( QString::number( m_dbId ) );
            CollectionManager::instance()->sqlStorage()->query( query );
            saveTracks();
        }
    }
    else
    {
        //insert new
        QString query = "INSERT INTO playlists ( parent_id, name, description, urlid ) "
                        "VALUES ( %1, '%2', '%3', '%4' );";
        query = query.arg( QString::number( parentId ) )
                .arg( sql->escape( m_name ) )
                .arg( sql->escape( m_description ) )
                .arg( sql->escape( m_urlId ) );
        m_dbId = CollectionManager::instance()->sqlStorage()->insert( query, "playlists" );
        if( tracks )
            saveTracks();
    }

    //HACK! if this has just been added from the collection scanner, the list is full of "dirty"
    //tracks that might not all have been properly trackForUrl'ed, so clear the track list so we
    //reload if we ever need them!
    if( !m_urlId.isEmpty() )
    {
        m_tracks.clear();
        m_tracksLoaded = false;
    }

    //clean the cache
    if( m_parent )
        m_parent->clear();

    return true;
}

void
SqlPlaylist::saveTracks()
{
    int trackNum = 1;
    SqlStorage *sql = CollectionManager::instance()->sqlStorage();

    foreach( Meta::TrackPtr trackPtr, m_tracks )
    {
        if( trackPtr )
        {
            debug() << "saving track with url " << trackPtr->uidUrl();
            QString query = "INSERT INTO playlist_tracks ( playlist_id, track_num, url, title, "
                            "album, artist, length, uniqueid ) VALUES ( %1, %2, '%3', '%4', '%5', "
                            "'%6', %7, '%8' );";
            query = query.arg( QString::number( m_dbId ), QString::number( trackNum ),
                        sql->escape( trackPtr->uidUrl() ),
                        sql->escape( trackPtr->prettyName() ),
                        trackPtr->album() ? sql->escape( trackPtr->album()->prettyName() ) : "",
                        trackPtr->artist()? sql->escape( trackPtr->artist()->prettyName() ) : "",
                        QString::number( trackPtr->length() ),
                        sql->escape( trackPtr->uidUrl() ) );
            sql->insert( query, "playlist_tracks" );

            trackNum++;
        }
    }
}

int
SqlPlaylist::trackCount() const
{
    if( m_tracksLoaded )
        return m_tracks.count();

    QString query = "SELECT COUNT(id) FROM playlist_tracks WHERE playlist_id=%1;";

    SqlStorage *sql = CollectionManager::instance()->sqlStorage();
    Q_ASSERT( sql );

    QStringList results = sql->query( query.arg( m_dbId ) );
    debug() << results;
    if( results.isEmpty() )
    {
        error() << "no results for COUNT query on playlist_tracks table!";
        return -1;
    }
    int trackCount = results.first().toInt();
    return trackCount;
}

Meta::TrackList
SqlPlaylist::tracks()
{
    //If you do not load before, m_tracks
    //can be empty before usage.
    triggerTrackLoad();

    return m_tracks;
}

void
SqlPlaylist::triggerTrackLoad()
{
    if( !m_tracksLoaded )
        loadTracks();
}

void
SqlPlaylist::addTrack( Meta::TrackPtr track, int position )
{
    if( !m_tracksLoaded )
        loadTracks();

    if( position < 0 )
        position = m_tracks.count();
    else
        position = qMin( position, m_tracks.count() );
    subscribeTo( track ); //keep track of metadata changes.
    m_tracks.insert( position, track );
    saveToDb( true );
    notifyObserversTrackAdded( track, position );
}

void
SqlPlaylist::removeTrack( int position )
{
    if( !m_tracksLoaded )
        loadTracks();

    if( position < 0 || position >= m_tracks.size() )
        return;
    Meta::TrackPtr track = m_tracks.takeAt( position );
    unsubscribeFrom( track );
    saveToDb( true );
    notifyObserversTrackRemoved( position );
}

void
SqlPlaylist::metadataChanged( Meta::TrackPtr track )
{
    //TODO: do we really need to observe track changes? Metadata will be properly saved
    //on exit and proxy tracks are loaded with uidUrl. So at worse the save metadata in
    //playlist_tracks will be outdated until real track is loaded.
    if( !m_tracksLoaded )
        loadTracks();

    //When AFT detects a moved file it will update the track and make it signal it's observers.
    if( !m_tracks.contains( track ) )
    {
        error() << "Got a metadataChanged for a track that is not in the playlist.";
        return;
    }

    //force update of tracks in database
    saveToDb();
}

void
SqlPlaylist::loadTracks()
{
    QString query = "SELECT playlist_id, track_num, url, title, album, artist, length FROM "
                    "playlist_tracks WHERE playlist_id=%1 ORDER BY track_num";
    query = query.arg( QString::number( m_dbId ) );

    QStringList result = CollectionManager::instance()->sqlStorage()->query( query );

    int resultRows = result.count() / 7;

    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*7, 7 );
        KUrl url = KUrl( row[2] );

        MetaProxy::Track *proxyTrack = new MetaProxy::Track( url );

        proxyTrack->setName( row[3] );
        proxyTrack->setAlbum( row[4] );
        proxyTrack->setArtist( row[5] );
        Meta::TrackPtr trackPtr = Meta::TrackPtr( proxyTrack );
        //subscribed to force a save to db on any change (such as AFT file move)
        subscribeTo( trackPtr );
        m_tracks << trackPtr;
    }

    m_tracksLoaded = true;
}

void
SqlPlaylist::setName( const QString &name )
{
    m_name = name;
    saveToDb( false ); //no need to resave all tracks
}

void
SqlPlaylist::removeFromDb()
{
    QString query = "DELETE FROM playlist_tracks WHERE playlist_id=%1";
    query = query.arg( QString::number( m_dbId ) );
    CollectionManager::instance()->sqlStorage()->query( query );

    query = "DELETE FROM playlists WHERE id=%1";
    query = query.arg( QString::number( m_dbId ) );
    CollectionManager::instance()->sqlStorage()->query( query );
}

} //namespace Playlists
