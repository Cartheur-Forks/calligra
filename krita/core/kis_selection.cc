/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *
 *  this program is free software; you can redistribute it and/or modify
 *  it under the terms of the gnu general public license as published by
 *  the free software foundation; either version 2 of the license, or
 *  (at your option) any later version.
 *
 *  this program is distributed in the hope that it will be useful,
 *  but without any warranty; without even the implied warranty of
 *  merchantability or fitness for a particular purpose.  see the
 *  gnu general public license for more details.
 *
 *  you should have received a copy of the gnu general public license
 *  along with this program; if not, write to the free software
 *  foundation, inc., 675 mass ave, cambridge, ma 02139, usa.
 */

#include <qimage.h>

#include <kdebug.h>
#include <klocale.h>
#include <qcolor.h>

#include "kis_layer.h"
#include "kis_selection.h"
#include "kis_global.h"
#include "kis_types.h"
#include "kis_colorspace_registry.h"
#include "kis_fill_painter.h"
#include "kis_colorspace_alpha.h"
#include "kis_iterators_pixel.h"
#include "kis_integer_maths.h"

KisSelection::KisSelection(KisPaintDeviceSP layer, const QString& name)
 	: super(
		layer -> image(), 
 		new KisColorSpaceAlpha(), // Note that the alpha color
					  // model has _state_, so we
					  // create a new one, instead
		name)
{
	m_parentLayer = layer;
	m_maskColor = Qt::white;
	m_alpha = KisColorSpaceAlphaSP(dynamic_cast<KisColorSpaceAlpha*> (colorStrategy().data()));
 	m_alpha -> setMaskColor(m_maskColor);
}

KisSelection::KisSelection(KisPaintDeviceSP layer, const QString& name, QColor color)
	: super(layer -> colorStrategy(), name)
{
	m_parentLayer = layer;
	m_maskColor = color;
}


KisSelection::~KisSelection()
{
}

QUANTUM KisSelection::selected(Q_INT32 x, Q_INT32 y)
{
	QColor c;
	QUANTUM opacity;
	if (pixel(x, y, &c, &opacity)) {
		return opacity;
	}
	else {
		return MIN_SELECTED;
	}
}

void KisSelection::setSelected(Q_INT32 x, Q_INT32 y, QUANTUM s)
{
	setPixel(x, y, m_maskColor, s);
}

QImage KisSelection::maskImage()
{
	Q_INT32 x, y, w, h, y2, x2;
	m_parentLayer -> exactBounds(x, y, w, h);
	QImage img = QImage(w, h, 32);;

	for (y2 = y; y2 < h - y; ++y2) {
		KisHLineIteratorPixel it = createHLineIterator(x, y2, w, false);
		x2 = 0;
		while (!it.isDone()) {
			Q_UINT8 s = MAX_SELECTED - *(it.rawData());
			Q_INT32 c = qRgb(s, s, s);
			img.setPixel(x2, y2, c);
			++x2;
			++it;
		}
	}
	return img;
}

void KisSelection::select(QRect r)
{
	KisFillPainter painter(this);
	painter.fillRect(r, m_maskColor, MAX_SELECTED);
	Q_INT32 x, y, w, h;
	extent(x, y, w, h);
	kdDebug (DBG_AREA_CORE) << "Selected rect: x:" << x << ", y: " << y << ", w: " << w << ", h: " << h << "\n";
}

void KisSelection::clear(QRect r)
{
	KisFillPainter painter(this);
	painter.fillRect(r, m_maskColor, MIN_SELECTED);
}

void KisSelection::clear()
{
	Q_UINT8 defPixel = MIN_SELECTED;
	m_datamanager -> setDefaultPixel(&defPixel);
	m_datamanager -> clear();
}

void KisSelection::invert()
{
	Q_INT32 x,y,w,h;

	extent(x, y, w, h);
	KisRectIterator it = createRectIterator(x, y, w, h, true);
	while ( ! it.isDone() )
	{
		// CBR this is wrong only first byte is inverted
		// BSAR: But we have always only one byte in this color model :-).
		*(it.rawData()) = MAX_SELECTED - *(it.rawData());
		++it;
	}
	Q_UINT8 defPixel = MAX_SELECTED - *(m_datamanager -> defaultPixel());
	m_datamanager -> setDefaultPixel(&defPixel);
}

void KisSelection::setMaskColor(QColor c)
{
	m_alpha -> setMaskColor(c);
	m_maskColor = c;
}

bool KisSelection::isTotallyUnselected(QRect r)
{
	if(*(m_datamanager -> defaultPixel()) != MIN_SELECTED)
		return false;
	
	return ! r.intersects(extent());
}

QRect KisSelection::selectedRect()
{
	if(*(m_datamanager -> defaultPixel()) == MIN_SELECTED)
		return extent();
	else
		return extent().unite(m_parentLayer->extent());
}

QRect KisSelection::selectedExactRect()
{
	if(*(m_datamanager -> defaultPixel()) == MIN_SELECTED)
		return exactBounds();
	else
		return exactBounds().unite(m_parentLayer->exactBounds());
}

void KisSelection::paintSelection(QImage img, Q_INT32 x, Q_INT32 y, Q_INT32 w, Q_INT32 h)
{
	Q_INT32 x2;
	uchar *j = img.bits();

#if 1 // blueish monocrome with outline
	for (Q_INT32 y2 = y; y2 < h + y; ++y2) {
		KisHLineIteratorPixel it = createHLineIterator(x, y2, w+2, false);
		Q_UINT8 preS = *(it.rawData());
		++it;
		x2 = 0;
		KisHLineIteratorPixel prevLineIt = createHLineIterator(x, y2-1, w, false);
		KisHLineIteratorPixel nextLineIt = createHLineIterator(x, y2+1, w, false);
		while (!it.isDone() && x2<w) {
			Q_UINT8 s = *(it.rawData());
			++it;
			if(s!=MAX_SELECTED)
			{
				Q_UINT8 invs = MAX_SELECTED - s;
				
				Q_UINT8 g = (*(j + 0)  + *(j + 1 ) + *(j + 2 )) / 9;

				if(s==MIN_SELECTED)
				{
					*(j+0) = 165+g ;
					*(j+1) = 128+g;
					*(j+2) = 128+g;
					
					// now for a simple outline based on 4-connectivity
					if(preS != MIN_SELECTED
						|| *(it.rawData()) != MIN_SELECTED
						|| *(prevLineIt.rawData()) != MIN_SELECTED
						|| *(nextLineIt.rawData()) != MIN_SELECTED)
					{
						*(j+0) = 0;
						*(j+1) = 0;
						*(j+2) = 255;
					}
				}
				else
				{
					*(j+0) = UINT8_BLEND(*(j+0), g+165, s);
					*(j+1) = UINT8_BLEND(*(j+1), g+128, s);
					*(j+2) = UINT8_BLEND(*(j+2), g+128, s);
				}
			}
			j+=4;
			++x2;
			preS=s;
			++prevLineIt;
			++nextLineIt;
		}
	}
#endif
#if 0 // blueish monocrome
	for (Q_INT32 y2 = y; y2 < h + y; ++y2) {
		KisHLineIteratorPixel it = createHLineIterator(x, y2, w, false);
		x2 = 0;
		while (!it.isDone()) {
			Q_UINT8 s = *(it.rawData());
			if(s!=MAX_SELECTED)
			{
				Q_UINT8 invs = MAX_SELECTED - s;
				
				Q_UINT8 g = (*(j + 0)  + *(j + 1 ) + *(j + 2 )) / 9;

				if(s==MIN_SELECTED)
				{
					*(j+0) = 165+g ;
					*(j+1) = 128+g;
					*(j+2) = 128+g;
				}
				else
				{
					*(j+0) = UINT8_BLEND(*(j+0), g+165, s);
					*(j+1) = UINT8_BLEND(*(j+1), g+128, s);
					*(j+2) = UINT8_BLEND(*(j+2), g+128, s);
				}
			}
			j+=4;
			++x2;
			++it;
		}
	}
#endif
#if 0 // inverted grayscale with jump at crossing
	for (Q_INT32 y2 = y; y2 < h + y; ++y2) {
		KisHLineIteratorPixel it = createHLineIterator(x, y2, w, false);
		x2 = 0;
		while (!it.isDone()) {
			Q_UINT8 s = *(it.rawData());
			if(s!=MAX_SELECTED)
			{
				Q_UINT8 invs = MAX_SELECTED - s;
				
				Q_UINT8 g = 255 - (*(j + 0)  + *(j + 1 ) + *(j + 2 )) / 6;
				if(g<256/3-5)
					g-=30;
				if(s==MIN_SELECTED)
				{
					*(j+0) = g ;
					*(j+1) = g;
					*(j+2) = g;
				}
				else
				{
					 g = (invs*g)>>8;
					*(j+0) = g + ((s**(j+0))>>8);
					*(j+1) = g + ((s**(j+1))>>8);
					*(j+2) = g + ((s**(j+2))>>8);
				}
			}
			j+=4;
			++x2;
			++it;
		}
	}
#endif
}
