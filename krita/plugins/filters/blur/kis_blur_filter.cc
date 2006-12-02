/*
 * This file is part of Krita
 *
 * Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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

#include "kis_blur_filter.h"

#include <kcombobox.h>
#include <knuminput.h>

#include <kis_autobrush_resource.h>
#include <kis_convolution_painter.h>
#include <kis_iterators_pixel.h>


#include "kis_wdg_blur.h"
#include "ui_wdgblur.h"

// KisKernelSP kernelFromQImage(const QImage& img)
// {
//     KisKernelSP k = new KisKernel;
//     k->width = img.width();
//     k->height = img.height();
//     k->offset = 0;
//     uint count = k->width * k->height;
//     k->data = new Q_INT32[count];
//     Q_INT32* itData = k->data;
//     Q_UINT8* itImg = img.bits();
//     k->factor = 0;
//     for(uint i = 0; i < count; ++i , ++itData, itImg+=4)
//     {
//         *itData = 255 - ( *itImg + *(itImg+1) + *(itImg+2) ) / 3;
//         k->factor += *itData;
//     }
//     return k;
// }

KisBlurFilter::KisBlurFilter() : KisFilter(id(), "blur", i18n("&Blur..."))
{
}

KisFilterConfigWidget * KisBlurFilter::createConfigurationWidget(QWidget* parent, KisPaintDeviceSP )
{
    return new KisWdgBlur(this, parent);
}

KisFilterConfiguration* KisBlurFilter::designerConfiguration(const KisPaintDeviceSP)
{
    KisFilterConfiguration* config = new KisFilterConfiguration(id().id(), 1);
    config->setProperty("halfWidth", 5 );
    config->setProperty("halfHeight", 5 );
    config->setProperty("rotate", 0 );
    config->setProperty("strength", 0 );
    config->setProperty("shape", 0);
    return config;
}

void KisBlurFilter::process(const KisPaintDeviceSP src, const QPoint& srcTopLeft, KisPaintDeviceSP dst, const QPoint& dstTopLeft, const QSize& size, KisFilterConfiguration* config)
{
    Q_ASSERT(src != 0);
    Q_ASSERT(dst != 0);
    

    if (dst != src) { // TODO: fix the convolution painter to avoid that stupid copy
        kDebug() << "src != dst\n";
        KisPainter gc(dst);
        gc.bitBlt(dstTopLeft.x(), dstTopLeft.y(), COMPOSITE_COPY, src, srcTopLeft.x(), srcTopLeft.y(), size.width(), size.height());
        gc.end();
    }
    
    setProgressTotalSteps(size.width() * size.height());

    if(!config) config = new KisFilterConfiguration(id().id(), 1);
    
    QVariant value;
    int shape = (config->getProperty("shape", value)) ? value.toInt() : 0;
    uint halfWidth = (config->getProperty("halfWidth", value)) ? value.toUInt() : 5;
    uint width = 2 * halfWidth + 1;
    uint halfHeight = (config->getProperty("halfHeight", value)) ? value.toUInt() : 5;
    uint height = 2 * halfHeight + 1;
    int rotate = (config->getProperty("rotate", value)) ? value.toInt() : 0;
    int strength = 100 - (config->getProperty("strength", value)) ? value.toUInt() : 0;
    
    int hFade = (halfWidth * strength) / 100;
    int vFade = (halfHeight * strength) / 100;
    
    KisAutobrushShape* kas;
    kDebug() << width << " " << height << " " << hFade << " " << vFade << endl;
    switch(shape)
    {
        case 1:
            kas = new KisAutobrushRectShape(width, height , hFade, vFade);
            break;
        case 0:
        default:
            kas = new KisAutobrushCircleShape(width, height, hFade, vFade);
            break;
    }
    QImage mask;
    kas->createBrush(&mask);
    
    mask.convertDepth(1); 
    
    if( rotate != 0)
    {
        QWMatrix m;
        m.rotate( rotate );
        mask = mask.xForm( m );
        if( (mask.height() & 1) || mask.width() & 1)
        {
            mask.smoothScale( mask.width() + !(mask.width() & 1), mask.height() + !(mask.height() & 1) );
        }
    }
    
    KisKernelSP kernel = KisKernelSP(KisKernel::fromQImage(mask)); 
    KisConvolutionPainter painter( dst );
    painter.applyMatrix(kernel, dstTopLeft.x(), dstTopLeft.y(), size.width(), size.height(), BORDER_REPEAT);
    
    if (painter.cancelRequested()) {
        cancel();
    }

    setProgressDone(); // Must be called even if you don't really support progression
}

