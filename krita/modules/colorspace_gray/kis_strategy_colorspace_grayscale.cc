/*
 *  Copyright (c) 2002 Patrick Julien  <freak@codepimps.org>
 *  Copyright (c) 2004 Cyrille Berger
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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

#include <limits.h>
#include <stdlib.h>

#include <config.h>
#include LCMS_HEADER

#include <qimage.h>

#include <klocale.h>
#include <kdebug.h>

#include "kis_strategy_colorspace.h"
#include "kis_colorspace_registry.h"
#include "kis_image.h"
#include "kis_strategy_colorspace_grayscale.h"
#include "kis_iterators_pixel.h"
#include "kis_integer_maths.h"

#define downscale(quantum)  (quantum) //((unsigned char) ((quantum)/257UL))
#define upscale(value)  (value) // ((QUANTUM) (257UL*(value)))

namespace {
	const Q_INT32 MAX_CHANNEL_GRAYSCALE = 1;
	const Q_INT32 MAX_CHANNEL_GRAYSCALEA = 2;
}

KisStrategyColorSpaceGrayscale::KisStrategyColorSpaceGrayscale() :
	KisStrategyColorSpace(KisID("GRAYA", i18n("Grayscale/Alpha")), TYPE_GRAYA_8, icSigGrayData)
{
	m_channels.push_back(new KisChannelInfo(i18n("Gray"), 0, COLOR));
	m_channels.push_back(new KisChannelInfo(i18n("Alpha"), 1, ALPHA));
}


KisStrategyColorSpaceGrayscale::~KisStrategyColorSpaceGrayscale()
{
}

void KisStrategyColorSpaceGrayscale::setPixel(Q_UINT8 *pixel, Q_UINT8 gray, Q_UINT8 alpha) const
{
	pixel[PIXEL_GRAY] = gray;
	pixel[PIXEL_GRAY_ALPHA] = alpha;
}

void KisStrategyColorSpaceGrayscale::getPixel(const Q_UINT8 *pixel, Q_UINT8 *gray, Q_UINT8 *alpha) const
{
	*gray = pixel[PIXEL_GRAY];
	*alpha = pixel[PIXEL_GRAY_ALPHA];
}

void KisStrategyColorSpaceGrayscale::nativeColor(const QColor& c, Q_UINT8 *dst, KisProfileSP /*profile*/)
{
	// Use qGray for a better rgb -> gray formula: (r*11 + g*16 + b*5)/32.
	dst[PIXEL_GRAY] = upscale(qGray(c.red(), c.green(), c.blue()));
}

void KisStrategyColorSpaceGrayscale::nativeColor(const QColor& c, QUANTUM opacity, Q_UINT8 *dst, KisProfileSP /*profile*/)
{
	dst[PIXEL_GRAY] = upscale(qGray(c.red(), c.green(), c.blue()));
	dst[PIXEL_GRAY_ALPHA] = opacity;
}

void KisStrategyColorSpaceGrayscale::getAlpha(const Q_UINT8 *pixel, Q_UINT8 *alpha)
{
	*alpha = pixel[PIXEL_GRAY_ALPHA];
}

void KisStrategyColorSpaceGrayscale::setAlpha(Q_UINT8 *pixels, Q_UINT8 alpha, Q_INT32 nPixels)
{
	while (nPixels > 0) {
		pixels[PIXEL_GRAY_ALPHA] = alpha;
		--nPixels;
		pixels += MAX_CHANNEL_GRAYSCALEA;
	}
}

void KisStrategyColorSpaceGrayscale::toQColor(const Q_UINT8 *src, QColor *c, KisProfileSP /*profile*/)
{
	c -> setRgb(downscale(src[PIXEL_GRAY]), downscale(src[PIXEL_GRAY]), downscale(src[PIXEL_GRAY]));
}

void KisStrategyColorSpaceGrayscale::toQColor(const Q_UINT8 *src, QColor *c, QUANTUM *opacity, KisProfileSP /*profile*/)
{
	c -> setRgb(downscale(src[PIXEL_GRAY]), downscale(src[PIXEL_GRAY]), downscale(src[PIXEL_GRAY]));
	*opacity = src[PIXEL_GRAY_ALPHA];
}

Q_INT8 KisStrategyColorSpaceGrayscale::difference(const Q_UINT8 *src1, const Q_UINT8 *src2)
{
	return QABS(src2[PIXEL_GRAY] - src1[PIXEL_GRAY]);
}

void KisStrategyColorSpaceGrayscale::mixColors(const Q_UINT8 **colors, const Q_UINT8 *weights, Q_UINT32 nColors, Q_UINT8 *dst) const
{
	Q_UINT32 totalGray = 0, newAlpha = 0;

	while (nColors--)
	{
		Q_UINT32 alpha = (*colors)[PIXEL_GRAY_ALPHA];
		Q_UINT32 alphaTimesWeight = UINT8_MULT(alpha, *weights);

		totalGray += (*colors)[PIXEL_GRAY] * alphaTimesWeight;
		newAlpha += alphaTimesWeight;

		weights++;
		colors++;
	}

	Q_ASSERT(newAlpha <= 255);

	dst[PIXEL_GRAY_ALPHA] = newAlpha;

	if (newAlpha > 0) {
		totalGray = UINT8_DIVIDE(totalGray, newAlpha);
	}

	// Divide by 255.
	totalGray += 0x80;
	Q_UINT32 dstGray = ((totalGray >> 8) + totalGray) >> 8;
	Q_ASSERT(dstGray <= 255);
	dst[PIXEL_GRAY] = dstGray;
}

vKisChannelInfoSP KisStrategyColorSpaceGrayscale::channels() const
{
	return m_channels;
}

bool KisStrategyColorSpaceGrayscale::hasAlpha() const
{
	return true;
}

Q_INT32 KisStrategyColorSpaceGrayscale::nChannels() const
{
	return MAX_CHANNEL_GRAYSCALEA;
}

Q_INT32 KisStrategyColorSpaceGrayscale::nColorChannels() const
{
	return MAX_CHANNEL_GRAYSCALE;
}

Q_INT32 KisStrategyColorSpaceGrayscale::pixelSize() const
{
	return MAX_CHANNEL_GRAYSCALEA;
}


QImage KisStrategyColorSpaceGrayscale::convertToQImage(const Q_UINT8 *data, Q_INT32 width, Q_INT32 height,
						       KisProfileSP srcProfile, KisProfileSP dstProfile,
							   Q_INT32 renderingIntent, KisRenderInformationSP /*renderInfo*/)
{

	QImage img(width, height, 32, 0, QImage::LittleEndian);

	// No profiles
	if (srcProfile == 0 || dstProfile == 0) {
		Q_INT32 i = 0;
		uchar *j = img.bits();

		while ( i < width * height * MAX_CHANNEL_GRAYSCALEA) {
			Q_UINT8 q = *( data + i + PIXEL_GRAY );

			// XXX: Temporarily moved here to get rid of these global constants
			const Q_UINT8 PIXEL_BLUE = 0;
			const Q_UINT8 PIXEL_GREEN = 1;
			const Q_UINT8 PIXEL_RED = 2;
			const Q_UINT8 PIXEL_ALPHA = 3;

			*( j + PIXEL_ALPHA ) = *( data + i + PIXEL_GRAY_ALPHA );
			*( j + PIXEL_RED )   = q;
			*( j + PIXEL_GREEN ) = q;
			*( j + PIXEL_BLUE )  = q;

			i += MAX_CHANNEL_GRAYSCALEA;

			j += 4; // Because we're hard-coded 32 bits deep, 4 bytes
		}
		return img;
	}
	else {
		// Do a nice calibrated conversion
		KisStrategyColorSpaceSP dstCS = KisColorSpaceRegistry::instance() -> get("RGBA");
		convertPixelsTo(const_cast<Q_UINT8 *>(data), srcProfile,
				img.bits(), dstCS, dstProfile,
				width * height, renderingIntent);
	}


	// Create display transform if not present
	return img;
}

void KisStrategyColorSpaceGrayscale::adjustBrightnessContrast(const Q_UINT8 *src, Q_UINT8 *dst, Q_INT8 brightness, Q_INT8 contrast, Q_INT32 nPixels) const
{
	//XXX does nothing for now
}


void KisStrategyColorSpaceGrayscale::bitBlt(Q_UINT8 *dst,
				      Q_INT32 dstRowStride,
				      const Q_UINT8 *src,
				      Q_INT32 srcRowStride,
				      const Q_UINT8 *mask,
				      Q_INT32 maskRowStride,
				      QUANTUM opacity,
				      Q_INT32 rows,
				      Q_INT32 cols,
				      const KisCompositeOp& op)
{
	switch (op.op()) {
	case COMPOSITE_OVER:
		compositeOver(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_MULT:
		compositeMultiply(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_DIVIDE:
		compositeDivide(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_DARKEN:
		compositeDarken(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_LIGHTEN:
		compositeLighten(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_SCREEN:
		compositeScreen(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_OVERLAY:
		compositeOverlay(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_DODGE:
		compositeDodge(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_BURN:
		compositeBurn(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_ERASE:
		compositeErase(dst, dstRowStride, src, srcRowStride, mask, maskRowStride, rows, cols, opacity);
		break;
	case COMPOSITE_COPY: {
		Q_UINT8 *d;
		const Q_UINT8 *s;
		Q_INT32 linesize;

		linesize = MAX_CHANNEL_GRAYSCALEA*sizeof(Q_UINT8) * cols;
		d = dst;
		s = src;
		while (rows-- > 0) {
			memcpy(d, s, linesize);
			d += dstRowStride;
			s += srcRowStride;
		}
	}
		break;
	case COMPOSITE_CLEAR: {
		Q_UINT8 *d;
		Q_INT32 linesize;

		linesize = MAX_CHANNEL_GRAYSCALEA*sizeof(Q_UINT8) * cols;
		d = dst;
		while (rows-- > 0) {
			memset(d, 0, linesize);
			d += dstRowStride;
		}
	}
		break;
	default:
		break;
	}
}

KisCompositeOpList KisStrategyColorSpaceGrayscale::userVisiblecompositeOps() const
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

	return list;
}

void KisStrategyColorSpaceGrayscale::compositeOver(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, QUANTUM opacity)
{
	while (rows > 0) {

		const Q_UINT8 *src = srcRowStart;
		Q_UINT8 *dst = dstRowStart;
		Q_INT32 columns = numColumns;
		const Q_UINT8 *mask = maskRowStart;

		while (columns > 0) {

			Q_UINT8 srcAlpha = src[PIXEL_GRAY_ALPHA];

			// apply the alphamask
			if(mask != 0)
			{
				if(*mask != OPACITY_OPAQUE)
					srcAlpha = UINT8_MULT(srcAlpha, *mask);
				mask++;
			}
			
			if (srcAlpha != OPACITY_TRANSPARENT) {

				if (opacity != OPACITY_OPAQUE) {
					srcAlpha = UINT8_MULT(src[PIXEL_GRAY_ALPHA], opacity);
				}

				if (srcAlpha == OPACITY_OPAQUE) {
					memcpy(dst, src, MAX_CHANNEL_GRAYSCALEA * sizeof(Q_UINT8));
				} else {
					Q_UINT8 dstAlpha = dst[PIXEL_GRAY_ALPHA];

					Q_UINT8 srcBlend;

					if (dstAlpha == OPACITY_OPAQUE) {
						srcBlend = srcAlpha;
					} else {
						Q_UINT8 newAlpha = dstAlpha + UINT8_MULT(OPACITY_OPAQUE - dstAlpha, srcAlpha);
						dst[PIXEL_GRAY_ALPHA] = newAlpha;

						if (newAlpha != 0) {
							srcBlend = UINT8_DIVIDE(srcAlpha, newAlpha);
						} else {
							srcBlend = srcAlpha;
						}
					}

					if (srcBlend == OPACITY_OPAQUE) {
						memcpy(dst, src, MAX_CHANNEL_GRAYSCALE * sizeof(Q_UINT8));
					} else {
						dst[PIXEL_GRAY] = UINT8_BLEND(src[PIXEL_GRAY], dst[PIXEL_GRAY], srcBlend);
					}
				}
			}

			columns--;
			src += MAX_CHANNEL_GRAYSCALEA;
			dst += MAX_CHANNEL_GRAYSCALEA;
		}

		rows--;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
		if(maskRowStart)
			maskRowStart += maskRowStride;
	}
}

void KisStrategyColorSpaceGrayscale::compositeMultiply(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, QUANTUM opacity)
{
	while (rows > 0) {

		const Q_UINT8 *src = srcRowStart;
		Q_UINT8 *dst = dstRowStart;
		Q_INT32 columns = numColumns;
		const Q_UINT8 *mask = maskRowStart;

		while (columns > 0) {

			Q_UINT8 srcAlpha = src[PIXEL_GRAY_ALPHA];
			Q_UINT8 dstAlpha = dst[PIXEL_GRAY_ALPHA];

			srcAlpha = QMIN(srcAlpha, dstAlpha);

			// apply the alphamask
			if(mask != 0)
			{
				if(*mask != OPACITY_OPAQUE)
					srcAlpha = UINT8_MULT(srcAlpha, *mask);
				mask++;
			}
			
			if (srcAlpha != OPACITY_TRANSPARENT) {

				if (opacity != OPACITY_OPAQUE) {
					srcAlpha = UINT8_MULT(src[PIXEL_GRAY_ALPHA], opacity);
				}

				Q_UINT8 srcBlend;

				if (dstAlpha == OPACITY_OPAQUE) {
					srcBlend = srcAlpha;
				} else {
					Q_UINT8 newAlpha = dstAlpha + UINT8_MULT(OPACITY_OPAQUE - dstAlpha, srcAlpha);
					dst[PIXEL_GRAY_ALPHA] = newAlpha;

					if (newAlpha != 0) {
						srcBlend = UINT8_DIVIDE(srcAlpha, newAlpha);
					} else {
						srcBlend = srcAlpha;
					}
				}

				Q_UINT8 srcColor = src[PIXEL_GRAY];
				Q_UINT8 dstColor = dst[PIXEL_GRAY];

				srcColor = UINT8_MULT(srcColor, dstColor);

				dst[PIXEL_GRAY] = UINT8_BLEND(srcColor, dstColor, srcBlend);
			}

			columns--;
			src += MAX_CHANNEL_GRAYSCALEA;
			dst += MAX_CHANNEL_GRAYSCALEA;
		}

		rows--;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
		if(maskRowStart)
			maskRowStart += maskRowStride;
	}
}

void KisStrategyColorSpaceGrayscale::compositeDivide(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, QUANTUM opacity)
{
	while (rows > 0) {

		const Q_UINT8 *src = srcRowStart;
		Q_UINT8 *dst = dstRowStart;
		Q_INT32 columns = numColumns;
		const Q_UINT8 *mask = maskRowStart;

		while (columns > 0) {

			Q_UINT8 srcAlpha = src[PIXEL_GRAY_ALPHA];
			Q_UINT8 dstAlpha = dst[PIXEL_GRAY_ALPHA];

			srcAlpha = QMIN(srcAlpha, dstAlpha);

			// apply the alphamask
			if(mask != 0)
			{
				if(*mask != OPACITY_OPAQUE)
					srcAlpha = UINT8_MULT(srcAlpha, *mask);
				mask++;
			}
			
			if (srcAlpha != OPACITY_TRANSPARENT) {

				if (opacity != OPACITY_OPAQUE) {
					srcAlpha = UINT8_MULT(src[PIXEL_GRAY_ALPHA], opacity);
				}

				Q_UINT8 srcBlend;

				if (dstAlpha == OPACITY_OPAQUE) {
					srcBlend = srcAlpha;
				} else {
					Q_UINT8 newAlpha = dstAlpha + UINT8_MULT(OPACITY_OPAQUE - dstAlpha, srcAlpha);
					dst[PIXEL_GRAY_ALPHA] = newAlpha;

					if (newAlpha != 0) {
						srcBlend = UINT8_DIVIDE(srcAlpha, newAlpha);
					} else {
						srcBlend = srcAlpha;
					}
				}

				for (int channel = 0; channel < MAX_CHANNEL_GRAYSCALE; channel++) {

					Q_UINT8 srcColor = src[channel];
					Q_UINT8 dstColor = dst[channel];

					srcColor = QMIN((dstColor * (UINT8_MAX + 1)) / (1 + srcColor), UINT8_MAX);

					Q_UINT8 newColor = UINT8_BLEND(srcColor, dstColor, srcBlend);

					dst[channel] = newColor;
				}
			}

			columns--;
			src += MAX_CHANNEL_GRAYSCALEA;
			dst += MAX_CHANNEL_GRAYSCALEA;
		}

		rows--;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
		if(maskRowStart)
			maskRowStart += maskRowStride;
	}
}

void KisStrategyColorSpaceGrayscale::compositeScreen(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, QUANTUM opacity)
{
	while (rows > 0) {

		const Q_UINT8 *src = srcRowStart;
		Q_UINT8 *dst = dstRowStart;
		Q_INT32 columns = numColumns;
		const Q_UINT8 *mask = maskRowStart;

		while (columns > 0) {

			Q_UINT8 srcAlpha = src[PIXEL_GRAY_ALPHA];
			Q_UINT8 dstAlpha = dst[PIXEL_GRAY_ALPHA];

			srcAlpha = QMIN(srcAlpha, dstAlpha);

			// apply the alphamask
			if(mask != 0)
			{
				if(*mask != OPACITY_OPAQUE)
					srcAlpha = UINT8_MULT(srcAlpha, *mask);
				mask++;
			}
			
			if (srcAlpha != OPACITY_TRANSPARENT) {

				if (opacity != OPACITY_OPAQUE) {
					srcAlpha = UINT8_MULT(src[PIXEL_GRAY_ALPHA], opacity);
				}

				Q_UINT8 srcBlend;

				if (dstAlpha == OPACITY_OPAQUE) {
					srcBlend = srcAlpha;
				} else {
					Q_UINT8 newAlpha = dstAlpha + UINT8_MULT(OPACITY_OPAQUE - dstAlpha, srcAlpha);
					dst[PIXEL_GRAY_ALPHA] = newAlpha;

					if (newAlpha != 0) {
						srcBlend = UINT8_DIVIDE(srcAlpha, newAlpha);
					} else {
						srcBlend = srcAlpha;
					}
				}

				for (int channel = 0; channel < MAX_CHANNEL_GRAYSCALE; channel++) {

					Q_UINT8 srcColor = src[channel];
					Q_UINT8 dstColor = dst[channel];

					srcColor = UINT8_MAX - UINT8_MULT(UINT8_MAX - dstColor, UINT8_MAX - srcColor);

					Q_UINT8 newColor = UINT8_BLEND(srcColor, dstColor, srcBlend);

					dst[channel] = newColor;
				}
			}

			columns--;
			src += MAX_CHANNEL_GRAYSCALEA;
			dst += MAX_CHANNEL_GRAYSCALEA;
		}

		rows--;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
		if(maskRowStart)
			maskRowStart += maskRowStride;
	}
}

void KisStrategyColorSpaceGrayscale::compositeOverlay(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, QUANTUM opacity)
{
	while (rows > 0) {

		const Q_UINT8 *src = srcRowStart;
		Q_UINT8 *dst = dstRowStart;
		Q_INT32 columns = numColumns;
		const Q_UINT8 *mask = maskRowStart;

		while (columns > 0) {

			Q_UINT8 srcAlpha = src[PIXEL_GRAY_ALPHA];
			Q_UINT8 dstAlpha = dst[PIXEL_GRAY_ALPHA];

			srcAlpha = QMIN(srcAlpha, dstAlpha);

			// apply the alphamask
			if(mask != 0)
			{
				if(*mask != OPACITY_OPAQUE)
					srcAlpha = UINT8_MULT(srcAlpha, *mask);
				mask++;
			}
			
			if (srcAlpha != OPACITY_TRANSPARENT) {

				if (opacity != OPACITY_OPAQUE) {
					srcAlpha = UINT8_MULT(src[PIXEL_GRAY_ALPHA], opacity);
				}

				Q_UINT8 srcBlend;

				if (dstAlpha == OPACITY_OPAQUE) {
					srcBlend = srcAlpha;
				} else {
					Q_UINT8 newAlpha = dstAlpha + UINT8_MULT(OPACITY_OPAQUE - dstAlpha, srcAlpha);
					dst[PIXEL_GRAY_ALPHA] = newAlpha;

					if (newAlpha != 0) {
						srcBlend = UINT8_DIVIDE(srcAlpha, newAlpha);
					} else {
						srcBlend = srcAlpha;
					}
				}

				for (int channel = 0; channel < MAX_CHANNEL_GRAYSCALE; channel++) {

					Q_UINT8 srcColor = src[channel];
					Q_UINT8 dstColor = dst[channel];

					srcColor = UINT8_MULT(dstColor, dstColor + UINT8_MULT(2 * srcColor, UINT8_MAX - dstColor));

					Q_UINT8 newColor = UINT8_BLEND(srcColor, dstColor, srcBlend);

					dst[channel] = newColor;
				}
			}

			columns--;
			src += MAX_CHANNEL_GRAYSCALEA;
			dst += MAX_CHANNEL_GRAYSCALEA;
		}

		rows--;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
		if(maskRowStart)
			maskRowStart += maskRowStride;
	}
}

void KisStrategyColorSpaceGrayscale::compositeDodge(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, QUANTUM opacity)
{
	while (rows > 0) {

		const Q_UINT8 *src = srcRowStart;
		Q_UINT8 *dst = dstRowStart;
		Q_INT32 columns = numColumns;
		const Q_UINT8 *mask = maskRowStart;

		while (columns > 0) {

			Q_UINT8 srcAlpha = src[PIXEL_GRAY_ALPHA];
			Q_UINT8 dstAlpha = dst[PIXEL_GRAY_ALPHA];

			srcAlpha = QMIN(srcAlpha, dstAlpha);

			// apply the alphamask
			if(mask != 0)
			{
				if(*mask != OPACITY_OPAQUE)
					srcAlpha = UINT8_MULT(srcAlpha, *mask);
				mask++;
			}
			
			if (srcAlpha != OPACITY_TRANSPARENT) {

				if (opacity != OPACITY_OPAQUE) {
					srcAlpha = UINT8_MULT(src[PIXEL_GRAY_ALPHA], opacity);
				}

				Q_UINT8 srcBlend;

				if (dstAlpha == OPACITY_OPAQUE) {
					srcBlend = srcAlpha;
				} else {
					Q_UINT8 newAlpha = dstAlpha + UINT8_MULT(OPACITY_OPAQUE - dstAlpha, srcAlpha);
					dst[PIXEL_GRAY_ALPHA] = newAlpha;

					if (newAlpha != 0) {
						srcBlend = UINT8_DIVIDE(srcAlpha, newAlpha);
					} else {
						srcBlend = srcAlpha;
					}
				}

				for (int channel = 0; channel < MAX_CHANNEL_GRAYSCALE; channel++) {

					Q_UINT8 srcColor = src[channel];
					Q_UINT8 dstColor = dst[channel];

					srcColor = QMIN((dstColor * (UINT8_MAX + 1)) / (UINT8_MAX + 1 - srcColor), UINT8_MAX);

					Q_UINT8 newColor = UINT8_BLEND(srcColor, dstColor, srcBlend);

					dst[channel] = newColor;
				}
			}

			columns--;
			src += MAX_CHANNEL_GRAYSCALEA;
			dst += MAX_CHANNEL_GRAYSCALEA;
		}

		rows--;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
		if(maskRowStart)
			maskRowStart += maskRowStride;
	}
}

void KisStrategyColorSpaceGrayscale::compositeBurn(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, QUANTUM opacity)
{
	while (rows > 0) {

		const Q_UINT8 *src = srcRowStart;
		Q_UINT8 *dst = dstRowStart;
		Q_INT32 columns = numColumns;
		const Q_UINT8 *mask = maskRowStart;

		while (columns > 0) {

			Q_UINT8 srcAlpha = src[PIXEL_GRAY_ALPHA];
			Q_UINT8 dstAlpha = dst[PIXEL_GRAY_ALPHA];

			srcAlpha = QMIN(srcAlpha, dstAlpha);

			// apply the alphamask
			if(mask != 0)
			{
				if(*mask != OPACITY_OPAQUE)
					srcAlpha = UINT8_MULT(srcAlpha, *mask);
				mask++;
			}
			
			if (srcAlpha != OPACITY_TRANSPARENT) {

				if (opacity != OPACITY_OPAQUE) {
					srcAlpha = UINT8_MULT(src[PIXEL_GRAY_ALPHA], opacity);
				}

				Q_UINT8 srcBlend;

				if (dstAlpha == OPACITY_OPAQUE) {
					srcBlend = srcAlpha;
				} else {
					Q_UINT8 newAlpha = dstAlpha + UINT8_MULT(OPACITY_OPAQUE - dstAlpha, srcAlpha);
					dst[PIXEL_GRAY_ALPHA] = newAlpha;

					if (newAlpha != 0) {
						srcBlend = UINT8_DIVIDE(srcAlpha, newAlpha);
					} else {
						srcBlend = srcAlpha;
					}
				}

				for (int channel = 0; channel < MAX_CHANNEL_GRAYSCALE; channel++) {

					Q_UINT8 srcColor = src[channel];
					Q_UINT8 dstColor = dst[channel];

					srcColor = QMIN(((UINT8_MAX - dstColor) * (UINT8_MAX + 1)) / (srcColor + 1), UINT8_MAX);
					srcColor = CLAMP(UINT8_MAX - srcColor, 0, UINT8_MAX);

					Q_UINT8 newColor = UINT8_BLEND(srcColor, dstColor, srcBlend);

					dst[channel] = newColor;
				}
			}

			columns--;
			src += MAX_CHANNEL_GRAYSCALEA;
			dst += MAX_CHANNEL_GRAYSCALEA;
		}

		rows--;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
		if(maskRowStart)
			maskRowStart += maskRowStride;
	}
}

void KisStrategyColorSpaceGrayscale::compositeDarken(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, QUANTUM opacity)
{
	while (rows > 0) {

		const Q_UINT8 *src = srcRowStart;
		Q_UINT8 *dst = dstRowStart;
		Q_INT32 columns = numColumns;
		const Q_UINT8 *mask = maskRowStart;

		while (columns > 0) {

			Q_UINT8 srcAlpha = src[PIXEL_GRAY_ALPHA];
			Q_UINT8 dstAlpha = dst[PIXEL_GRAY_ALPHA];

			srcAlpha = QMIN(srcAlpha, dstAlpha);

			// apply the alphamask
			if(mask != 0)
			{
				if(*mask != OPACITY_OPAQUE)
					srcAlpha = UINT8_MULT(srcAlpha, *mask);
				mask++;
			}
			
			if (srcAlpha != OPACITY_TRANSPARENT) {

				if (opacity != OPACITY_OPAQUE) {
					srcAlpha = UINT8_MULT(src[PIXEL_GRAY_ALPHA], opacity);
				}

				Q_UINT8 srcBlend;

				if (dstAlpha == OPACITY_OPAQUE) {
					srcBlend = srcAlpha;
				} else {
					Q_UINT8 newAlpha = dstAlpha + UINT8_MULT(OPACITY_OPAQUE - dstAlpha, srcAlpha);
					dst[PIXEL_GRAY_ALPHA] = newAlpha;

					if (newAlpha != 0) {
						srcBlend = UINT8_DIVIDE(srcAlpha, newAlpha);
					} else {
						srcBlend = srcAlpha;
					}
				}

				for (int channel = 0; channel < MAX_CHANNEL_GRAYSCALE; channel++) {

					Q_UINT8 srcColor = src[channel];
					Q_UINT8 dstColor = dst[channel];

					srcColor = QMIN(srcColor, dstColor);

					Q_UINT8 newColor = UINT8_BLEND(srcColor, dstColor, srcBlend);

					dst[channel] = newColor;
				}
			}

			columns--;
			src += MAX_CHANNEL_GRAYSCALEA;
			dst += MAX_CHANNEL_GRAYSCALEA;
		}

		rows--;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
		if(maskRowStart)
			maskRowStart += maskRowStride;
	}
}

void KisStrategyColorSpaceGrayscale::compositeLighten(Q_UINT8 *dstRowStart, Q_INT32 dstRowStride, const Q_UINT8 *srcRowStart, Q_INT32 srcRowStride, const Q_UINT8 *maskRowStart, Q_INT32 maskRowStride, Q_INT32 rows, Q_INT32 numColumns, QUANTUM opacity)
{
	while (rows > 0) {

		const Q_UINT8 *src = srcRowStart;
		Q_UINT8 *dst = dstRowStart;
		Q_INT32 columns = numColumns;
		const Q_UINT8 *mask = maskRowStart;

		while (columns > 0) {

			Q_UINT8 srcAlpha = src[PIXEL_GRAY_ALPHA];
			Q_UINT8 dstAlpha = dst[PIXEL_GRAY_ALPHA];

			srcAlpha = QMIN(srcAlpha, dstAlpha);

			// apply the alphamask
			if(mask != 0)
			{
				if(*mask != OPACITY_OPAQUE)
					srcAlpha = UINT8_MULT(srcAlpha, *mask);
				mask++;
			}
			
			if (srcAlpha != OPACITY_TRANSPARENT) {

				if (opacity != OPACITY_OPAQUE) {
					srcAlpha = UINT8_MULT(src[PIXEL_GRAY_ALPHA], opacity);
				}

				Q_UINT8 srcBlend;

				if (dstAlpha == OPACITY_OPAQUE) {
					srcBlend = srcAlpha;
				} else {
					Q_UINT8 newAlpha = dstAlpha + UINT8_MULT(OPACITY_OPAQUE - dstAlpha, srcAlpha);
					dst[PIXEL_GRAY_ALPHA] = newAlpha;

					if (newAlpha != 0) {
						srcBlend = UINT8_DIVIDE(srcAlpha, newAlpha);
					} else {
						srcBlend = srcAlpha;
					}
				}

				for (int channel = 0; channel < MAX_CHANNEL_GRAYSCALE; channel++) {

					Q_UINT8 srcColor = src[channel];
					Q_UINT8 dstColor = dst[channel];

					srcColor = QMAX(srcColor, dstColor);

					Q_UINT8 newColor = UINT8_BLEND(srcColor, dstColor, srcBlend);

					dst[channel] = newColor;
				}
			}

			columns--;
			src += MAX_CHANNEL_GRAYSCALEA;
			dst += MAX_CHANNEL_GRAYSCALEA;
		}

		rows--;
		srcRowStart += srcRowStride;
		dstRowStart += dstRowStride;
		if(maskRowStart)
			maskRowStart += maskRowStride;
	}
}

void KisStrategyColorSpaceGrayscale::compositeErase(Q_UINT8 *dst, 
		    Q_INT32 dstRowSize,
		    const Q_UINT8 *src, 
		    Q_INT32 srcRowSize,
		    const Q_UINT8 *srcAlphaMask,
		    Q_INT32 maskRowStride,
		    Q_INT32 rows, 
		    Q_INT32 cols, 
		    QUANTUM /*opacity*/)
{
	Q_INT32 i;
	Q_UINT8 srcAlpha;
	
	while (rows-- > 0)
	{
		const Q_UINT8 *s = src;
		Q_UINT8 *d = dst;
		const Q_UINT8 *mask = srcAlphaMask;

		for (i = cols; i > 0; i--, s+=MAX_CHANNEL_GRAYSCALEA, d+=MAX_CHANNEL_GRAYSCALEA)
		{
			srcAlpha = s[PIXEL_GRAY_ALPHA];
			// apply the alphamask
			if(mask != 0)
			{
				if(*mask != OPACITY_OPAQUE)
					srcAlpha = UINT8_BLEND(srcAlpha, OPACITY_OPAQUE, *mask);
				mask++;
			}
			d[PIXEL_GRAY_ALPHA] = UINT8_MULT(srcAlpha, d[PIXEL_GRAY_ALPHA]);
		}

		dst += dstRowSize;
		if(srcAlphaMask)
			srcAlphaMask += maskRowStride;
		src += srcRowSize;
	}
}

QString KisStrategyColorSpaceGrayscale::channelValueText(const Q_UINT8 *pixel, Q_UINT32 channelIndex) const
{
	Q_ASSERT(channelIndex < nChannels());
	Q_UINT32 channelPosition = m_channels[channelIndex] -> pos();

	return QString().setNum(pixel[channelPosition]);
}

QString KisStrategyColorSpaceGrayscale::normalisedChannelValueText(const Q_UINT8 *pixel, Q_UINT32 channelIndex) const
{
	Q_ASSERT(channelIndex < nChannels());
	Q_UINT32 channelPosition = m_channels[channelIndex] -> pos();

	return QString().setNum(static_cast<float>(pixel[channelPosition]) / UINT8_MAX);
}

