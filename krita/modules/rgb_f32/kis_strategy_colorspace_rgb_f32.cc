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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <config.h>
#include <limits.h>
#include <stdlib.h>
#include LCMS_HEADER

#include <qimage.h>

#include <kdebug.h>
#include <klocale.h>

#include "kis_image.h"
#include "kis_strategy_colorspace_rgb_f32.h"
#include "kis_iterators_pixel.h"
#include "kis_color_conversions.h"

namespace {
	const Q_INT32 MAX_CHANNEL_RGB = 3;
	const Q_INT32 MAX_CHANNEL_RGBA = 4;
}

#include "kis_integer_maths.h"

inline float UINT8_TO_FLOAT(uint c)
{
	return static_cast<float>(c) / UINT8_MAX;
}

inline uint FLOAT_TO_UINT8(float c)
{
	return QMIN(static_cast<uint>(c * UINT8_MAX + 0.5), UINT8_MAX);
}

inline float FLOAT_BLEND(float a, float b, float alpha)
{
	return (a - b) * alpha + b;
}

#define FLOAT_MAX 1.0f //temp

#define EPSILON 1e-6

// FIXME: lcms doesn't support 32-bit float
#define F32_LCMS_TYPE TYPE_BGRA_16

KisF32RgbColorSpace::KisF32RgbColorSpace() :
	KisAbstractColorSpace(KisID("RGBAF32", i18n("RGB/Alpha (32-bit float/channel)")), F32_LCMS_TYPE, icSigRgbData)
{
	m_channels.push_back(new KisChannelInfo(i18n("Red"), PIXEL_RED * sizeof(float), COLOR, sizeof(float)));
	m_channels.push_back(new KisChannelInfo(i18n("Green"), PIXEL_GREEN * sizeof(float), COLOR, sizeof(float)));
	m_channels.push_back(new KisChannelInfo(i18n("Blue"), PIXEL_BLUE * sizeof(float), COLOR, sizeof(float)));
	m_channels.push_back(new KisChannelInfo(i18n("Alpha"), PIXEL_ALPHA * sizeof(float), ALPHA, sizeof(float)));

	
	cmsHPROFILE hProfile = cmsCreate_sRGBProfile();
	setDefaultProfile( new KisProfile(hProfile, F32_LCMS_TYPE) );
}

KisF32RgbColorSpace::~KisF32RgbColorSpace()
{
}

void KisF32RgbColorSpace::setPixel(Q_UINT8 *dst, float red, float green, float blue, float alpha) const
{
	Pixel *dstPixel = reinterpret_cast<Pixel *>(dst);

	dstPixel -> red = red;
	dstPixel -> green = green;
	dstPixel -> blue = blue;
	dstPixel -> alpha = alpha;
}

void KisF32RgbColorSpace::getPixel(const Q_UINT8 *src, float *red, float *green, float *blue, float *alpha) const
{
	const Pixel *srcPixel = reinterpret_cast<const Pixel *>(src);

	*red = srcPixel -> red;
	*green = srcPixel -> green;
	*blue = srcPixel -> blue;
	*alpha = srcPixel -> alpha;
}

void KisF32RgbColorSpace::nativeColor(const QColor& c, Q_UINT8 *dstU8, KisProfileSP /*profile*/)
{
	Pixel *dst = reinterpret_cast<Pixel *>(dstU8);

	dst -> red = UINT8_TO_FLOAT(c.red());
	dst -> green = UINT8_TO_FLOAT(c.green());
	dst -> blue = UINT8_TO_FLOAT(c.blue());
}

void KisF32RgbColorSpace::nativeColor(const QColor& c, QUANTUM opacity, Q_UINT8 *dstU8, KisProfileSP /*profile*/)
{
	Pixel *dst = reinterpret_cast<Pixel *>(dstU8);

	dst -> red = UINT8_TO_FLOAT(c.red());
	dst -> green = UINT8_TO_FLOAT(c.green());
	dst -> blue = UINT8_TO_FLOAT(c.blue());
	dst -> alpha = UINT8_TO_FLOAT(opacity);
}

void KisF32RgbColorSpace::getAlpha(const Q_UINT8 *U8_pixel, Q_UINT8 *alpha)
{
	const Pixel *pixel = reinterpret_cast<const Pixel *>(U8_pixel);
	*alpha = FLOAT_TO_UINT8(pixel -> alpha);
}

void KisF32RgbColorSpace::setAlpha(Q_UINT8 *pixels, Q_UINT8 alpha, Q_INT32 nPixels)
{
	Pixel *pixel = reinterpret_cast<Pixel *>(pixels);

	while (nPixels > 0) {
		pixel -> alpha = UINT8_TO_FLOAT(alpha);
		--nPixels;
		++pixel;
	}
}

void KisF32RgbColorSpace::toQColor(const Q_UINT8 *srcU8, QColor *c, KisProfileSP /*profile*/)
{
	const Pixel *src = reinterpret_cast<const Pixel *>(srcU8);

	c -> setRgb(FLOAT_TO_UINT8(src -> red), FLOAT_TO_UINT8(src -> green), FLOAT_TO_UINT8(src -> blue));
}

void KisF32RgbColorSpace::toQColor(const Q_UINT8 *srcU8, QColor *c, QUANTUM *opacity, KisProfileSP /*profile*/)
{
	const Pixel *src = reinterpret_cast<const Pixel *>(srcU8);

	c -> setRgb(FLOAT_TO_UINT8(src -> red), FLOAT_TO_UINT8(src -> green), FLOAT_TO_UINT8(src -> blue));
	*opacity = FLOAT_TO_UINT8(src -> alpha);
}

Q_INT8 KisF32RgbColorSpace::difference(const Q_UINT8 *src1U8, const Q_UINT8 *src2U8)
{
	const Pixel *src1 = reinterpret_cast<const Pixel *>(src1U8);
	const Pixel *src2 = reinterpret_cast<const Pixel *>(src2U8);

	return FLOAT_TO_UINT8(QMAX(QABS(src2 -> red - src1 -> red),
				QMAX(QABS(src2 -> green - src1 -> green),
				     QABS(src2 -> blue - src1 -> blue))));
}

void KisF32RgbColorSpace::mixColors(const Q_UINT8 **colors, const Q_UINT8 *weights, Q_UINT32 nColors, Q_UINT8 *dst) const
{
	float totalRed = 0, totalGreen = 0, totalBlue = 0, newAlpha = 0;
	
	while (nColors--)
	{
		const Pixel *pixel = reinterpret_cast<const Pixel *>(*colors);

		float alpha = pixel -> alpha;
		float alphaTimesWeight = alpha * UINT8_TO_FLOAT(*weights);

		totalRed += pixel -> red * alphaTimesWeight;
		totalGreen += pixel -> green * alphaTimesWeight;
		totalBlue += pixel -> blue * alphaTimesWeight;
		newAlpha += alphaTimesWeight;

		weights++;
		colors++;
	}

	Q_ASSERT(newAlpha <= F32_OPACITY_OPAQUE);

	Pixel *dstPixel = reinterpret_cast<Pixel *>(dst);

	dstPixel -> alpha = newAlpha;

	if (newAlpha > EPSILON) {
		totalRed = totalRed / newAlpha;
		totalGreen = totalGreen / newAlpha;
		totalBlue = totalBlue / newAlpha;
	}

	dstPixel -> red = totalRed;
	dstPixel -> green = totalGreen;
	dstPixel -> blue = totalBlue;
}

vKisChannelInfoSP KisF32RgbColorSpace::channels() const
{
	return m_channels;
}

bool KisF32RgbColorSpace::hasAlpha() const
{
	return true;
}

Q_INT32 KisF32RgbColorSpace::nChannels() const
{
	return MAX_CHANNEL_RGBA;
}

Q_INT32 KisF32RgbColorSpace::nColorChannels() const
{
	return MAX_CHANNEL_RGB;
}

Q_INT32 KisF32RgbColorSpace::pixelSize() const
{
	return MAX_CHANNEL_RGBA * sizeof(float);
}

Q_UINT8 convertToDisplay(float value, float exposureFactor, float gamma)
{
	//value *= pow(2, exposure + 2.47393);
	value *= exposureFactor;

	value = powf(value, gamma);

	// scale middle gray to the target framebuffer value

	value *= 84.66f;

	int valueInt = (int)(value + 0.5);

	return CLAMP(valueInt, 0, 255);
}

QImage KisF32RgbColorSpace::convertToQImage(const Q_UINT8 *dataU8, Q_INT32 width, Q_INT32 height,
						 KisProfileSP srcProfile, KisProfileSP dstProfile,
						 Q_INT32 renderingIntent, float exposure)

{
	const float *data = reinterpret_cast<const float *>(dataU8);

	QImage img = QImage(width, height, 32, 0, QImage::LittleEndian);
	img.setAlphaBuffer(true);

	Q_INT32 i = 0;
	uchar *j = img.bits();

	// XXX: For now assume gamma 2.2.
	float gamma = 1 / 2.2;
	float exposureFactor = powf(2, exposure + 2.47393);

	while ( i < width * height * MAX_CHANNEL_RGBA) {
#ifdef __BIG_ENDIAN__
		*( j + 0)  = FLOAT_TO_UINT8(*( data + i + PIXEL_ALPHA ));
		*( j + 1 ) = convertToDisplay(*( data + i + PIXEL_RED ), exposureFactor, gamma);
		*( j + 2 ) = convertToDisplay(*( data + i + PIXEL_GREEN ), exposureFactor, gamma);
		*( j + 3 ) = convertToDisplay(*( data + i + PIXEL_BLUE ), exposureFactor, gamma);
#else
		*( j + 3)  = FLOAT_TO_UINT8(*( data + i + PIXEL_ALPHA ));
		*( j + 2 ) = convertToDisplay(*( data + i + PIXEL_RED ), exposureFactor, gamma);
		*( j + 1 ) = convertToDisplay(*( data + i + PIXEL_GREEN ), exposureFactor, gamma);
		*( j + 0 ) = convertToDisplay(*( data + i + PIXEL_BLUE ), exposureFactor, gamma);
#endif
		i += MAX_CHANNEL_RGBA;
		j += MAX_CHANNEL_RGBA;
	}

	/*
	if (srcProfile != 0 && dstProfile != 0) {
		convertPixelsTo(img.bits(), srcProfile,
				img.bits(), this, dstProfile,
				width * height, renderingIntent);
	}
	*/
	return img;
}

void KisF32RgbColorSpace::adjustBrightnessContrast(const Q_UINT8 *src, Q_UINT8 *dst, Q_INT8 brightness, Q_INT8 contrast, Q_INT32 nPixels) const
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

void KisF32RgbColorSpace::compositeOver(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	while (rows > 0) {

		const float *src = reinterpret_cast<const float *>(srcRowStart);
		float *dst = reinterpret_cast<float *>(dstRowStart);
		const Q_UINT8 *mask = maskRowStart;
		Q_INT32 columns = numColumns;

		while (columns > 0) {

			float srcAlpha = src[PIXEL_ALPHA];

			// apply the alphamask
			if (mask != 0) {
				Q_UINT8 U8_mask = *mask;

				if (U8_mask != OPACITY_OPAQUE) {
					srcAlpha *= UINT8_TO_FLOAT(U8_mask);
				}
				mask++;
			}
			
			if (srcAlpha > F32_OPACITY_TRANSPARENT + EPSILON) {

				if (opacity < F32_OPACITY_OPAQUE - EPSILON) {
					srcAlpha *= opacity;
				}

				if (srcAlpha > F32_OPACITY_OPAQUE - EPSILON) {
					memcpy(dst, src, MAX_CHANNEL_RGBA * sizeof(float));
				} else {
					float dstAlpha = dst[PIXEL_ALPHA];

					float srcBlend;

					if (dstAlpha > F32_OPACITY_OPAQUE - EPSILON) {
						srcBlend = srcAlpha;
					} else {
						float newAlpha = dstAlpha + (F32_OPACITY_OPAQUE - dstAlpha) * srcAlpha;
						dst[PIXEL_ALPHA] = newAlpha;

						if (newAlpha > EPSILON) {
							srcBlend = srcAlpha / newAlpha;
						} else {
							srcBlend = srcAlpha;
						}
					}

					if (srcBlend > F32_OPACITY_OPAQUE - EPSILON) {
						memcpy(dst, src, MAX_CHANNEL_RGB * sizeof(float));
					} else {
						dst[PIXEL_RED] = FLOAT_BLEND(src[PIXEL_RED], dst[PIXEL_RED], srcBlend);
						dst[PIXEL_GREEN] = FLOAT_BLEND(src[PIXEL_GREEN], dst[PIXEL_GREEN], srcBlend);
						dst[PIXEL_BLUE] = FLOAT_BLEND(src[PIXEL_BLUE], dst[PIXEL_BLUE], srcBlend);
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
		const float *src = reinterpret_cast<const float *>(srcRowStart); \
		float *dst = reinterpret_cast<float *>(dstRowStart); \
		Q_INT32 columns = numColumns; \
		const Q_UINT8 *mask = maskRowStart; \
	\
		while (columns > 0) { \
	\
			float srcAlpha = src[PIXEL_ALPHA]; \
			float dstAlpha = dst[PIXEL_ALPHA]; \
	\
			srcAlpha = QMIN(srcAlpha, dstAlpha); \
	\
			if (mask != 0) { \
				Q_UINT8 U8_mask = *mask; \
	\
				if (U8_mask != OPACITY_OPAQUE) { \
					srcAlpha *= UINT8_TO_FLOAT(U8_mask); \
				} \
				mask++; \
			} \
	\
			if (srcAlpha > F32_OPACITY_TRANSPARENT + EPSILON) { \
	\
				if (opacity < F32_OPACITY_OPAQUE - EPSILON) { \
					srcAlpha *= opacity; \
				} \
	\
				float srcBlend; \
	\
				if (dstAlpha > F32_OPACITY_OPAQUE - EPSILON) { \
					srcBlend = srcAlpha; \
				} else { \
					float newAlpha = dstAlpha + (F32_OPACITY_OPAQUE - dstAlpha) * srcAlpha; \
					dst[PIXEL_ALPHA] = newAlpha; \
	\
					if (newAlpha > EPSILON) { \
						srcBlend = srcAlpha / newAlpha; \
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

void KisF32RgbColorSpace::compositeMultiply(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		float srcColor = src[PIXEL_RED];
		float dstColor = dst[PIXEL_RED];

		srcColor = srcColor * dstColor;

		dst[PIXEL_RED] = FLOAT_BLEND(srcColor, dstColor, srcBlend);

		srcColor = src[PIXEL_GREEN];
		dstColor = dst[PIXEL_GREEN];

		srcColor = srcColor * dstColor;

		dst[PIXEL_GREEN] = FLOAT_BLEND(srcColor, dstColor, srcBlend);

		srcColor = src[PIXEL_BLUE];
		dstColor = dst[PIXEL_BLUE];

		srcColor = srcColor * dstColor;

		dst[PIXEL_BLUE] = FLOAT_BLEND(srcColor, dstColor, srcBlend);
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeDivide(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			float srcColor = src[channel];
			float dstColor = dst[channel];

			srcColor = QMIN(dstColor / (srcColor + EPSILON), FLOAT_MAX);

			float newColor = FLOAT_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeScreen(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			float srcColor = src[channel];
			float dstColor = dst[channel];

			srcColor = FLOAT_MAX - ((FLOAT_MAX - dstColor) * (FLOAT_MAX - srcColor));

			float newColor = FLOAT_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeOverlay(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			float srcColor = src[channel];
			float dstColor = dst[channel];

			srcColor = dstColor * (dstColor + 2 * (srcColor * (FLOAT_MAX - dstColor)));

			float newColor = FLOAT_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeDodge(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			float srcColor = src[channel];
			float dstColor = dst[channel];

			srcColor = QMIN(dstColor / (FLOAT_MAX + EPSILON - srcColor), FLOAT_MAX);

			float newColor = FLOAT_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeBurn(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			float srcColor = src[channel];
			float dstColor = dst[channel];

			srcColor = QMIN((FLOAT_MAX - dstColor) / (srcColor + EPSILON), FLOAT_MAX);
			srcColor = CLAMP(FLOAT_MAX - srcColor, 0, FLOAT_MAX);

			float newColor = FLOAT_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeDarken(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			float srcColor = src[channel];
			float dstColor = dst[channel];

			srcColor = QMIN(srcColor, dstColor);

			float newColor = FLOAT_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeLighten(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		for (int channel = 0; channel < MAX_CHANNEL_RGB; channel++) {

			float srcColor = src[channel];
			float dstColor = dst[channel];

			srcColor = QMAX(srcColor, dstColor);

			float newColor = FLOAT_BLEND(srcColor, dstColor, srcBlend);

			dst[channel] = newColor;
		}
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeHue(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		float srcRed = src[PIXEL_RED];
		float srcGreen = src[PIXEL_GREEN];
		float srcBlue = src[PIXEL_BLUE];

		float dstRed = dst[PIXEL_RED];
		float dstGreen = dst[PIXEL_GREEN];
		float dstBlue = dst[PIXEL_BLUE];

		float srcHue;
		float srcSaturation;
		float srcValue;

		float dstHue;
		float dstSaturation;
		float dstValue;

		RGBToHSV(srcRed, srcGreen, srcBlue, &srcHue, &srcSaturation, &srcValue);
		RGBToHSV(dstRed, dstGreen, dstBlue, &dstHue, &dstSaturation, &dstValue);

		HSVToRGB(srcHue, dstSaturation, dstValue, &srcRed, &srcGreen, &srcBlue);

		dst[PIXEL_RED] = FLOAT_BLEND(srcRed, dstRed, srcBlend);
		dst[PIXEL_GREEN] = FLOAT_BLEND(srcGreen, dstGreen, srcBlend);
		dst[PIXEL_BLUE] = FLOAT_BLEND(srcBlue, dstBlue, srcBlend);
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeSaturation(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		float srcRed = src[PIXEL_RED];
		float srcGreen = src[PIXEL_GREEN];
		float srcBlue = src[PIXEL_BLUE];

		float dstRed = dst[PIXEL_RED];
		float dstGreen = dst[PIXEL_GREEN];
		float dstBlue = dst[PIXEL_BLUE];

		float srcHue;
		float srcSaturation;
		float srcValue;

		float dstHue;
		float dstSaturation;
		float dstValue;

		RGBToHSV(srcRed, srcGreen, srcBlue, &srcHue, &srcSaturation, &srcValue);
		RGBToHSV(dstRed, dstGreen, dstBlue, &dstHue, &dstSaturation, &dstValue);

		HSVToRGB(dstHue, srcSaturation, dstValue, &srcRed, &srcGreen, &srcBlue);

		dst[PIXEL_RED] = FLOAT_BLEND(srcRed, dstRed, srcBlend);
		dst[PIXEL_GREEN] = FLOAT_BLEND(srcGreen, dstGreen, srcBlend);
		dst[PIXEL_BLUE] = FLOAT_BLEND(srcBlue, dstBlue, srcBlend);
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeValue(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		float srcRed = src[PIXEL_RED];
		float srcGreen = src[PIXEL_GREEN];
		float srcBlue = src[PIXEL_BLUE];

		float dstRed = dst[PIXEL_RED];
		float dstGreen = dst[PIXEL_GREEN];
		float dstBlue = dst[PIXEL_BLUE];

		float srcHue;
		float srcSaturation;
		float srcValue;

		float dstHue;
		float dstSaturation;
		float dstValue;

		RGBToHSV(srcRed, srcGreen, srcBlue, &srcHue, &srcSaturation, &srcValue);
		RGBToHSV(dstRed, dstGreen, dstBlue, &dstHue, &dstSaturation, &dstValue);

		HSVToRGB(dstHue, dstSaturation, srcValue, &srcRed, &srcGreen, &srcBlue);

		dst[PIXEL_RED] = FLOAT_BLEND(srcRed, dstRed, srcBlend);
		dst[PIXEL_GREEN] = FLOAT_BLEND(srcGreen, dstGreen, srcBlend);
		dst[PIXEL_BLUE] = FLOAT_BLEND(srcBlue, dstBlue, srcBlend);
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeColor(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, float opacity)
{
	COMMON_COMPOSITE_OP_PROLOG();

	{
		float srcRed = src[PIXEL_RED];
		float srcGreen = src[PIXEL_GREEN];
		float srcBlue = src[PIXEL_BLUE];

		float dstRed = dst[PIXEL_RED];
		float dstGreen = dst[PIXEL_GREEN];
		float dstBlue = dst[PIXEL_BLUE];

		float srcHue;
		float srcSaturation;
		float srcLightness;

		float dstHue;
		float dstSaturation;
		float dstLightness;

		RGBToHSL(srcRed, srcGreen, srcBlue, &srcHue, &srcSaturation, &srcLightness);
		RGBToHSL(dstRed, dstGreen, dstBlue, &dstHue, &dstSaturation, &dstLightness);

		HSLToRGB(srcHue, srcSaturation, dstLightness, &srcRed, &srcGreen, &srcBlue);

		dst[PIXEL_RED] = FLOAT_BLEND(srcRed, dstRed, srcBlend);
		dst[PIXEL_GREEN] = FLOAT_BLEND(srcGreen, dstGreen, srcBlend);
		dst[PIXEL_BLUE] = FLOAT_BLEND(srcBlue, dstBlue, srcBlend);
	}

	COMMON_COMPOSITE_OP_EPILOG();
}

void KisF32RgbColorSpace::compositeErase(Q_UINT8 *dst, 
		    Q_INT32 dstRowSize,
		    const Q_UINT8 *src, 
		    Q_INT32 srcRowSize,
		    const Q_UINT8 *srcAlphaMask,
		    Q_INT32 maskRowStride,
		    Q_INT32 rows, 
		    Q_INT32 cols, 
		    float /*opacity*/)
{
	while (rows-- > 0)
	{
		const Pixel *s = reinterpret_cast<const Pixel *>(src);
		Pixel *d = reinterpret_cast<Pixel *>(dst);
		const Q_UINT8 *mask = srcAlphaMask;

		for (Q_INT32 i = cols; i > 0; i--, s++, d++)
		{
			float srcAlpha = s -> alpha;

			// apply the alphamask
			if (mask != 0) {
				Q_UINT8 U8_mask = *mask;

				if (U8_mask != OPACITY_OPAQUE) {
					srcAlpha = FLOAT_BLEND(srcAlpha, F32_OPACITY_OPAQUE, UINT8_TO_FLOAT(U8_mask));
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

void KisF32RgbColorSpace::compositeCopy(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, 
						const Q_UINT8 */*maskRowStart*/, Q_INT32 /*maskRowStride*/, Q_INT32 rows, Q_INT32 numColumns, float /*opacity*/)
{
	while (rows > 0) {
		memcpy(dstRowStart, srcRowStart, numColumns * sizeof(Pixel));
		--rows;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
	}
}

void KisF32RgbColorSpace::bitBlt(Q_UINT8 *dst,
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
	float opacity = UINT8_TO_FLOAT(U8_opacity);

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

KisCompositeOpList KisF32RgbColorSpace::userVisiblecompositeOps() const
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

QString KisF32RgbColorSpace::channelValueText(const Q_UINT8 *U8_pixel, Q_UINT32 channelIndex) const
{
	Q_ASSERT(channelIndex < nChannels());
	const float *pixel = reinterpret_cast<const float *>(U8_pixel);
	Q_UINT32 channelPosition = m_channels[channelIndex] -> pos() / sizeof(float);

	return QString().setNum(pixel[channelPosition]);
}

QString KisF32RgbColorSpace::normalisedChannelValueText(const Q_UINT8 *U8_pixel, Q_UINT32 channelIndex) const
{
	return channelValueText(U8_pixel, channelIndex);
}

