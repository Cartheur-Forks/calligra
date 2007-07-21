/*
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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
#ifndef KIS_STRATEGY_COLORSPACE_RGB_32F_HDR_H_
#define KIS_STRATEGY_COLORSPACE_RGB_32F_HDR_H_

#include <klocale.h>

#include "kis_rgb_float_hdr_colorspace.h"
#include "krita_rgbf32_export.h"

#include <KoColorSpaceTraits.h>

typedef KoRgbTraits<float> RgbF32Traits;

class KRITA_RGBF32_EXPORT KisRgbF32HDRColorSpace : public KisRgbFloatHDRColorSpace<RgbF32Traits>
{
    public:
        KisRgbF32HDRColorSpace(KoColorSpaceRegistry * parent, KoColorProfile *p);
};

class KRITA_RGBF32_EXPORT KisRgbF32HDRColorSpaceFactory : public KisRgbFloatHDRColorSpaceFactory
{
public:
    /**
     * Krita definition for use in .kra files and internally: unchanging name +
     * i18n'able description.
     */
    virtual QString id() const { return "RGBAF32"; }
    virtual QString name() const { return i18n("RGB (32-bit float/channel) for High Dynamic Range imaging"); }
    
    virtual KoColorSpace *createColorSpace(KoColorSpaceRegistry * parent, KoColorProfile * p) { return new KisRgbF32HDRColorSpace(parent, p); }
};

#endif

