/*
 *  Copyright (c) 2002 Patrick Julien  <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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

#include <config.h>
#include <limits.h>
#include <stdlib.h>
#include LCMS_HEADER

#include <qimage.h>

#include <kdebug.h>
#include <klocale.h>

#include "kis_image.h"
#include "kis_strategy_colorspace_rgb_u16.h"
#include "kis_iterators_pixel.h"
#include "kis_color_conversions.h"
#include "kis_integer_maths.h"
#include "kis_colorspace_registry.h"

namespace {
	const Q_INT32 MAX_CHANNEL_RGB = 3;
	const Q_INT32 MAX_CHANNEL_RGBA = 4;
}

const Q_UINT16 KisStrategyColorSpaceRGBU16::U16_OPACITY_OPAQUE;
const Q_UINT16 KisStrategyColorSpaceRGBU16::U16_OPACITY_TRANSPARENT;

KisStrategyColorSpaceRGBU16::KisStrategyColorSpaceRGBU16() :
	KisStrategyColorSpace(KisID("RGBA16", i18n("RGB/Alpha (16-bit integer/channel)")), TYPE_BGRA_16, icSigRgbData)
{
	m_channels.push_back(new KisChannelInfo(i18n("Red"), PIXEL_RED * sizeof(Q_UINT16), COLOR, sizeof(Q_UINT16)));
	m_channels.push_back(new KisChannelInfo(i18n("Green"), PIXEL_GREEN * sizeof(Q_UINT16), COLOR, sizeof(Q_UINT16)));
	m_channels.push_back(new KisChannelInfo(i18n("Blue"), PIXEL_BLUE * sizeof(Q_UINT16), COLOR, sizeof(Q_UINT16)));
	m_channels.push_back(new KisChannelInfo(i18n("Alpha"), PIXEL_ALPHA * sizeof(Q_UINT16), ALPHA, sizeof(Q_UINT16)));
}

KisStrategyColorSpaceRGBU16::~KisStrategyColorSpaceRGBU16()
{
}

void KisStrategyColorSpaceRGBU16::setPixel(Q_UINT8 *dst, Q_UINT16 red, Q_UINT16 green, Q_UINT16 blue, Q_UINT16 alpha) const
{
	Pixel *dstPixel = reinterpret_cast<Pixel *>(dst);

	dstPixel -> red = red;
	dstPixel -> green = green;
	dstPixel -> blue = blue;
	dstPixel -> alpha = alpha;
}

void KisStrategyColorSpaceRGBU16::getPixel(const Q_UINT8 *src, Q_UINT16 *red, Q_UINT16 *green, Q_UINT16 *blue, Q_UINT16 *alpha) const
{
	const Pixel *srcPixel = reinterpret_cast<const Pixel *>(src);

	*red = srcPixel -> red;
	*green = srcPixel -> green;
	*blue = srcPixel -> blue;
	*alpha = srcPixel -> alpha;
}

void KisStrategyColorSpaceRGBU16::nativeColor(const QColor& c, Q_UINT8 *dstU8, KisProfileSP /*profile*/)
{
	Pixel *dst = reinterpret_cast<Pixel *>(dstU8);

	dst -> red = UINT8_TO_UINT16(c.red());
	dst -> green = UINT8_TO_UINT16(c.green());
	dst -> blue = UINT8_TO_UINT16(c.blue());
}

void KisStrategyColorSpaceRGBU16::nativeColor(const QColor& c, QUANTUM opacity, Q_UINT8 *dstU8, KisProfileSP /*profile*/)
{
	Pixel *dst = reinterpret_cast<Pixel *>(dstU8);

	dst -> red = UINT8_TO_UINT16(c.red());
	dst -> green = UINT8_TO_UINT16(c.green());
	dst -> blue = UINT8_TO_UINT16(c.blue());
	dst -> alpha = UINT8_TO_UINT16(opacity);
}

void KisStrategyColorSpaceRGBU16::getAlpha(const Q_UINT8 *U8_pixel, Q_UINT8 *alpha)
{
	const Pixel *pixel = reinterpret_cast<const Pixel *>(U8_pixel);
	*alpha = UINT16_TO_UINT8(pixel -> alpha);
}

void KisStrategyColorSpaceRGBU16::setAlpha(Q_UINT8 *pixels, Q_UINT8 alpha, Q_INT32 nPixels)
{
	Pixel *pixel = reinterpret_cast<Pixel *>(pixels);

	while (nPixels > 0) {
		pixel -> alpha = UINT8_TO_UINT16(alpha);
		--nPixels;
		++pixel;
	}
}

void KisStrategyColorSpaceRGBU16::toQColor(const Q_UINT8 *srcU8, QColor *c, KisProfileSP /*profile*/)
{
	const Pixel *src = reinterpret_cast<const Pixel *>(srcU8);

	c -> setRgb(UINT16_TO_UINT8(src -> red), UINT16_TO_UINT8(src -> green), UINT16_TO_UINT8(src -> blue));
}

void KisStrategyColorSpaceRGBU16::toQColor(const Q_UINT8 *srcU8, QColor *c, QUANTUM *opacity, KisProfileSP /*profile*/)
{
	const Pixel *src = reinterpret_cast<const Pixel *>(srcU8);

	c -> setRgb(UINT16_TO_UINT8(src -> red), UINT16_TO_UINT8(src -> green), UINT16_TO_UINT8(src -> blue));
	*opacity = UINT16_TO_UINT8(src -> alpha);
}

Q_INT8 KisStrategyColorSpaceRGBU16::difference(const Q_UINT8 *src1U8, const Q_UINT8 *src2U8)
{
	const Pixel *src1 = reinterpret_cast<const Pixel *>(src1U8);
	const Pixel *src2 = reinterpret_cast<const Pixel *>(src2U8);

	return UINT16_TO_UINT8(QMAX(QABS(src2 -> red - src1 -> red),
				QMAX(QABS(src2 -> green - src1 -> green),
				     QABS(src2 -> blue - src1 -> blue))));
}

void KisStrategyColorSpaceRGBU16::mixColors(const Q_UINT8 **colors, const Q_UINT8 *weights, Q_UINT32 nColors, Q_UINT8 *dst) const
{
	Q_UINT32 totalRed = 0, totalGreen = 0, totalBlue = 0, newAlpha = 0;
	
	while (nColors--)
	{
		const Pixel *pixel = reinterpret_cast<const Pixel *>(*colors);

		Q_UINT32 alpha = pixel -> alpha;
		Q_UINT32 alphaTimesWeight = UINT16_MULT(alpha, UINT8_TO_UINT16(*weights));

		totalRed += UINT16_MULT(pixel -> red, alphaTimesWeight);
		totalGreen += UINT16_MULT(pixel -> green, alphaTimesWeight);
		totalBlue += UINT16_MULT(pixel -> blue, alphaTimesWeight);
		newAlpha += alphaTimesWeight;

		weights++;
		colors++;
	}

	Q_ASSERT(newAlpha <= U16_OPACITY_OPAQUE);

	Pixel *dstPixel = reinterpret_cast<Pixel *>(dst);

	dstPixel -> alpha = newAlpha;

	if (newAlpha > 0) {
		totalRed = UINT16_DIVIDE(totalRed, newAlpha);
		totalGreen = UINT16_DIVIDE(totalGreen, newAlpha);
		totalBlue = UINT16_DIVIDE(totalBlue, newAlpha);
	}

	dstPixel -> red = totalRed;
	dstPixel -> green = totalGreen;
	dstPixel -> blue = totalBlue;
}

vKisChannelInfoSP KisStrategyColorSpaceRGBU16::channels() const
{
	return m_channels;
}

bool KisStrategyColorSpaceRGBU16::hasAlpha() const
{
	return true;
}

Q_INT32 KisStrategyColorSpaceRGBU16::nChannels() const
{
	return MAX_CHANNEL_RGBA;
}

Q_INT32 KisStrategyColorSpaceRGBU16::nColorChannels() const
{
	return MAX_CHANNEL_RGB;
}

Q_INT32 KisStrategyColorSpaceRGBU16::pixelSize() const
{
	return MAX_CHANNEL_RGBA * sizeof(Q_UINT16);
}

QImage KisStrategyColorSpaceRGBU16::convertToQImage(const Q_UINT8 *dataU8, Q_INT32 width, Q_INT32 height,
						 KisProfileSP srcProfile, KisProfileSP dstProfile,
						 Q_INT32 renderingIntent, float /*exposure*/)

{
	kdDebug(DBG_AREA_CMS) << "convertToQImage: (" << width << ", " << height << ")"
		  << " srcProfile: " << srcProfile << ", " << "dstProfile: " << dstProfile << "\n";

	QImage img = QImage(width, height, 32, 0, QImage::LittleEndian);
	img.setAlphaBuffer(true);

	if (srcProfile != 0 && dstProfile != 0) {
		KisStrategyColorSpaceSP dstCS = KisColorSpaceRegistry::instance() -> get("RGBA");

		convertPixelsTo(dataU8, srcProfile,
				img.bits(), dstCS, dstProfile,
				width * height, renderingIntent);
	} else {
		const Q_UINT16 *data = reinterpret_cast<const Q_UINT16 *>(dataU8);


		Q_INT32 i = 0;
		uchar *j = img.bits();

		while ( i < width * height * MAX_CHANNEL_RGBA) {
	#ifdef __BIG_ENDIAN__
			*( j + 0)  = UINT16_TO_UINT8(*( data + i + PIXEL_ALPHA ));
			*( j + 1 ) = UINT16_TO_UINT8(*( data + i + PIXEL_RED ));
			*( j + 2 ) = UINT16_TO_UINT8(*( data + i + PIXEL_GREEN ));
			*( j + 3 ) = UINT16_TO_UINT8(*( data + i + PIXEL_BLUE ));
	#else
			*( j + 3)  = UINT16_TO_UINT8(*( data + i + PIXEL_ALPHA ));
			*( j + 2 ) = UINT16_TO_UINT8(*( data + i + PIXEL_RED ));
			*( j + 1 ) = UINT16_TO_UINT8(*( data + i + PIXEL_GREEN ));
			*( j + 0 ) = UINT16_TO_UINT8(*( data + i + PIXEL_BLUE ));
	#endif
			i += MAX_CHANNEL_RGBA;
			j += MAX_CHANNEL_RGBA;
		}
	}

	return img;
}

void KisStrategyColorSpaceRGBU16::adjustBrightnessContrast(const Q_UINT8 *src, Q_UINT8 *dst, Q_INT8 brightness, Q_INT8 contrast, Q_INT32 nPixels) const
{
	/*
	static cmsHPROFILE profiles[3];
	static cmsHTRANSFORM transform=0;
	static Q_INT8 oldb=0;
	static Q_INT8 oldc=0;
	
	if((oldb != brightness || oldc != contrast) && transform!=0)
	{
		cmsDeleteTransform(transform);
		cmsCloseProfile(profiles[0]);
		cmsCloseProfile(profiles[1]);
		cmsCloseProfile(profiles[2]);
		transform=0;
	}

	if(transform==0)
	{
		double a,b;
		a=contrast/100.0+1.0;
		a *= a;
		b= 50 -50*a + brightness;
		profiles[0] = cmsCreate_sRGBProfile();
		profiles[1] = cmsCreateBCHSWabstractProfile(30, b, a, 0, 0, 6504, 6504);
		profiles[2] = cmsCreate_sRGBProfile();
		transform  = cmsCreateMultiprofileTransform(profiles, 3, TYPE_BGRA_8, TYPE_BGRA_8, INTENT_PERCEPTUAL, 0);
		oldb=brightness;
		oldc=contrast;
	}
	cmsDoTransform(transform, const_cast<Q_UINT8 *>(src), dst, nPixels);
	*/
}

void KisStrategyColorSpaceRGBU16::compositeOver(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	while (rows > 0) {

		const Q_UINT16 *src = reinterpret_cast<const Q_UINT16 *>(srcRowStart);
		Q_UINT16 *dst = reinterpret_cast<Q_UINT16 *>(dstRowStart);
		const Q_UINT8 *mask = maskRowStart;
		Q_INT32 columns = numColumns;

		while (columns > 0) {

			Q_UINT16 srcAlpha = src[PIXEL_ALPHA];

			// apply the alphamask
			if (mask != 0) {
				Q_UINT8 U8_mask = *mask;

				if (U8_mask != OPACITY_OPAQUE) {
					srcAlpha = UINT16_MULT(srcAlpha, UINT8_TO_UINT16(U8_mask));
				}
				mask++;
			}
			
			if (srcAlpha != U16_OPACITY_TRANSPARENT) {

				if (opacity != U16_OPACITY_OPAQUE) {
					srcAlpha = UINT16_MULT(srcAlpha, opacity);
				}

				if (srcAlpha == U16_OPACITY_OPAQUE) {
					memcpy(dst, src, MAX_CHANNEL_RGBA * sizeof(Q_UINT16));
				} else {
					Q_UINT16 dstAlpha = dst[PIXEL_ALPHA];

					Q_UINT16 srcBlend;

					if (dstAlpha == U16_OPACITY_OPAQUE) {
						srcBlend = srcAlpha;
					} else {
						Q_UINT16 newAlpha = dstAlpha + UINT16_MULT(U16_OPACITY_OPAQUE - dstAlpha, srcAlpha);
						dst[PIXEL_ALPHA] = newAlpha;

						if (newAlpha != 0) {
							srcBlend = UINT16_DIVIDE(srcAlpha, newAlpha);
						} else {
							srcBlend = srcAlpha;
						}
					}

					if (srcBlend == U16_OPACITY_OPAQUE) {
						memcpy(dst, src, MAX_CHANNEL_RGB * sizeof(Q_UINT16));
					} else {
						dst[PIXEL_RED] = UINT16_BLEND(src[PIXEL_RED], dst[PIXEL_RED], srcBlend);
						dst[PIXEL_GREEN] = UINT16_BLEND(src[PIXEL_GREEN], dst[PIXEL_GREEN], srcBlend);
						dst[PIXEL_BLUE] = UINT16_BLEND(src[PIXEL_BLUE], dst[PIXEL_BLUE], srcBlend);
					}
				}
			}

			columns--;
			src += MAX_CHANNEL_RGBA;
			dst += MAX_CHANNEL_RGBA;
		}

		rows--;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
		if(maskRowStart) {
			maskRowStart += maskRowStride;
		}
	}
}

#define COMMON_COMPOSITE_OP_PROLOG() \
	while (rows > 0) { \
	\
		const Q_UINT16 *src = reinterpret_cast<const Q_UINT16 *>(srcRowStart); \
		Q_UINT16 *dst = reinterpret_cast<Q_UINT16 *>(dstRowStart); \
		Q_INT32 columns = numColumns; \
		const Q_UINT8 *mask = maskRowStart; \
	\
		while (columns > 0) { \
	\
			Q_UINT16 srcAlpha = src[PIXEL_ALPHA]; \
			Q_UINT16 dstAlpha = dst[PIXEL_ALPHA]; \
	\
			srcAlpha = QMIN(srcAlpha, dstAlpha); \
	\
			if (mask != 0) { \
				Q_UINT8 U8_mask = *mask; \
	\
				if (U8_mask != OPACITY_OPAQUE) { \
					srcAlpha = UINT16_MULT(srcAlpha, UINT8_TO_UINT16(U8_mask)); \
				} \
				mask++; \
			} \
	\
			if (srcAlpha != U16_OPACITY_TRANSPARENT) { \
	\
				if (opacity != U16_OPACITY_OPAQUE) { \
					srcAlpha = UINT16_MULT(srcAlpha, opacity); \
				} \
	\
				Q_UINT16 srcBlend; \
	\
				if (dstAlpha == U16_OPACITY_OPAQUE) { \
					srcBlend = srcAlpha; \
				} else { \
					Q_UINT16 newAlpha = dstAlpha + UINT16_MULT(U16_OPACITY_OPAQUE - dstAlpha, srcAlpha); \
					dst[PIXEL_ALPHA] = newAlpha; \
	\
					if (newAlpha != 0) { \
						srcBlend = UINT16_DIVIDE(srcAlpha, newAlpha); \
					} else { \
						srcBlend = srcAlpha; \
					} \
				}

#define COMMON_COMPOSITE_OP_EPILOG() \
			} \
	\
			columns--; \
			src += MAX_CHANNEL_RGBA; \
			dst += MAX_CHANNEL_RGBA; \
		} \
	\
		rows--; \
		srcRowStart += srcRowStride; \
		dstRowStart += dstRowStride; \
		if(maskRowStart) { \
			maskRowStart += maskRowStride; \
		} \
	}

void KisStrategyColorSpaceRGBU16::compositeMultiply(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		Q_UINT16 srcColor = src[PIXEL_RED];
		Q_UINT16 dstColor = dst[PIXEL_RED];

		srcColor = UINT16_MULT(srcColor, dstColor);

		dst[PIXEL_RED] = UINT16_BLEND(srcColor, dstColor, srcBlend);

		srcColor = src[PIXEL_GREEN];
		dstColor = dst[PIXEL_GREEN];

		srcColor = UINT16_MULT(srcColor, dstColor);

		dst[PIXEL_GREEN] = UINT16_BLEND(srcColor, dstColor, srcBlend);

		srcColor = src[PIXEL_BLUE];
		dstColor = dst[PIXEL_BLUE];

		srcColor = UINT16_MULT(srcColor, dstColor);

		dst[PIXEL_BLUE] = UINT16_BLEND(srcColor, dstColor, srcBlend);
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeDivide(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			Q_UINT16 srcColor = src[channel];
			Q_UINT16 dstColor = dst[channel];

			srcColor = QMIN((dstColor * (UINT16_MAX + 1u) + (srcColor / 2u)) / (1u + srcColor), UINT16_MAX);

			Q_UINT16 newColor = UINT16_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeScreen(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			Q_UINT16 srcColor = src[channel];
			Q_UINT16 dstColor = dst[channel];

			srcColor = UINT16_MAX - UINT16_MULT(UINT16_MAX - dstColor, UINT16_MAX - srcColor);

			Q_UINT16 newColor = UINT16_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeOverlay(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			Q_UINT16 srcColor = src[channel];
			Q_UINT16 dstColor = dst[channel];

			srcColor = UINT16_MULT(dstColor, dstColor + 2u * UINT16_MULT(srcColor, UINT16_MAX - dstColor));

			Q_UINT16 newColor = UINT16_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeDodge(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			Q_UINT16 srcColor = src[channel];
			Q_UINT16 dstColor = dst[channel];

			srcColor = QMIN((dstColor * (UINT16_MAX + 1u)) / (UINT16_MAX + 1u - srcColor), UINT16_MAX);

			Q_UINT16 newColor = UINT16_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeBurn(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			Q_UINT16 srcColor = src[channel];
			Q_UINT16 dstColor = dst[channel];

			srcColor = QMIN(((UINT16_MAX - dstColor) * (UINT16_MAX + 1u)) / (srcColor + 1u), UINT16_MAX);
			srcColor = CLAMP(UINT16_MAX - srcColor, 0u, UINT16_MAX);

			Q_UINT16 newColor = UINT16_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeDarken(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			Q_UINT16 srcColor = src[channel];
			Q_UINT16 dstColor = dst[channel];

			srcColor = QMIN(srcColor, dstColor);

			Q_UINT16 newColor = UINT16_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeLighten(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			Q_UINT16 srcColor = src[channel];
			Q_UINT16 dstColor = dst[channel];

			srcColor = QMAX(srcColor, dstColor);

			Q_UINT16 newColor = UINT16_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeHue(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		float FSrcRed = static_cast<float>(src[PIXEL_RED]) / UINT16_MAX;
		float FSrcGreen = static_cast<float>(src[PIXEL_GREEN]) / UINT16_MAX;
		float FSrcBlue = static_cast<float>(src[PIXEL_BLUE]) / UINT16_MAX;

		Q_UINT16 dstRed = dst[PIXEL_RED];
		Q_UINT16 dstGreen = dst[PIXEL_GREEN];
		Q_UINT16 dstBlue = dst[PIXEL_BLUE];

		float FDstRed = static_cast<float>(dstRed) / UINT16_MAX;
		float FDstGreen = static_cast<float>(dstGreen) / UINT16_MAX;
		float FDstBlue = static_cast<float>(dstBlue) / UINT16_MAX;

		float srcHue;
		float srcSaturation;
		float srcValue;

		float dstHue;
		float dstSaturation;
		float dstValue;

		RGBToHSV(FSrcRed, FSrcGreen, FSrcBlue, &srcHue, &srcSaturation, &srcValue);
		RGBToHSV(FDstRed, FDstGreen, FDstBlue, &dstHue, &dstSaturation, &dstValue);

		HSVToRGB(srcHue, dstSaturation, dstValue, &FSrcRed, &FSrcGreen, &FSrcBlue);

		Q_UINT16 srcRed = static_cast<Q_UINT16>(FSrcRed * UINT16_MAX + 0.5);
		Q_UINT16 srcGreen = static_cast<Q_UINT16>(FSrcGreen * UINT16_MAX + 0.5);
		Q_UINT16 srcBlue = static_cast<Q_UINT16>(FSrcBlue * UINT16_MAX + 0.5);

		dst[PIXEL_RED] = UINT16_BLEND(srcRed, dstRed, srcBlend);
		dst[PIXEL_GREEN] = UINT16_BLEND(srcGreen, dstGreen, srcBlend);
		dst[PIXEL_BLUE] = UINT16_BLEND(srcBlue, dstBlue, srcBlend);
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeSaturation(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		float FSrcRed = static_cast<float>(src[PIXEL_RED]) / UINT16_MAX;
		float FSrcGreen = static_cast<float>(src[PIXEL_GREEN]) / UINT16_MAX;
		float FSrcBlue = static_cast<float>(src[PIXEL_BLUE]) / UINT16_MAX;

		Q_UINT16 dstRed = dst[PIXEL_RED];
		Q_UINT16 dstGreen = dst[PIXEL_GREEN];
		Q_UINT16 dstBlue = dst[PIXEL_BLUE];

		float FDstRed = static_cast<float>(dstRed) / UINT16_MAX;
		float FDstGreen = static_cast<float>(dstGreen) / UINT16_MAX;
		float FDstBlue = static_cast<float>(dstBlue) / UINT16_MAX;

		float srcHue;
		float srcSaturation;
		float srcValue;

		float dstHue;
		float dstSaturation;
		float dstValue;

		RGBToHSV(FSrcRed, FSrcGreen, FSrcBlue, &srcHue, &srcSaturation, &srcValue);
		RGBToHSV(FDstRed, FDstGreen, FDstBlue, &dstHue, &dstSaturation, &dstValue);

		HSVToRGB(dstHue, srcSaturation, dstValue, &FSrcRed, &FSrcGreen, &FSrcBlue);

		Q_UINT16 srcRed = static_cast<Q_UINT16>(FSrcRed * UINT16_MAX + 0.5);
		Q_UINT16 srcGreen = static_cast<Q_UINT16>(FSrcGreen * UINT16_MAX + 0.5);
		Q_UINT16 srcBlue = static_cast<Q_UINT16>(FSrcBlue * UINT16_MAX + 0.5);

		dst[PIXEL_RED] = UINT16_BLEND(srcRed, dstRed, srcBlend);
		dst[PIXEL_GREEN] = UINT16_BLEND(srcGreen, dstGreen, srcBlend);
		dst[PIXEL_BLUE] = UINT16_BLEND(srcBlue, dstBlue, srcBlend);
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeValue(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		float FSrcRed = static_cast<float>(src[PIXEL_RED]) / UINT16_MAX;
		float FSrcGreen = static_cast<float>(src[PIXEL_GREEN]) / UINT16_MAX;
		float FSrcBlue = static_cast<float>(src[PIXEL_BLUE]) / UINT16_MAX;

		Q_UINT16 dstRed = dst[PIXEL_RED];
		Q_UINT16 dstGreen = dst[PIXEL_GREEN];
		Q_UINT16 dstBlue = dst[PIXEL_BLUE];

		float FDstRed = static_cast<float>(dstRed) / UINT16_MAX;
		float FDstGreen = static_cast<float>(dstGreen) / UINT16_MAX;
		float FDstBlue = static_cast<float>(dstBlue) / UINT16_MAX;

		float srcHue;
		float srcSaturation;
		float srcValue;

		float dstHue;
		float dstSaturation;
		float dstValue;

		RGBToHSV(FSrcRed, FSrcGreen, FSrcBlue, &srcHue, &srcSaturation, &srcValue);
		RGBToHSV(FDstRed, FDstGreen, FDstBlue, &dstHue, &dstSaturation, &dstValue);

		HSVToRGB(dstHue, dstSaturation, srcValue, &FSrcRed, &FSrcGreen, &FSrcBlue);

		Q_UINT16 srcRed = static_cast<Q_UINT16>(FSrcRed * UINT16_MAX + 0.5);
		Q_UINT16 srcGreen = static_cast<Q_UINT16>(FSrcGreen * UINT16_MAX + 0.5);
		Q_UINT16 srcBlue = static_cast<Q_UINT16>(FSrcBlue * UINT16_MAX + 0.5);

		dst[PIXEL_RED] = UINT16_BLEND(srcRed, dstRed, srcBlend);
		dst[PIXEL_GREEN] = UINT16_BLEND(srcGreen, dstGreen, srcBlend);
		dst[PIXEL_BLUE] = UINT16_BLEND(srcBlue, dstBlue, srcBlend);
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeColor(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		float FSrcRed = static_cast<float>(src[PIXEL_RED]) / UINT16_MAX;
		float FSrcGreen = static_cast<float>(src[PIXEL_GREEN]) / UINT16_MAX;
		float FSrcBlue = static_cast<float>(src[PIXEL_BLUE]) / UINT16_MAX;

		Q_UINT16 dstRed = dst[PIXEL_RED];
		Q_UINT16 dstGreen = dst[PIXEL_GREEN];
		Q_UINT16 dstBlue = dst[PIXEL_BLUE];

		float FDstRed = static_cast<float>(dstRed) / UINT16_MAX;
		float FDstGreen = static_cast<float>(dstGreen) / UINT16_MAX;
		float FDstBlue = static_cast<float>(dstBlue) / UINT16_MAX;

		float srcHue;
		float srcSaturation;
		float srcLightness;

		float dstHue;
		float dstSaturation;
		float dstLightness;

		RGBToHSL(FSrcRed, FSrcGreen, FSrcBlue, &srcHue, &srcSaturation, &srcLightness);
		RGBToHSL(FDstRed, FDstGreen, FDstBlue, &dstHue, &dstSaturation, &dstLightness);

		HSLToRGB(srcHue, srcSaturation, dstLightness, &FSrcRed, &FSrcGreen, &FSrcBlue);

		Q_UINT16 srcRed = static_cast<Q_UINT16>(FSrcRed * UINT16_MAX + 0.5);
		Q_UINT16 srcGreen = static_cast<Q_UINT16>(FSrcGreen * UINT16_MAX + 0.5);
		Q_UINT16 srcBlue = static_cast<Q_UINT16>(FSrcBlue * UINT16_MAX + 0.5);

		dst[PIXEL_RED] = UINT16_BLEND(srcRed, dstRed, srcBlend);
		dst[PIXEL_GREEN] = UINT16_BLEND(srcGreen, dstGreen, srcBlend);
		dst[PIXEL_BLUE] = UINT16_BLEND(srcBlue, dstBlue, srcBlend);
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisStrategyColorSpaceRGBU16::compositeErase(Q_UINT8 *dst, 
		    Q_INT32 dstRowSize,
		    const Q_UINT8 *src, 
		    Q_INT32 srcRowSize,
		    const Q_UINT8 *srcAlphaMask,
		    Q_INT32 maskRowStride,
		    Q_INT32 rows, 
		    Q_INT32 cols, 
		    Q_UINT16 /*opacity*/)
{
	while (rows-- > 0)
	{
		const Pixel *s = reinterpret_cast<const Pixel *>(src);
		Pixel *d = reinterpret_cast<Pixel *>(dst);
		const Q_UINT8 *mask = srcAlphaMask;

		for (Q_INT32 i = cols; i > 0; i--, s++, d++)
		{
			Q_UINT16 srcAlpha = s -> alpha;

			// apply the alphamask
			if (mask != 0) {
				Q_UINT8 U8_mask = *mask;

				if (U8_mask != OPACITY_OPAQUE) {
					srcAlpha = UINT16_BLEND(srcAlpha, U16_OPACITY_OPAQUE, UINT8_TO_UINT16(U8_mask));
				}
				mask++;
			}
			d -> alpha = UINT16_MULT(srcAlpha, d -> alpha);
		}

		dst += dstRowSize;
		src += srcRowSize;
		if(srcAlphaMask) {
			srcAlphaMask += maskRowStride;
		}
	}
}

void KisStrategyColorSpaceRGBU16::compositeCopy(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, 
						const Q_UINT8 */*maskRowStart*/, Q_INT32 /*maskRowStride*/, Q_INT32 rows, Q_INT32 numColumns, Q_UINT16 /*opacity*/)
{
	while (rows > 0) {
		memcpy(dstRowStart, srcRowStart, numColumns * sizeof(Pixel));
		--rows;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
	}
}

void KisStrategyColorSpaceRGBU16::bitBlt(Q_UINT8 *dst,
				      Q_INT32 dstRowStride,
				      const Q_UINT8 *src,
				      Q_INT32 srcRowStride,
				      const Q_UINT8 *mask,
				      Q_INT32 maskRowStride,
				      QUANTUM U8_opacity,
				      Q_INT32 rows,
				      Q_INT32 cols,
				      const KisCompositeOp& op)
{
	Q_UINT16 opacity = UINT8_TO_UINT16(U8_opacity);

	switch (op.op()) {
	case COMPOSITE_UNDEF:
		// Undefined == no composition
		break;
	case COMPOSITE_OVER:
		compositeOver(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_IN:
		//compositeIn(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
	case COMPOSITE_OUT:
		//compositeOut(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_ATOP:
		//compositeAtop(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_XOR:
		//compositeXor(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_PLUS:
		//compositePlus(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_MINUS:
		//compositeMinus(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_ADD:
		//compositeAdd(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_SUBTRACT:
		//compositeSubtract(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_DIFF:
		//compositeDiff(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_MULT:
		compositeMultiply(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_DIVIDE:
		compositeDivide(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_BUMPMAP:
		//compositeBumpmap(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_COPY:
		compositeCopy(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_COPY_RED:
		//compositeCopyRed(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_COPY_GREEN:
		//compositeCopyGreen(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_COPY_BLUE:
		//compositeCopyBlue(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_COPY_OPACITY:
		//compositeCopyOpacity(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_CLEAR:
		//compositeClear(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_DISSOLVE:
		//compositeDissolve(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_DISPLACE:
		//compositeDisplace(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
#if 0
	case COMPOSITE_MODULATE:
		compositeModulate(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_THRESHOLD:
		compositeThreshold(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
#endif
	case COMPOSITE_NO:
		// No composition.
		break;
	case COMPOSITE_DARKEN:
		compositeDarken(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_LIGHTEN:
		compositeLighten(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_HUE:
		compositeHue(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_SATURATION:
		compositeSaturation(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_VALUE:
		compositeValue(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_COLOR:
		compositeColor(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_COLORIZE:
		//compositeColorize(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_LUMINIZE:
		//compositeLuminize(pixelSize(), dst, dstRowStride, src, srcRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_SCREEN:
		compositeScreen(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_OVERLAY:
		compositeOverlay(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_ERASE:
		compositeErase(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_DODGE:
		compositeDodge(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_BURN:
		compositeBurn(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	default:
		break;
	}
}

KisCompositeOpList KisStrategyColorSpaceRGBU16::userVisiblecompositeOps() const
{
	KisCompositeOpList list;

	list.append(KisCompositeOp(COMPOSITE_OVER));
	list.append(KisCompositeOp(COMPOSITE_MULT));
	list.append(KisCompositeOp(COMPOSITE_BURN));
	list.append(KisCompositeOp(COMPOSITE_DODGE));
	list.append(KisCompositeOp(COMPOSITE_DIVIDE));
	list.append(KisCompositeOp(COMPOSITE_SCREEN));
	list.append(KisCompositeOp(COMPOSITE_OVERLAY));
	list.append(KisCompositeOp(COMPOSITE_DARKEN));
	list.append(KisCompositeOp(COMPOSITE_LIGHTEN));
	list.append(KisCompositeOp(COMPOSITE_HUE));
	list.append(KisCompositeOp(COMPOSITE_SATURATION));
	list.append(KisCompositeOp(COMPOSITE_VALUE));
	list.append(KisCompositeOp(COMPOSITE_COLOR));

	return list;
}

QString KisStrategyColorSpaceRGBU16::channelValueText(const Q_UINT8 *U8_pixel, Q_UINT32 channelIndex) const
{
	Q_ASSERT(channelIndex < nChannels());
	const Q_UINT16 *pixel = reinterpret_cast<const Q_UINT16 *>(U8_pixel);
	Q_UINT32 channelPosition = m_channels[channelIndex] -> pos() / sizeof(Q_UINT16);

	return QString().setNum(pixel[channelPosition]);
}

QString KisStrategyColorSpaceRGBU16::normalisedChannelValueText(const Q_UINT8 *U8_pixel, Q_UINT32 channelIndex) const
{
	Q_ASSERT(channelIndex < nChannels());
	const Q_UINT16 *pixel = reinterpret_cast<const Q_UINT16 *>(U8_pixel);
	Q_UINT32 channelPosition = m_channels[channelIndex] -> pos() / sizeof(Q_UINT16);

	return QString().setNum(static_cast<float>(pixel[channelPosition]) / UINT16_MAX);
}

