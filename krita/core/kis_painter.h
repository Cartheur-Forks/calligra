/*
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
#if !defined KIS_PAINTER_H_
#define KIS_PAINTER_H_

#include <qbrush.h>
#include <qcolor.h>
#include <qfontinfo.h>
#include <qfontmetrics.h>
#include <qpen.h>
#include <qpointarray.h>
#include <qregion.h>
#include <qwmatrix.h>
#include <qimage.h>
#include <qmap.h>
#include <qpixmap.h>
#include <qpointarray.h>
#include <qrect.h>
#include <qstring.h>
#include <qpainter.h>

#include <koColor.h>

#include "kis_global.h"
#include "kis_types.h"
#include "kis_paint_device.h"
#include "kis_gradient.h"
#include "kis_brush.h"
#include "kis_pattern.h"

class QRect;
class KCommand;
class KMacroCommand;

/*
  KisPainter contains the graphics primitives necessary to draw on a
  KisPaintDevice. This is the same kind of abstraction as used in Qt
  itself, where you have QPainter and QPaintDevice.
  
  However, KisPainter works on a tiled image and supports different
  colour models, and that's a lot more complicated.
 
  Usage: 

    KisImageSP img = m_view -> currentImg();
    if (img) {
        KisPaintDeviceSP device = img -> activeDevice();
        if (device) {
            KisPainter *p = new KisPainter();
            p->begin(device);
            // Do your painting, frex:
            p->fillRect( x,  y,  10,  10,  KoColor(QColor( "red" )) );
            // Finish painting -- at least that's true with QPainter,
            // here, it might have something do with transactions.
            p->end();
            // Register do/undo information
            m_doc -> addCommand(new TestCmd(img));
            // This somehow, despite being an empty method, makes
            // sure the window gets updated.
            device->anchor();
         }

    }
    // ???
    img -> invalidate(QRect( x,  y,  10,  10 ));
    // ???
    m_view -> updateCanvas();

  Note that I currently think that it's necessary to call anchor,
  invalidate and updateCanvas to make sure the painted bits show up on
  the screen; I don't know why, I don't know whether that's true, but
  it seems to work.

  KisPainter supports transactions that can group various paint operations
  in one undoable step.

 */
class KisPainter {
public:
	KisPainter();
	KisPainter(KisPaintDeviceSP device);
	~KisPainter();

public:
        // Start painting on the specified device.
        void begin(KisPaintDeviceSP device);

        // ??? 
	KCommand *end();

        // ???
	void beginTransaction(KMacroCommand *command);

        // ???
	void beginTransaction(const QString& customName = QString::null);

        // ???
	KCommand *endTransaction();


        // The current paint device.
	KisPaintDeviceSP device() const;

        // ----------------------------------------------------------------------------------------
        // Native paint methods that are tile aware, undo/redo-able,
        // use the color strategies and the composite operations.

        // Blast the specified region from src onto the current paint device.
	void bitBlt(Q_INT32 dx, Q_INT32 dy, CompositeOp op, KisPaintDeviceSP src,
                    Q_INT32 sx = 0, Q_INT32 sy = 0, Q_INT32 sw = -1, Q_INT32 sh = -1);

        // Overloaded version of the previous, differs in that it is possible to specify
        // a value for opacity
	void bitBlt(Q_INT32 dx, Q_INT32 dy, CompositeOp op, KisPaintDeviceSP src,
                    QUANTUM opacity, 
                    Q_INT32 sx = 0, Q_INT32 sy = 0, Q_INT32 sw = -1, Q_INT32 sh = -1);

	void eraseRect(Q_INT32 x1, Q_INT32 y1, Q_INT32 w, Q_INT32 h);
	void eraseRect(const QRect& rc);

        // Fill a rectangle with a certain color
	void fillRect(Q_INT32 x, Q_INT32 y, Q_INT32 w, Q_INT32 h, const KoColor& c);
	void fillRect(const QRect& rc, const KoColor& c);
	void fillRect(Q_INT32 x, Q_INT32 y, Q_INT32 w, Q_INT32 h, const KoColor& c, QUANTUM opacity);
	void fillRect(const QRect& rc, const KoColor& c, QUANTUM opacity);

	// Draw a point with the specified brush in the specified color at x, y
	// XXX: brush, color, gradient, pattern etc. should be set
	// as in QPainter.
	void drawPoint(Q_INT32 x, Q_INT32 y, const KoColor &c, const KisBrush &brush);
	void drawPoint(const QPoint &p,  const KoColor &c, const KisBrush &brush);


        // ----------------------------------------------------------------------------------------
        // QPainter-using methods.

        // XXX: we use a simple black pen for everything, for now.
        // XXX: we actually use QPainter for everything.
        // XXX: we even use QPixmaps, for added sluggishness.
        // XXX: we don't even think about compositing.
        // XXX: let alone colour strategies.

        void drawPolyline ( const QPointArray & polyline, 
                            const QColor & c );

        // ----------------------------------------------------------------------------------------

private:
	void tileBlt(QUANTUM *dst, KisTileSP dsttile, QUANTUM *src,
                     KisTileSP srctile, Q_INT32 rows, Q_INT32 cols, 
                     CompositeOp op);
	void tileBlt(QUANTUM *dst, KisTileSP dsttile, QUANTUM *src,
                     KisTileSP srctile, QUANTUM opacity, Q_INT32 rows, Q_INT32 cols, 
                     CompositeOp op);
	KisPainter(const KisPainter&);
	KisPainter& operator=(const KisPainter&);

private:
	KisPaintDeviceSP m_device;	
	KMacroCommand *m_transaction;


};

inline
void KisPainter::fillRect(Q_INT32 x, Q_INT32 y, Q_INT32 w, Q_INT32 h, const KoColor& c)
{
	fillRect(x, y, w, h, c, OPACITY_OPAQUE);
}

inline
void KisPainter::fillRect(const QRect& rc, const KoColor& c)
{
	fillRect(rc.x(), rc.y(), rc.width(), rc.height(), c, OPACITY_OPAQUE);
}

inline
void KisPainter::eraseRect(Q_INT32 x1, Q_INT32 y1, Q_INT32 w, Q_INT32 h)
{
	fillRect(x1, y1, w, h, KoColor::black(), OPACITY_TRANSPARENT);
}

inline
void KisPainter::eraseRect(const QRect& rc)
{
	fillRect(rc, KoColor::black(), OPACITY_TRANSPARENT);
}

inline
void KisPainter::fillRect(const QRect& rc, const KoColor& c, QUANTUM opacity)
{
	fillRect(rc.x(), rc.y(), rc.width(), rc.height(), c, opacity);
}

inline
void KisPainter::drawPoint(const QPoint &p,  const KoColor &c, const KisBrush &brush) 
{
	drawPoint(p.x(), p.y(), c, brush);
}


inline
KisPaintDeviceSP KisPainter::device() const
{
	return m_device;
}

#endif // KIS_PAINTER_H_

