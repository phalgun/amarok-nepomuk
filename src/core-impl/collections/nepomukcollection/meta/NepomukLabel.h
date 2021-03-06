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

#ifndef NEPOMUKLABEL_H
#define NEPOMUKLABEL_H

#include "core/meta/Meta.h"

namespace Meta
{
/**
 * Represents a unit label/tag resource in Amarok
 */
class NepomukLabel : public Label
{
public:
    NepomukLabel( const QString &name );
    virtual QString name() const;

private:
    QString m_name;
};

}
#endif // NEPOMUKLABEL_H
