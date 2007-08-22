/*
 * Copyright (C) 2007 Boudewijn Rempt <boud@valdyas.org>, (C)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef _KIS_ABSTRACT_CANVAS_WIDGET_
#define _KIS_ABSTRACT_CANVAS_WIDGET_

class QWidget;
class QRect;
class QPoint;
class QImage;
class QPainter;

class KoToolProxy;

class KisCanvas2;
class KisGridDrawer;

class KisAbstractCanvasWidget {

public:

    KisAbstractCanvasWidget() {}

    virtual ~KisAbstractCanvasWidget() {}

    virtual QWidget * widget() = 0;

    virtual KoToolProxy * toolProxy() = 0;

    virtual void documentOffsetMoved( QPoint ) = 0;

    /**
     * Draw the specified decorations on the view.
     */
    void drawDecorations( QPainter & gc, bool ants, bool grids, bool tools,
                          const QPoint & documentOffset,
                          const QRect & clipRect,
                          KisCanvas2 * canvas, KisGridDrawer * gridDrawer );

    /**
     * Prescale the canvas represention of the image (if necessary, it
     * is for QPainter, not for OpenGL).
     */
    virtual void preScale() {}

    /**
     * Prescale the canvas represetation of the image.
     *
     * @param rc The target rect in view coordinates of the prescaled
     * image, pre-translated with the document offset
     */
    virtual void preScale( const QRect & rc ) { Q_UNUSED( rc ); }

    /**
     * Returns one check of the background checkerboard pattern.
     *
     * @param checkSize the size of the check
     */
    QImage checkImage(qint32 checkSize);
};

#endif // _KIS_ABSTRACT_CANVAS_WIDGET_
