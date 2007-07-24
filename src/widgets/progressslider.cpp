/***************************************************************************
 * copyright     : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>   *
 **************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokconfig.h"
#include "debug.h"
#include "enginecontroller.h"
#include "meta/meta.h"
#include "meta/MetaUtility.h"
#include "progressslider.h"
#include "timeLabel.h"

#include <kpassivepopup.h>
#include <khbox.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPolygon>

// Class ProgressSlider
ProgressSlider *ProgressSlider::s_instance = 0;
ProgressSlider::ProgressSlider( QWidget *parent ) :
        Amarok::PrettySlider( Qt::Horizontal, Amarok::PrettySlider::Normal, parent )
{
    s_instance = this;
    setMouseTracking( true );
}

// Add A bookmark to the slider at given second
void
ProgressSlider::addBookmark( uint second )
{
    if( !m_bookmarks.contains(second) )
        m_bookmarks << second;
    update();
}


void
ProgressSlider::addBookmarks( QList<uint> seconds )
{
    foreach( uint it, seconds )
        addBookmark( it );
}

void
ProgressSlider::paintEvent( QPaintEvent *e )
{
    Amarok::PrettySlider::paintEvent( e );

    if( isEnabled() )
    {
        QPainter p( this );
        foreach( uint it, m_bookmarks )
        {
            const int pos = int( double( width() ) / maximum() * ( it * 1000 ) );
            Bookmark pa( 3, it );
            pa.setPoint( 0, pos - 5, 1 );
            pa.setPoint( 1, pos + 5, 1 );
            pa.setPoint( 2, pos,     9 );
            p.setBrush( Qt::red );
            m_polygons << pa;
            p.drawConvexPolygon( pa );
        }
    }
}

void
ProgressSlider::mouseMoveEvent( QMouseEvent *e )
{
    foreach( Bookmark p, m_polygons )
    {
        //only create a popup if the mouse enters a bookmark, not if it moves inside of one.
        if( p.containsPoint( e->pos(), Qt::OddEvenFill ) && !p.containsPoint( oldpoint, Qt::OddEvenFill ) )
        {
            m_popup = new KPassivePopup( parentWidget() );
            KHBox *khb = new KHBox( m_popup );
            new QLabel( p.time(), khb );
            m_popup->setView( khb );
            m_popup->setAutoDelete( false );
            m_popup->show( mapToGlobal( e->pos() ) );
            oldpoint = e->pos();
        }
        //If the mouse moves outside of the bookmark, hide the popup
        else if ( p.containsPoint( oldpoint, Qt::OddEvenFill) && !p.containsPoint( e->pos(), Qt::OddEvenFill ) )
        {
            if( m_popup->isVisible() )
                m_popup->deleteLater();
        }
    }
    oldpoint = e->pos();

    Amarok::PrettySlider::mouseMoveEvent( e );
}

void
ProgressSlider::mousePressEvent( QMouseEvent *e )
{
    EngineController *ec = EngineController::instance();
    foreach( Bookmark p, m_polygons )
        if( p.containsPoint( e->pos(), Qt::OddEvenFill ) )
            ec->seek( p.seconds() * 1000 );

    Amarok::PrettySlider::mousePressEvent( e );
}
// END Class ProgressSlider

//Class ProgressWidget
ProgressWidget *ProgressWidget::s_instance = 0;
ProgressWidget::ProgressWidget( QWidget *parent )
    : QWidget( parent )
    , EngineObserver( EngineController::instance() )
    , m_timeLength( 0 )
{
    s_instance = this;

    QHBoxLayout *box = new QHBoxLayout( this );
    setLayout( box );
    box->setMargin( 1 );
    box->setSpacing( 3 );

    m_slider = new ProgressSlider( this );

    // the two time labels. m_timeLabel is the left one,
    // m_timeLabel2 the right one.
    m_timeLabel = new TimeLabel( this );
    m_timeLabel->setToolTip( i18n( "The amount of time elapsed in current song" ) );

    m_timeLabel2 = new TimeLabel( this );
    m_timeLabel->setToolTip( i18n( "The amount of time remaining in current song" ) );

    box->addSpacing( 3 );
    box->addWidget( m_timeLabel );
    box->addWidget( m_slider );
    box->addWidget( m_timeLabel2 );
#ifdef Q_WS_MAC
    // don't overlap the resize handle with the time display
    box->addSpacing( 12 );
#endif

    if( !AmarokConfig::leftTimeDisplayEnabled() )
        m_timeLabel->hide();

    engineStateChanged( Engine::Empty );

    connect( m_slider, SIGNAL(sliderReleased( int )), EngineController::instance(), SLOT(seek( int )) );
    connect( m_slider, SIGNAL(valueChanged( int )), SLOT(drawTimeDisplay( int )) );

}

void
ProgressWidget::drawTimeDisplay( int ms )  //SLOT
{
    int seconds = ms / 1000;
    int seconds2 = seconds; // for the second label.
    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    if( !track )
        return;
    const uint trackLength = track->length();

    if( AmarokConfig::leftTimeDisplayEnabled() )
        m_timeLabel->show();
    else
        m_timeLabel->hide();

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        seconds2 = seconds;
        seconds = trackLength - seconds;
    // when the left label shows the remaining time and it's a stream
    } else if( AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        seconds2 = seconds;
        seconds = 0; // for streams
    // when the right label shows the remaining time and it's not a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        seconds2 = trackLength - seconds;
    // when the right label shows the remaining time and it's a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        seconds2 = 0;
    }

    //put Utility functions somewhere
    QString s1 = Meta::secToPrettyTime( seconds );
    QString s2 = Meta::secToPrettyTime( seconds2 );

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 ) {
        s1.prepend( '-' );
    // when the right label shows the remaining time and it's not a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        s2.prepend( '-' );
    }

    if( m_timeLength > s1.length() )
        s1.prepend( QString( m_timeLength - s1.length(), ' ' ) );

    if( m_timeLength > s2.length() )
        s2.prepend( QString( m_timeLength - s2.length(), ' ' ) );

    s1 += ' ';
    s2 += ' ';

    m_timeLabel->setText( s1 );
    m_timeLabel2->setText( s2 );

    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabel->setEnabled( false );
        m_timeLabel2->setEnabled( true );
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabel->setEnabled( true );
        m_timeLabel2->setEnabled( false );
    } else
    {
        m_timeLabel->setEnabled( true );
        m_timeLabel2->setEnabled( true );
    }
}

void
ProgressWidget::engineTrackPositionChanged( long position, bool /*userSeek*/ )
{
    m_slider->setValue( position );

    if ( !m_slider->isEnabled() )
        drawTimeDisplay( position );
}

void
ProgressWidget::engineStateChanged( Engine::State state, Engine::State /*oldState*/ )
{

    switch ( state ) {
        case Engine::Empty:
            m_slider->setEnabled( false );
            m_slider->setMinimum( 0 ); //needed because setMaximum() calls with bogus values can change minValue
            m_slider->setMaximum( 0 );
            m_slider->newBundle( MetaBundle() ); // Set an empty bundle
            m_timeLabel->setEnabled( false ); //must be done after the setValue() above, due to a signal connection
            m_timeLabel2->setEnabled( false );
            break;

        case Engine::Playing:
        case Engine::Paused:
            DEBUG_LINE_INFO
            m_timeLabel->setEnabled( true );
            m_timeLabel2->setEnabled( true );
            break;

        case Engine::Idle:
            ; //just do nothing, idle is temporary and a limbo state
    }
}

void
ProgressWidget::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    m_slider->newBundle( bundle );
    engineTrackLengthChanged( bundle.length() );
    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    if( !track )
        return;
    m_slider->setMaximum( track->length() );

    ProgressSlider::instance()->addBookmark( 20 );
    ProgressSlider::instance()->addBookmark( 40);
    QList<uint> bookmarkList;
    bookmarkList << 30 << 50 << 45;
    ProgressSlider::instance()->addBookmarks( bookmarkList );
}

void
ProgressWidget::engineTrackLengthChanged( long length )
{
    m_slider->setMinimum( 0 );
    m_slider->setMaximum( length * 1000 );
    m_slider->setEnabled( length > 0 );
    m_timeLength = Meta::secToPrettyTime( length ).length()+1; // account for - in remaining time
}

#include "progressslider.moc"
