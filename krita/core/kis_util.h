/*
 *  kis_util.h - part of Krayon
 *
 *  Copyright (c) 1999 Matthias Elter <me@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __kis_util_h__
#define __kis_util_h__

#include <qrect.h>
#include <qpoint.h>
#include <qstring.h>
#include <qimage.h>

#include "kis_global.h"

class KisUtil {
public:
    static void printRect( const QRect&, const QString& name = "Rect" );
    static void printPoint( const QPoint&, const QString& name = "Point" );

    static void enlargeRectToContainPoint( QRect& r, QPoint p );
    static QRect findTileExtents(QRect r );

//    static QString channelIdtoString(cId);

    static QImage roughScaleQImage(QImage & src, int width, int height);
    static QRect findBoundingTiles(const QRect& area);
//    static int calcNumChannels(cMode cm);
};

#endif
