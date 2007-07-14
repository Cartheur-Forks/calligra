/*
 *  Copyright (c) 2006-2007 Cyrille Berger <cberger@cberger.net>
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

#ifndef KIS_YCBCR_U16_COLORSPACE_H
#define KIS_YCBCR_U16_COLORSPACE_H

#include "klocale.h"
#include "kis_ycbcr_base_colorspace.h"

#include "kis_ycbcr_traits.h"

typedef KoYCbCrTraits<quint16> YCbCrU16Traits;

class KisYCbCrU16ColorSpace : public KisYCbCrBaseColorSpace<YCbCrU16Traits>
{
    public:
        KisYCbCrU16ColorSpace(KoColorSpaceRegistry * parent, KoColorProfile *p);
        virtual bool willDegrade(ColorSpaceIndependence independence) const {
          if (independence == TO_RGBA8 )
            return true;
          else
            return false;
        }
};

class KisYCbCrU16ColorSpaceFactory : public KoColorSpaceFactory
{
public:
    virtual QString id() const { return "YCbCrAU16"; }
    virtual QString name() const { return i18n("YCBCR (16-bit integer/channel)"); }

    virtual bool profileIsCompatible(KoColorProfile* /*profile*/) const
    {
        return false;
    }

    virtual KoColorSpace *createColorSpace(KoColorSpaceRegistry * parent, KoColorProfile * p) { return new KisYCbCrU16ColorSpace(parent, p); }

    virtual QString defaultProfile() { return ""; }
};

#endif

