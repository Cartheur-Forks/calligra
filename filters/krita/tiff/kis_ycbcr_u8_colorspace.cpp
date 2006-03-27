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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kis_ycbcr_u8_colorspace.h"

#include <qimage.h>

#include <klocale.h>

#include <kis_integer_maths.h>

namespace {
    const qint32 MAX_CHANNEL_YCbCr = 3;
    const qint32 MAX_CHANNEL_YCbCrA = 4;
}

KisYCbCrU8ColorSpace::KisYCbCrU8ColorSpace(KisColorSpaceFactoryRegistry* parent, KisProfile* p)
    : KisU8BaseColorSpace(KisID("YCbCrAU8", "YCbCr (8-bit integer/channel)"), TYPE_YCbCr_8, icSigYCbCrData, parent, p)
{
    m_channels.push_back(new KisChannelInfo("Y", PIXEL_Y * sizeof(quint8), KisChannelInfo::COLOR, KisChannelInfo::UINT8, sizeof(quint8)));
    m_channels.push_back(new KisChannelInfo("Cb", PIXEL_Cb * sizeof(quint8), KisChannelInfo::COLOR, KisChannelInfo::UINT8, sizeof(quint8)));
    m_channels.push_back(new KisChannelInfo("Cr", PIXEL_Cr * sizeof(quint8), KisChannelInfo::COLOR, KisChannelInfo::UINT8, sizeof(quint8)));
    m_channels.push_back(new KisChannelInfo(i18n("Alpha"), PIXEL_ALPHA * sizeof(quint8), KisChannelInfo::ALPHA, KisChannelInfo::UINT8, sizeof(quint8)));

    m_alphaPos = PIXEL_ALPHA * sizeof(quint8);
}


KisYCbCrU8ColorSpace::~KisYCbCrU8ColorSpace()
{
}

void KisYCbCrU8ColorSpace::setPixel(quint8 *dst, quint8 Y, quint8 Cb, quint8 Cr, quint8 alpha) const
{
    Pixel *dstPixel = reinterpret_cast<Pixel *>(dst);

    dstPixel->Y = Y;
    dstPixel->Cb = Cb;
    dstPixel->Cr = Cr;
    dstPixel->alpha = alpha;
}

void KisYCbCrU8ColorSpace::getPixel(const quint8 *src, quint8 *Y, quint8 *Cb, quint8 *Cr, quint8 *alpha) const
{
    const Pixel *srcPixel = reinterpret_cast<const Pixel *>(src);

    *Y = srcPixel->Y;
    *Cb = srcPixel->Cb;
    *Cr = srcPixel->Cr;
    *alpha = srcPixel->alpha;

}

void KisYCbCrU8ColorSpace::fromQColor(const QColor& c, quint8 *dstU8, KisProfile * profile )
{
    if(getProfile())
    {
        KisU8BaseColorSpace::fromQColor(c, dstU8, profile);
    } else {
        Pixel *dst = reinterpret_cast<Pixel *>(dstU8);
        dst->Y = computeY( c.Qt::red(), c.Qt::green(), c.Qt::blue());
        dst->Cb = computeCb( c.Qt::red(), c.Qt::green(), c.Qt::blue());
        dst->Cr = computeCr( c.Qt::red(), c.Qt::green(), c.Qt::blue());
    }
}

void KisYCbCrU8ColorSpace::fromQColor(const QColor& c, quint8 opacity, quint8 *dstU8, KisProfile * profile )
{
    if(getProfile())
    {
        KisU8BaseColorSpace::fromQColor(c, opacity, dstU8, profile);
    } else {
        Pixel *dst = reinterpret_cast<Pixel *>(dstU8);
        dst->Y = computeY( c.Qt::red(), c.Qt::green(), c.Qt::blue());
        dst->Cb = computeCb( c.Qt::red(), c.Qt::green(), c.Qt::blue());
        dst->Cr = computeCr( c.Qt::red(), c.Qt::green(), c.Qt::blue());
        dst->alpha = opacity;
    }
}

void KisYCbCrU8ColorSpace::toQColor(const quint8 *srcU8, QColor *c, KisProfile * profile)
{
    if(getProfile())
    {
        KisU8BaseColorSpace::toQColor(srcU8, c, profile);
        
    } else {
        const Pixel *src = reinterpret_cast<const Pixel *>(srcU8);
        c->setRgb(computeRed(src->Y,src->Cb,src->Cr), computeGreen(src->Y,src->Cb,src->Cr),     computeBlue(src->Y,src->Cb,src->Cr));
    }
}

void KisYCbCrU8ColorSpace::toQColor(const quint8 *srcU8, QColor *c, quint8 *opacity, KisProfile * profile)
{
    if(getProfile())
    {
        KisU8BaseColorSpace::toQColor(srcU8, c, opacity, profile);
    } else {
        const Pixel *src = reinterpret_cast<const Pixel *>(srcU8);
        c->setRgb(computeRed(src->Y,src->Cb,src->Cr), computeGreen(src->Y,src->Cb,src->Cr), computeBlue(src->Y,src->Cb,src->Cr));
        *opacity = src->alpha;
    }
}

quint8 KisYCbCrU8ColorSpace::difference(const quint8 *src1U8, const quint8 *src2U8)
{
    if(getProfile())
        return KisU8BaseColorSpace::difference(src1U8, src2U8);
    const Pixel *src1 = reinterpret_cast<const Pixel *>(src1U8);
    const Pixel *src2 = reinterpret_cast<const Pixel *>(src2U8);

    return qMax(QABS(src2->Y - src1->Y), qMax(QABS(src2->Cb - src1->Cb), QABS(src2->Cr - src1->Cr)));

}

void KisYCbCrU8ColorSpace::mixColors(const quint8 **colors, const quint8 *weights, quint32 nColors, quint8 *dst) const
{
    quint8 totalY = 0, totalCb = 0, totalCr = 0, newAlpha = 0;

    while (nColors--)
    {
        const Pixel *pixel = reinterpret_cast<const Pixel *>(*colors);

        quint8 alpha = pixel->alpha;
        float alphaTimesWeight = alpha * *weights;

        totalY += (quint8)(pixel->Y * alphaTimesWeight);
        totalCb += (quint8)(pixel->Cb * alphaTimesWeight);
        totalCr += (quint8)(pixel->Cr * alphaTimesWeight);
        newAlpha += (quint8)(alphaTimesWeight);

        weights++;
        colors++;
    }

    Pixel *dstPixel = reinterpret_cast<Pixel *>(dst);

    dstPixel->alpha = newAlpha;

    if (newAlpha > 0) {
        totalY = totalY / newAlpha;
        totalCb = totalCb / newAlpha;
        totalCr = totalCr / newAlpha;
    }

    dstPixel->Y = totalY;
    dstPixel->Cb = totalCb;
    dstPixel->Cr = totalCr;
}

Q3ValueVector<KisChannelInfo *> KisYCbCrU8ColorSpace::channels() const {
    return m_channels;
}

quint32 KisYCbCrU8ColorSpace::nChannels() const {
    return MAX_CHANNEL_YCbCrA;
}

quint32 KisYCbCrU8ColorSpace::nColorChannels() const {
    return MAX_CHANNEL_YCbCr;
}

quint32 KisYCbCrU8ColorSpace::pixelSize() const {
    return MAX_CHANNEL_YCbCrA*sizeof(quint8);
}


QImage KisYCbCrU8ColorSpace::convertToQImage(const quint8 *data, qint32 width, qint32 height, KisProfile *  dstProfile, qint32 renderingIntent, float exposure )
{
    if(getProfile())
        return KisU8BaseColorSpace::convertToQImage( data, width, height, dstProfile, renderingIntent, exposure);
    
    QImage img = QImage(width, height, 32, 0, QImage::LittleEndian);
    img.setAlphaBuffer(true);

    qint32 i = 0;
    uchar *j = img.bits();

    while ( i < width * height * MAX_CHANNEL_YCbCrA) {
        quint8 Y = *( data + i + PIXEL_Y );
        quint8 Cb = *( data + i + PIXEL_Cb );
        quint8 Cr = *( data + i + PIXEL_Cr );
#ifdef __BIG_ENDIAN__
        *( j + 0)  = *( data + i + PIXEL_ALPHA );
        *( j + 1 ) = computeRed(Y,Cb,Cr);
        *( j + 2 ) = computeGreen(Y,Cb,Cr);
        *( j + 3 ) = computeBlue(Y,Cr,Cr);
#else
        *( j + 3)  = *( data + i + PIXEL_ALPHA );
        *( j + 2 ) = computeRed(Y,Cb,Cr);
        *( j + 1 ) = computeGreen(Y,Cb,Cr);
        *( j + 0 ) = computeBlue(Y,Cb,Cr);
/*        *( j + 2 ) = Y;
        *( j + 1 ) = Cb;
        *( j + 0 ) = Cr;*/
#endif
        i += MAX_CHANNEL_YCbCrA;
        j += MAX_CHANNEL_YCbCrA;
    }
    return img;
}


void KisYCbCrU8ColorSpace::bitBlt(quint8 *dst, qint32 dstRowStride, const quint8 *src, qint32 srcRowStride, const quint8 *srcAlphaMask, qint32 maskRowStride, quint8 opacity, qint32 rows, qint32 cols, const KisCompositeOp& op)
{
    switch (op.op()) {
        case COMPOSITE_UNDEF:
        // Undefined == no composition
            break;
        case COMPOSITE_OVER:
            compositeOver(dst, dstRowStride, src, srcRowStride, srcAlphaMask, maskRowStride, rows, cols, opacity);
            break;
        case COMPOSITE_COPY:
            compositeCopy(dst, dstRowStride, src, srcRowStride, srcAlphaMask, maskRowStride, rows, cols, opacity);
            break;
        case COMPOSITE_ERASE:
            compositeErase(dst, dstRowStride, src, srcRowStride, srcAlphaMask, maskRowStride, rows, cols, opacity);
            break;
        default:
            break;
    }
}

void KisYCbCrU8ColorSpace::compositeOver(quint8 *dstRowStart, qint32 dstRowStride, const quint8 *srcRowStart, qint32 srcRowStride, const quint8 *maskRowStart, qint32 maskRowStride, qint32 rows, qint32 numColumns, quint8 opacity)
{
    while (rows > 0) {

        const quint8 *src = srcRowStart;
        quint8 *dst = dstRowStart;
        const quint8 *mask = maskRowStart;
        qint32 columns = numColumns;

        while (columns > 0) {

            quint8 srcAlpha = src[PIXEL_ALPHA];

            // apply the alphamask
            if (mask != 0) {
                if (*mask != OPACITY_OPAQUE) {
                    srcAlpha *= *mask;
                }
                mask++;
            }

            if (srcAlpha > OPACITY_TRANSPARENT) {

                if (opacity < OPACITY_OPAQUE) {
                    srcAlpha *= opacity;
                }

                if (srcAlpha == OPACITY_OPAQUE) {
                    memcpy(dst, src, MAX_CHANNEL_YCbCrA * sizeof(quint8));
                } else {
                    quint8 dstAlpha = dst[PIXEL_ALPHA];

                    quint8 srcBlend;

                    if (dstAlpha == OPACITY_OPAQUE ) {
                        srcBlend = srcAlpha;
                    } else {
                        quint8 newAlpha = dstAlpha + (OPACITY_OPAQUE - dstAlpha) * srcAlpha;
                        dst[PIXEL_ALPHA] = newAlpha;

                        if (newAlpha > 0) {
                            srcBlend = srcAlpha / newAlpha;
                        } else {
                            srcBlend = srcAlpha;
                        }
                    }

                    if (srcBlend == OPACITY_OPAQUE) {
                        memcpy(dst, src, MAX_CHANNEL_YCbCr * sizeof(quint8));
                    } else {
                        dst[PIXEL_Y] = UINT8_BLEND(src[PIXEL_Y], dst[PIXEL_Y], srcBlend);
                        dst[PIXEL_Cb] = UINT8_BLEND(src[PIXEL_Cb], dst[PIXEL_Cb], srcBlend);
                        dst[PIXEL_Cr] = UINT8_BLEND(src[PIXEL_Cr], dst[PIXEL_Cr], srcBlend);
                    }
                }
            }

            columns--;
            src += MAX_CHANNEL_YCbCrA;
            dst += MAX_CHANNEL_YCbCrA;
        }

        rows--;
        srcRowStart += srcRowStride;
        dstRowStart += dstRowStride;
        if(maskRowStart) {
            maskRowStart += maskRowStride;
        }
    }
}

void KisYCbCrU8ColorSpace::compositeErase(quint8 *dst, qint32 dstRowSize, const quint8 *src, qint32 srcRowSize, const quint8 *srcAlphaMask, qint32 maskRowStride, qint32 rows, qint32 cols, quint8 /*opacity*/)
{
    while (rows-- > 0)
    {
        const Pixel *s = reinterpret_cast<const Pixel *>(src);
        Pixel *d = reinterpret_cast<Pixel *>(dst);
        const quint8 *mask = srcAlphaMask;

        for (qint32 i = cols; i > 0; i--, s++, d++)
        {
            quint8 srcAlpha = s -> alpha;

            // apply the alphamask
            if (mask != 0) {
                if (*mask != OPACITY_OPAQUE) {
                    srcAlpha = *mask;
                }
                mask++;
            }
            d -> alpha = srcAlpha * d -> alpha;
        }

        dst += dstRowSize;
        src += srcRowSize;
        if(srcAlphaMask) {
            srcAlphaMask += maskRowStride;
        }
    }
}

void KisYCbCrU8ColorSpace::compositeCopy(quint8 *dstRowStart, qint32 dstRowStride, const quint8 *srcRowStart, qint32 srcRowStride, const quint8 */*mask*/, qint32 /*maskRowStride*/, qint32 rows, qint32 numColumns, quint8 /*opacity*/)
{
    while (rows > 0) {
        memcpy(dstRowStart, srcRowStart, numColumns * sizeof(Pixel));
        --rows;
        srcRowStart += srcRowStride;
        dstRowStart += dstRowStride;
    }
}

KisCompositeOpList KisYCbCrU8ColorSpace::userVisiblecompositeOps() const
{
    KisCompositeOpList list;

    list.append(KisCompositeOp(COMPOSITE_OVER));
    return list;
}
