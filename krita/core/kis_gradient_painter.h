/*
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
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
#ifndef KIS_GRADIENT_PAINTER_H_
#define KIS_GRADIENT_PAINTER_H_

#include <qbrush.h>
#include <qcolor.h>
#include <qfontinfo.h>
#include <qfontmetrics.h>
#include <qpen.h>
#include <qregion.h>
#include <qwmatrix.h>
#include <qimage.h>
#include <qmap.h>
#include <qpixmap.h>
#include <qpointarray.h>
#include <qstring.h>
#include <qpainter.h>
#include <qvaluevector.h>

#include <kcommand.h>

#include "kis_global.h"
#include "kis_types.h"
#include "kis_paint_device.h"
#include "kis_point.h"
#include "kis_matrix.h"
#include "kis_progress_subject.h"
#include "kis_painter.h"

class KisGradient;


// XXX: Need to set dirtyRect in KisPainter
class KisGradientPainter : public KisPainter
{

	typedef KisPainter super;

public:

        KisGradientPainter();
        KisGradientPainter(KisPaintDeviceSP device);


	enum enumGradientShape {
		GradientShapeLinear,
		GradientShapeBiLinear,
		GradientShapeRadial,
		GradientShapeSquare,
		GradientShapeConical,
		GradientShapeConicalSymetric
	};

	enum enumGradientRepeat {
		GradientRepeatNone,
		GradientRepeatForwards,
		GradientRepeatAlternate
	};

	void setGradient(KisGradient& gradient) { m_gradient = &gradient; }
	void setGradient(KisGradient* gradient) { m_gradient = gradient; }

	bool paintGradient(const KisPoint& gradientVectorStart,
			   const KisPoint& gradientVectorEnd,
			   enumGradientShape shape,
			   enumGradientRepeat repeat,
			   double antiAliasThreshold,
			   bool reverseGradient = false);


private:
	KisGradient *m_gradient;


};
#endif //KIS_GRADIENT_PAINTER_H_
