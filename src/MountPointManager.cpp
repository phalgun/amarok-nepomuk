/****************************************************************************************
 * Copyright (c) 2006-2007 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#define DEBUG_PREFIX "MountPointManager"

#include "MountPointManager.h"

#include "MediaDeviceCache.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/collections/support/SqlStorage.h"

//solid stuff
#include <solid/predicate.h>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/storageaccess.h>

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QList>
#include <QStringList>
#include <QTimer>

DeviceHandlerFactory::DeviceHandlerFactory( QObject *parent, const QVariantList &args )
    : Plugins::PluginFactory( parent, args )
{
    m_type = Plugins::PluginFactory::Device;
}

MountPointManager::MountPointManager( QObject *parent, SqlStorage *storage )
    : QObject( parent )
    , m_storage( storage )
    , m_ready( false )
{
    DEBUG_BLOCK
    setObjectName( "MountPointManager" );

    if ( !Amarok::config( "Collection" ).readEntry( "DynamicCollection", true ) )
    {
        debug() << "Dynamic Collection deactivated in amarokrc, not loading plugins, not connecting signals";
        return;
    }

    connect( MediaDeviceCache::instance(), SIGNAL( deviceAdded( QString ) ), SLOT( deviceAdded( QString ) ) );
    connect( MediaDeviceCache::instance(), SIGNAL( deviceRemoved( QString ) ), SLOT( deviceRemoved( QString ) ) );

//     SqlStorage *collDB = CollectionManager::instance()->sqlStorage();

    //FIXME: Port 2.0
//     if ( collDB->adminValue( "Database Stats Version" ).toInt() >= 9 && /* make sure that deviceid actually exists*/
//          collDB->query( "SELECT COUNT(url) FROM statistics WHERE deviceid = -2;" ).first().toInt() != 0 )
//     {
//         connect( this, SIGNAL( mediumConnected( int ) ), SLOT( migrateStatistics() ) );
//         QTimer::singleShot( 0, this, SLOT( migrateStatistics() ) );
//     }
    updateStatisticsURLs();
}


MountPointManager::~MountPointManager()
{
    DEBUG_BLOCK

    m_handlerMapMutex.lock();
    foreach( DeviceHandler *dh, m_handlerMap )
        delete dh;

    while( !m_mediumFactories.isEmpty() )
        delete m_mediumFactories.takeFirst();
    while( !m_remoteFactories.isEmpty() )
        delete m_remoteFactories.takeFirst();
    m_handlerMapMutex.unlock();
}


void
MountPointManager::loadDevicePlugins( const QList<Plugins::PluginFactory*> &factories )
{
    DEBUG_BLOCK
    foreach( Plugins::PluginFactory *pFactory, factories )
    {
        DeviceHandlerFactory *factory = qobject_cast<DeviceHandlerFactory*>( pFactory );
        if( !factory )
            continue;

        KPluginInfo info = factory->info();
        QString name = info.pluginName();
        bool enabled = Amarok::config( "Plugins" ).readEntry( name + "Enabled", info.isPluginEnabledByDefault() );
        if( !enabled )
            continue;

        debug() << "initializing:" << name;
        factory->init();
        if( factory->canCreateFromMedium() )
            m_mediumFactories.append( factory );
        else if (factory->canCreateFromConfig() )
            m_remoteFactories.append( factory );
        else //FIXME max: better error message
            debug() << "Unknown DeviceHandlerFactory";

        Solid::Predicate predicate = Solid::Predicate( Solid::DeviceInterface::StorageAccess );
        QList<Solid::Device> devices = Solid::Device::listFromQuery( predicate );
        foreach( const Solid::Device &device, devices )
            createHandlerFromDevice( device, device.udi() );
    }
    m_ready = true;
}

int
MountPointManager::getIdForUrl( const KUrl &url )
{
    int mountPointLength = 0;
    int id = -1;
    m_handlerMapMutex.lock();
    foreach( DeviceHandler *dh, m_handlerMap )
    {
        if ( url.path().startsWith( dh->getDevicePath() ) && mountPointLength < dh->getDevicePath().length() )
        {
            id = m_handlerMap.key( dh );
            mountPointLength = dh->getDevicePath().length();
        }
    }
    m_handlerMapMutex.unlock();
    if ( mountPointLength > 0 )
    {
        return id;
    }
    else
    {
        //default fallback if we could not identify the mount point.
        //treat -1 as mount point / in all other methods
        return -1;
    }
}

bool
MountPointManager::isMounted( const int deviceId ) const
{
    m_handlerMapMutex.lock();
    const bool result = m_handlerMap.contains( deviceId );
    m_handlerMapMutex.unlock();
    return result;
}

QString
MountPointManager::getMountPointForId( const int id ) const
{
    QString mountPoint;
    if ( isMounted( id ) )
    {
        m_handlerMapMutex.lock();
        mountPoint = m_handlerMap[id]->getDevicePath();
        m_handlerMapMutex.unlock();
    }
    else
        //TODO better error handling
        mountPoint = '/';
    return mountPoint;
}

QString
MountPointManager::getAbsolutePath( const int deviceId, const QString& relativePath ) const
{
    // TODO: someone who clearly understands KUrl should clean this up.
    KUrl rpath;
    rpath.setPath( relativePath );
    KUrl absolutePath;

    // debug() << "id is " << deviceId << ", relative path is " << relativePath;
    if ( deviceId == -1 )
    {
#ifdef Q_OS_WIN32
        absolutePath.setPath( rpath.toLocalFile() );
#else
        absolutePath.setPath( "/" );
        absolutePath.addPath( rpath.path() );
#endif
        absolutePath.cleanPath();
        // debug() << "Deviceid is -1, using relative Path as absolute Path, returning " << absolutePath.path();
    }
    else
    {
        m_handlerMapMutex.lock();
        if ( m_handlerMap.contains( deviceId ) )
        {
            m_handlerMap[deviceId]->getURL( absolutePath, rpath );
            m_handlerMapMutex.unlock();
        }
        else
        {
            m_handlerMapMutex.unlock();
            const QStringList lastMountPoint = m_storage->query(
                                                                QString( "SELECT lastmountpoint FROM devices WHERE id = %1" )
                                                                .arg( deviceId ) );
            if ( lastMountPoint.count() == 0 )
            {
                //hmm, no device with that id in the DB...serious problem
                warning() << "Device " << deviceId << " not in database, this should never happen!";
                return getAbsolutePath( -1, relativePath );
            }
            else
            {
                absolutePath.setPath( lastMountPoint.first() );
                absolutePath.addPath( rpath.path() );
                absolutePath.cleanPath();
                //debug() << "Device " << deviceId << " not mounted, using last mount point and returning " << absolutePath.path();
            }
        }
    }

    #ifdef Q_OS_WIN32
        return absolutePath.toLocalFile();
    #else
        return absolutePath.path();
    #endif
}

QString
MountPointManager::getRelativePath( const int deviceId, const QString& absolutePath ) const
{
    QMutexLocker locker(&m_handlerMapMutex);
    if ( deviceId != -1 && m_handlerMap.contains( deviceId ) )
    {
        //FIXME max: returns garbage if the absolute path is actually not under the device's mount point
        return KUrl::relativePath( m_handlerMap[deviceId]->getDevicePath(), absolutePath );
    }
    else
    {
        //TODO: better error handling
#ifdef Q_OS_WIN32
        return KUrl( absolutePath ).toLocalFile();
#else
        return KUrl::relativePath( "/", absolutePath );
#endif
    }
}


// void
// MountPointManager::mediumChanged( const Medium *m )
// {
//     DEBUG_BLOCK
//     if ( !m ) return;
//     if ( m->isMounted() )
//     {
//         foreach( DeviceHandlerFactory *factory, m_mediumFactories )
//         {
//             if ( factory->canHandle ( m ) )
//             {
//                 debug() << "found handler for " << m->id();
//                 DeviceHandler *handler = factory->createHandler( m );
//                 if( !handler )
//                 {
//                     debug() << "Factory " << factory->type() << "could not create device handler";
//                     break;
//                 }
//                 int key = handler->getDeviceID();
//                 m_handlerMapMutex.lock();
//                 if ( m_handlerMap.contains( key ) )
//                 {
//                     debug() << "Key " << key << " already exists in handlerMap, replacing";
//                     delete m_handlerMap[key];
//                     m_handlerMap.remove( key );
//                 }
//                 m_handlerMap.insert( key, handler );
//                 m_handlerMapMutex.unlock();
//                 debug() << "added device " << key << " with mount point " << m->mountPoint();
//                 emit mediumConnected( key );
//                 break;  //we found the added medium and don't have to check the other device handlers
//             }
//         }
//     }
//     else
//     {
//         m_handlerMapMutex.lock();
//         foreach( DeviceHandler *dh, m_handlerMap )
//         {
//             if ( dh->deviceIsMedium( m ) )
//             {
//                 int key = m_handlerMap.key( dh );
//                 m_handlerMap.remove( key );
//                 delete dh;
//                 debug() << "removed device " << key;
//                 m_handlerMapMutex.unlock();
//                 emit mediumRemoved( key );
//                 //we found the medium which was removed, so we can abort the loop
//                 return;
//             }
//         }
//         m_handlerMapMutex.unlock();
//     }
// }
//

IdList
MountPointManager::getMountedDeviceIds() const
{
    m_handlerMapMutex.lock();
    IdList list( m_handlerMap.keys() );
    m_handlerMapMutex.unlock();
    list.append( -1 );
    return list;
}

QStringList
MountPointManager::collectionFolders() const
{
    if (!m_ready)
    {
        debug() << "requested collectionFolders from MountPointManager that is not yet ready";
        return QStringList();
    }

    //TODO max: cache data
    QStringList result;
    KConfigGroup folders = Amarok::config( "Collection Folders" );
    const IdList ids = getMountedDeviceIds();

    foreach( int id, ids )
    {
        const QStringList rpaths = folders.readEntry( QString::number( id ), QStringList() );
        foreach( const QString &strIt, rpaths )
        {
            const KUrl url = ( strIt == "./" ) ? getMountPointForId( id ) : getAbsolutePath( id, strIt );
            const QString absPath = url.toLocalFile( KUrl::RemoveTrailingSlash );
            if ( !result.contains( absPath ) )
                result.append( absPath );
        }
    }

    // For users who were using QDesktopServices::MusicLocation exclusively up
    // to v2.2.2, which did not store the location into config.
    const KConfigGroup generalConfig = KGlobal::config()->group( "General" );
    if( result.isEmpty() && folders.readEntry( "Use MusicLocation", true )
                         && !generalConfig.readEntry( "First Run", true ) )
    {
        const KUrl musicUrl = QDesktopServices::storageLocation( QDesktopServices::MusicLocation );
        const QString musicDir = musicUrl.toLocalFile( KUrl::RemoveTrailingSlash );
        const QDir dir( musicDir );
        bool useMusicLocation( false );
        if( dir.exists() && dir.isReadable() )
        {
            result << musicDir;
            const_cast<MountPointManager*>(this)->setCollectionFolders( result );
            useMusicLocation = true;
        }
        folders.writeEntry( "Use MusicLocation", useMusicLocation );
    }
    return result;
}

void
MountPointManager::setCollectionFolders( const QStringList &folders )
{
    typedef QMap<int, QStringList> FolderMap;
    KConfigGroup folderConf = Amarok::config( "Collection Folders" );
    FolderMap folderMap;

    foreach( const QString &folder, folders )
    {
        int id = getIdForUrl( folder );
        const QString rpath = getRelativePath( id, folder );
        if( folderMap.contains( id ) ) {
            if( !folderMap[id].contains( rpath ) )
                folderMap[id].append( rpath );
        }
        else
            folderMap[id] = QStringList( rpath );
    }
    //make sure that collection folders on devices which are not in foldermap are deleted
    IdList ids = getMountedDeviceIds();
    foreach( int deviceId, ids )
    {
        if( !folderMap.contains( deviceId ) )
        {
            folderConf.deleteEntry( QString::number( deviceId ) );
        }
    }
    QMapIterator<int, QStringList> i( folderMap );
    while( i.hasNext() )
    {
        i.next();
        folderConf.writeEntry( QString::number( i.key() ), i.value() );
    }
}

void
MountPointManager::migrateStatistics()
{
    QStringList urls = m_storage->query( "SELECT url FROM statistics WHERE deviceid = -2;" );
    foreach( const QString &url, urls )
    {
        if ( QFile::exists( url ) )
        {
            int deviceid = getIdForUrl( url );
            QString rpath = getRelativePath( deviceid, url );
            QString update = QString( "UPDATE statistics SET deviceid = %1, url = '%2'" )
                                      .arg( deviceid )
                                      .arg( m_storage->escape( rpath ) );
            update += QString( " WHERE url = '%1' AND deviceid = -2;" )
                               .arg( m_storage->escape( url ) );
            m_storage->query( update );
        }
    }
}

void
MountPointManager::updateStatisticsURLs( bool changed )
{
    if ( changed )
        QTimer::singleShot( 0, this, SLOT( startStatisticsUpdateJob() ) );
}

void
MountPointManager::startStatisticsUpdateJob()
{
    AMAROK_NOTIMPLEMENTED
    //ThreadWeaver::Weaver::instance()->enqueue( new UrlUpdateJob( this ) );
}

void
MountPointManager::checkDeviceAvailability()
{
    //code to actively scan for devices which are not supported by KDE mediamanager should go here
    //method is not actually called yet
}

void
MountPointManager::deviceAdded( const QString &udi )
{
    DEBUG_BLOCK
    Solid::Predicate predicate = Solid::Predicate( Solid::DeviceInterface::StorageAccess );
    QList<Solid::Device> devices = Solid::Device::listFromQuery( predicate );
    //Looking for a specific udi in predicate seems flaky/buggy; the foreach loop barely
    //takes any time, so just be safe
    bool found = false;
    debug() << "looking for udi " << udi;
    foreach( Solid::Device device, devices )
    {
        if( device.udi() == udi )
        {
            createHandlerFromDevice( device, udi );
            found = true;
        }
    }
    if( !found )
        debug() << "Did not find device from Solid for udi " << udi;
}

void
MountPointManager::deviceRemoved( const QString &udi )
{
    DEBUG_BLOCK
    m_handlerMapMutex.lock();
    foreach( DeviceHandler *dh, m_handlerMap )
    {
        if( dh->deviceMatchesUdi( udi ) )
        {
            int key = m_handlerMap.key( dh );
            m_handlerMap.remove( key );
            delete dh;
            debug() << "removed device " << key;
            m_handlerMapMutex.unlock();
            //we found the medium which was removed, so we can abort the loop
            emit deviceRemoved( key );
            return;
        }
    }
    m_handlerMapMutex.unlock();
}

void MountPointManager::createHandlerFromDevice( const Solid::Device& device, const QString &udi )
{
    DEBUG_BLOCK
    if ( device.isValid() )
    {
        debug() << "Device added and mounted, checking handlers";
        foreach( DeviceHandlerFactory *factory, m_mediumFactories )
        {
            if( factory->canHandle( device ) )
            {
                debug() << "found handler for " << udi;
                DeviceHandler *handler = factory->createHandler( device, udi, m_storage );
                if( !handler )
                {
                    debug() << "Factory " << factory->type() << "could not create device handler";
                    break;
                }
                int key = handler->getDeviceID();
                m_handlerMapMutex.lock();
                if( m_handlerMap.contains( key ) )
                {
                    debug() << "Key " << key << " already exists in handlerMap, replacing";
                    delete m_handlerMap[key];
                    m_handlerMap.remove( key );
                }
                m_handlerMap.insert( key, handler );
                m_handlerMapMutex.unlock();
//                 debug() << "added device " << key << " with mount point " << volumeAccess->mountPoint();
                emit deviceAdded( key );
                break;  //we found the added medium and don't have to check the other device handlers
            }
            else
                debug() << "Factory can't handle device " << udi;
        }
    }
    else
        debug() << "Device not valid!";
}


//UrlUpdateJob

UrlUpdateJob::UrlUpdateJob( QObject *dependent )
    : ThreadWeaver::Job( dependent )
{
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( deleteLater() ) );
}

void
UrlUpdateJob::run()
{
    DEBUG_BLOCK
    updateStatistics();
    updateLabels();
}

void UrlUpdateJob::updateStatistics( )
{
    AMAROK_NOTIMPLEMENTED
#if 0
    SqlStorage *db = CollectionManager::instance()->sqlStorage();
    MountPointManager *mpm = MountPointManager::instance();
    QStringList urls = db->query( "SELECT s.deviceid,s.url "
                                      "FROM statistics AS s LEFT JOIN tags AS t ON s.deviceid = t.deviceid AND s.url = t.url "
                                      "WHERE t.url IS NULL AND s.deviceid != -2;" );
    debug() << "Trying to update " << urls.count() / 2 << " statistics rows";
    oldForeach( urls )
    {
        int deviceid = (*it).toInt();
        QString rpath = *++it;
        QString realURL = mpm->getAbsolutePath( deviceid, rpath );
        if( QFile::exists( realURL ) )
        {
            int newDeviceid = mpm->getIdForUrl( realURL );
            if( newDeviceid == deviceid )
                continue;
            QString newRpath = mpm->getRelativePath( newDeviceid, realURL );

            int statCount = db->query(
                            QString( "SELECT COUNT( url ) FROM statistics WHERE deviceid = %1 AND url = '%2';" )
                                        .arg( newDeviceid )
                                        .arg( db->escape( newRpath ) ) ).first().toInt();
            if( statCount )
                continue;       //statistics row with new URL/deviceid values already exists

            QString sql = QString( "UPDATE statistics SET deviceid = %1, url = '%2'" )
                                .arg( newDeviceid ).arg( db->escape( newRpath ) );
            sql += QString( " WHERE deviceid = %1 AND url = '%2';" )
                                .arg( deviceid ).arg( db->escape( rpath ) );
            db->query( sql );
        }
    }
#endif
}

void UrlUpdateJob::updateLabels( )
{
    AMAROK_NOTIMPLEMENTED

#if 0
    SqlStorage *db = CollectionManager::instance()->sqlStorage();
    MountPointManager *mpm = MountPointManager::instance();
    QStringList labels = db->query( "SELECT l.deviceid,l.url "
                                        "FROM tags_labels AS l LEFT JOIN tags as t ON l.deviceid = t.deviceid AND l.url = t.url "
                                        "WHERE t.url IS NULL;" );
    debug() << "Trying to update " << labels.count() / 2 << " tags_labels rows";
    oldForeach( labels )
    {
        int deviceid = (*it).toInt();
        QString rpath = *++it;
        QString realUrl = mpm->getAbsolutePath( deviceid, rpath );
        if( QFile::exists( realUrl ) )
        {
            int newDeviceid = mpm->getIdForUrl( realUrl );
            if( newDeviceid == deviceid )
                continue;
            QString newRpath = mpm->getRelativePath( newDeviceid, realUrl );

            //only update rows if there is not already a row with the new deviceid/rpath and the same labelid
            QStringList labelids = db->query(
                                        QString( "SELECT labelid FROM tags_labels WHERE deviceid = %1 AND url = '%2';" )
                                                 .arg( QString::number( newDeviceid ), db->escape( newRpath ) ) );
            QString existingLabelids;
            if( !labelids.isEmpty() )
            {
                existingLabelids = " AND labelid NOT IN (";
                oldForeach( labelids )
                {
                    if( it != labelids.constBegin() )
                        existingLabelids += ',';
                    existingLabelids += *it;
                }
                existingLabelids += ')';
            }
            QString sql = QString( "UPDATE tags_labels SET deviceid = %1, url = '%2' "
                                    "WHERE deviceid = %3 AND url = '%4'%5;" )
                                    .arg( newDeviceid )
                                    .arg( db->escape( newRpath ),
                                          QString::number( deviceid ),
                                          db->escape( rpath ),
                                          existingLabelids );
            db->query( sql );
        }
    }
#endif
}

#include "MountPointManager.moc"
