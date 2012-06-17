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

#ifndef AMAROK_MOUNTPOINTMANAGER_H
#define AMAROK_MOUNTPOINTMANAGER_H

#include "core/support/Amarok.h"
#include "core/support/PluginFactory.h"
#include "shared/amarok_export.h"

#include <KConfigGroup>
#include <KUrl>
#include <KPluginInfo>
#include <solid/device.h>
#include <threadweaver/Job.h>

#include <QMap>
#include <QMutex>
#include <QStringList>

class DeviceHandler;
class DeviceHandlerFactory;
class SqlStorage;

typedef QList<int> IdList;
typedef QList<DeviceHandlerFactory*> FactoryList;
typedef QMap<int, DeviceHandler*> HandlerMap;

class AMAROK_EXPORT DeviceHandlerFactory : public Plugins::PluginFactory
{
    Q_OBJECT

public:
    DeviceHandlerFactory( QObject *parent, const QVariantList &args );
    virtual ~DeviceHandlerFactory() {}

    /**
     * checks whether a DeviceHandler subclass can handle a given Medium.
     * @param volume the connected solid volume
     * @return true if the DeviceHandler implementation can handle the medium,
     * false otherwise
     */
    virtual bool canHandle( const Solid::Device &device ) const = 0;

    /**
     * tells the MountPointManager whether it makes sense to ask the factory to
     * create a Devicehandler when a new Medium was connected
     * @return true if the factory can create DeviceHandlers from Medium instances
     */
    virtual bool canCreateFromMedium() const = 0;

    /**
     * creates a DeviceHandler which represents the Medium.
     * @param volume the Volume for which a DeviceHandler is required
     * @return a DeviceHandler or 0 if the factory cannot handle the Medium
     */
    virtual DeviceHandler* createHandler( const Solid::Device &device, const QString &udi, SqlStorage *s ) const = 0;

    virtual bool canCreateFromConfig() const = 0;

    virtual DeviceHandler* createHandler( KSharedConfigPtr c, SqlStorage *s ) const = 0;

    /**
     * returns the type of the DeviceHandler. Should be the same as the value used in
     * ~/.kde/share/config/amarokrc
     * @return a QString describing the type of the DeviceHandler
     */
    virtual QString type() const = 0;
};

/**
 *
 *
 */
class AMAROK_EXPORT DeviceHandler
{
public:
    DeviceHandler() {}
    virtual ~DeviceHandler() {}


    virtual bool isAvailable() const = 0;

    /**
     * returns the type of the DeviceHandler. Should be the same as the value used in
     * ~/.kde/share/config/amarokrc
     * @return a QString describing the type of the DeviceHandler
     */
    virtual QString type() const = 0;

    /**
     * returns an absolute path which is guaranteed to be playable by amarok's current engine. (based on an
     * idea by andrewt512: this method would only be called when we actually want to play the file, not when we
     * simply want to show it to the user. It could for example download a file using KIO and return a path to a
     * temporary file. Needs some more thought and is not actually used at the moment.
     * @param absolutePath
     * @param relativePath
     */
    virtual void getPlayableURL( KUrl &absolutePath, const KUrl &relativePath ) = 0;

    /**
     * builds an absolute path from a relative path and DeviceHandler specific information. The absolute path
     * is not necessarily playable! (based on an idea by andrewt512: allows better handling of files stored in remote  * collections. this method would return a "pretty" URL which might not be playable by amarok's engines.
     * @param absolutePath the not necessarily playbale absolute path
     * @param relativePath the device specific relative path
     */
    virtual void getURL( KUrl &absolutePath, const KUrl &relativePath ) = 0;

    /**
     * retrieves the unique database id of a given Medium. Implementations are responsible
     * for generating a (sufficiently) unique value which identifies the Medium.
     * Additionally, implementations must recognize unknown mediums and store the necessary
     * information to recognize them the next time they are connected in the database.
     * @return unique identifier which can be used as a foreign key to the media table.
     */
    virtual int getDeviceID() = 0;

    virtual const QString &getDevicePath() const = 0;

    /**
     * allows MountPointManager to check if a device handler handles a specific medium.
     * @param m
     * @return true if the device handler handles the Medium m
     */
    virtual bool deviceMatchesUdi( const QString &udi ) const = 0;
};

/**
 *	@author Maximilian Kossick <maximilian.kossick@googlemail.com>
 */
class AMAROK_EXPORT MountPointManager : public QObject
{
    Q_OBJECT

public:
    MountPointManager( QObject *parent, SqlStorage *storage );
    ~MountPointManager();

    /**
     *
     * @param url
     * @return
     */
    virtual int getIdForUrl( const KUrl &url );

    /**
     *
     * @param id
     * @return
     */
    virtual QString getMountPointForId( const int id ) const;

    /**
     * builds the absolute path from the mount point of the medium and the given relative
     * path.
     * @param deviceId the medium(device)'s unique id
     * @param relativePath relative path on the medium
     * @return the absolute path
     */
    virtual QString getAbsolutePath( const int deviceId, const QString& relativePath ) const;

    /**
     * calculates a file's/directory's relative path on a given device.
     * @param deviceId the unique id which identifies the device the file/directory is supposed to be on
     * @param absolutePath the file's/directory's absolute path
     * @param relativePath the calculated relative path
     */
    virtual QString getRelativePath( const int deviceId, const QString& absolutePath ) const;

    /**
     * allows calling code to access the ids of all active devices
     * @return the ids of all devices which are currently mounted or otherwise accessible
     */
    virtual IdList getMountedDeviceIds() const;

    virtual QStringList collectionFolders() const;
    virtual void setCollectionFolders( const QStringList &folders );

    void loadDevicePlugins( const QList<Plugins::PluginFactory*> &factories );

public slots:
//     void mediumAdded( const Medium *m );
//     /**
//      * initiates the update of the class' internal list of mounted mediums.
//      * @param m the medium whose status changed
//      */
//     void mediumChanged( const Medium* m );
//     void mediumRemoved( const Medium* m );

    virtual void updateStatisticsURLs( bool changed = true );

signals:
    void deviceAdded( int id );
    void deviceRemoved( int id );

private slots:
    void migrateStatistics();
    void checkDeviceAvailability();
    void startStatisticsUpdateJob();

private:

    /**
     * checks whether a medium identified by its unique database id is currently mounted.
     * Note: does not handle deviceId = -1! It only checks real devices
     * @param deviceId the mediums unique id
     * @return true if the medium is mounted, false otherwise
     */
    bool isMounted ( const int deviceId ) const;

    SqlStorage *m_storage;
    /**
     * maps a device id to a mount point. does only work for mountable filesystems and needs to be
     * changed for the real Dynamic Collection implementation.
     */
    HandlerMap m_handlerMap;
    mutable QMutex m_handlerMapMutex;
    FactoryList m_mediumFactories;
    FactoryList m_remoteFactories;
    bool m_ready;

//Solid specific
    void createHandlerFromDevice( const Solid::Device &device, const QString &udi );
private slots:
    void deviceAdded( const QString &udi );
    void deviceRemoved( const QString &udi );

};

class UrlUpdateJob : public ThreadWeaver::Job
{
public:
    UrlUpdateJob( QObject *dependent );

    virtual void run();

private:
    void updateStatistics();
    void updateLabels();
};

#define AMAROK_EXPORT_DEVICE_PLUGIN(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("amarok_device_" #libname))\


#endif
