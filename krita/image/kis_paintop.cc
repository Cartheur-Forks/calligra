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
#include <QWidget>

#include "kis_painter.h"
#include "kis_layer.h"
#include "kis_types.h"
#include "kis_paintop.h"
#include "kis_qimage_mask.h"

#include "KoColorSpace.h"
#include "kis_global.h"
#include "kis_iterators_pixel.h"
#include "KoColor.h"
#include "kis_datamanager.h"

KisPaintOp::KisPaintOp(KisPainter * painter) : m_dab(0)
{
    m_painter = painter;
    setSource(painter->device());
}

KisPaintOp::~KisPaintOp()
{
}

KisPaintDeviceSP KisPaintOp::computeDab(KisQImagemaskSP mask) {
    return computeDab(mask, m_painter->device()->colorSpace());
}

KisPaintDeviceSP KisPaintOp::computeDab(KisQImagemaskSP mask, KoColorSpace *cs)
{
    // XXX: According to the SeaShore source, the Gimp uses a
    // temporary layer the size of the layer that is being painted
    // on. This layer is cleared between painting actions. Our
    // temporary layer, dab, is for every paintAt, composited with
    // the target layer. We only use a real temporary layer for things
    // like filter tools -- and for indirect painting, it turns out.

    // Convert the kiscolor to the right colorspace.
    KoColor kc = m_painter->paintColor();
    kc.convertTo(cs);

    qint32 maskWidth = mask->width();
    qint32 maskHeight = mask->height();

    if( !m_dab || m_dab->colorSpace() != cs) {
        m_dab = KisPaintDeviceSP(new KisPaintDevice(cs, "dab"));
        m_dab->dataManager()->setDefaultPixel( kc.data() );
    }
    Q_CHECK_PTR(m_dab);

    quint8 * maskData = mask->data();

    // Apply the alpha mask
    KisHLineIteratorPixel hiter = m_dab->createHLineIterator(0, 0, maskWidth);
    for (int y = 0; y < maskHeight; y++)
    {
        while(! hiter.isDone())
        {
            int hiterConseq = hiter.nConseqHPixels();
            cs->setAlpha( hiter.rawData(), OPACITY_OPAQUE, hiterConseq );
            cs->applyAlphaU8Mask( hiter.rawData(), maskData, hiterConseq);
            hiter += hiterConseq;
            maskData += hiterConseq;
        }
        hiter.nextRow();
    }

    return m_dab;
}

void KisPaintOp::splitCoordinate(double coordinate, qint32 *whole, double *fraction)
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

void KisPaintOp::setSource(KisPaintDeviceSP p) {
    Q_ASSERT(p);
    m_source = p;
}


KisPaintOpSettings* KisPaintOpFactory::settings(QWidget* /*parent*/, const KoInputDevice& /*inputDevice*/) { return 0; }

