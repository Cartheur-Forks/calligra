/*
 *  Copyright (c) 2004 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
 *  Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
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

#ifndef KIS_TRANSFORM_VISITOR_H_
#define KIS_TRANSFORM_VISITOR_H_

#include "kis_types.h"
#include "kis_progress_subject.h"

class KisPaintDeviceImpl;
class KisProgressDisplayInterface;
class KisHLineIteratorPixel;
class KisVLineIteratorPixel;
class KisFilterStrategy;

class KisTransformVisitor : public KisProgressSubject {
    typedef KisProgressSubject super;

public:
    KisTransformVisitor(double  xscale, double  yscale, 
        double  xshear, double  yshear, double rotation,
        Q_INT32  xtranslate, Q_INT32  ytranslate,
        KisProgressDisplayInterface *progress, KisFilterStrategy *filter);
    ~KisTransformVisitor();

public:
	bool isCanceled() { return m_cancelRequested;};
private:
    bool visit(KisPainter& gc, KisPaintDeviceImpl *dev);
    
    // XXX (BSAR): Why didn't we use the shared-pointer versions of the paint device classes?
    template <class T> void transformPass(KisPaintDeviceImpl *src, KisPaintDeviceImpl *dst, double xscale, double  shear, Q_INT32 dx,   KisFilterStrategy *filterStrategy);
    
    void rotateRight90(KisPaintDeviceImplSP src, KisPaintDeviceImplSP dst);
    void rotateLeft90(KisPaintDeviceImplSP src, KisPaintDeviceImplSP dst);
    void rotate180(KisPaintDeviceImplSP src, KisPaintDeviceImplSP dst);

private:
    double  m_xscale, m_yscale;
    double  m_xshear, m_yshear, m_rotation;
    Q_INT32  m_xtranslate, m_ytranslate;
    KisProgressDisplayInterface *m_progress;
    KisFilterStrategy *m_filter;
    // Implement KisProgressSubject
    bool m_cancelRequested;
    virtual void cancel() { m_cancelRequested = true; }
    Q_INT32 m_progressTotalSteps;
    Q_INT32 m_progressStep;
};


inline KisTransformVisitor::~KisTransformVisitor()
{
}

#endif // KIS_TRANSFORM_VISITOR_H_
