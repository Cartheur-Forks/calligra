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
#ifndef KIS_RGB_F16_HDR_COLORSPACE_H_
#define KIS_RGB_F16_HDR_COLORSPACE_H_

#include <klocale.h>

#include <half.h>

#include "kis_rgb_float_hdr_colorspace.h"
#include <KoColorModelStandardIds.h>

#include "krita_rgbf16_export.h"

#include "KoColorSpaceTraits.h"
#include "KoScaleColorConversionTransformation.h"

typedef KoRgbTraits<half> RgbF16Traits;

class KRITA_RGBF16_EXPORT KisRgbF16HDRColorSpace : public KisRgbFloatHDRColorSpace<RgbF16Traits>
{
public:
    KisRgbF16HDRColorSpace( KoColorProfile *p);

    virtual KoID colorModelId() const { return RGBAColorModelID; }
    virtual KoID colorDepthId() const { return Float16BitsColorDepthID; }
    virtual KoColorSpace* clone() const;
    /**
     * The ID that identifies this colorspace. Pass this as the colorSpaceId parameter 
     * to the KoColorSpaceRegistry::colorSpace() functions to obtain this colorspace.
     * This is the value that the member function id() returns.
     */
    static QString colorSpaceId();
};

class KRITA_RGBF16_EXPORT KisRgbF16HDRColorSpaceFactory : public KisRgbFloatHDRColorSpaceFactory
{
public:
    /**
     * Krita definition for use in .kra files and internally: unchanging name +
     * i18n'able description.
     */
    virtual QString id() const { return KisRgbF16HDRColorSpace::colorSpaceId(); }
    virtual QString name() const { return i18n("RGB (16-bit float/channel) for High Dynamic Range imaging"); }
    virtual KoID colorModelId() const { return RGBAColorModelID; }
    virtual KoID colorDepthId() const { return Float16BitsColorDepthID; }
    virtual int referenceDepth() const { return 16; }
    virtual bool userVisible() const { return true; }

    virtual KoColorSpace *createColorSpace( const KoColorProfile * p) const
    {
        return new KisRgbF16HDRColorSpace( p->clone() );
    }
    virtual QList<KoColorConversionTransformationFactory*> colorConversionLinks() const
    {
        QList<KoColorConversionTransformationFactory*> list;
        // Conversion to RGB Float 16bit
        list.append(new KoScaleColorConversionTransformationFactory< KoRgbTraits<quint8>, RgbF16Traits >( RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), Float16BitsColorDepthID.id() ) );
        list.append(new KoScaleColorConversionTransformationFactory< KoRgbU16Traits, RgbF16Traits >( RGBAColorModelID.id(), Integer16BitsColorDepthID.id(), Float16BitsColorDepthID.id() ) );
        list.append(new KoScaleColorConversionTransformationFactory< KoRgbTraits<float>, RgbF16Traits >( RGBAColorModelID.id(), Float32BitsColorDepthID.id(), Float16BitsColorDepthID.id() ) );
        // Conversion from RGB Float 16bit
        list.append(new KoScaleColorConversionTransformationFactory< RgbF16Traits, KoRgbTraits<quint8> >( RGBAColorModelID.id(), Float16BitsColorDepthID.id(), Integer8BitsColorDepthID.id() ) );
        list.append(new KoScaleColorConversionTransformationFactory< RgbF16Traits, KoRgbU16Traits  >( RGBAColorModelID.id(), Float16BitsColorDepthID.id(), Integer16BitsColorDepthID.id() ) );
        list.append(new KoScaleColorConversionTransformationFactory< RgbF16Traits, KoRgbTraits<float> >( RGBAColorModelID.id(), Float16BitsColorDepthID.id(), Float32BitsColorDepthID.id() ) );
        
        return list;
    }
    virtual KoColorConversionTransformationFactory* createICCColorConversionTransformationFactory(QString _colorModelId, QString _colorDepthId) const
    {
        Q_UNUSED(_colorModelId);
        Q_UNUSED(_colorDepthId);
        return 0;
    }
};

#endif

