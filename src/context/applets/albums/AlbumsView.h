/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_ALBUMSVIEW_H
#define AMAROK_ALBUMSVIEW_H

#include "core/meta/Meta.h"

#include <QGraphicsWidget>
#include <QStyledItemDelegate>

class QAbstractItemModel;
class QGraphicsSceneContextMenuEvent;
class QTreeView;
namespace Plasma
{
    class ScrollBar;
}

class AlbumsView : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY( QAbstractItemModel* model READ model WRITE setModel )
    Q_PROPERTY( QTreeView* nativeWidget READ nativeWidget )

public:
    explicit AlbumsView( QGraphicsWidget *parent = 0 );
    ~AlbumsView();

    /**
     * Sets a model for this weather view
     *
     * @arg model the model to display
     */
    void setModel( QAbstractItemModel *model );

    /**
     * @return the model shown by this view
     */
    QAbstractItemModel *model() const;

    /**
     * @return the native widget wrapped by this AlbumsView
     */
    QTreeView* nativeWidget() const;

public slots:
    void setRecursiveExpanded( const QModelIndex &index, bool expanded );

protected:
    void contextMenuEvent( QGraphicsSceneContextMenuEvent *event );
    void resizeEvent( QGraphicsSceneResizeEvent *event );
    
private slots:
    void itemClicked( const QModelIndex &index );
    void slotAppendSelected();
    void slotEditSelected();
    void slotPlaySelected();
    void slotQueueSelected();
    void slotScrollBarRangeChanged( int min, int max );

private:
    void updateScrollBarVisibility();

    Meta::TrackList getSelectedTracks() const;
    QTreeView *m_treeView;
    Plasma::ScrollBar *m_scrollBar;
};

class AlbumsItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    AlbumsItemDelegate( QObject *parent = 0 ) : QStyledItemDelegate( parent ) {}
    ~AlbumsItemDelegate() {}

    void paint( QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index ) const;

private:
    void drawAlbumText( QPainter *p, const QStyleOptionViewItemV4 &option ) const;
    void drawTrackText( QPainter *p, const QStyleOptionViewItemV4 &option ) const;
    void applyCommonStyle( QPainter *p, const QStyleOptionViewItemV4 &option ) const;
};

#endif // multiple inclusion guard
