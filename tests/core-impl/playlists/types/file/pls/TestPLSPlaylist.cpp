/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@getamarok.com>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "TestPLSPlaylist.h"

#include "core/support/Components.h"
#include "config-amarok-test.h"
#include "core-impl/playlists/types/file/pls/PLSPlaylist.h"
#include "EngineController.h"

#include <QtTest/QTest>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include <qtest_kde.h>
#include <ThreadWeaver/Weaver>

QTEST_KDEMAIN_CORE( TestPLSPlaylist )

TestPLSPlaylist::TestPLSPlaylist()
{}

QString
TestPLSPlaylist::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void TestPLSPlaylist::initTestCase()
{
    // EngineController is used in a connection in MetaProxy::Track; avoid null sender
    // warning
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );

    qRegisterMetaType<Meta::TrackPtr>( "Meta::TrackPtr" );

    const KUrl url = dataPath( "data/playlists/test.pls" );
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream1;

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream1.setDevice( &playlistFile1 );
    QVERIFY( playlistStream1.device() );

    m_testPlaylist1 = new Playlists::PLSPlaylist( url );
    QVERIFY( m_testPlaylist1 );
    QVERIFY( m_testPlaylist1->load( playlistStream1 ) );
    QCOMPARE( m_testPlaylist1->tracks().size(), 4 );
    playlistFile1.close();
}

void TestPLSPlaylist::cleanupTestCase()
{
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Weaver::instance()->finish();

    delete m_testPlaylist1;
    delete Amarok::Components::setEngineController( 0 );
}

void TestPLSPlaylist::testSetAndGetName()
{
    QCOMPARE( m_testPlaylist1->name(), QString( "test.pls" ) );

    m_testPlaylist1->setName( "set name test" );
    QCOMPARE( m_testPlaylist1->name(), QString( "set name test" ) );

    m_testPlaylist1->setName( "set name test aäoöuüß" );
    QCOMPARE( m_testPlaylist1->name(), QString( "set name test aäoöuüß" ) );

    m_testPlaylist1->setName( "" );
    QCOMPARE( m_testPlaylist1->name(), QString( "playlists" ) );
}

void TestPLSPlaylist::testPrettyName()
{
    QCOMPARE( m_testPlaylist1->prettyName(), QString( "playlists" ) );
}

void TestPLSPlaylist::testTracks()
{
    Meta::TrackList tracklist = m_testPlaylist1->tracks();

    QCOMPARE( tracklist.at( 0 ).data()->name(), QString( "::darkerradio:: - DIE Alternative im Netz ::www.darkerradio.de:: Tune In, Turn On, Burn Out!" ) );
    QCOMPARE( tracklist.at( 1 ).data()->name(), QString( "::darkerradio:: - DIE Alternative im Netz ::www.darkerradio.de:: Tune In, Turn On, Burn Out!" ) );
    QCOMPARE( tracklist.at( 2 ).data()->name(), QString( "::darkerradio:: - DIE Alternative im Netz ::www.darkerradio.de:: Tune In, Turn On, Burn Out!" ) );
    QCOMPARE( tracklist.at( 3 ).data()->name(), QString( "::darkerradio:: - DIE Alternative im Netz ::www.darkerradio.de:: Tune In, Turn On, Burn Out!" ) );
}

void TestPLSPlaylist::testRetrievableUrl()
{
    m_testPlaylist1->setName( "test.pls" );
    QCOMPARE( m_testPlaylist1->uidUrl().pathOrUrl(), dataPath( "data/playlists/test.pls" ) );
}

void TestPLSPlaylist::testIsWritable()
{
    QVERIFY( m_testPlaylist1->isWritable() );
}

void TestPLSPlaylist::testSave()
{
    QFile::remove( QDir::tempPath() + QDir::separator() + "test.pls" );
    QVERIFY( m_testPlaylist1->save( QDir::tempPath() + QDir::separator() + "test.pls", false ) );
}
