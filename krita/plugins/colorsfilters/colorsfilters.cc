/* This file is part of the KDE project
   Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <math.h>

#include <stdlib.h>

#include <qslider.h>
#include <qpoint.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <kgenericfactory.h>

#include <kis_doc.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_global.h>
#include <kis_tile_command.h>
#include <kis_types.h>
#include <kis_view.h>
#include <kis_iterators_pixel.h>
#include <kis_pixel.h>
// #include <kmessagebox.h>

#include "colorsfilters.h"
#include "kis_brightness_contrast_filter.h"

typedef KGenericFactory<ColorsFilters> ColorsFiltersFactory;
K_EXPORT_COMPONENT_FACTORY( kritacolorsfilters, ColorsFiltersFactory( "krita" ) )

ColorsFilters::ColorsFilters(QObject *parent, const char *name, const QStringList &)
		: KParts::Plugin(parent, name)
{
	setInstance(ColorsFiltersFactory::instance());

 	kdDebug() << "ColorsFilters plugin. Class: " 
 		  << className() 
 		  << ", Parent: " 
 		  << parent -> className()
 		  << "\n";


	if ( !parent->inherits("KisView") )
	{
		return;
	} else {
		m_view = (KisView*) parent;
	}

	KisFilterSP kbc = createFilter<KisBrightnessContrastFilter>(m_view);
	(void) new KAction(i18n("&Brightness / Contrast..."), 0, 0, kbc, SLOT(slotActivated()), actionCollection(), "brightnesscontrast");
	KisFilterSP kac = createFilter<KisAutoContrast>(m_view);
	(void) new KAction(i18n("&Auto Contrast"), 0, 0, kac, SLOT(slotActivated()), actionCollection(), "autocontrast");
	KisFilterSP kgc = createFilter<KisGammaCorrectionFilter>(m_view);
	(void) new KAction(i18n("&Gamma Correction..."), 0, 0, kgc, SLOT(slotActivated()), actionCollection(), "gammacorrection");
	KisFilterSP kfca = createFilter<KisColorAdjustmentFilter>(m_view);
	(void) new KAction(i18n("&Color Adjustment..."), 0, 0, kfca, SLOT(slotActivated()), actionCollection(), "coloradjustment");
	KisFilterSP kdf = createFilter<KisDesaturateFilter>(m_view);
	(void) new KAction(i18n("&Desaturate"), 0, 0, kdf, SLOT(slotActivated()), actionCollection(), "desaturate");
}

ColorsFilters::~ColorsFilters()
{
}

//==================================================================

KisColorAdjustmentFilter::KisColorAdjustmentFilter(KisView * view) : 
	KisPerChannelFilter(view, name(), -255, 255, 0)
{
}

void KisColorAdjustmentFilter::process(KisPaintDeviceSP src, KisPaintDeviceSP dst, KisFilterConfiguration* config, const QRect& rect,KisTileCommand* ktc)
{
	KisPerChannelFilterConfiguration* configPC = (KisPerChannelFilterConfiguration*) config;
	KisIteratorLinePixel lineIt = src->iteratorPixelSelectionBegin(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() );
	KisIteratorLinePixel dstLineIt = dst->iteratorPixelSelectionBegin(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() );
	KisIteratorLinePixel lastLine = src->iteratorPixelSelectionEnd(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() + rect.height() - 1);
	Q_INT32 depth = src->depth() - 1;
	while( lineIt <= lastLine )
	{
		KisIteratorPixel pixelIt = *lineIt;
		KisIteratorPixel dstPixelIt = *dstLineIt;
		KisIteratorPixel lastPixel = lineIt.end();
		while( pixelIt <= lastPixel )
		{
			KisPixelRO data = pixelIt.oldValue();
			KisPixel dstData = dstPixelIt;
			for( int i = 0; i < depth; i++)
			{
				KisQuantum d = pixelIt[ configPC->channel( i ) ];
				Q_INT32 s = configPC->valueFor( i );
				if( d < -s  ) dstData[ configPC->channel( i ) ] = 0;
				else if( d > QUANTUM_MAX - s) dstData[ configPC->channel( i ) ] = QUANTUM_MAX;
				else dstData[ configPC->channel( i ) ] = d + s;
			}
			++pixelIt;
			++dstPixelIt;
		}
		++lineIt;
		++dstLineIt;
	}
}


//==================================================================

KisGammaCorrectionFilter::KisGammaCorrectionFilter(KisView * view)
	: KisPerChannelFilter(view, name(), 1, 600, 1)
{
}

void KisGammaCorrectionFilter::process(KisPaintDeviceSP src, KisPaintDeviceSP dst, KisFilterConfiguration* config, const QRect& rect,KisTileCommand* ktc)
{
	KisPerChannelFilterConfiguration* configPC = (KisPerChannelFilterConfiguration*) config;
	KisIteratorLinePixel lineIt = src->iteratorPixelSelectionBegin(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() );
	KisIteratorLinePixel dstLineIt = dst->iteratorPixelSelectionBegin(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() );
	KisIteratorLinePixel lastLine = src->iteratorPixelSelectionEnd(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() + rect.height() - 1);
	Q_INT32 depth = src->depth() - 1;
	while( lineIt <= lastLine )
	{
		KisIteratorPixel pixelIt = *lineIt;
		KisIteratorPixel dstPixelIt = *dstLineIt;
		KisIteratorPixel lastPixel = lineIt.end();
		while( pixelIt <= lastPixel )
		{
			KisPixel data = pixelIt;
			for( int i = 0; i < depth; i++)
			{
				QUANTUM sd = pixelIt.oldValue()[ configPC->channel( i ) ];
				KisQuantum dd = dstPixelIt[ configPC->channel( i ) ];
				dd = (QUANTUM)( QUANTUM_MAX * pow( ((float)sd)/QUANTUM_MAX, 1.0 / configPC->valueFor( i ) ) );
			}
			++pixelIt;
			++dstPixelIt;
		}
		++lineIt;
		++dstLineIt;
	}
}

//==================================================================


KisAutoContrast::KisAutoContrast(KisView* view) : KisFilter(name(), view)
{
	
}
void KisAutoContrast::process(KisPaintDeviceSP src, KisPaintDeviceSP dst, KisFilterConfiguration* , const QRect& rect, KisTileCommand* ktc)
{

	KisIteratorLinePixel lineIt = src -> iteratorPixelSelectionBegin(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() );
	KisIteratorLinePixel lastLine = src -> iteratorPixelSelectionEnd(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() + rect.height() - 1);

	// Number of channels in this device except alpha
 	Q_INT32 depth = src -> colorStrategy() -> nColorChannels();

	Q_INT32 histo[QUANTUM_MAX+1];
	for( Q_INT32 i = 0; i < QUANTUM_MAX+1; i++)
	{
		histo[i] = 0;
// 		kdDebug() << histo[i] << endl;
	}
	Q_INT32 maxvalue = 0;
	while( lineIt <= lastLine )
	{
		KisIteratorPixel pixelIt = *lineIt;
		KisIteratorPixel lastPixel = lineIt.end();
		while( pixelIt <= lastPixel )
		{
			KisPixel data = pixelIt;
			Q_INT32 lightness = ( QMAX(QMAX(data[0], data[1]), data[2])
					+ QMIN(QMIN(data[0], data[1]), data[2]) ) / 2; 
			histo[ lightness ] ++;
// 			kdDebug() << " histo[ " << lightness << " ] = " << histo[ lightness ] << " " << maxvalue << endl;
			if( histo[ lightness ] > maxvalue )
			{
// 				kdDebug() << lightness << endl;
// 				kdDebug() << " change max value from " << maxvalue << " to " << histo[ lightness ] << endl;
				maxvalue = histo[ lightness ];
			}
			++pixelIt;
		}
		++lineIt;
	}
// 	kdDebug() << maxvalue << endl;
	Q_INT32 start;
// 	kdDebug() << histo[0] << " " << maxvalue * 0.1 << endl;
	for( start = 0; histo[start] < maxvalue * 0.1; start++) { /*kdDebug() << start << endl;*/ }
	Q_INT32 end;
// 	kdDebug() << histo[QUANTUM_MAX] << " " << maxvalue * 0.1 << endl;
	for( end = QUANTUM_MAX; histo[end] < maxvalue * 0.1; end--) { /*kdDebug() << end << endl;*/ }
	double factor = QUANTUM_MAX / (double) (end - start);
// 	kdDebug() << "end=" << end << " start=" << start << " factor=" << factor << endl;
	KisIteratorLinePixel lineIt2 = src->iteratorPixelSelectionBegin(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() );
	KisIteratorLinePixel dstLineIt = dst->iteratorPixelSelectionBegin(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() );
	while( lineIt2 <= lastLine )
	{
		KisIteratorPixel pixelIt = *lineIt2;
		KisIteratorPixel dstPixelIt = *dstLineIt;
		KisIteratorPixel lastPixel = lineIt2.end();
		while( pixelIt <= lastPixel )
		{
			KisPixel srcData = pixelIt;
			KisPixel dstData = dstPixelIt;

			// Iterate through all channels except alpha
			// XXX: Check for off-by-one errors
			for (int i = 0; i < depth; ++i) {
				dstData[i] = (QUANTUM) QMIN ( QUANTUM_MAX, QMAX(0, (srcData[i] - start) * factor) );
			}
			++dstPixelIt;
			++pixelIt;
		}
		++dstLineIt;
		++lineIt2;
	}

}


//==================================================================

KisDesaturateFilter::KisDesaturateFilter(KisView * view) 
	: KisFilter(name(), view)
{
}

void KisDesaturateFilter::process(KisPaintDeviceSP src, KisPaintDeviceSP dst, KisFilterConfiguration* /*config*/, const QRect& rect,KisTileCommand* ktc)
{
	if(colorStrategy()->name() != "RGBA")
		return;
	KisIteratorLinePixel lineIt = src->iteratorPixelSelectionBegin(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() );
	KisIteratorLinePixel dstLineIt = dst->iteratorPixelSelectionBegin(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() );
	KisIteratorLinePixel lastLine = src->iteratorPixelSelectionEnd(ktc, rect.x(), rect.x() + rect.width() - 1, rect.y() + rect.height() - 1);
// 	Q_INT32 depth = device->depth() - 1;
	while( lineIt <= lastLine )
	{
		KisIteratorPixel pixelIt = *lineIt;
		KisIteratorPixel dstPixelIt = *dstLineIt;
		KisIteratorPixel lastPixel = lineIt.end();
		while( pixelIt <= lastPixel )
		{
			KisPixelRO srcData = pixelIt.oldValue();
			KisPixel dstData = dstPixelIt;
			/* I thought of using the HSV model, but GIMP seems to use
			HSL for desaturating. Better use the gimp model for now 
					(HSV produces a lighter image than HSL) */
			Q_INT32 lightness = ( QMAX(QMAX(srcData[0], srcData[1]), srcData[2])
					+ QMIN(QMIN(srcData[0], srcData[1]), srcData[2]) ) / 2; 
			dstPixelIt[0] = lightness;
			dstPixelIt[1] = lightness;
			dstPixelIt[2] = lightness;
			++pixelIt;
			++dstPixelIt;
		}
		++lineIt;
		++dstLineIt;
	}
}
