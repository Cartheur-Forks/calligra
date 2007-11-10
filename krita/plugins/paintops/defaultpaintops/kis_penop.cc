/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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
#include "klocale.h"

#include "kis_painter.h"
#include "kis_types.h"
#include "kis_paintop.h"
#include "kis_iterator.h"
#include "kis_selection.h"
#include "kis_iterators_pixel.h"

#include "kis_penop.h"


KisPaintOp * KisPenOpFactory::createOp(const KisPaintOpSettings */*settings*/, KisPainter * painter, KisImageSP image)
{
    Q_UNUSED( image );
    KisPaintOp * op = new KisPenOp(painter);
    Q_CHECK_PTR(op);
    return op;
}


KisPenOp::KisPenOp(KisPainter * painter)
    : KisPaintOp(painter)
{
}

KisPenOp::~KisPenOp()
{
}

void KisPenOp::paintAt(const KisPaintInformation& info)
{
    if (!painter()) return;
    KisPaintDeviceSP device = painter()->device();
    if (!device) return;
    KisBrush * brush = painter()->brush();
    if (!brush) return;
    if (! brush->canPaintFor(info) )
        return;

    QPointF hotSpot = brush->hotSpot(info);
    QPointF pt = info.pos() - hotSpot;

    qint32 x = qRound(pt.x());
    qint32 y = qRound(pt.y());

    KisPaintDeviceSP dab = KisPaintDeviceSP(0);
    if (brush->brushType() == IMAGE ||
        brush->brushType() == PIPE_IMAGE) {
        dab = brush->image(device->colorSpace(), info);
    }
    else {
        // Compute mask without sub-pixel positioning
        KisQImagemaskSP mask = brush->mask(info);
        dab = computeDab(mask);
    }

    painter()->setPressure(info.pressure());
    QRect dabRect = QRect(0, 0, brush->maskWidth(info), brush->maskHeight(info));
    QRect dstRect = QRect(x, y, dabRect.width(), dabRect.height());


    if ( painter()->bounds().isValid() ) {
        dstRect &= painter()->bounds();
    }

    if (dstRect.isNull() || dstRect.isEmpty() || !dstRect.isValid()) return;

    KoColorSpace * cs = dab->colorSpace();

    // Set all alpha > opaque/2 to opaque, the rest to transparent.
    // XXX: Using 4/10 as the 1x1 circle brush paints nothing with 0.5.

    KisRectIteratorPixel pixelIt = dab->createRectIterator(dabRect.x(), dabRect.y(), dabRect.width(), dabRect.height());

    while (!pixelIt.isDone()) {
        quint8 alpha = cs->alpha(pixelIt.rawData());

        if (alpha < (4 * OPACITY_OPAQUE) / 10) {
            cs->setAlpha(pixelIt.rawData(), OPACITY_TRANSPARENT, 1);
        } else {
            cs->setAlpha(pixelIt.rawData(), OPACITY_OPAQUE, 1);
        }

        ++pixelIt;
    }

    qint32 sx = dstRect.x() - x;
    qint32 sy = dstRect.y() - y;
    qint32 sw = dstRect.width();
    qint32 sh = dstRect.height();

    painter()->bltSelection(dstRect.x(), dstRect.y(), painter()->compositeOp(), dab, painter()->opacity(), sx, sy, sw, sh);
}
