/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
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

#define DEBUG_PREFIX "Constraint::PreventDuplicates"

#include "PreventDuplicates.h"
#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include "core/meta/Meta.h"

#include <QSet>

#include <math.h>

Constraint*
ConstraintTypes::PreventDuplicates::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p )
        return new PreventDuplicates( xmlelem, p );
    else
        return 0;
}

Constraint*
ConstraintTypes::PreventDuplicates::createNew( ConstraintNode* p )
{
    if ( p )
        return new PreventDuplicates( p );
    else
        return 0;
}

ConstraintFactoryEntry*
ConstraintTypes::PreventDuplicates::registerMe()
{
    return new ConstraintFactoryEntry( "PreventDuplicates",
                                       i18n("Prevent Duplicates"),
                                       i18n("Prevents duplicate tracks, albums, or artists from appearing in the playlist"),
                                       &PreventDuplicates::createFromXml, &PreventDuplicates::createNew );
}

ConstraintTypes::PreventDuplicates::PreventDuplicates( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
{
    QDomAttr a;

    a = xmlelem.attributeNode( "field" );
    if ( !a.isNull() ) {
        m_field = static_cast<DupeField>( a.value().toInt() );
    }
}

ConstraintTypes::PreventDuplicates::PreventDuplicates( ConstraintNode* p )
        : Constraint( p )
        , m_field( DupeTrack )
{
}

QWidget*
ConstraintTypes::PreventDuplicates::editWidget() const
{
    PreventDuplicatesEditWidget* e = new PreventDuplicatesEditWidget( m_field );
    connect( e, SIGNAL( fieldChanged( const int ) ), this, SLOT( setField( const int ) ) );
    return e;
}

void
ConstraintTypes::PreventDuplicates::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( "constraint" );
    c.setAttribute( "type", "PreventDuplicates" );
    c.setAttribute( "field", QString::number( m_field ) );
    elem.appendChild( c );
}

QString
ConstraintTypes::PreventDuplicates::getName() const
{
    switch ( m_field ) {
        case DupeTrack:
            return i18n("Prevent duplicate tracks");
        case DupeArtist:
            return i18n("Prevent duplicate artists");
        case DupeAlbum:
            return i18n("Prevent duplicate albums");
    }
    return QString();
}

double
ConstraintTypes::PreventDuplicates::satisfaction( const Meta::TrackList& tl ) const
{
    int d = 0;
    QSet<Meta::TrackPtr> tracks;
    QSet<Meta::AlbumPtr> albums;
    QSet<Meta::ArtistPtr> artists;
    switch ( m_field ) {
        case DupeTrack:
            foreach( Meta::TrackPtr t, tl ) {
                if ( tracks.contains(t) ) {
                    d++;
                } else {
                    tracks.insert(t);
                }
            }
            break;
        case DupeAlbum:
            foreach( Meta::TrackPtr t, tl ) {
                if ( albums.contains(t->album()) ) {
                    d++;
                } else {
                    albums.insert(t->album());
                }
            }
            break;
        case DupeArtist:
            foreach( Meta::TrackPtr t, tl ) {
                if ( artists.contains(t->artist()) ) {
                    d++;
                } else {
                    artists.insert(t->artist());
                }
            }
            break;
    }
            
    return exp( (double)d / -3.0 );
}

void
ConstraintTypes::PreventDuplicates::setField( const int c )
{
    m_field = static_cast<DupeField>( c );
    emit dataChanged();
}


/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::PreventDuplicatesEditWidget::PreventDuplicatesEditWidget( const int field )
    : QWidget( 0 )
{
    ui.setupUi( this );
    ui.comboBox_Field->setCurrentIndex( field );
}

void
ConstraintTypes::PreventDuplicatesEditWidget::on_comboBox_Field_currentIndexChanged( const int v )
{
    emit fieldChanged( v );
    emit updated();
}
