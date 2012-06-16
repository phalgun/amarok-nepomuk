/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#include <Nepomuk/ResourceManager>

#include "NepomukCollectionFactory.h"

void
NepomukCollectionFactory::init()
{
    // check if Nepomuk service is already initialized
    if ( Nepomuk::ResourceManager::instance()->initialized() )
    {

    }

    else
    {
        // Nepomuk not initialized, so initiate it
        if ( Nepomuk::ResourceManager::instance()->init() )
        {

        }

        else
        {
            //TODO:
            // Generate appropriate warning since Nepomuk not found/enabled
        }
    }
}

NepomukCollectionFactory::NepomukCollectionFactory( QObject *parent, const QVariantList &args )
 : CollectionFactory( parent, args)
{

}

NepomukCollectionFactory::~NepomukCollectionFactory()
{

}
