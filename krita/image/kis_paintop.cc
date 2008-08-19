/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004,2007 Cyrille Berger <cberger@cberger.net>
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

#include "kis_paintop.h"
#include <QWidget>

#include <QString>

#include <KoColor.h>
#include <KoColorSpace.h>
#include <KoPointerEvent.h>

#include "kis_painter.h"
#include "kis_layer.h"
#include "kis_types.h"

#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_global.h"
#include "kis_iterators_pixel.h"
#include "kis_datamanager.h"
#include "kis_paintop_settings.h"
#include "kis_paintop_preset.h"
#include "kis_paint_information.h"
#include "kis_vec.h"

#define BEZIER_FLATNESS_THRESHOLD 0.5
#define MAXIMUM_SCALE 2

struct KisPaintOp::Private
{
    Private() : dab(0) {}
    KisPaintDeviceSP dab;
    KoColor color;
    KoColor previousPaintColor;
    KisPainter * painter;
};


KisPaintOp::KisPaintOp( KisPainter * painter) : d(new Private)
{
    d->painter = painter;
}

KisPaintOp::~KisPaintOp()
{
    delete d;
}

KisPaintDeviceSP KisPaintOp::cachedDab(  )
{
  return cachedDab( d->painter->device()->colorSpace() );
}

KisPaintDeviceSP KisPaintOp::cachedDab( const KoColorSpace *cs )
{
  if( !d->dab || !(*d->dab->colorSpace() == *cs )) {
      d->dab = KisPaintDeviceSP(new KisPaintDevice(cs, "dab"));
  }
  return d->dab;
}

void KisPaintOp::splitCoordinate(double coordinate, qint32 *whole, double *fraction) const
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

double KisPaintOp::paintBezierCurve(const KisPaintInformation &pi1,
                                    const QPointF &control1,
                                    const QPointF &control2,
                                    const KisPaintInformation &pi2,
                                    const double savedDist)
{
    double newDistance;
    double d1 = KisPainter::pointToLineDistance(control1, pi1.pos(), pi2.pos());
    double d2 = KisPainter::pointToLineDistance(control2, pi1.pos(), pi2.pos());

    if (d1 < BEZIER_FLATNESS_THRESHOLD && d2 < BEZIER_FLATNESS_THRESHOLD) {
        newDistance = paintLine(pi1, pi2, savedDist);
    } else {
        // Midpoint subdivision. See Foley & Van Dam Computer Graphics P.508
        KisVector2D p1 = pi1.pos();
        KisVector2D p2 = control1;
        KisVector2D p3 = control2;
        KisVector2D p4 = pi2.pos();

        KisVector2D l2 = (p1 + p2) / 2;
        KisVector2D h = (p2 + p3) / 2;
        KisVector2D l3 = (l2 + h) / 2;
        KisVector2D r3 = (p3 + p4) / 2;
        KisVector2D r2 = (h + r3) / 2;
        KisVector2D l4 = (l3 + r2) / 2;

        double midPressure = (pi1.pressure() + pi2.pressure()) / 2;
        double midXTilt = (pi1.xTilt() + pi2.xTilt()) / 2;
        double midYTilt = (pi1.yTilt() + pi2.yTilt()) / 2;

        KisPaintInformation middlePI( l4.toKoPoint(), midPressure, midXTilt, midYTilt );
        newDistance = paintBezierCurve( pi1,
                                       l2.toKoPoint(), l3.toKoPoint(),
                                       middlePI, savedDist);
        newDistance = paintBezierCurve(middlePI,
                                       r2.toKoPoint(),
                                       r3.toKoPoint(),
                                       pi2, newDistance);
    }

    return newDistance;
}


double KisPaintOp::paintLine(const KisPaintInformation &pi1,
                     const KisPaintInformation &pi2,
                     double savedDist)
{
    KisVector2D end(pi2.pos());
    KisVector2D start(pi1.pos());

    KisVector2D dragVec = end - start;

    if (savedDist < 0) {
        paintAt(pi1);
        savedDist = 0;
    }

    double xSpacing, ySpacing = 1;
    double sp = spacing(xSpacing, ySpacing, pi1.pressure(), pi2.pressure());

    double xScale = 1;
    double yScale = 1;

    // Scale x or y so that we effectively have a square brush
    // and calculate distance in that coordinate space. We reverse this scaling
    // before drawing the brush. This produces the correct spacing in both
    // x and y directions, even if the brush's aspect ratio is not 1:1.
    if (xSpacing > ySpacing) {
        yScale = xSpacing / ySpacing;
    }
    else {
        xScale = ySpacing / xSpacing;
    }

    dragVec.setX(dragVec.x() * xScale);
    dragVec.setY(dragVec.y() * yScale);

    double newDist = dragVec.length();
    double dist = savedDist + newDist;
    double l_savedDist = savedDist;

    if (dist < sp) {
        return dist;
    }

    dragVec.normalize();
    KisVector2D step(0, 0);

    while (dist >= sp) {
        if (l_savedDist > 0) {
            step += dragVec * (sp - l_savedDist);
            l_savedDist -= sp;
        }
        else {
            step += dragVec * sp;
        }

        QPointF p(start.x() + (step.x() / xScale), start.y() + (step.y() / yScale));

        double distanceMoved = step.length();
        double t = 0;

        if (newDist > DBL_EPSILON) {
            t = distanceMoved / newDist;
        }

        double pressure = (1 - t) * pi1.pressure() + t * pi2.pressure();
        double xTilt = (1 - t) * pi1.xTilt() + t * pi2.xTilt();
        double yTilt = (1 - t) * pi1.yTilt() + t * pi2.yTilt();

        paintAt(KisPaintInformation(p, pressure, xTilt, yTilt, dragVec));
        dist -= sp;
    }

    QRect r( pi1.pos().toPoint(), pi2.pos().toPoint() );
    d->painter->addDirtyRect( r.normalized() );

    if (dist > 0)
        return dist;
    else
        return 0;
}

KisPainter* KisPaintOp::painter() const
{
    return d->painter;
}

KisPaintDeviceSP KisPaintOp::source() const
{
    return d->painter->device();
}

double KisPaintOp::scaleForPressure(double pressure)
{
    double scale = pressure / PRESSURE_DEFAULT;

    if (scale < 0) {
        scale = 0;
    }

    if (scale > MAXIMUM_SCALE) {
        scale = MAXIMUM_SCALE;
    }

    return scale;
}

//------------------------ KisPaintOpFactory ------------------------//

KisPaintOpFactory::KisPaintOpFactory()
{
}

bool KisPaintOpFactory::userVisible(const KoColorSpace * cs )
{
    return cs && cs->id() != "WET";
}


KisPaintOpSettingsSP KisPaintOpFactory::settings(QWidget* /*parent*/, const KoInputDevice& /*inputDevice*/, KisImageSP /*image*/)
{
    return 0;
}

KisPaintOpSettingsSP KisPaintOpFactory::settings(KisImageSP image)
{
    Q_UNUSED(image);
    return 0;
}


QString KisPaintOpFactory::pixmap()
{
    return "";
}
