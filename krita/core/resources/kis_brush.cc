/*
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *  Copyright (c) 2003 Patrick Julien  <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <netinet/in.h>
#include <limits.h>
#include <stdlib.h>
#include <cfloat>

#include <qimage.h>
#include <qpoint.h>
#include <qvaluevector.h>

#include <kdebug.h>

#include "kis_global.h"
#include "kis_brush.h"
#include "kis_alpha_mask.h"
#include "kis_colorspace_registry.h"

namespace {
	struct GimpBrushV1Header {
		Q_UINT32 header_size;  /*  header_size = sizeof (BrushHeader) + brush name  */
		Q_UINT32 version;      /*  brush file version #  */
		Q_UINT32 width;        /*  width of brush  */
		Q_UINT32 height;       /*  height of brush  */
		Q_UINT32 bytes;        /*  depth of brush in bytes */
	};

	struct GimpBrushHeader {
		Q_UINT32 header_size;  /*  header_size = sizeof (BrushHeader) + brush name  */
		Q_UINT32 version;      /*  brush file version #  */
		Q_UINT32 width;        /*  width of brush  */
		Q_UINT32 height;       /*  height of brush  */
		Q_UINT32 bytes;        /*  depth of brush in bytes */

				       /*  The following are only defined in version 2 */
		Q_UINT32 magic_number; /*  GIMP brush magic number  */
		Q_UINT32 spacing;      /*  brush spacing as % of width & height, 0 - 1000 */
	};
}

#define DEFAULT_SPACING 0.25
#define MAXIMUM_SCALE 2

KisBrush::KisBrush(const QString& filename) : super(filename)
{
	m_brushType = INVALID;
	m_ownData = true;
	m_useColorAsMask = false;
	m_hasColor = false;
	m_spacing = DEFAULT_SPACING;
}

KisBrush::KisBrush(const QString& filename,
		   const QByteArray& data,
		   Q_UINT32 & dataPos) : super(filename)
{
	m_brushType = INVALID;
	m_ownData = false;
	m_useColorAsMask = false;
	m_hasColor = false;
	m_spacing = DEFAULT_SPACING;

	m_data.setRawData(data.data() + dataPos, data.size() - dataPos);
	ioResult(0);
	m_data.resetRawData(data.data() + dataPos, data.size() - dataPos);
	dataPos += m_header_size + (width() * height() * m_bytes);
}


KisBrush::~KisBrush()
{
	m_scaledBrushes.clear();
}

bool KisBrush::loadAsync()
{
	KIO::Job *job = KIO::get(filename(), false, false);

	connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)), this, SLOT(ioData(KIO::Job*, const QByteArray&)));
	connect(job, SIGNAL(result(KIO::Job*)), SLOT(ioResult(KIO::Job*)));
	return true;
}

bool KisBrush::saveAsync()
{
	return false;
}

QImage KisBrush::img()
{
	QImage image = m_img;

	if (hasColor() && useColorAsMask()) {
		image.detach();

		for (int x = 0; x < image.width(); x++) {
			for (int y = 0; y < image.height(); y++) {
				QRgb c = image.pixel(x, y);
				int a = (qGray(c) * qAlpha(c)) / 255;
				image.setPixel(x, y, qRgba(a, 0, a, a));
			}
		}
	}

	return image;
}

KisAlphaMaskSP KisBrush::mask(double pressure, double subPixelX, double subPixelY) const
{
	if (m_scaledBrushes.isEmpty()) {
		createScaledBrushes();
	}

	double scale = scaleForPressure(pressure);

	const ScaledBrush *aboveBrush = 0;
	const ScaledBrush *belowBrush = 0;

	findScaledBrushes(scale, &aboveBrush,  &belowBrush);
	Q_ASSERT(aboveBrush != 0);

	KisAlphaMaskSP outputMask = 0;

	if (belowBrush != 0) {
		// We're in between two masks. Interpolate between them.

		KisAlphaMaskSP scaledAboveMask = scaleMask(aboveBrush, scale, subPixelX, subPixelY);
		KisAlphaMaskSP scaledBelowMask = scaleMask(belowBrush, scale, subPixelX, subPixelY);

		double t = (scale - belowBrush -> scale()) / (aboveBrush -> scale() - belowBrush -> scale());

		outputMask = KisAlphaMask::interpolate(scaledBelowMask, scaledAboveMask, t);
	} else {
		if (fabs(scale - aboveBrush -> scale()) < DBL_EPSILON) {
			// Exact match.
			outputMask = scaleMask(aboveBrush, scale, subPixelX, subPixelY);
		} else {
			// We are smaller than the smallest mask, which is always 1x1.
			double s = scale / aboveBrush -> scale();
			outputMask = scaleSinglePixelMask(s, aboveBrush -> mask() -> alphaAt(0, 0), subPixelX, subPixelY);
		}
	}

	return outputMask;
}

KisLayerSP KisBrush::image(KisStrategyColorSpaceSP colorSpace, double pressure, double subPixelX, double subPixelY) const
{
	if (m_scaledBrushes.isEmpty()) {
		createScaledBrushes();
	}

	double scale = scaleForPressure(pressure);

	const ScaledBrush *aboveBrush = 0;
	const ScaledBrush *belowBrush = 0;

	findScaledBrushes(scale, &aboveBrush,  &belowBrush);
	Q_ASSERT(aboveBrush != 0);

	QImage outputImage;

	if (belowBrush != 0) {
		// We're in between two brushes. Interpolate between them.

		QImage scaledAboveImage = scaleImage(aboveBrush, scale, subPixelX, subPixelY);
		QImage scaledBelowImage = scaleImage(belowBrush, scale, subPixelX, subPixelY);

		double t = (scale - belowBrush -> scale()) / (aboveBrush -> scale() - belowBrush -> scale());

		outputImage = interpolate(scaledBelowImage, scaledAboveImage, t);
	} else {
		if (fabs(scale - aboveBrush -> scale()) < DBL_EPSILON) {
			// Exact match.
			outputImage = scaleImage(aboveBrush, scale, subPixelX, subPixelY);
		} else {
			// We are smaller than the smallest brush, which is always 1x1.
			double s = scale / aboveBrush -> scale();
			outputImage = scaleSinglePixelImage(s, aboveBrush -> image().pixel(0, 0), subPixelX, subPixelY);
		}
	}

	int outputWidth = outputImage.width();
	int outputHeight = outputImage.height();

	KisLayer *layer = new KisLayer(outputWidth, outputHeight, colorSpace, "brush image");

	for (int y = 0; y < outputHeight; y++) {
		for (int x = 0; x < outputWidth; x++) {

			QRgb pixel = outputImage.pixel(x, y);
			int red = qRed(pixel);
			int green = qGreen(pixel);
			int blue = qBlue(pixel);
			int alpha = qAlpha(pixel);

			// Scaled images are in pre-multiplied alpha form so
			// divide by alpha.
			if (alpha != 0) {
				red = (red * 255) / alpha;
				green = (green * 255) / alpha;
				blue = (blue * 255) / alpha;
			}

			KoColor colour = KoColor(red, green, blue);
			QUANTUM a = (alpha * OPACITY_OPAQUE) / 255;

			layer -> setPixel(x, y, colour, a);
		}
	}

	return layer;
}

void KisBrush::setHotSpot(KisPoint pt)
{
	double x = pt.x();
	double y = pt.y();

	if (x < 0)
		x = 0;
	else if (x >= width())
		x = width() - 1;

	if (y < 0)
		y = 0;
	else if (y >= height())
		y = height() - 1;

	m_hotSpot = KisPoint(x, y);
}

KisPoint KisBrush::hotSpot(double pressure) const
{
	double scale = scaleForPressure(pressure);
	double w = width() * scale;
	double h = height() * scale;

	// The smallest brush we can produce is a single pixel.
	if (w < 1) {
		w = 1;
	}

	if (h < 1) {
		h = 1;
	}

	// XXX: This should take m_hotSpot into account, though it
	// isn't specified by gimp brushes so it would default to the centre
	// anyway.
	KisPoint p(w / 2, h / 2);
	return p;
}

enumBrushType KisBrush::brushType() const
{
	if (m_brushType == IMAGE && useColorAsMask()) {
		return MASK;
	}
	else {
		return m_brushType;
	}
}

bool KisBrush::hasColor() const
{
	return m_hasColor;
}

void KisBrush::ioData(KIO::Job * /*job*/, const QByteArray& data)
{
	if (!data.isEmpty()) {
		Q_INT32 startPos = m_data.size();

		m_data.resize(m_data.size() + data.count());
		memcpy(&m_data[startPos], data.data(), data.count());
	}
}

void KisBrush::ioResult(KIO::Job * /*job*/)
{
	GimpBrushHeader bh;

	if (sizeof(GimpBrushHeader) > m_data.size()) {
		emit ioFailed(this);
		return;
	}

	memcpy(&bh, &m_data[0], sizeof(GimpBrushHeader));
	bh.header_size = ntohl(bh.header_size);
	m_header_size = bh.header_size;

	bh.version = ntohl(bh.version);
	m_version = bh.version;

	bh.width = ntohl(bh.width);
	bh.height = ntohl(bh.height);

	bh.bytes = ntohl(bh.bytes);
	m_bytes = bh.bytes;

	bh.magic_number = ntohl(bh.magic_number);
	m_magic_number = bh.magic_number;

	if (bh.version == 1) {
		// No spacing in version 1 files so use Gimp default
		bh.spacing = static_cast<int>(DEFAULT_SPACING * 100);
	}
	else {
		bh.spacing = ntohl(bh.spacing);
	
		if (bh.spacing > 1000) {
			emit ioFailed(this);
			return;
		}
	}

	setSpacing(bh.spacing / 100.0);

	if (bh.header_size > m_data.size() || bh.header_size == 0) {
		emit ioFailed(this);
		return;
	}

	QString name;

	if (bh.version == 1) {
		// Version 1 has no magic number or spacing, so the name
		// is at a different offset. Character encoding is undefined.
		const char *text = &m_data[sizeof(GimpBrushV1Header)];
		name = QString::fromAscii(text, bh.header_size - sizeof(GimpBrushV1Header));
	} else {
		name = QString::fromAscii(&m_data[sizeof(GimpBrushHeader)], bh.header_size - sizeof(GimpBrushHeader));
	}

	setName(name);

	if (bh.width == 0 || bh.height == 0 || !m_img.create(bh.width, bh.height, 32)) {
		emit ioFailed(this);
		return;
	}

	Q_INT32 k = bh.header_size;

	if (bh.bytes == 1) {
		// Grayscale

		if (static_cast<Q_UINT32>(k + bh.width * bh.height) > m_data.size()) {
			emit ioFailed(this);
			return;
		}

		m_brushType = MASK;
		m_hasColor = false;

		for (Q_UINT32 y = 0; y < bh.height; y++) {
			for (Q_UINT32 x = 0; x < bh.width; x++, k++) {
				Q_INT32 val = 255 - static_cast<uchar>(m_data[k]);
				m_img.setPixel(x, y, qRgb(val, val, val));
			}
		}
	} else if (bh.bytes == 4) {
		// RGBA

		if (static_cast<Q_UINT32>(k + (bh.width * bh.height * 4)) > m_data.size()) {
			emit ioFailed(this);
			return;
		}

		m_brushType = IMAGE;
		m_img.setAlphaBuffer(true);
		m_hasColor = true;

		for (Q_UINT32 y = 0; y < bh.height; y++) {
			for (Q_UINT32 x = 0; x < bh.width; x++, k += 4) {
				m_img.setPixel(x, y, qRgba(m_data[k],
							   m_data[k+1],
							   m_data[k+2],
							   m_data[k+3]));
			}
		}
	} else {
		emit ioFailed(this);
		return;
	}

	setWidth(m_img.width());
	setHeight(m_img.height());
	//createScaledBrushes();
	if (m_ownData) {
		m_data.resize(0); // Save some memory, we're using enough of it as it is.
	}
 	//kdDebug() << "Brush: " << &name[0] << " spacing: " << spacing() << "\n";
	setValid(true);
	emit loadComplete(this);
}

void KisBrush::createScaledBrushes() const
{
	if (!m_scaledBrushes.isEmpty())
		m_scaledBrushes.clear();

	// Construct a series of brushes where each one's dimensions are
	// half the size of the previous one.
	int width = m_img.width() * MAXIMUM_SCALE;
	int height = m_img.height() * MAXIMUM_SCALE;

	QImage scaledImage;

	while (true) {
		if (width >= m_img.width() && height >= m_img.height()) {
			scaledImage = scaleImage(m_img, width, height);
		}
		else {
			// Scale down the previous image once we're below 1:1.
			scaledImage = scaleImage(scaledImage, width, height);
		}

		KisAlphaMaskSP scaledMask = new KisAlphaMask(scaledImage, hasColor());

		double xScale = static_cast<double>(width) / m_img.width();
		double yScale = static_cast<double>(height) / m_img.height();
		double scale = xScale;

		m_scaledBrushes.append(ScaledBrush(scaledMask, hasColor() ? scaledImage : QImage(), scale, xScale, yScale));

		if (width == 1 && height == 1) {
			break;
		}

		// Round up so that we never have to scale an image by less than 1/2.
		width = (width + 1) / 2;
		height = (height + 1) / 2;
	}
}

double KisBrush::xSpacing(double pressure) const
{
	return width() * scaleForPressure(pressure) * m_spacing;
}

double KisBrush::ySpacing(double pressure) const
{
	return height() * scaleForPressure(pressure) * m_spacing;
}

double KisBrush::scaleForPressure(double pressure)
{
	double scale = pressure / PRESSURE_DEFAULT;

	if (scale < 0) {
		scale = 0;
	}

	if (scale > MAXIMUM_SCALE) {
		scale = MAXIMUM_SCALE;
	}

	return scale;
}

KisAlphaMaskSP KisBrush::scaleMask(const ScaledBrush *srcBrush, double scale, double subPixelX, double subPixelY) const
{
	// Add one pixel for sub-pixel shifting
	int dstWidth = static_cast<int>(ceil(scale * width())) + 1;
	int dstHeight = static_cast<int>(ceil(scale * height())) + 1;

	KisAlphaMaskSP dstMask = new KisAlphaMask(dstWidth, dstHeight);

	KisAlphaMaskSP srcMask = srcBrush -> mask();

	// Compute scales to map the scaled brush onto the required scale.
	double xScale = srcBrush -> xScale() / scale;
	double yScale = srcBrush -> yScale() / scale;

	int srcWidth = srcMask -> width();
	int srcHeight = srcMask -> height();

	for (int dstY = 0; dstY < dstHeight; dstY++) {
		for (int dstX = 0; dstX < dstWidth; dstX++) {

			double srcX = (dstX - subPixelX + 0.5) * xScale;
			double srcY = (dstY - subPixelY + 0.5) * yScale;

			srcX -= 0.5;
			srcY -= 0.5;

			int leftX = static_cast<int>(srcX);

			if (srcX < 0) {
				leftX--;
			}

			double xInterp = srcX - leftX;

			int topY = static_cast<int>(srcY);

			if (srcY < 0) {
				topY--;
			}

			double yInterp = srcY - topY;

			QUANTUM topLeft = (leftX >= 0 && leftX < srcWidth && topY >= 0 && topY < srcHeight) ? srcMask -> alphaAt(leftX, topY) : OPACITY_TRANSPARENT;
			QUANTUM bottomLeft = (leftX >= 0 && leftX < srcWidth && topY + 1 >= 0 && topY + 1 < srcHeight) ? srcMask -> alphaAt(leftX, topY + 1) : OPACITY_TRANSPARENT;
			QUANTUM topRight = (leftX + 1 >= 0 && leftX + 1 < srcWidth && topY >= 0 && topY < srcHeight) ? srcMask -> alphaAt(leftX + 1, topY) : OPACITY_TRANSPARENT;
			QUANTUM bottomRight = (leftX + 1 >= 0 && leftX + 1 < srcWidth && topY + 1 >= 0 && topY + 1 < srcHeight) ? srcMask -> alphaAt(leftX + 1, topY + 1) : OPACITY_TRANSPARENT;

			double a = 1 - xInterp;
			double b = 1 - yInterp;

			// Bi-linear interpolation
			int d = static_cast<int>(a * b * topLeft
				+ a * (1 - b) * bottomLeft
				+ (1 - a) * b * topRight
				+ (1 - a) * (1 - b) * bottomRight + 0.5);

			if (d < OPACITY_TRANSPARENT) {
				d = OPACITY_TRANSPARENT;
			}
			else
			if (d > OPACITY_OPAQUE) {
				d = OPACITY_OPAQUE;
			}

			dstMask -> setAlphaAt(dstX, dstY, static_cast<QUANTUM>(d));
		}
	}

	return dstMask;
}

QImage KisBrush::scaleImage(const ScaledBrush *srcBrush, double scale, double subPixelX, double subPixelY) const
{
	// Add one pixel for sub-pixel shifting
	int dstWidth = static_cast<int>(ceil(scale * width())) + 1;
	int dstHeight = static_cast<int>(ceil(scale * height())) + 1;

	QImage dstImage(dstWidth, dstHeight, 32);
	dstImage.setAlphaBuffer(true);

	const QImage srcImage = srcBrush -> image();

	// Compute scales to map the scaled brush onto the required scale.
	double xScale = srcBrush -> xScale() / scale;
	double yScale = srcBrush -> yScale() / scale;

	int srcWidth = srcImage.width();
	int srcHeight = srcImage.height();

	for (int dstY = 0; dstY < dstHeight; dstY++) {
		for (int dstX = 0; dstX < dstWidth; dstX++) {

			double srcX = (dstX - subPixelX + 0.5) * xScale;
			double srcY = (dstY - subPixelY + 0.5) * yScale;

			srcX -= 0.5;
			srcY -= 0.5;

			int leftX = static_cast<int>(srcX);

			if (srcX < 0) {
				leftX--;
			}

			double xInterp = srcX - leftX;

			int topY = static_cast<int>(srcY);

			if (srcY < 0) {
				topY--;
			}

			double yInterp = srcY - topY;

			QRgb topLeft = (leftX >= 0 && leftX < srcWidth && topY >= 0 && topY < srcHeight) ? srcImage.pixel(leftX, topY) : qRgba(0, 0, 0, 0);
			QRgb bottomLeft = (leftX >= 0 && leftX < srcWidth && topY + 1 >= 0 && topY + 1 < srcHeight) ? srcImage.pixel(leftX, topY + 1) : qRgba(0, 0, 0, 0);
			QRgb topRight = (leftX + 1 >= 0 && leftX + 1 < srcWidth && topY >= 0 && topY < srcHeight) ? srcImage.pixel(leftX + 1, topY) : qRgba(0, 0, 0, 0);
			QRgb bottomRight = (leftX + 1 >= 0 && leftX + 1 < srcWidth && topY + 1 >= 0 && topY + 1 < srcHeight) ? srcImage.pixel(leftX + 1, topY + 1) : qRgba(0, 0, 0, 0);

			double a = 1 - xInterp;
			double b = 1 - yInterp;

			// Bi-linear interpolation. Image is pre-multiplied by alpha.
			int red = static_cast<int>(a * b * qRed(topLeft)
				+ a * (1 - b) * qRed(bottomLeft)
				+ (1 - a) * b * qRed(topRight)
				+ (1 - a) * (1 - b) * qRed(bottomRight) + 0.5);
			int green = static_cast<int>(a * b * qGreen(topLeft)
				+ a * (1 - b) * qGreen(bottomLeft)
				+ (1 - a) * b * qGreen(topRight)
				+ (1 - a) * (1 - b) * qGreen(bottomRight) + 0.5);
			int blue = static_cast<int>(a * b * qBlue(topLeft)
				+ a * (1 - b) * qBlue(bottomLeft)
				+ (1 - a) * b * qBlue(topRight)
				+ (1 - a) * (1 - b) * qBlue(bottomRight) + 0.5);
			int alpha = static_cast<int>(a * b * qAlpha(topLeft)
				+ a * (1 - b) * qAlpha(bottomLeft)
				+ (1 - a) * b * qAlpha(topRight)
				+ (1 - a) * (1 - b) * qAlpha(bottomRight) + 0.5);

			if (red < 0) {
				red = 0;
			}
			else
			if (red > 255) {
				red = 255;
			}

			if (green < 0) {
				green = 0;
			}
			else
			if (green > 255) {
				green = 255;
			}

			if (blue < 0) {
				blue = 0;
			}
			else
			if (blue > 255) {
				blue = 255;
			}

			if (alpha < 0) {
				alpha = 0;
			}
			else
			if (alpha > 255) {
				alpha = 255;
			}

			dstImage.setPixel(dstX, dstY, qRgba(red, green, blue, alpha));
		}
	}

	return dstImage;
}

QImage KisBrush::scaleImage(const QImage& srcImage, int width, int height)
{
	QImage scaledImage;
	//QString filename;

	int srcWidth = srcImage.width();
	int srcHeight = srcImage.height();

	double xScale = static_cast<double>(srcWidth) / width;
	double yScale = static_cast<double>(srcHeight) / height;

	if (xScale > 2 + DBL_EPSILON || yScale > 2 + DBL_EPSILON || xScale < 1 - DBL_EPSILON || yScale < 1 - DBL_EPSILON) {
		// smoothScale gives better results when scaling an image up
		// or scaling it to less than half size.
		scaledImage = srcImage.smoothScale(width, height);

		//filename = QString("smoothScale_%1x%2.png").arg(width).arg(height);
	}
	else {
		scaledImage.create(width, height, 32);
		scaledImage.setAlphaBuffer(srcImage.hasAlphaBuffer());

		for (int dstY = 0; dstY < height; dstY++) {
			for (int dstX = 0; dstX < width; dstX++) {

				double srcX = (dstX + 0.5) * xScale;
				double srcY = (dstY + 0.5) * yScale;

				srcX -= 0.5;
				srcY -= 0.5;

				int leftX = static_cast<int>(srcX);

				if (srcX < 0) {
					leftX--;
				}

				double xInterp = srcX - leftX;

				int topY = static_cast<int>(srcY);

				if (srcY < 0) {
					topY--;
				}

				double yInterp = srcY - topY;

				QRgb topLeft = (leftX >= 0 && leftX < srcWidth && topY >= 0 && topY < srcHeight) ? srcImage.pixel(leftX, topY) : qRgba(0, 0, 0, 0);
				QRgb bottomLeft = (leftX >= 0 && leftX < srcWidth && topY + 1 >= 0 && topY + 1 < srcHeight) ? srcImage.pixel(leftX, topY + 1) : qRgba(0, 0, 0, 0);
				QRgb topRight = (leftX + 1 >= 0 && leftX + 1 < srcWidth && topY >= 0 && topY < srcHeight) ? srcImage.pixel(leftX + 1, topY) : qRgba(0, 0, 0, 0);
				QRgb bottomRight = (leftX + 1 >= 0 && leftX + 1 < srcWidth && topY + 1 >= 0 && topY + 1 < srcHeight) ? srcImage.pixel(leftX + 1, topY + 1) : qRgba(0, 0, 0, 0);

				double a = 1 - xInterp;
				double b = 1 - yInterp;

				int red;
				int green;
				int blue;
				int alpha;

				if (srcImage.hasAlphaBuffer()) {
					red = static_cast<int>(a * b * qRed(topLeft)         * qAlpha(topLeft)
						+ a * (1 - b) * qRed(bottomLeft)             * qAlpha(bottomLeft)
						+ (1 - a) * b * qRed(topRight)               * qAlpha(topRight)
						+ (1 - a) * (1 - b) * qRed(bottomRight)      * qAlpha(bottomRight) + 0.5);
					green = static_cast<int>(a * b * qGreen(topLeft)     * qAlpha(topLeft)      
						+ a * (1 - b) * qGreen(bottomLeft)           * qAlpha(bottomLeft)   
						+ (1 - a) * b * qGreen(topRight)             * qAlpha(topRight)     
						+ (1 - a) * (1 - b) * qGreen(bottomRight)    * qAlpha(bottomRight) + 0.5);
					blue = static_cast<int>(a * b * qBlue(topLeft)       * qAlpha(topLeft)      
						+ a * (1 - b) * qBlue(bottomLeft)            * qAlpha(bottomLeft)   
						+ (1 - a) * b * qBlue(topRight)              * qAlpha(topRight)     
						+ (1 - a) * (1 - b) * qBlue(bottomRight)     * qAlpha(bottomRight) + 0.5);
					alpha = static_cast<int>(a * b * qAlpha(topLeft)
						+ a * (1 - b) * qAlpha(bottomLeft)
						+ (1 - a) * b * qAlpha(topRight)
						+ (1 - a) * (1 - b) * qAlpha(bottomRight) + 0.5);

					if (alpha != 0) {
						red /= alpha;
						green /= alpha;
						blue /= alpha;
					}
				}
				else {
					red = static_cast<int>(a * b * qRed(topLeft)
						+ a * (1 - b) * qRed(bottomLeft)
						+ (1 - a) * b * qRed(topRight)
						+ (1 - a) * (1 - b) * qRed(bottomRight) + 0.5);
					green = static_cast<int>(a * b * qGreen(topLeft)
						+ a * (1 - b) * qGreen(bottomLeft)
						+ (1 - a) * b * qGreen(topRight)
						+ (1 - a) * (1 - b) * qGreen(bottomRight) + 0.5);
					blue = static_cast<int>(a * b * qBlue(topLeft)
						+ a * (1 - b) * qBlue(bottomLeft)
						+ (1 - a) * b * qBlue(topRight)
						+ (1 - a) * (1 - b) * qBlue(bottomRight) + 0.5);
					alpha = 255;
				}

				if (red < 0) {
					red = 0;
				}
				else
				if (red > 255) {
					red = 255;
				}

				if (green < 0) {
					green = 0;
				}
				else
				if (green > 255) {
					green = 255;
				}

				if (blue < 0) {
					blue = 0;
				}
				else
				if (blue > 255) {
					blue = 255;
				}

				if (alpha < 0) {
					alpha = 0;
				}
				else
				if (alpha > 255) {
					alpha = 255;
				}

				scaledImage.setPixel(dstX, dstY, qRgba(red, green, blue, alpha));
			}
		}

		//filename = QString("bilinear_%1x%2.png").arg(width).arg(height);
	}

	//scaledImage.save(filename, "PNG");

	return scaledImage;
}

void KisBrush::findScaledBrushes(double scale, const ScaledBrush **aboveBrush, const ScaledBrush **belowBrush) const
{
	uint current = 0;

	while (true) {
		*aboveBrush = &(m_scaledBrushes[current]);

		if (fabs((*aboveBrush) -> scale() - scale) < DBL_EPSILON) {
			// Scale matches exactly
			break;
		}

		if (current == m_scaledBrushes.count() - 1) {
			// This is the last one
			break;
		}

		if (scale > m_scaledBrushes[current + 1].scale() + DBL_EPSILON) {
			// We fit in between the two.
			*belowBrush = &(m_scaledBrushes[current + 1]);
			break;
		}

		current++;
	}
}

KisAlphaMaskSP KisBrush::scaleSinglePixelMask(double scale, QUANTUM maskValue, double subPixelX, double subPixelY)
{
	int srcWidth = 1;
	int srcHeight = 1;
	int dstWidth = 2;
	int dstHeight = 2;
	KisAlphaMaskSP outputMask = new KisAlphaMask(dstWidth, dstHeight);

	double a = subPixelX;
	double b = subPixelY;

	for (int y = 0; y < dstHeight; y++) {
		for (int x = 0; x < dstWidth; x++) {

			QUANTUM topLeft = (x > 0 && y > 0) ? maskValue : OPACITY_TRANSPARENT;
			QUANTUM bottomLeft = (x > 0 && y < srcHeight) ? maskValue : OPACITY_TRANSPARENT;
			QUANTUM topRight = (x < srcWidth && y > 0) ? maskValue : OPACITY_TRANSPARENT;
			QUANTUM bottomRight = (x < srcWidth && y < srcHeight) ? maskValue : OPACITY_TRANSPARENT;

			// Bi-linear interpolation
			int d = static_cast<int>(a * b * topLeft
				+ a * (1 - b) * bottomLeft
				+ (1 - a) * b * topRight
				+ (1 - a) * (1 - b) * bottomRight + 0.5);

			// Multiply by the square of the scale because a 0.5x0.5 pixel
			// has 0.25 the value of the 1x1.
			d = static_cast<int>(d * scale * scale + 0.5);

			if (d < OPACITY_TRANSPARENT) {
				d = OPACITY_TRANSPARENT;
			}
			else
			if (d > OPACITY_OPAQUE) {
				d = OPACITY_OPAQUE;
			}

			outputMask -> setAlphaAt(x, y, static_cast<QUANTUM>(d));
		}
	}

	return outputMask;
}

QImage KisBrush::scaleSinglePixelImage(double scale, QRgb pixel, double subPixelX, double subPixelY)
{
	int srcWidth = 1;
	int srcHeight = 1;
	int dstWidth = 2;
	int dstHeight = 2;

	QImage outputImage(dstWidth, dstHeight, 32);
	outputImage.setAlphaBuffer(true);

	double a = subPixelX;
	double b = subPixelY;

	for (int y = 0; y < dstHeight; y++) {
		for (int x = 0; x < dstWidth; x++) {

			QRgb topLeft = (x > 0 && y > 0) ? pixel : qRgba(0, 0, 0, 0);
			QRgb bottomLeft = (x > 0 && y < srcHeight) ? pixel : qRgba(0, 0, 0, 0);
			QRgb topRight = (x < srcWidth && y > 0) ? pixel : qRgba(0, 0, 0, 0);
			QRgb bottomRight = (x < srcWidth && y < srcHeight) ? pixel : qRgba(0, 0, 0, 0);

			// Bi-linear interpolation. Images are in pre-multiplied form.
			int red = static_cast<int>(a * b * qRed(topLeft)
				+ a * (1 - b) * qRed(bottomLeft)
				+ (1 - a) * b * qRed(topRight)
				+ (1 - a) * (1 - b) * qRed(bottomRight) + 0.5);
			int green = static_cast<int>(a * b * qGreen(topLeft)
				+ a * (1 - b) * qGreen(bottomLeft)
				+ (1 - a) * b * qGreen(topRight)
				+ (1 - a) * (1 - b) * qGreen(bottomRight) + 0.5);
			int blue = static_cast<int>(a * b * qBlue(topLeft)
				+ a * (1 - b) * qBlue(bottomLeft)
				+ (1 - a) * b * qBlue(topRight)
				+ (1 - a) * (1 - b) * qBlue(bottomRight) + 0.5);
			int alpha = static_cast<int>(a * b * qAlpha(topLeft)
				+ a * (1 - b) * qAlpha(bottomLeft)
				+ (1 - a) * b * qAlpha(topRight)
				+ (1 - a) * (1 - b) * qAlpha(bottomRight) + 0.5);

			// Multiply by the square of the scale because a 0.5x0.5 pixel
			// has 0.25 the value of the 1x1.
			alpha = static_cast<int>(alpha * scale * scale + 0.5);

			// Apply to the colour channels too since we are
			// storing pre-multiplied by alpha.
			red = static_cast<int>(red * scale * scale + 0.5);
			green = static_cast<int>(green * scale * scale + 0.5);
			blue = static_cast<int>(blue * scale * scale + 0.5);

			if (red < 0) {
				red = 0;
			}
			else
			if (red > 255) {
				red = 255;
			}

			if (green < 0) {
				green = 0;
			}
			else
			if (green > 255) {
				green = 255;
			}

			if (blue < 0) {
				blue = 0;
			}
			else
			if (blue > 255) {
				blue = 255;
			}

			if (alpha < 0) {
				alpha = 0;
			}
			else
			if (alpha > 255) {
				alpha = 255;
			}

			outputImage.setPixel(x, y, qRgba(red, green, blue, alpha));
		}
	}

	return outputImage;
}

QImage KisBrush::interpolate(const QImage& image1, const QImage& image2, double t)
{
	Q_ASSERT((image1.width() == image2.width()) && (image1.height() == image2.height()));
	Q_ASSERT(t > -DBL_EPSILON && t < 1 + DBL_EPSILON);

	int width = image1.width();
	int height = image1.height();

	QImage outputImage(width, height, 32);
	outputImage.setAlphaBuffer(true);

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			QRgb image1pixel = image1.pixel(x, y);
			QRgb image2pixel = image2.pixel(x, y);

			// Images are in pre-multiplied alpha format.
			int red = static_cast<int>((1 - t) * qRed(image1pixel) + t * qRed(image2pixel) + 0.5);
			int green = static_cast<int>((1 - t) * qGreen(image1pixel) + t * qGreen(image2pixel) + 0.5);
			int blue = static_cast<int>((1 - t) * qBlue(image1pixel) + t * qBlue(image2pixel) + 0.5);
			int alpha = static_cast<int>((1 - t) * qAlpha(image1pixel) + t * qAlpha(image2pixel) + 0.5);

			if (red < 0) {
				red = 0;
			}
			else
			if (red > 255) {
				red = 255;
			}

			if (green < 0) {
				green = 0;
			}
			else
			if (green > 255) {
				green = 255;
			}

			if (blue < 0) {
				blue = 0;
			}
			else
			if (blue > 255) {
				blue = 255;
			}

			if (alpha < 0) {
				alpha = 0;
			}
			else
			if (alpha > 255) {
				alpha = 255;
			}

			outputImage.setPixel(x, y, qRgba(red, green, blue, alpha));
		}
	}

	return outputImage;
}

KisBrush::ScaledBrush::ScaledBrush()
{
	m_mask = 0;
	m_image = QImage();
	m_scale = 1;
	m_xScale = 1;
	m_yScale = 1;
}

KisBrush::ScaledBrush::ScaledBrush(KisAlphaMaskSP scaledMask, const QImage& scaledImage, double scale, double xScale, double yScale)
{
	m_mask = scaledMask;
	m_image = scaledImage;
	m_scale = scale;
	m_xScale = xScale;
	m_yScale = yScale;

	if (!m_image.isNull()) {
		// Convert image to pre-multiplied by alpha.

		m_image.detach();

		for (int y = 0; y < m_image.height(); y++) {
			for (int x = 0; x < m_image.width(); x++) {

				QRgb pixel = m_image.pixel(x, y);

				int red = qRed(pixel);
				int green = qGreen(pixel);
				int blue = qBlue(pixel);
				int alpha = qAlpha(pixel);

				red = (red * alpha) / 255;
				green = (green * alpha) / 255;
				blue = (blue * alpha) / 255;

				m_image.setPixel(x, y, qRgba(red, green, blue, alpha));
			}
		}
	}
}

void KisBrush::setImage(const QImage& img)
{
	m_img = img;
	m_img.detach();

	setWidth(img.width());
	setHeight(img.height());

	m_scaledBrushes.clear();

	setValid(true);
}

#include "kis_brush.moc"

