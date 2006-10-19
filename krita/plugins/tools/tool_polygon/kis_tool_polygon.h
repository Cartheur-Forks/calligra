/*
 *  kis_tool_polygon.h - part of Krita
 *
 *  Copyright (c) 2004 Michael Thaler <michael Thaler@physik.tu-muenchen.de>
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

#ifndef KIS_TOOL_POLYGON_H_
#define KIS_TOOL_POLYGON_H_

#include <q3valuevector.h>
//Added by qt3to4:
#include <QKeyEvent>

#include "kis_tool_shape.h"

class KisCanvas;
class KisPainter;
class KisRect;

class KisToolPolygon : public KisToolShape {

    typedef KisToolShape super;
    Q_OBJECT

public:
    KisToolPolygon();
    virtual ~KisToolPolygon();

    //
    // KisCanvasObserver interface
    //

    virtual void update (KisCanvasSubject *subject);

    //
    // KisToolPaint interface
    //

    virtual void setup(KActionCollection *collection);
    virtual enumToolType toolType() { return TOOL_SHAPE; }
    virtual quint32 priority() { return 4; }
    virtual void buttonPress(KoPointerEvent *event);
    virtual void move(KoPointerEvent *event);
    virtual void buttonRelease(KisButtonReleaseEvent *event);
    virtual QString quickHelp() const {
        return i18n("Shift-click will end the polygon.");
    }
    virtual void doubleClick(KoPointerEvent * event);

protected:
    virtual void paint(QPainter& gc);
    virtual void paint(QPainter& gc, const QRect& rc);
    void draw(QPainter& gc);
    void draw();
    void finish();
    virtual void keyPress(QKeyEvent *e);
protected:
    KoPoint m_dragStart;
    KoPoint m_dragEnd;

    bool m_dragging;
    KisImageSP m_currentImage;
private:
    typedef Q3ValueVector<KoPoint> KoPointVector;
    KoPointVector m_points;
};


#include "kis_tool_factory.h"

class KisToolPolygonFactory : public KisToolFactory {
    typedef KisToolFactory super;
public:
    KisToolPolygonFactory() : super() {};
    virtual ~KisToolPolygonFactory(){};

    virtual KisTool * createTool(KActionCollection * ac) {
        KisTool * t =  new KisToolPolygon();
        Q_CHECK_PTR(t);
        t->setup(ac);
        return t;
    }
    virtual KoID id() { return KoID("polygon", i18n("Polygon Tool")); }
};


#endif //__KIS_TOOL_POLYGON_H__
