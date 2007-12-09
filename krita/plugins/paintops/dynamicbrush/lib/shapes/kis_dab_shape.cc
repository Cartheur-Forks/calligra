/*
 *  Copyright (c) 2006-2007 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_dab_shape.h"

#include <kis_autobrush_resource.h>
#include <kis_iterators_pixel.h>
#include <kis_paint_device.h>
#include <kis_painter.h>

#include "kis_dynamic_coloring.h"

#if 0

quint8 KisAlphaMaskShape::alphaAt(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    return 255;
//     return alphaMask->alphaAt(x,y);
}

quint8 KisAutoMaskShape::alphaAt(int x, int y)
{
    return 255 - m_shape->valueAt(x,y);
}

KisAutoMaskShape::~KisAutoMaskShape() { if(m_shape) delete m_shape; }


KisAlphaMaskShape::KisAlphaMaskShape() {}

#endif

KisDabShape::~KisDabShape() { }
KisDabShape::KisDabShape(KisBrush* brush) : m_scaleX(1.0), m_scaleY(1.0), m_rotate(0.0), m_dab(0), m_brush(brush) { }

KisDynamicShape* KisDabShape::clone() const
{
    return new KisDabShape(*this);
}

inline void splitCoordinate(double coordinate, qint32 *whole, double *fraction)
{
    qint32 i = static_cast<qint32>(coordinate);

    if (coordinate < 0) {
        // We always want the fractional part to be positive.
        // E.g. -1.25 becomes -2 and +0.75
        i--;
    }

    double f = coordinate - i;

    *whole = i;
    *fraction = f;
}

void KisDabShape::paintAt(const QPointF &pos, const KisPaintInformation& info, KisDynamicColoring* coloringsrc)
{

    if(not m_dab)
    {
      m_dab = new KisPaintDevice(painter()->device()->colorSpace());
    }


    // Split the coordinates into integer plus fractional parts. The integer
    // is where the dab will be positioned and the fractional part determines
    // the sub-pixel positioning.
    qint32 x;
    double xFraction;
    qint32 y;
    double yFraction;

    splitCoordinate(pos.x(), &x, &xFraction);
    splitCoordinate(pos.y(), &y, &yFraction);

    KoColor color = painter()->paintColor();
    color.convertTo( m_dab->colorSpace() );
    m_brush->mask(m_dab, color, 0.5 * (m_scaleX + m_scaleY), m_rotate, info, xFraction, yFraction);

//     createStamp(m_dab, coloringsrc, pos, info);

    // paint the dab
    QRect dabRect = rect();
    QRect dstRect = QRect(x + dabRect.x(), y + dabRect.y(), dabRect.width(), dabRect.height());

    if ( painter()->bounds().isValid() ) {
        dstRect &= painter()->bounds();
    }

    if (dstRect.isNull() or dstRect.isEmpty() or not dstRect.isValid()) return;

    qint32 sx = 0 ;//dstRect.x() - x;
    qint32 sy = 0;//dstRect.y() - y;
    qint32 sw = dstRect.width();
    qint32 sh = dstRect.height();

    painter()->bitBlt(dstRect.x(), dstRect.y(), painter()->compositeOp(), m_dab, painter()->opacity(), sx, sy, sw, sh);
//    }

}

void KisDabShape::resize(double xs, double ys)
{
    m_scaleX *= xs;
    m_scaleY *= ys;
}
void KisDabShape::rotate(double r)
{
    m_rotate += r;
}

QRect KisDabShape::rect() const
{
    int width = m_brush->maskWidth( m_scaleX );
    int height = m_brush->maskHeight( m_scaleY );
    return QRect(-width/2, -height/2, width, height);
}


#if 0

void KisAlphaMaskShape::resize(double xs, double ys)
{
    Q_UNUSED(xs);
    Q_UNUSED(ys);
    // TODO: implement it
}

KisDynamicShape* KisAutoMaskShape::clone() const
{
    return new KisAutoMaskShape(*this);
}

void KisAutoMaskShape::resize(double xs, double ys)
{
    autoDab.width = (int)(autoDab.width * xs);
    autoDab.hfade = (int)(autoDab.hfade * xs);
    autoDab.height = (int)(autoDab.height * ys);
    autoDab.vfade = (int)(autoDab.vfade * ys);
}

void KisAutoMaskShape::createStamp(KisPaintDeviceSP stamp, KisDynamicColoring* coloringsrc,const QPointF &/*pos*/, const KisPaintInformation& /*info*/)
{
    // Transform into the paintdevice to apply
    switch(autoDab.shape)
    {
        case KisAutoDab::ShapeCircle:
            m_shape = new KisAutobrushCircleShape(autoDab.width, autoDab.height, autoDab.hfade, autoDab.vfade);
            break;
        case KisAutoDab::ShapeRectangle:
            m_shape = new KisAutobrushRectShape(autoDab.width, autoDab.height, autoDab.hfade, autoDab.vfade);
            break;
    }

    // Apply the coloring
    const KoColorSpace * colorSpace = stamp->colorSpace();

    // Convert the kiscolor to the right colorspace.
    KoColor kc;
    qint32 pixelSize = colorSpace->pixelSize();

    KisHLineIteratorPixel hiter = stamp->createHLineIterator( -autoDab.width/2, -autoDab.height/2, autoDab.width); // hum cheating (can't remember what?)
    for (int y = 0; y < autoDab.height; y++) // hum cheating (once again) /me slaps himself (again, what ?)
    {
        int x=0;
        while(! hiter.isDone())
        {
            coloringsrc->colorAt(x,y, &kc);
            colorSpace->setAlpha(kc.data(), alphaAt(x, y), 1);
            memcpy(hiter.rawData(), kc.data(), pixelSize);
            x++;
            ++hiter;
        }
        hiter.nextRow();
    }
}

KisAlphaMaskShape::~KisAlphaMaskShape()
{
}

#endif
