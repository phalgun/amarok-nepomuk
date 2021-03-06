/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "SynchronizeTracksJob.h"

#include "core/meta/Statistics.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "statsyncing/Controller.h"
#include "statsyncing/TrackTuple.h"

#include <threadweaver/Thread.h>

using namespace StatSyncing;

static const int denom = 20; // emit incementProgress() signal each N tracks
static const int fuzz = denom / 2;

SynchronizeTracksJob::SynchronizeTracksJob( const QList<TrackTuple> &tuples,
                                            const TrackList &tracksToScrobble,
                                            const Options &options, QObject *parent )
    : Job( parent )
    , m_abort( false )
    , m_tuples( tuples )
    , m_tracksToScrobble( tracksToScrobble )
    , m_updatedTracksCount( 0 )
    , m_options( options )
{
}

void
SynchronizeTracksJob::abort()
{
    m_abort = true;
}

void
SynchronizeTracksJob::run()
{
    emit totalSteps( ( m_tuples.size() + fuzz ) / denom );

    Controller *controller = Amarok::Components::statSyncingController();
    if( controller )
    {
        connect( this, SIGNAL(scrobble(Meta::TrackPtr,double,QDateTime)),
                 controller, SLOT(scrobble(Meta::TrackPtr,double,QDateTime)) );
        // we don't run an event loop, we must use direct connection for controller to talk to us
        connect( controller, SIGNAL(trackScrobbled(ScrobblingServicePtr,Meta::TrackPtr)),
                 SLOT(slotTrackScrobbled(ScrobblingServicePtr,Meta::TrackPtr)),
                 Qt::DirectConnection );
        connect( controller, SIGNAL(scrobbleFailed(ScrobblingServicePtr,Meta::TrackPtr,int)),
                 SLOT(slotScrobbleFailed(ScrobblingServicePtr,Meta::TrackPtr,int)),
                 Qt::DirectConnection );
    }
    else
        warning() << __PRETTY_FUNCTION__ << "StatSyncing::Controller not available!";

    // first, queue tracks for scrobbling, because after syncing their recent playcount is
    // reset
    foreach( const TrackPtr &track, m_tracksToScrobble )
    {
        Meta::TrackPtr metaTrack = track->metaTrack();
        int playcount = track->recentPlayCount();
        if( metaTrack && playcount > 0 )
        {
            m_scrobbledTracks << metaTrack;
            emit scrobble( metaTrack, playcount, track->lastPlayed() );
        }
    }

    int i = 0;
    foreach( TrackTuple tuple, m_tuples )
    {
        if( m_abort )
            break;

        // no point in checking for hasUpdate() here, synchronize() is witty enough
        m_updatedTracksCount += tuple.synchronize( m_options );
        if( ( i + fuzz ) % denom == 0 )
            emit incrementProgress();
        i++;
    }

    // we need to reset playCount of scrobbled tracks to reset their recent play count
    foreach( Meta::TrackPtr track, m_scrobbledTracks )
    {
        Meta::StatisticsPtr statistics = track->statistics();
        statistics->setPlayCount( statistics->playCount() );
    }

    if( !m_tracksToScrobble.isEmpty() )
        // wait 3 seconds so that we have chance to catch slotTrackScrobbled()..
        thread()->msleep( 3000 );
    if( controller )
        disconnect( controller, SIGNAL(trackScrobbled(ScrobblingServicePtr,Meta::TrackPtr)), this, 0 );
        disconnect( controller, SIGNAL(scrobbleFailed(ScrobblingServicePtr,Meta::TrackPtr,int)), this, 0 );

    emit endProgressOperation( this );
}

void
SynchronizeTracksJob::slotTrackScrobbled( const ScrobblingServicePtr &service,
                                          const Meta::TrackPtr &track )
{
    slotScrobbleFailed( service, track, ScrobblingService::NoError );
}

void
SynchronizeTracksJob::slotScrobbleFailed( const ScrobblingServicePtr &service,
                                          const Meta::TrackPtr &track, int error )
{
    // only count tracks scrobbled by us. Still chance for false-positives, though
    if( m_scrobbledTracks.contains( track ) )
    {
        ScrobblingService::ScrobbleError errorEnum = ScrobblingService::ScrobbleError( error );
        m_scrobbles[ service ][ errorEnum ]++;
    }
}

int
SynchronizeTracksJob::updatedTracksCount() const
{
    return m_updatedTracksCount;
}

QMap<ScrobblingServicePtr, QMap<ScrobblingService::ScrobbleError, int> >
SynchronizeTracksJob::scrobbles()
{
    return m_scrobbles;
}
