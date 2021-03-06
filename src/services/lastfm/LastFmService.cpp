/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#define DEBUG_PREFIX "LastFmService"
#include "core/support/Debug.h"

#include "LastFmService.h"

#include "AvatarDownloader.h"
#include "EngineController.h"
#include "biases/LastFmBias.h"
#include "biases/WeeklyTopBias.h"
#include "LastFmServiceCollection.h"
#include "LastFmServiceConfig.h"
#include "LoveTrackAction.h"
#include "SimilarArtistsAction.h"
#include "LastFmTreeModel.h"
#include "LastFmTreeView.h"
#include "ScrobblerAdapter.h"
#include "GlobalCurrentTrackActions.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"
#include "meta/LastFmMeta.h"
#include "SynchronizationAdapter.h"
#include "statsyncing/Controller.h"
#include "widgets/SearchWidget.h"

#include <KLineEdit>
#include <KStandardDirs>

#include <QCryptographicHash>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextDocument>        //Qt::escape

#include <XmlQuery.h>

AMAROK_EXPORT_SERVICE_PLUGIN( lastfm, LastFmServiceFactory )

QString md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}

LastFmServiceFactory::LastFmServiceFactory( QObject *parent, const QVariantList &args )
    : ServiceFactory( parent, args )
{
    KPluginInfo pluginInfo(  "amarok_service_lastfm.desktop", "services" );
    pluginInfo.setConfig( config() );
    m_info = pluginInfo;
}

void
LastFmServiceFactory::init()
{
    ServiceBase *service = new LastFmService( this, "Last.fm" );
    m_activeServices << service;
    emit newService( service );
    m_initialized = true;
}

QString
LastFmServiceFactory::name()
{
    return "Last.fm";
}

KConfigGroup
LastFmServiceFactory::config()
{
    return Amarok::config( LastFmServiceConfig::configSectionName() );
}

bool
LastFmServiceFactory::possiblyContainsTrack( const KUrl &url ) const
{
    return url.protocol() == "lastfm";
}


LastFmService::LastFmService( LastFmServiceFactory *parent, const QString &name )
    : ServiceBase( name, parent, false ),
      m_inited( false),
      m_collection( 0 ),
      m_polished( false ),
      m_avatarLabel( 0 ),
      m_profile( 0 ),
      m_userinfo( 0 ),
      m_subscriber( false )
{
    DEBUG_BLOCK
    setShortDescription( i18n( "Last.fm: The social music revolution" ) );
    setIcon( KIcon( "view-services-lastfm-amarok" ) );
    setLongDescription( i18n( "Last.fm is a popular online service that provides personal radio stations and music recommendations. A personal listening station is tailored based on your listening habits and provides you with recommendations for new music. It is also possible to play stations with music that is similar to a particular artist as well as listen to streams from people you have added as friends or that Last.fm considers your musical \"neighbors\"" ) );
    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_lastfm.png" ) );

    if( !m_config.username().isEmpty() && !m_config.password().isEmpty() )
        init();
}


LastFmService::~LastFmService()
{
    DEBUG_BLOCK
    if( m_collection && CollectionManager::instance() )
    {
        CollectionManager::instance()->removeUnmanagedCollection( m_collection );
        delete m_collection;
        m_collection = 0;
    }
    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    if( m_scrobbler && controller )
        controller->unregisterScrobblingService( StatSyncing::ScrobblingServicePtr( m_scrobbler.data() ) );
    if( m_synchronizationAdapter && controller )
        controller->unregisterProvider( m_synchronizationAdapter );
}

void
LastFmService::init()
{
    // set the global static Lastfm::Ws stuff
    lastfm::ws::ApiKey = Amarok::lastfmApiKey();
    lastfm::ws::SharedSecret = Amarok::lastfmApiSharedSecret();
    lastfm::ws::Username = m_config.username();
    if( lastfm::nam() != The::networkAccessManager() )
        lastfm::setNetworkAccessManager( The::networkAccessManager() );

    debug() << "username:" << QString( QUrl::toPercentEncoding( lastfm::ws::Username ) );

    const QString authToken = md5( QString( "%1%2" ).arg( m_config.username() ).arg(
            md5( m_config.password().toUtf8() ) ).toUtf8() );

    // now authenticate w/ last.fm and get our session key if we don't have one
    if( m_config.sessionKey().isEmpty() )
    {
        debug() << "got no saved session key, authenticating with last.fm";
        QMap<QString, QString> query;
        query[ "method" ] = "auth.getMobileSession";
        query[ "username" ] = m_config.username();
        query[ "authToken" ] = authToken;
        m_jobs[ "auth" ] = lastfm::ws::post( query );

        connect( m_jobs[ "auth" ], SIGNAL( finished() ), SLOT( onAuthenticated() ) );

    }
    else
    {
        debug() << "using saved sessionkey from last.fm";
        lastfm::ws::SessionKey = m_config.sessionKey();

        StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
        Q_ASSERT( controller );
        if( m_config.scrobble() )
        {
            m_scrobbler = new ScrobblerAdapter( "ark", &m_config );
            controller->registerScrobblingService(
                    StatSyncing::ScrobblingServicePtr( m_scrobbler.data() ) );
        }
        m_synchronizationAdapter = new SynchronizationAdapter( &m_config );
        controller->registerProvider( m_synchronizationAdapter );

        QMap< QString, QString > params;
        params[ "method" ] = "user.getInfo";
        m_jobs[ "getUserInfo" ] = lastfm::ws::post( params );

        connect( m_jobs[ "getUserInfo" ], SIGNAL( finished() ), SLOT( onGetUserInfo() ) );
    }


    //We have no use for searching currently..
    m_searchWidget->setVisible( false );

    // enable custom bias
    Dynamic::BiasFactory::instance()->registerNewBiasFactory( new Dynamic::LastFmBiasFactory() );
    Dynamic::BiasFactory::instance()->registerNewBiasFactory( new Dynamic::WeeklyTopBiasFactory() );

    m_collection = new Collections::LastFmServiceCollection( m_config.username() );
    CollectionManager::instance()->addUnmanagedCollection( m_collection, CollectionManager::CollectionDisabled );


    //add the "play similar artists" action to all artist
    The::globalCollectionActions()->addArtistAction( new SimilarArtistsAction( this ) );
    The::globalCollectionActions()->addTrackAction( new LoveTrackAction( this ) );


    QAction * loveAction = new QAction( KIcon( "love-amarok" ), i18n( "Last.fm: Love" ), this );
    connect( loveAction, SIGNAL( triggered() ), this, SLOT( love() ) );
    loveAction->setShortcut( i18n( "Ctrl+L" ) );
    The::globalCurrentTrackActions()->addAction( loveAction );

    m_serviceready = true;

    m_inited = true;
}


void
LastFmService::onAuthenticated()
{
    if( !m_jobs[ "auth" ] )
    {
        debug() << "WARNING: GOT RESULT but no object";
        return;
    }

    switch ( m_jobs[ "auth" ]->error() )
    {
        case QNetworkReply::NoError:
        {

            lastfm::XmlQuery lfm;
            lfm.parse( m_jobs[ "auth" ]->readAll() );

            if( lfm.children( "error" ).size() > 0 )
            {
                debug() << "error from authenticating with last.fm service:" << lfm.text();
                m_config.clearSessionKey();
                break;
            }
            m_config.setSessionKey( lfm[ "session" ][ "key" ].text() );
            m_config.save();

            lastfm::ws::SessionKey = m_config.sessionKey();

            StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
            Q_ASSERT( controller );
            if( m_config.scrobble() )
            {
                m_scrobbler = new ScrobblerAdapter( "ark", &m_config );
                controller->registerScrobblingService(
                        StatSyncing::ScrobblingServicePtr( m_scrobbler.data() ) );
            }
            m_synchronizationAdapter = new SynchronizationAdapter( &m_config );
            controller->registerProvider( m_synchronizationAdapter );

            QMap< QString, QString > params;
            params[ "method" ] = "user.getInfo";
            m_jobs[ "getUserInfo" ] = lastfm::ws::post( params );

            connect( m_jobs[ "getUserInfo" ], SIGNAL( finished() ), SLOT( onGetUserInfo() ) );

            break;
        }
        case QNetworkReply::AuthenticationRequiredError:
            Amarok::Components::logger()->longMessage( i18nc("Last.fm: errorMessage",
                    "Either the username was not recognized, or the password was incorrect." ) );
            break;

        default:
            Amarok::Components::logger()->longMessage( i18nc("Last.fm: errorMessage",
                    "There was a problem communicating with the Last.fm services. Please try again later." ) );
            break;
    }
    m_jobs[ "auth" ]->deleteLater();
}

void
LastFmService::onGetUserInfo()
{
    DEBUG_BLOCK
    if( !m_jobs[ "getUserInfo" ] )
    {
        debug() << "GOT RESULT FROM USER QUERY, but no object..!";
        return;
    }
    switch (m_jobs[ "getUserInfo" ]->error())
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm;
            if( lfm.parse( m_jobs[ "getUserInfo" ]->readAll() ) ) {
                m_country = lfm["user"]["country"].text();
                m_age = lfm["user"]["age"].text();
                m_gender = lfm["user"]["gender"].text();
                m_playcount = lfm["user"]["playcount"].text();
                m_subscriber = lfm["user"]["subscriber"].text() == "1";

                debug() << "profile info "  << m_country << " " << m_age << " " << m_gender << " " << m_playcount << " " << m_subscriber;
                if( !lfm["user"][ "image" ].text().isEmpty() )
                {
                    debug() << "profile avatar: " <<lfm["user"][ "image" ].text();
                    AvatarDownloader* downloader = new AvatarDownloader();
                    KUrl url( lfm["user"][ "image" ].text() );
                    downloader->downloadAvatar( m_config.username(),  url);
                    connect( downloader, SIGNAL(avatarDownloaded(const QString&, QPixmap)),
                                         SLOT(onAvatarDownloaded(const QString&, QPixmap)) );
                }
                updateProfileInfo();

            }
            else
            {
                debug() << "Got exception in parsing from last.fm:" << lfm.parseError().message();
            }
            break;
        } case QNetworkReply::AuthenticationRequiredError:
            debug() << "Last.fm: errorMessage: Sorry, we don't recognise that username, or you typed the password incorrectly.";
            break;

        default:
            debug() << "Last.fm: errorMessage: There was a problem communicating with the Last.fm services. Please try again later.";
            break;
    }

    m_jobs[ "getUserInfo" ]->deleteLater();
}

void
LastFmService::onAvatarDownloaded( const QString &username, QPixmap avatar )
{
    DEBUG_BLOCK
    if( username == m_config.username() && !avatar.isNull() ) {

        if( !m_polished )
            polish();

        LastFmTreeModel* lfm = dynamic_cast<LastFmTreeModel*>( model() );

        int m = lfm->avatarSize();
        avatar = avatar.scaled( m, m, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        lfm->prepareAvatar( avatar, m );
        m_avatar = avatar;

        if( m_avatarLabel )
            m_avatarLabel->setPixmap( m_avatar );
    }
    sender()->deleteLater();
}

void
LastFmService::updateEditHint( int index )
{
    if( !m_customStationEdit )
        return;
    QString hint;
    switch ( index ) {
        case 0:
            hint = i18n( "Enter an artist name" );
            break;
        case 1:
            hint = i18n( "Enter a tag" );
            break;
        case 2:
            hint = i18n( "Enter a Last.fm user name" );
            break;
        default:
            return;
    }
    m_customStationEdit->setClickMessage( hint );
}

void
LastFmService::updateProfileInfo()
{
    if( m_userinfo )
    {
        m_userinfo->setText( i18n( "Username: %1", Qt::escape( m_config.username() ) ) );
    }

    if( m_profile && !m_playcount.isEmpty() )
    {
        m_profile->setText( i18np( "Play Count: %1 play", "Play Count: %1 plays", m_playcount.toInt() ) );
    }
}

void
LastFmService::polish()
{
    if( !m_polished )
    {
        LastFmTreeView* view = new LastFmTreeView( this );
        view->setFrameShape( QFrame::NoFrame );
        view->setDragEnabled ( true );
        view->setSortingEnabled( false );
        view->setDragDropMode ( QAbstractItemView::DragOnly );
        setView( view );
        setModel( new LastFmTreeModel( m_config.username(), this ) );

        //m_bottomPanel->setMaximumHeight( 300 );
        m_bottomPanel->hide();

        m_topPanel->setMaximumHeight( 300 );
        KHBox * outerProfilebox = new KHBox( m_topPanel );
        outerProfilebox->setSpacing(1);
        outerProfilebox->setMargin(0);

        m_avatarLabel = new QLabel(outerProfilebox);
        if( !m_avatar )
        {
            int m = dynamic_cast<LastFmTreeModel*>( model() )->avatarSize();
            m_avatarLabel->setPixmap( KIcon( "filename-artist-amarok" ).pixmap(m, m) );
            m_avatarLabel->setFixedSize( m, m );
        }
        else
        {
            m_avatarLabel->setPixmap( m_avatar );
            m_avatarLabel->setFixedSize( m_avatar.width(), m_avatar.height() );
            m_avatarLabel->setMargin( 5 );
        }

        KVBox * innerProfilebox = new KVBox( outerProfilebox );
        innerProfilebox->setSpacing(0);
        innerProfilebox->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
        m_userinfo = new QLabel(innerProfilebox);
        m_userinfo->setText( m_config.username() );
        m_profile = new QLabel(innerProfilebox);
        m_profile->setText(QString());
        updateProfileInfo();


        QGroupBox *customStation = new QGroupBox( i18n( "Create a Custom Last.fm Station" ), m_topPanel );
        m_customStationCombo = new QComboBox;
        QStringList choices;
        choices << i18n( "Artist" ) << i18n( "Tag" ) << i18n( "User" );
        m_customStationCombo->insertItems(0, choices);
        m_customStationEdit = new KLineEdit;
        m_customStationEdit->setClearButtonShown( true );
        updateEditHint( m_customStationCombo->currentIndex() );
        m_customStationButton = new QPushButton;
        m_customStationButton->setObjectName( "customButton" );
        m_customStationButton->setIcon( KIcon( "media-playback-start-amarok" ) );
        QHBoxLayout *hbox = new QHBoxLayout();
        hbox->addWidget(m_customStationCombo);
        hbox->addWidget(m_customStationEdit);
        hbox->addWidget(m_customStationButton);
        customStation->setLayout(hbox);

        connect( m_customStationEdit, SIGNAL( returnPressed() ), this, SLOT( playCustomStation() ) );
        connect( m_customStationButton, SIGNAL( clicked() ), this, SLOT( playCustomStation() ) );
        connect( m_customStationCombo, SIGNAL( currentIndexChanged(int) ), this, SLOT( updateEditHint(int) ));

        QList<int> levels;
        levels << CategoryId::Genre << CategoryId::Album;
        m_polished = true;
    }
}


void
LastFmService::love()
{
    DEBUG_BLOCK

    Meta::TrackPtr track = The::engineController()->currentTrack();
    LastFm::Track* lastfmTrack = dynamic_cast< LastFm::Track* >( track.data() );

    if( lastfmTrack )
    {
        lastfmTrack->love();
        Amarok::Components::logger()->shortMessage( i18nc( "As in, lastfm", "Loved Track: %1", track->prettyName() ) );
    }
    else
    {
        m_scrobbler->loveTrack( track );
    }

}

void LastFmService::love( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    m_scrobbler->loveTrack( track );
}


void
LastFmService::ban()
{
    DEBUG_BLOCK

    Meta::TrackPtr track = The::engineController()->currentTrack();
    LastFm::Track* lastfmTrack = dynamic_cast< LastFm::Track* >( track.data() );
    if( lastfmTrack )
        lastfmTrack->ban();
}

void LastFmService::playCustomStation()
{
    DEBUG_BLOCK
    QString text = m_customStationEdit->text();
    QString station;
    debug() << "Selected combo " <<m_customStationCombo->currentIndex();
    switch ( m_customStationCombo->currentIndex() ) {
        case 0:
            station = "lastfm://artist/" + text + "/similarartists";
            break;
        case 1:
            station = "lastfm://globaltags/" + text;
            break;
        case 2:
            station = "lastfm://user/" + text + "/personal";
            break;
        default:
            return;
    }

    if ( !station.isEmpty() ) {
        playLastFmStation( station );
    }
}

void LastFmService::playLastFmStation( const KUrl &url )
{
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
    The::playlistController()->insertOptioned( track, Playlist::AppendAndPlay );
}

Collections::Collection * LastFmService::collection()
{
    return m_collection;
}
