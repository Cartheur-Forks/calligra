/*
 *  Copyright (c) 2002 Patrick Julien  <freak@codepimps.org>
 *  Copyright (c) 2005 Adrian Page <adrian@pagenet.plus.com>
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
#ifndef KIS_STRATEGY_COLORSPACE_RGB_U16_H_
#define KIS_STRATEGY_COLORSPACE_RGB_U16_H_

#include <qcolor.h>

#include <koffice_export.h>

#include "kis_global.h"
#include "kis_u16_base_colorspace.h"
#include "kis_abstract_colorspace.h"
#include "kis_pixel.h"
#include "kis_integer_maths.h"


class KRITATOOL_EXPORT KisRgbU16ColorSpace : public KisU16BaseColorSpace {
public:

    struct Pixel {
        Q_UINT16 blue;
        Q_UINT16 green;
        Q_UINT16 red;
        Q_UINT16 alpha;
    };

public:
    KisRgbU16ColorSpace();
    virtual ~KisRgbU16ColorSpace();

public:
    void setPixel(Q_UINT8 *pixel, Q_UINT16 red, Q_UINT16 green, Q_UINT16 blue, Q_UINT16 alpha) const;
    void getPixel(const Q_UINT8 *pixel, Q_UINT16 *red, Q_UINT16 *green, Q_UINT16 *blue, Q_UINT16 *alpha) const;

    virtual void fromQColor(const QColor& c, Q_UINT8 *dst, KisProfileSP profile = 0);
    virtual void fromQColor(const QColor& c, QUANTUM opacity, Q_UINT8 *dst, KisProfileSP profile = 0);

    virtual void getAlpha(const Q_UINT8 *pixel, Q_UINT8 *alpha);
    virtual void setAlpha(Q_UINT8 *pixels, Q_UINT8 alpha, Q_INT32 nPixels);

    virtual void toQColor(const Q_UINT8 *src, QColor *c, KisProfileSP profile = 0);
    virtual void toQColor(const Q_UINT8 *src, QColor *c, QUANTUM *opacity, KisProfileSP profile = 0);

    //XXX: KisPixel(RO) does not work with this colourspace as it only handles 8-bit channels.
    virtual KisPixelRO toKisPixelRO(const Q_UINT8 *src, KisProfileSP profile = 0)
        { return KisPixelRO (src, src + PIXEL_ALPHA * sizeof(Q_UINT16), this, profile); }
    virtual KisPixel toKisPixel(Q_UINT8 *src, KisProfileSP profile = 0)
        { return KisPixel (src, src + PIXEL_ALPHA * sizeof(Q_UINT16), this, profile); }

    virtual Q_INT8 difference(const Q_UINT8 *src1, const Q_UINT8 *src2);
    virtual void mixColors(const Q_UINT8 **colors, const Q_UINT8 *weights, Q_UINT32 nColors, Q_UINT8 *dst) const;

    virtual vKisChannelInfoSP channels() const;
    virtual bool hasAlpha() const;
    virtual Q_INT32 nChannels() const;
    virtual Q_INT32 nColorChannels() const;
    virtual Q_INT32 pixelSize() const;

    virtual QString channelValueText(const Q_UINT8 *pixel, Q_UINT32 channelIndex) const;
    virtual QString normalisedChannelValueText(const Q_UINT8 *pixel, Q_UINT32 channelIndex) const;

    virtual QImage convertToQImage(const Q_UINT8 *data, Q_INT32 width, Q_INT32 height,
                       KisProfileSP srcProfile, KisProfileSP dstProfile,
                       Q_INT32 renderingIntent,
                       float exposure = 0.0f);

    virtual KisCompositeOpList userVisiblecompositeOps() const;
    
    virtual void adjustBrightnessContrast(const Q_UINT8 *src, Q_UINT8 *dst, Q_INT8 brightness, Q_INT8 contrast, Q_INT32 nPixels) const;

protected:

    virtual void bitBlt(Q_UINT8 *dst,
                Q_INT32 dstRowStride,
                const Q_UINT8 *src,
                Q_INT32 srcRowStride,
                const Q_UINT8 *srcAlphaMask,
                Q_INT32 maskRowStride,
                QUANTUM opacity,
                Q_INT32 rows,
                Q_INT32 cols,
                const KisCompositeOp& op);

    void compositeOver(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeMultiply(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeDivide(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeScreen(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeOverlay(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeDodge(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeBurn(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeDarken(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeLighten(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeHue(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeSaturation(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeValue(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeColor(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeErase(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);
    void compositeCopy(Q_UINT8 *dst, Q_INT32 dstRowStride, const Q_UINT8 *src, Q_INT32 srcRowStride, const Q_UINT8 *mask, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 columns, Q_UINT16 opacity);

private:
    friend class KisRgbU16ColorSpaceTester;

    vKisChannelInfoSP m_channels;

    static const Q_UINT8 PIXEL_BLUE = 0;
    static const Q_UINT8 PIXEL_GREEN = 1;
    static const Q_UINT8 PIXEL_RED = 2;
    static const Q_UINT8 PIXEL_ALPHA = 3;
};

#endif // KIS_STRATEGY_COLORSPACE_RGB_U16_H_
