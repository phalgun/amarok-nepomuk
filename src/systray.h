/******************************************************************************
 * Copyright (c) 2003 Stanislav Karchebny <berkus@users.sf.net>               *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#ifndef AMAROK_SYSTRAY_H
#define AMAROK_SYSTRAY_H

#include "EngineObserver.h" //baseclass

#include <KAction>
#include <ksystemtrayicon.h>

#include <QPixmap>

class QEvent;

class App;

class PopupDropperAction;

namespace Amarok {

class TrayIcon : public KSystemTrayIcon, public EngineObserver
{
public:
    TrayIcon( QWidget* );
    friend class ::App;

protected:
    // reimpl from engineobserver
    virtual void engineStateChanged( Phonon::State state, Phonon::State oldState = Phonon::StoppedState );
    virtual void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged );
    virtual void engineTrackPositionChanged( long position, bool /*userSeek*/ );
    // get notified of 'highlight' color change
    virtual void paletteChange( const QPalette & oldPalette );

private:
    virtual bool event( QEvent *e );
    //void setLastFm( bool );
    void setupMenu();

    // repaints trayIcon showing progress (and overlay if present)
    void paintIcon( int mergePixels = -1, bool force = false );
    // blend an overlay icon over 'sourcePixmap' and repaint trayIcon
    void blendOverlay( QPixmap &sourcePixmap );

    long trackLength, mergeLevel;
    QIcon baseIcon;
    QPixmap grayedIcon, alternateIcon;
    QPixmap playOverlay, pauseOverlay;
    QPixmap *overlay;   // the current overlay (may be NULL)
    int blinkTimerID;   // timer ID returned by QObject::startTimer()
    bool overlayVisible;// used for blinking / hiding overlay
    QList<PopupDropperAction *> m_extraActions;
};

}

#endif // AMAROK_SYSTRAY_H

