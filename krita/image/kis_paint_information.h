/*
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef _KIS_PAINT_INFORMATION_
#define _KIS_PAINT_INFORMATION_

#include "kis_global.h"
#include "kis_vec.h"
#include "krita_export.h"

class QDomDocument;
class QDomElement;

/**
 * This class keeps information that can be used in the painting process, for example by
 * brushes.
 **/
class KRITAIMAGE_EXPORT KisPaintInformation {

public:

    KisPaintInformation(const QPointF & pos = QPointF(), double pressure = PRESSURE_DEFAULT,
                        double xTilt = 0.0, double yTilt = 0.0,
                        KisVector2D movement = KisVector2D());
    KisPaintInformation(const KisPaintInformation& rhs);
    void operator=(const KisPaintInformation& rhs);
    ~KisPaintInformation();

    QPointF pos() const;
    void setPos(const QPointF& p);
        
    /// The pressure of the value (from 0.0 to 1.0)
    double pressure() const;
    /// Set the pressure
    void setPressure(double p);

    /// The tilt of the pen on the horizontal axis (from 0.0 to 1.0)
    double xTilt() const;

    /// The tilt of the pen on the vertical axis (from 0.0 to 1.0)
    double yTilt() const;

    /// The movement of the pen is equal to current position minus the last position of the call to paintAt
    KisVector2D movement() const;
    
    /// Rotation computed from the movement
    double angle() const;
    
    void toXML(QDomDocument&, QDomElement&) const;
    static KisPaintInformation fromXML(const QDomElement&);
private:
    struct Private;
    Private* const d;
};
#endif
