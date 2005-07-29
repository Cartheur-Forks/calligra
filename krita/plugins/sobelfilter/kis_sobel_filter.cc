/*
 * This file is part of Krita
 *
 * Copyright (c) 2005 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
 *
 * ported from Gimp, Copyright (C) 1997 Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 * original pixelize.c for GIMP 0.54 by Tracy Scott
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

#include <stdlib.h>
#include <vector>

#include <qpoint.h>
#include <qspinbox.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <knuminput.h>

#include <kis_doc.h>
#include <kis_image.h>
#include <kis_iterators_pixel.h>
#include <kis_layer.h>
#include <kis_filter_registry.h>
#include <kis_global.h>
#include <kis_types.h>
#include <kis_progress_display_interface.h>

#include "kis_multi_integer_filter_widget.h"
#include "kis_sobel_filter.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

KisSobelFilter::KisSobelFilter() : KisFilter(id(), "edge", "&Sobel...")
{
}

void KisSobelFilter::process(KisPaintDeviceSP src, KisPaintDeviceSP dst, KisFilterConfiguration* configuration, const QRect& rect)
{
        Q_INT32 x = rect.x(), y = rect.y();
        Q_INT32 width = rect.width();
        Q_INT32 height = rect.height();
        
        //read the filter configuration values from the KisFilterConfiguration object
        Q_UINT32 pixelWidth = ((KisSobelFilterConfiguration*)configuration)->pixelWidth();
        Q_UINT32 pixelHeight = ((KisSobelFilterConfiguration*)configuration)->pixelHeight();
        
        //pixelize(src, dst, x, y, width, height, pixelWidth, pixelHeight);
	sobel(src, dst, true, true, true);
}

void KisSobelFilter::prepareRow (KisPaintDeviceSP src, Q_UINT8* data, Q_UINT32 x, Q_UINT32 y, Q_UINT32 w, Q_UINT32 h)
{
	y = CLAMP (y, 0, h - 1);
	Q_UINT32 pixelSize = src -> pixelSize();

	src -> readBytes( data, x, y, w, 1 );

	for (Q_UINT32 b = 0; b < pixelSize; b++)
	{
		data[-(int)pixelSize + b] = data[b];
		data[w * pixelSize + b] = data[(w - 1) * pixelSize + b];
	}	
}

#define RMS(a, b) (sqrt ((a) * (a) + (b) * (b)))
#define ROUND(x) ((int) ((x) + 0.5))

void KisSobelFilter::sobel(KisPaintDeviceSP src, KisPaintDeviceSP dst, bool doHorizontal, bool doVertical, bool keepSign)
{
	QRect rect = src -> exactBounds();
	Q_UINT32 x = rect.x();
	Q_UINT32 y = rect.y();
	Q_UINT32 width = rect.width();
	Q_UINT32 height = rect.height();
	Q_UINT32 pixelSize = src -> pixelSize();
	bool hasAlpha = src -> hasAlpha();

	setProgressTotalSteps( height );
        setProgressStage(i18n("Applying sobel filter..."),0);

	/*  allocate row buffers  */
	Q_UINT8* prevRow = new Q_UINT8[ (width + 2) * pixelSize];
	Q_CHECK_PTR(prevRow);
	Q_UINT8* curRow = new Q_UINT8[ (width + 2) * pixelSize];
	Q_CHECK_PTR(curRow);
	Q_UINT8* nextRow = new Q_UINT8[ (width + 2) * pixelSize];
	Q_CHECK_PTR(nextRow);
	Q_UINT8* dest = new Q_UINT8[ width  * pixelSize];
	Q_CHECK_PTR(dest);

	Q_UINT8* pr = prevRow + pixelSize;
	Q_UINT8* cr = curRow + pixelSize;
	Q_UINT8* nr = nextRow + pixelSize;

	prepareRow (src, pr, x, y - 1, width, height);
	prepareRow (src, cr, x, y, width, height);
  
	Q_UINT32 counter =0;
	Q_UINT8* d;
	Q_UINT8* tmp;
	Q_INT32 gradient, horGradient, verGradient;
	// loop through the rows, applying the sobel convolution 
	for (Q_UINT32 row = 0; row < height; row++)
	{
		
		// prepare the next row 
		prepareRow (src, nr, x, row + 1, width, height);
		d = dest;
		
		for (Q_UINT32 col = 0; col < width * pixelSize; col++)
		{
			horGradient = (doHorizontal ?
					((pr[col - pixelSize] +  2 * pr[col] + pr[col + pixelSize]) -
					(nr[col - pixelSize] + 2 * nr[col] + nr[col + pixelSize]))
					: 0);
			
			verGradient = (doVertical ?
					((pr[col - pixelSize] + 2 * cr[col - pixelSize] + nr[col - pixelSize]) -
					(pr[col + pixelSize] + 2 * cr[col + pixelSize] + nr[col + pixelSize]))
					: 0);
			gradient = (doVertical && doHorizontal) ?
			(ROUND (RMS (horGradient, verGradient)) / 5.66) // always >0 
			: (keepSign ? (127 + (ROUND ((horGradient + verGradient) / 8.0)))
			: (ROUND (QABS (horGradient + verGradient) / 4.0)));
			
			if (hasAlpha && (((col + 1) % pixelSize) == 0))
			{ // the alpha channel
				*d++ = (counter == 0) ? 0 : 255;
				counter = 0;
			}
			else
			{
				*d++ = gradient;
				if (gradient > 10) counter ++;
			}
        	}
		
		//  shuffle the row pointers 
		tmp = pr;
		pr = cr;
		cr = nr;
		nr = tmp;
			
		//store the dest 
		dst -> writeBytes(dest, x, row, width, 1);
		setProgress(row);		
	}
	setProgressDone();

	delete[] prevRow;
	delete[] curRow;
	delete[] nextRow;
	delete[] dest;
}


KisFilterConfigWidget * KisSobelFilter::createConfigurationWidget(QWidget* parent, KisPaintDeviceSP dev)
{
	vKisIntegerWidgetParam param;
	param.push_back( KisIntegerWidgetParam( 2, 40, 10, i18n("Pixelwidth") ) );
	param.push_back( KisIntegerWidgetParam( 2, 40, 10, i18n("Pixelheight") ) );
	return new KisMultiIntegerFilterWidget(parent, id().id().ascii(), id().id().ascii(), param );
}

KisFilterConfiguration* KisSobelFilter::configuration(QWidget* nwidget, KisPaintDeviceSP dev)
{
	KisMultiIntegerFilterWidget* widget = (KisMultiIntegerFilterWidget*) nwidget;
	if( widget == 0 )
	{
		return new KisSobelFilterConfiguration( 10, 10);
	} else {
		return new KisSobelFilterConfiguration( widget->valueAt( 0 ), widget->valueAt( 1 ) );
	}
}
