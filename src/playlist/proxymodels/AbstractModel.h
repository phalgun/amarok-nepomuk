/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
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

#ifndef AMAROK_PLAYLISTABSTRACTMODEL_H
#define AMAROK_PLAYLISTABSTRACTMODEL_H

#include "core/meta/Meta.h"
#include "playlist/PlaylistDefines.h"
#include "playlist/PlaylistItem.h"

#include <QAbstractItemModel>

namespace Playlist
{

/**
 * An abstract base class that defines a common interface of any playlist model.
 * Members declared here must be implemented by Playlist::Model and all proxies.
 * @author Téo Mrnjavac <teo@kde.org>
 */

//NOTE: While AbstractModel is ideally and logically an abstract base class with all pure
//      virtual methods, there are a few exceptions.
//      Some of the public methods are virtual but not pure. The implementations of those
//      are inline, and are dummies (if they return anything, they return default values).
//      Almost all of them are reimplemented in Playlist::ProxyBase anyway, so the only
//      reason why they are not pure virtual is that this would require the dummies to
//      exist in Playlist::Model, which just looks wrong.
//      These methods are needed because even though they don't do anything in Playlist::
//      Model, they are important parts of the interface that a complete playlist model
//      must implement. Non trivial implementations are handled by the proxies, and they
//      are used in the view(s), Playlist::Controller, Playlist::Actions and probably in
//      many other places.  --Téo 19/7/2009

class AbstractModel
{
public:

    //! Management of objects and inheritance

    /**
     * Virtual destructor
     * (Make it OK to delete an instance of a derived class through a pointer to this base class.)
     */
    virtual ~AbstractModel() { };

    /**
     * If you need QAbstractItemModel functions, access them through this pointer.
     *
     * This is a work-around for Qt's poor support for multiple inheritance and/or templates.
     * If Qt supported the full C++ inheritance system well, this class could have inherited
     * from QAbstractItemModel, and our descendants could have inherited from more specific
     * implementations of QAbstractItemModel, like QSortFilterProxyModel.
     * In the current Qt reality, that causes clashes.
     */
    virtual QAbstractItemModel* qaim() const = 0;


    //! Playlist-specific API; the functions QAbstractItemModel doesn't already offer.

    /**
     * Returns the unique playlist item id of the active track
     * (or 0 if no track is active).
     * @return The playlist item's id.
     */
    virtual quint64 activeId() const = 0;

    /**
     * Returns the currently active row, translated to proxy rows
     * (or -1 if the current row is not represented by this proxy).
     * @return The currently active (playing) row in proxy terms.
     */
    virtual int activeRow() const = 0;

    /**
     * Returns a pointer to the currently active track (or a default constructed value if
     * no track is active).
     * @return A pointer to the track.
     */
    virtual Meta::TrackPtr activeTrack() const = 0;

    /**
     * Returns all rows in the current model which match a given track pointer.
     * @see firstRowForTrack
     * @param track the track.
     * @return collection of rows, empty if the track pointer is invalid.
     */
    virtual QSet<int> allRowsForTrack( const Meta::TrackPtr track ) const = 0;

    /**
     * Clears the current search term.
     *
     * Filtering MUST adapt its filter parameters to this call, but SHOULD delay doing
     * any work until it gets a 'filterUpdated()' call.
     */
    virtual void clearSearchTerm() {}    //dummy, needed by Playlist::Model

    /**
     * Reports if the current model exposes a given track.
     * @param track the track to check for.
     * @return true if the track is present, otherwise false.
     */
    virtual bool containsTrack( const Meta::TrackPtr track ) const = 0;

    /**
     * Get the current search fields bitmask.
     * @return The current search fields.
     */
    virtual int currentSearchFields() { return -1; } //dummy, needed by Playlist::Model

    /**
     * Get the current search term.
     * @return The curent search term.
     */
    virtual QString currentSearchTerm() { return QString(); }   //dummy, needed by Playlist::Model

    /**
     * Saves a playlist to a specified location.
     * @param path the path of the playlist file, as chosen by a FileDialog in MainWindow.
     * @param relative use relative paths. Defaults to false, uses absolute paths.
     */
    virtual bool exportPlaylist( const QString &path, bool relative = false ) const = 0;

    /**
     * Notify FilterProxy that the search term of searched fields has changed. Since this
     * call does not use the parent's filter values, this method needs to be called when the
     * values change.
     */
    virtual void filterUpdated() {}

    /**
     * Forwards a search down through the stack of ProxyModels.
     * Find the first track in the playlist that matches the search term in one of the
     * specified search fields. Playlist::Model::find() emits found() or notFound() depending
     * on whether a match is found.
     *
     * Filtering MUST take its filter parameters from this call, but SHOULD delay doing
     * any work until it gets a 'filterUpdated()' call.
     *
     * @param searchTerm The term to search for.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match, -1 if no match is found.
     */
    virtual int find( const QString &, int ) { return -1; }

    /**
     * Forwards through the stack of ProxyModels a top to bottom search for the next item.
     * Find the first track below a given row that matches the search term in one of the
     * specified search fields. Playlist::Model::findNext() emits found() or notFound()
     * depending on whether a match is found. If no row is found below the current row, the
     * function wraps around and returns the first match. If no match is found at all, -1
     * is returned.
     * @param searchTerm The term to search for.
     * @param selectedRow The offset row.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match below the offset, -1 if no match is found.
     */
    virtual int findNext( const QString &, int, int ) { return -1; }

    /**
     * Forwards through the stack of ProxyModels a bottom to top search for the next item.
     * Find the first track above a given row that matches the search term in one of the
     * specified search fields. Playlist::Model::findPrevious() emits found() or notFound()
     * depending on whether a match is found. If no row is found above the current row, the
     * function wraps around and returns the last match. If no match is found at all, -1
     * is returned.
     * @param searchTerm The term to search for.
     * @param selectedRow The offset row.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match above the offset, -1 if no match is found.
     */
    virtual int findPrevious( const QString &, int, int ) {return -1; }

    /**
     * Returns the first row in the current model which matches a given track pointer.
     * @see allRowsForTrack
     * @param track the track.
     * @return the row, or -1 if the track pointer is not found.
     */
    virtual int firstRowForTrack( const Meta::TrackPtr track ) const = 0;

    /**
     * Returns the unique 64-bit id for the given row in the current model.
     * @param row the row.
     * @return the unique id.
     */
    virtual quint64 idAt( const int row ) const = 0;

    /**
     * Checks if a row exists in the current model or proxy.
     * @param row the row in the model or proxy.
     * @return true is the row exists, otherwise false.
     */
    virtual bool rowExists( int row ) const = 0;

    /**
     * Returns the row in the current model for a given unique 64-bit id.
     * @param id the id.
     * @return the row, -1 if the id is invalid.
     */
    virtual int rowForId( const quint64 id ) const = 0;

    /**
     * Returns the row number of a track given the row number in the bottom model.
     * @param row the row in the bottom model.
     * @return the row in a proxy model
     */
    virtual int rowFromBottomModel( const int row ) = 0;

    /**
     * Returns the row number of a track in terms of the bottom model.
     * @param row the row in a proxy model
     * @return the row in the bottom model.
     */
    virtual int rowToBottomModel( const int row ) = 0;

    /**
     * Set the currently active track based on the playlist id given.
     * @param id the unique playlist id.
     */
    virtual void setActiveId( const quint64 id ) = 0;

    /**
     * Sets the currently active (playing) row, translated for this proxy.
     * @param row the row to be set as active.
     */
    virtual void setActiveRow( int row ) = 0;

    /**
     * Sets to uplayed the state of all the tracks exposed by this proxy.
     */
    virtual void setAllUnplayed() = 0;

    /**
     * Emit the queueChanged() signal. Call this after changing the queue in PlaylistActions.
     */
    virtual void emitQueueChanged() = 0;

    /**
     * Return position of @p row in the playlist queue, zero if not queued.
     */
    virtual int queuePositionOfRow( int row ) = 0;

    /**
     * Decides if FilterProxy or SearchProxy should be used.
     * @param onlyMatches true if one wants to use SearchProxy, false otherwise.
     */
    virtual void showOnlyMatches( bool ) {}

    /**
     * Get the state of a track by its id.
     * @param id The id of the track.
     * @return The state of the track.
     */
    virtual Item::State stateOfId( quint64 id ) const = 0;

    /**
     * Get the sate of the track at given row in the proxy model.
     * @param row The row in proxy terms.
     * @return The state of the track at the row.
     */
    virtual Item::State stateOfRow( int row ) const = 0;

    /**
     * Asks the model sitting below the total length of the playlist.
     * @return the total length of the playlist in milliseconds.
     */
    virtual qint64 totalLength() const = 0;

    /**
     * Asks the model sitting below the total size of the playlist.
     * @return the total size of the playlist.
     */
    virtual quint64 totalSize() const = 0;

    /**
     * Returns a pointer to the track at a given row.
     * @param row the row to return the track pointer for.
     * @return a pointer to the track at the given row.
     */
    virtual Meta::TrackPtr trackAt( int row ) const = 0;

    /**
     * Returns a pointer to the track with the given unique id.
     * @param id the id to return the track pointer for.
     * @return a pointer to the track with the given id.
     */
    virtual Meta::TrackPtr trackForId( const quint64 id ) const = 0;

    /**
     * Returns an ordered list of tracks exposed by the current model.
     * @return the tracklist.
     */
    virtual Meta::TrackList tracks() const = 0;
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTABSTRACTMODEL_H
