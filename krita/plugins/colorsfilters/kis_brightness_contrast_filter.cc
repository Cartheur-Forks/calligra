/*
 * This file is part of Krita
 *
 * Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
  * Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
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

#include <klocale.h>

#include <qlayout.h>

#include "kis_brightness_contrast_filter.h"
#include "wdg_brightness_contrast.h"
#include "kis_abstract_colorspace.h"
#include "kis_paint_device.h"
#include "kis_iterators_pixel.h"
#include "tiles/kis_iterator.h"
#include "kcurve.h"

KisBrightnessContrastFilterConfiguration::KisBrightnessContrastFilterConfiguration()
{
}
 
KisBrightnessContrastFilter::KisBrightnessContrastFilter()
    : KisFilter( id(), "adjust", "&Brightness/contrast...")
{

}

KisFilterConfigWidget * KisBrightnessContrastFilter::createConfigurationWidget(QWidget *parent, KisPaintDeviceSP)
{
    return new KisBrightnessContrastConfigWidget(parent);
}

KisFilterConfiguration* KisBrightnessContrastFilter::configuration(QWidget *nwidget, KisPaintDeviceSP)
{
    KisBrightnessContrastConfigWidget* widget = (KisBrightnessContrastConfigWidget*)nwidget;
    
    if ( widget == 0 )
    {
        return new KisBrightnessContrastFilterConfiguration();
    } else {
        return widget->config();
    }
}

std::list<KisFilterConfiguration*> KisBrightnessContrastFilter::listOfExamplesConfiguration(KisPaintDeviceSP /*dev*/)
{
//XXX should really come up with a list of configurations
    std::list<KisFilterConfiguration*> list;
    list.insert(list.begin(), new KisBrightnessContrastFilterConfiguration( ));
    return list;
}


void KisBrightnessContrastFilter::process(KisPaintDeviceSP src, KisPaintDeviceSP dst, KisFilterConfiguration* config, const QRect& rect)
{
    KisBrightnessContrastFilterConfiguration* configBC = (KisBrightnessContrastFilterConfiguration*) config;

    KisColorAdjustment *adj = src->colorStrategy()->createBrightnessContrastAdjustment(configBC->transfer);
        
    KisRectIteratorPixel dstIt = dst->createRectIterator(rect.x(), rect.y(), rect.width(), rect.height(), true );
    KisRectIteratorPixel srcIt = src->createRectIterator(rect.x(), rect.y(), rect.width(), rect.height(), false);

    setProgressTotalSteps(rect.width() * rect.height());
    Q_INT32 pixelsProcessed = 0;

    while( ! srcIt.isDone()  && !cancelRequested())
    {
        Q_UINT32 npix;
        npix = srcIt.nConseqPixels();
        
        // change the brightness and contrast
        src->colorStrategy()->applyAdjustment(srcIt.oldRawData(), dstIt.rawData(), adj, npix);
                    
        srcIt+=npix;
        dstIt+=npix;

        pixelsProcessed++;
        setProgress(pixelsProcessed);
    }

    setProgressDone();
}

KisBrightnessContrastConfigWidget::KisBrightnessContrastConfigWidget(QWidget * parent, const char * name, WFlags f)
    : KisFilterConfigWidget(parent, name, f)
{
    m_page = new WdgBrightnessContrast(this);
    QHBoxLayout * l = new QHBoxLayout(this);
    Q_CHECK_PTR(l);

    l -> add(m_page);

    connect( m_page->kCurve, SIGNAL(modified()), SIGNAL(sigPleaseUpdatePreview()));
}

KisBrightnessContrastFilterConfiguration * KisBrightnessContrastConfigWidget::config()
{
    KisBrightnessContrastFilterConfiguration * cfg = new KisBrightnessContrastFilterConfiguration();

    for(int i=0; i <256; i++)
    {
        Q_INT32 val;
        val = int(0xFFFF * m_page->kCurve->getCurveValue( i / 255.0));
        if(val >0xFFFF)
            val=0xFFFF;
        if(val <0)
            val = 0;
            
        cfg->transfer[i] = val;
    }
    
    return cfg;
}
