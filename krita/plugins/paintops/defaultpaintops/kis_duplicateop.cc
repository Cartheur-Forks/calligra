/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004-2006 Cyrille Berger <cberger@cberger.net>
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

#include "kis_duplicateop.h"

#include <qrect.h>

#include <kdebug.h>

#include <KoColorSpaceRegistry.h>

#include "kis_brush.h"
#include "kis_global.h"
#include "kis_paint_device.h"
#include "kis_layer.h"
#include "kis_painter.h"
#include "kis_types.h"
#include "kis_meta_registry.h"
#include "kis_paintop.h"
#include "kis_iterators_pixel.h"
#include "kis_selection.h"
#include "kis_perspective_grid.h"
#include "kis_random_sub_accessor.h"

KisPaintOp * KisDuplicateOpFactory::createOp(const KisPaintOpSettings */*settings*/, KisPainter * painter, KisImageSP image)
{
    KisPaintOp * op = new KisDuplicateOp(painter, image);
    Q_CHECK_PTR(op);
    return op;
}


KisDuplicateOp::KisDuplicateOp(KisPainter * painter, KisImageSP image)
    : super(painter), m_srcdev(0), m_target(0), m_image( image )
{
}

KisDuplicateOp::~KisDuplicateOp()
{
}

double KisDuplicateOp::minimizeEnergy(const double* m, double* sol, int w, int h)
{
    int rowstride = 3*w;
    double err = 0;
    memcpy(sol, m, 3* sizeof(double) * w);
    m+=rowstride;
    sol+=rowstride;
    for ( int i = 1; i < h - 1; i++)
    {
        memcpy(sol, m, 3* sizeof(double));
        m+=3; sol+=3;
        for ( int j = 3; j < rowstride-3; j++)
        {
            double tmp = *sol;
            *sol = ( ( *(m - 3 ) + *(m + 3) + *(m - rowstride ) + *(m + rowstride )) + 2 * *m ) /6;
            double diff = *sol - tmp;
            err += diff*diff;
            m ++; sol ++;
        }
        memcpy(sol, m, 3* sizeof(double));
        m+=3; sol+=3;
}
    memcpy(sol, m, 3* sizeof(double) * w);
    return err;
}

#define CLAMP(x,l,u) ((x)<(l)?(l):((x)>(u)?(u):(x)))

void KisDuplicateOp::paintAt(const QPointF &pos, const KisPaintInformation& info)
{
    if (!m_painter) return;

    bool heal = m_painter->duplicateHealing();
//     int healradius = m_painter->duplicateHealingRadius();

    KisPaintDeviceSP device = m_painter->device();
    if (m_source) device = m_source;
    if (!device) return;

    KisBrush * brush = m_painter->brush();
    if (!brush) return;
    if (! brush->canPaintFor(info) )
        return;

    QPointF hotSpot = brush->hotSpot(info);
    QPointF pt = pos - hotSpot;

    // Split the coordinates into integer plus fractional parts. The integer
    // is where the dab will be positioned and the fractional part determines
    // the sub-pixel positioning.
    qint32 x;
    double xFraction;
    qint32 y;
    double yFraction;

    splitCoordinate(pt.x(), &x, &xFraction);
    splitCoordinate(pt.y(), &y, &yFraction);
    xFraction = yFraction = 0.0;

    KisPaintDeviceSP dab = 0;

    if (brush->brushType() == IMAGE ||
        brush->brushType() == PIPE_IMAGE) {
        dab = brush->image(device->colorSpace(), info, xFraction, yFraction);
        dab->convertTo(KoColorSpaceRegistry::instance()->alpha8());
    }
    else {
        KisQImagemaskSP mask = brush->mask(info, xFraction, yFraction);
        dab = computeDab(mask, KoColorSpaceRegistry::instance()->alpha8());
    }

    m_painter->setPressure(info.pressure);

    QPointF srcPointF = pt - m_painter->duplicateOffset();
    QPoint srcPoint = QPoint(x - static_cast<qint32>(m_painter->duplicateOffset().x()),
                             y - static_cast<qint32>(m_painter->duplicateOffset().y()));


    qint32 sw = dab->extent().width();
    qint32 sh = dab->extent().height();

    if (srcPoint.x() < 0 )
        srcPoint.setX(0);

    if( srcPoint.y() < 0)
        srcPoint.setY(0);
    if( !(m_srcdev && m_srcdev->colorSpace() != device->colorSpace()) )
    {
        m_srcdev = new KisPaintDevice(device->colorSpace(), "duplicate source dev");
        m_target = new KisPaintDevice(device->colorSpace(), "duplicate target dev");
    }
    Q_CHECK_PTR(m_srcdev);

    // Perspective correction ?
    KisPainter copyPainter(m_srcdev);
    if(m_painter->duplicatePerspectiveCorrection())
    {
        double startM[3][3];
        double endM[3][3];
        for(int i = 0; i < 3; i++)
        {
            for(int j = 0; j < 3; j++)
            {
                startM[i][j] = 0.;
                endM[i][j] = 0.;
            }
            startM[i][i] = 1.;
            endM[i][i] = 1.;
        }
        // First look for the grid corresponding to the start point
        KisSubPerspectiveGrid* subGridStart = *m_image->perspectiveGrid()->begin();
        QRect r = QRect(0,0, m_image->width(), m_image->height());

#if 1
        if(subGridStart)
        {
//             kDebug() << "fgrid" << endl;
//             kDebug() << *subGridStart->topLeft() << " " << *subGridStart->topRight() << " " << *subGridStart->bottomLeft() << " " <<  *subGridStart->bottomRight() << endl;
            double* b = KisPerspectiveMath::computeMatrixTransfoFromPerspective( r, *subGridStart->topLeft(), *subGridStart->topRight(), *subGridStart->bottomLeft(), *subGridStart->bottomRight());
            for(int i = 0; i < 3; i++)
            {
                for(int j = 0; j < 3; j++)
                {
//                     kDebug() << "sol[" << 3*i+j << "]=" << b[3*i+j] << endl;
                    startM[i][j] = b[3*i+j];
                }
            }

        }
#endif
#if 1
        // Second look for the grid corresponding to the end point
        KisSubPerspectiveGrid* subGridEnd = *m_image->perspectiveGrid()->begin();
        if(subGridEnd)
        {
//             kDebug() << "second grid" << endl;
            double* b = KisPerspectiveMath::computeMatrixTransfoToPerspective(*subGridEnd->topLeft(), *subGridEnd->topRight(), *subGridEnd->bottomLeft(), *subGridEnd->bottomRight(), r);
            for(int i = 0; i < 3; i++)
            {
                for(int j = 0; j < 3; j++)
                {
//                     kDebug() << "sol[" << 3*i+j << "]=" << b[3*i+j] << endl;
                    endM[i][j] = b[3*i+j];
                }
            }
        }
#endif
//         kDebug()<< " oouuuuh" << srcPointF << KisPerspectiveMath::matProd(startM,  KisPerspectiveMath::matProd(endM, srcPointF ) ) << KisPerspectiveMath::matProd(endM,  KisPerspectiveMath::matProd(startM, srcPointF ) );

        // Compute the translation in the perspective transformation space:
        QPointF positionStartPaintingT = KisPerspectiveMath::matProd(endM, QPointF(m_painter->duplicateStart()) );
        QPointF duplicateStartPoisitionT = KisPerspectiveMath::matProd(endM, QPointF(m_painter->duplicateStart()) - QPointF(m_painter->duplicateOffset()) );
        QPointF translat = duplicateStartPoisitionT - positionStartPaintingT;
        KisRectIteratorPixel dstIt = m_srcdev->createRectIterator(0, 0, sw, sh);
        KisRandomSubAccessorPixel srcAcc = device->createRandomSubAccessor();
        //Action
        while(!dstIt.isDone())
        {
            if(dstIt.isSelected())
            {
                QPointF p =  KisPerspectiveMath::matProd(startM, KisPerspectiveMath::matProd(endM, QPointF(dstIt.x() + x, dstIt.y() + y) ) + translat );
                srcAcc.moveTo( p );
                srcAcc.sampledOldRawData( dstIt.rawData() );
            }
            ++dstIt;
        }


    } else {
        // Or, copy the source data on the temporary device:
        copyPainter.bitBlt(0, 0, COMPOSITE_COPY, device, srcPoint.x(), srcPoint.y(), sw, sh);
        copyPainter.end();
    }

    // heal ?

    if(heal)
    {
        quint16 dataDevice[4];
        quint16 dataSrcDev[4];
        double* matrix = new double[ 3 * sw * sh ];
        // First divide
        KoColorSpace* deviceCs = device->colorSpace();
        KisHLineConstIteratorPixel deviceIt = device->createHLineConstIterator(x, y, sw );
        KisHLineIteratorPixel srcDevIt = m_srcdev->createHLineIterator(0, 0, sw);
        double* matrixIt = &matrix[0];
        for(int j = 0; j < sh; j++)
        {
            for(int i= 0; !srcDevIt.isDone(); i++)
            {
                deviceCs->toLabA16(deviceIt.rawData(), (quint8*)dataDevice, 1);
                deviceCs->toLabA16(srcDevIt.rawData(), (quint8*)dataSrcDev, 1);
                // Division
                for( int k = 0; k < 3; k++)
                {
                    matrixIt[k] = dataDevice[k] / (double)qMax( (int)dataSrcDev [k], 1);
                }
                ++deviceIt;
                ++srcDevIt;
                matrixIt +=3;
            }
            deviceIt.nextRow();
            srcDevIt.nextRow();
        }
        // Minimize energy
        {
            int iter = 0;
            double err;
            double* solution = new double [ 3 * sw * sh ];
            do {
                err = minimizeEnergy(&matrix[0], &solution[0], sw, sh);
                memcpy (&matrix[0], &solution[0], sw * sh * 3 * sizeof(double));
                iter++;
            } while( err < 0.00001 && iter < 100);
            delete [] solution;
        }

        // Finaly multiply
        deviceIt = device->createHLineIterator(x, y, sw);
        srcDevIt = m_srcdev->createHLineIterator(0, 0, sw);
        matrixIt = &matrix[0];
        for(int j = 0; j < sh; j++)
        {
            for(int i= 0; !srcDevIt.isDone(); i++)
            {
                deviceCs->toLabA16(deviceIt.rawData(), (quint8*)dataDevice, 1);
                deviceCs->toLabA16(srcDevIt.rawData(), (quint8*)dataSrcDev, 1);
                // Multiplication
                for( int k = 0; k < 3; k++)
                {
                    dataSrcDev[k] = (int)CLAMP( matrixIt[k] * qMax((int) dataSrcDev[k], 1), 0, 65535 );
                }
                deviceCs->fromLabA16((quint8*)dataSrcDev, srcDevIt.rawData(), 1);
                ++deviceIt;
                ++srcDevIt;
                matrixIt +=3;
            }
            deviceIt.nextRow();
            srcDevIt.nextRow();
        }
        delete [] matrix;
    }


    // Add the dab as selection to the srcdev
//     KisPainter copySelection(srcdev->selection().data());
//     copySelection.bitBlt(0, 0, COMPOSITE_OVER, dab, 0, 0, sw, sh);
//     copySelection.end();

    // copy the srcdev onto a new device, after applying the dab selection
    copyPainter.begin(m_target);

    copyPainter.bltMask(0, 0, m_srcdev->colorSpace()->compositeOp(COMPOSITE_OVER), m_srcdev, dab,
                             OPACITY_OPAQUE, 0, 0, sw, sh);
    copyPainter.end();

    QRect dabRect = QRect(0, 0, brush->maskWidth(info), brush->maskHeight(info));
    QRect dstRect = QRect(x, y, dabRect.width(), dabRect.height());


    if ( m_painter->bounds().isValid() ) {
        dstRect &= m_painter->bounds();
    }

    if (dstRect.isNull() || dstRect.isEmpty() || !dstRect.isValid()) return;

    qint32 sx = dstRect.x() - x;
    qint32 sy = dstRect.y() - y;
    sw = dstRect.width();
    sh = dstRect.height();

    if (m_source->hasSelection()) {
        m_painter->bltSelection(dstRect.x(), dstRect.y(), m_painter->compositeOp(), m_target,
                                m_source->selection(), m_painter->opacity(), sx, sy, sw, sh);
    }
    else {
        m_painter->bitBlt(dstRect.x(), dstRect.y(), m_painter->compositeOp(), m_target, m_painter->opacity(), sx, sy, sw, sh);
    }


}
