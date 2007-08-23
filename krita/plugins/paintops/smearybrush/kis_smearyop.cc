/*
 *  Copyright (c) 2005 Boudewijn Rempt <boud@valdyas.org>
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

#include <QRect>

#include <kdebug.h>

#include "kis_brush.h"
#include "kis_global.h"
#include "kis_paint_device.h"

#include "kis_painter.h"
#include "kis_types.h"
#include "kis_paintop.h"
#include "kis_iterator.h"
#include "kis_iterators_pixel.h"
#include "kis_selection.h"
#include "kis_smearyop.h"



const qint32 STARTING_PAINTLOAD = 100;
const qint32 CANVAS_WETNESS = 10;
const qint32 NUMBER_OF_TUFTS = 16;

class KisSmearyOp::SmearyTuft {

public:

    /**
     * Create a new smeary tuft.
     *
     * @param distanceFromCenter how far this tuft is from the center of the brush
     * @param paintload the initial amount of paint this tuft holds. May be replenished
     *        by picking up paint from the canvas
     * @param color the initial paint color. Will change through contact with color on the canvas
     */
    SmearyTuft(qint32 distanceFromCenter, qint32 paintload, KoColor color)
        : m_distanceFromCenter(distanceFromCenter)
        , m_paintload(paintload)
        , m_color(color) {}

    /**
     * Mix the current paint color with the color found
     * at pos in dev.
     */
        void mixAt(const QPointF & /*pos*/, double /*pressure*/, KisPaintDeviceSP /*dev*/)
    {
        // Get the image background color
        // Get the color at pos
        // if the color at pos is the same as the background color, don't mix
        // else mix the color; the harder the pressure, the more mixed the color will be
        // if the pressure is really hard, take the color from all layers under this layer
        // if there's little paint on the brush, pick up some from the canvas
    }

    /**
     * Paint the tuft footprint (calculated from the pressure) at the given position
     */
    void paintAt(const QPointF & /*pos*/, double /*pressure*/, KisPaintDeviceSP /*dev*/)
    {
        //
    }

public:
    qint32 m_distanceFromCenter;
    qint32 m_paintload;
    KoColor m_color;

};



KisSmearyOp::KisSmearyOp(KisPainter * painter)
    : KisPaintOp(painter)
{
    for (int i = 0; i < NUMBER_OF_TUFTS / 2; ++i) {
        m_leftTufts.append(new SmearyTuft(i, STARTING_PAINTLOAD, painter->paintColor()));
        m_rightTufts.append(new SmearyTuft(i, STARTING_PAINTLOAD, painter->paintColor()));
    }

}

KisSmearyOp::~KisSmearyOp()
{
}

void KisSmearyOp::paintAt(const KisPaintInformation& info)
{
    if (!painter()->device()) return;

    KisBrush *brush = painter()->brush();

    Q_ASSERT(brush);

    if (!brush) return;

    if (! brush->canPaintFor(info) )
        return;

    KisPaintDeviceSP device = painter()->device();
    KoColorSpace * colorSpace = device->colorSpace();
    KoColor kc = painter()->paintColor();
    kc.convertTo(colorSpace);

    QPointF hotSpot = brush->hotSpot(info);
    QPointF pt = info.pos - hotSpot;

    // Split the coordinates into integer plus fractional parts. The integer
    // is where the dab will be positioned and the fractional part determines
    // the sub-pixel positioning.
    qint32 x, y;
    double xFraction, yFraction;

    splitCoordinate(pt.x(), &x, &xFraction);
    splitCoordinate(pt.y(), &y, &yFraction);

    KisPaintDeviceSP dab = KisPaintDeviceSP(new KisPaintDevice(colorSpace, "smeary dab"));
    Q_CHECK_PTR(dab);

    painter()->setPressure(info.pressure);

    // Compute the position of the tufts. The tufts are arranged in a line
    // perpendicular to the motion of the brush, i.e, the straight line between
    // the current position and the previous position.
    // The tufts are spread out through the pressure

    QPointF previousPoint = info.movement.toKoPoint();
    KisVector2D brushVector(-previousPoint.y(), previousPoint.x());
    KisVector2D currentPointVector = KisVector2D(info.pos);
    brushVector.normalize();

    KisVector2D vl, vr;

    for (int i = 0; i < (NUMBER_OF_TUFTS / 2); ++i) {
        // Compute the positions on the new vector.
        vl = currentPointVector + i * brushVector;
        QPointF pl = vl.toKoPoint();
        dab->setPixel(qRound(pl.x()), qRound(pl.y()), kc);

        vr = currentPointVector - i * brushVector;
        QPointF pr = vr.toKoPoint();
        dab->setPixel(qRound(pr.x()), qRound(pr.y()), kc);
    }

    vr = vr - vl;
    vr.normalize();

    if (source()->hasSelection()) {
        painter()->bltSelection(x - 32, y - 32, painter()->compositeOp(), dab,
                                source()->selection(), painter()->opacity(), x - 32, y -32, 64, 64);
    }
    else {
        painter()->bitBlt(x - 32, y - 32, painter()->compositeOp(), dab, painter()->opacity(), x - 32, y -32, 64, 64);
    }

}


KisPaintOp * KisSmearyOpFactory::createOp(const KisPaintOpSettings */*settings*/, KisPainter * painter, KisImageSP image)
{
    Q_UNUSED( image );
    KisPaintOp * op = new KisSmearyOp(painter);
    Q_CHECK_PTR(op);
    return op;
}
