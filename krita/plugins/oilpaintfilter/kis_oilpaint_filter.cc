/*
 * This file is part of Krita
 *
 * Copyright (c) 2004 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
 *
 * ported from digikam, Copyright 2004 by Gilles Caulier,
 * Original Oilpaint algorithm copyrighted 2004 by
 * Pieter Z. Voloshyn <pieter_voloshyn at ame.com.br>.
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#include <kis_view.h>
#include <kis_progress_display_interface.h>

#include "kis_filter_configuration_widget.h"
#include "kis_multi_integer_filter_widget.h"
#include "kis_oilpaint_filter.h"

KisOilPaintFilter::KisOilPaintFilter(KisView * view) : KisFilter(id(), view)
{
}

void KisOilPaintFilter::process(KisPaintDeviceSP src, KisPaintDeviceSP dst, KisFilterConfiguration* configuration, const QRect& rect)
{
	kdDebug() << "Oilpaintfilter 2 called!\n";

	Q_UNUSED(dst);

	Q_INT32 x = rect.x(), y = rect.y();
	Q_INT32 width = rect.width();
	Q_INT32 height = rect.height();
	kdDebug() << "x: " << x << " y: " << y << " width: " << width << " height: " << height << endl;

	//read the filter configuration values from the KisFilterConfiguration object
	Q_UINT32 brushSize = ((KisOilPaintFilterConfiguration*)configuration)->brushSize();
	Q_UINT32 smooth = ((KisOilPaintFilterConfiguration*)configuration)->smooth();

	kdDebug() << "brushSize:" << brushSize << " smooth:" << smooth << "\n";

	OilPaint(src, x, y, width, height, brushSize, smooth);
}

// This method have been ported from Pieter Z. Voloshyn algorithm code.

/* Function to apply the OilPaint effect.
 *
 * data             => The image data in RGBA mode.
 * w                => Width of image.
 * h                => Height of image.
 * BrushSize        => Brush size.
 * Smoothness       => Smooth value.
 *
 * Theory           => Using MostFrequentColor function we take the main color in
 *                     a matrix and simply write at the original position.
 */

void KisOilPaintFilter::OilPaint(KisPaintDeviceSP src, int x, int y, int w, int h, int BrushSize, int Smoothness)
{
	setProgressTotalSteps(h);
	setProgressStage(i18n("Applying oilpaint filter..."),0);

	QRect bounds(x, y, w, h);

	for (Q_INT32 yOffset = 0; yOffset < h; yOffset++) {

		KisHLineIteratorPixel it = src -> createHLineIterator(x, y + yOffset, w, true);

		while (!it.isDone() && !cancelRequested()) {

			if (it.isSelected()) {

				uint color = MostFrequentColor(src, bounds, it.x(), it.y(), BrushSize, Smoothness);
				src -> colorStrategy() -> nativeColor(QColor(qRed(color), qGreen(color), qBlue(color)), qAlpha(color), it.rawData());
			}

			++it;
		}

		setProgress(yOffset);
	}

	setProgressDone();
}

// This method have been ported from Pieter Z. Voloshyn algorithm code.

/* Function to determine the most frequent color in a matrix
 *
 * Bits             => Bits array
 * Width            => Image width
 * Height           => Image height
 * X                => Position horizontal
 * Y                => Position vertical
 * Radius           => Is the radius of the matrix to be analized
 * Intensity        => Intensity to calcule
 *
 * Theory           => This function creates a matrix with the analized pixel in
 *                     the center of this matrix and find the most frequenty color
 */

uint KisOilPaintFilter::MostFrequentColor (KisPaintDeviceSP src, const QRect& bounds, int X, int Y, int Radius, int Intensity)
{
        uint color;
	uint I;

        double Scale = Intensity / 255.0;

        // Alloc some arrays to be used
        uchar *IntensityCount = new uchar[(Intensity + 1) * sizeof (uchar)];
        uint  *AverageColorR  = new uint[(Intensity + 1)  * sizeof (uint)];
        uint  *AverageColorG  = new uint[(Intensity + 1)  * sizeof (uint)];
        uint  *AverageColorB  = new uint[(Intensity + 1)  * sizeof (uint)];

        // Erase the array
        memset(IntensityCount, 0, (Intensity + 1) * sizeof (uchar));

        /*for (i = 0; i <= Intensity; ++i)
	  IntensityCount[i] = 0;*/

	KisRectIteratorPixel it = src -> createRectIterator(X - Radius, Y - Radius, (2 * Radius) + 1, (2 * Radius) + 1, false);

	while (!it.isDone()) {

		if (bounds.contains(it.x(), it.y())) {

			QColor c;
			src -> colorStrategy() -> toQColor(it.rawData(), &c);

			// Swapping red and blue here is done because that gives the same
			// output as digikam, even though it might be interpreted as a bug
			// in both applications.
			int b = c.red();
			int g = c.green();
			int r = c.blue();

			I = (uint)(GetIntensity (r, g, b) * Scale);
			IntensityCount[I]++;

			if (IntensityCount[I] == 1)
			{
				AverageColorR[I] = r;
				AverageColorG[I] = g;
				AverageColorB[I] = b;
			}
			else
			{
				AverageColorR[I] += r;
				AverageColorG[I] += g;
				AverageColorB[I] += b;
			}
		}

		++it;
	}

        I = 0;
        int MaxInstance = 0;

        for (int i = 0 ; i <= Intensity ; ++i)
        {
		if (IntensityCount[i] > MaxInstance)
                {
			I = i;
			MaxInstance = IntensityCount[i];
                }
        }

        int R, G, B;
		if (MaxInstance != 0) {
        	R = AverageColorR[I] / MaxInstance;
        	G = AverageColorG[I] / MaxInstance;
        	B = AverageColorB[I] / MaxInstance;
		} else {
			R = 0;
			G = 0;
			B = 0;
		}

	// Swap red and blue back to get the correct colour.
        color = qRgb (B, G, R);

        delete [] IntensityCount;        // free all the arrays
        delete [] AverageColorR;
        delete [] AverageColorG;
        delete [] AverageColorB;

        return (color);                    // return the most frequenty color
}


KisFilterConfigurationWidget* KisOilPaintFilter::createConfigurationWidget(QWidget* parent)
{
	vKisIntegerWidgetParam param;
	param.push_back( KisIntegerWidgetParam( 1, 5, 1, i18n("Brush size") ) );
	param.push_back( KisIntegerWidgetParam( 10, 255, 30, i18n("Smooth") ) );
	return new KisMultiIntegerFilterWidget(this, parent, id().id().ascii(), id().id().ascii(), param );
}

KisFilterConfiguration* KisOilPaintFilter::configuration(KisFilterConfigurationWidget* nwidget)
{
	KisMultiIntegerFilterWidget* widget = (KisMultiIntegerFilterWidget*) nwidget;
	if( widget == 0 )
	{
		return new KisOilPaintFilterConfiguration( 1, 30);
	} else {
		return new KisOilPaintFilterConfiguration( widget->valueAt( 0 ), widget->valueAt( 1 ) );
	}
}
