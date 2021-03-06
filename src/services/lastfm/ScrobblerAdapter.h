/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef LASTFMSCROBBLERADAPTER_H
#define LASTFMSCROBBLERADAPTER_H

#include "core/meta/Meta.h"
#include "statsyncing/ScrobblingService.h"

#include <QTimer>

#include <Audioscrobbler.h>
#include <Track.h>

class LastFmServiceConfig;

class ScrobblerAdapter : public QObject, public StatSyncing::ScrobblingService
{
    Q_OBJECT

    public:
        ScrobblerAdapter( const QString &clientId, const LastFmServiceConfig *config );
        virtual ~ScrobblerAdapter();

    public:
        // ScrobblingService methods:
        QString prettyName() const;
        ScrobbleError scrobble( const Meta::TrackPtr &track, double playedFraction = 1.0,
                                const QDateTime &time = QDateTime() );
        void updateNowPlaying( const Meta::TrackPtr &track );

    public slots:
        // own methods
        void loveTrack( const Meta::TrackPtr &track );
        void banTrack( const Meta::TrackPtr &track );

    private slots:
        void slotScrobblesSubmitted( const QList<lastfm::Track> &tracks );
        void slotNowPlayingError( int code, const QString &message );

    private:
        /**
         * Copies metadata from @param from to @param to.
         */
        void copyTrackMetadata( lastfm::MutableTrack& to, const Meta::TrackPtr &from );

        /**
         * Announces Last.fm suggested @param track corrections to Amarok pop-up log.
         */
        void announceTrackCorrections( const lastfm::Track &track );

        lastfm::Audioscrobbler m_scrobbler;
        QWeakPointer<const LastFmServiceConfig> m_config;
};

#endif // LASTFMSCROBBLERADAPTER_H
