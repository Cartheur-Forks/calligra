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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#include <qrect.h>

#include <kdebug.h>

#include "kis_brush.h"
#include "kis_global.h"
#include "kis_paint_device.h"
#include "kis_painter.h"
#include "kis_types.h"
#include "kis_paintop.h"

#include "kis_penop.h"


KisPaintOp * KisPenOpFactory::createOp(KisPainter * painter)
{ 
	KisPaintOp * op = new KisPenOp(painter); 
	return op; 
}


KisPenOp::KisPenOp(KisPainter * painter)
	: super(painter) 
{
}

KisPenOp::~KisPenOp() 
{
}

void KisPenOp::paintAt(const KisPoint &pos,
		       const double pressure,
		       const double /*xTilt*/,
		       const double /*yTilt*/)
{
	if (!m_painter) return;
	KisPaintDeviceSP device = m_painter -> device();
	if (!device) return;
	KisBrush * brush = m_painter -> brush();
	if (!brush) return;


	KisPoint hotSpot = brush -> hotSpot(pressure);
	KisPoint pt = pos - hotSpot;

	// Split the coordinates into integer plus fractional parts. The integer
	// is where the dab will be positioned and the fractional part determines
	// the sub-pixel positioning.
	Q_INT32 x;
	double xFraction;
	Q_INT32 y;
	double yFraction;

	splitCoordinate(pt.x(), &x, &xFraction);
	splitCoordinate(pt.y(), &y, &yFraction);

	KisPaintDeviceSP dab = 0;
	if (brush -> brushType() == IMAGE || 
	    brush -> brushType() == PIPE_IMAGE) {
		dab = brush -> image(device -> colorStrategy(), pressure);
	}
	else {
		// Compute mask without sub-pixel positioning
		KisAlphaMaskSP mask = brush -> mask(pressure);
		dab = computeDab(mask);
	}
	
	m_painter -> setPressure(pressure);

	Q_INT32 dabWidth = dab -> extent().width();
	Q_INT32 dabHeight = dab -> extent().height();

	Q_ASSERT(dab -> extent().x() == 0);
	Q_ASSERT(dab -> extent().y() == 0);

	m_painter->bltSelection( x,  y,  m_painter -> compositeOp(), dab.data(), m_painter -> opacity(), 0, 0, dabWidth, dabHeight);
	m_painter->addDirtyRect(QRect(x, y, dabWidth, dabHeight));
}
