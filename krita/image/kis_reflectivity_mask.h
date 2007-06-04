/*
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
 *
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
#ifndef _KIS_REFLECTIVITY_MASK_
#define _KIS_REFLECTIVITY_MASK_

#include "kis_types.h"

#include "kis_mask.h"

/**
   Reflectivity affects the measure in which paint allows the light to
   pass through or be reflected by the layer.
*/
class KRITAIMAGE_EXPORT KisReflectivityMask : public KisMask
{
    KisReflectivityMask( KisPaintDeviceSP device );
    virtual ~KisReflectivityMask();
    KisReflectivityMask( const KisReflectivityMask& rhs );
};

#endif //_KIS_REFLECTIVITY_MASK_
