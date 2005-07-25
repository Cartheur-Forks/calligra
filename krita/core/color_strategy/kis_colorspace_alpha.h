/*
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef KIS_COLORSPACE_ALPHA_H_
#define KIS_COLORSPACE_ALPHA_H_

#include <qcolor.h>

#include <qcolor.h>

#include "kis_global.h"
#include "kis_strategy_colorspace.h"
#include "kis_pixel.h"

/**
 * The alpha mask is a special color strategy that treats all pixels as
 * alpha value with a colour common to the mask. The default color is white.
 */
class KisColorSpaceAlpha : public KisStrategyColorSpace {
public:
	KisColorSpaceAlpha();
	virtual ~KisColorSpaceAlpha();

public:
	virtual void nativeColor(const QColor& c, Q_UINT8 *dst, KisProfileSP profile = 0);
	virtual void nativeColor(const QColor& c, QUANTUM opacity, Q_UINT8 *dst, KisProfileSP profile = 0);

	virtual void getAlpha(const Q_UINT8 *pixel, Q_UINT8 *alpha);

	virtual void toQColor(const Q_UINT8 *src, QColor *c, KisProfileSP profile = 0);
	virtual void toQColor(const Q_UINT8 *src, QColor *c, QUANTUM *opacity, KisProfileSP profile = 0);

	virtual KisPixelRO toKisPixelRO(const Q_UINT8 *src, KisProfileSP profile = 0) { return KisPixelRO (src, src, this, profile); }
	virtual KisPixel toKisPixel(Q_UINT8 *src, KisProfileSP profile = 0) { return KisPixel (src, src, this, profile); }
	
	virtual Q_INT8 difference(const Q_UINT8 *src1, const Q_UINT8 *src2);
	virtual void mixColors(const Q_UINT8 **colors, const Q_UINT8 *weights, Q_UINT32 nColors, Q_UINT8 *dst) const;

	virtual vKisChannelInfoSP channels() const;
	virtual bool hasAlpha() const;
	virtual Q_INT32 nChannels() const { return 1; };
	virtual Q_INT32 nColorChannels() const { return 0; };
	virtual Q_INT32 pixelSize() const { return 1; };

	virtual QString channelValueText(const Q_UINT8 *pixel, Q_UINT32 channelIndex) const;
	virtual QString normalisedChannelValueText(const Q_UINT8 *pixel, Q_UINT32 channelIndex) const;

	virtual QImage convertToQImage(const Q_UINT8 *data, Q_INT32 width, Q_INT32 height,
				       KisProfileSP srcProfile, KisProfileSP dstProfile,
				       Q_INT32 renderingIntent = INTENT_PERCEPTUAL,
					   KisRenderInformationSP renderInfo = 0);

	virtual void adjustBrightnessContrast(const Q_UINT8 *src, Q_UINT8 *dst, Q_INT8 brightness, Q_INT8 contrast, Q_INT32 nPixels) const;

	virtual void setMaskColor(QColor c) { m_maskColor = c; }

protected:

	/**
	 * Convert a byte array of srcLen pixels *src to the specified color space
	 * and put the converted bytes into the prepared byte array *dst.
	 *
	 * Returns false if the conversion failed, true if it succeeded
	 */
	virtual bool convertPixelsTo(const Q_UINT8 *src, KisProfileSP srcProfile,
				     Q_UINT8 *dst, KisStrategyColorSpaceSP dstColorStrategy, KisProfileSP dstProfile,
				     Q_UINT32 numPixels,
				     Q_INT32 renderingIntent = INTENT_PERCEPTUAL);



	virtual void bitBlt(Q_UINT8 *dst,
			    Q_INT32 dststride,
			    const Q_UINT8 *src,
			    Q_INT32 srcRowStride,
			    const Q_UINT8 *srcAlphaMask,
			    Q_INT32 maskRowStride,
			    QUANTUM opacity,
			    Q_INT32 rows,
			    Q_INT32 cols,
			    const KisCompositeOp& op);

	KisCompositeOpList userVisiblecompositeOps() const;

private:
	vKisChannelInfoSP m_channels;

	QColor m_maskColor;
	bool m_inverted;
};

#endif // KIS_COLORSPACE_ALPHA_H_
