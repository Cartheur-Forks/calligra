/*
 *  kis_pattern.cc - part of Krayon
 *
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 *                2001 John Califf
 *                2004 Boudewijn Rempt <boud@valdyas.org>
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

#include <netinet/in.h>

#include <limits.h>
#include <stdlib.h>

#include <qpoint.h>
#include <qsize.h>
#include <qimage.h>
#include <qvaluevector.h>
#include <qmap.h>

#include <kdebug.h>

#include "kis_pattern.h"
#include "kis_layer.h"
#include "kis_colorspace_registry.h"

namespace {
	struct GimpPatternHeader {
		Q_UINT32 header_size;  /*  header_size = sizeof (PatternHeader) + brush name  */
		Q_UINT32 version;      /*  pattern file version #  */
		Q_UINT32 width;        /*  width of pattern */
		Q_UINT32 height;       /*  height of pattern  */
		Q_UINT32 bytes;        /*  depth of pattern in bytes : 1, 2, 3 or 4*/
		Q_UINT32 magic_number; /*  GIMP brush magic number  */
	};
}

KisPattern::KisPattern(const QString& file) : super(file)
{
}

KisPattern::~KisPattern()
{
}

bool KisPattern::loadAsync()
{
	KIO::Job *job = KIO::get(filename(), false, false);

	connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)), this, SLOT(ioData(KIO::Job*, const QByteArray&)));
	connect(job, SIGNAL(result(KIO::Job*)), SLOT(ioResult(KIO::Job*)));
	return true;
}

bool KisPattern::saveAsync()
{
	return false;
}

QImage KisPattern::img()
{
	return m_img;
}

void KisPattern::ioData(KIO::Job * /*job*/, const QByteArray& data)
{
	if (!data.isEmpty()) {
		Q_INT32 startPos = m_data.size();

		m_data.resize(m_data.size() + data.count());
		memcpy(&m_data[startPos], data.data(), data.count());
	}
}

void KisPattern::ioResult(KIO::Job * /*job*/)
{
	// load Gimp patterns
	GimpPatternHeader bh;
	Q_INT32 k;
	QValueVector<char> name;

	if (sizeof(GimpPatternHeader) > m_data.size()) {
		emit ioFailed(this);
		return;
	}

	memcpy(&bh, &m_data[0], sizeof(GimpPatternHeader));
	bh.header_size = ntohl(bh.header_size);
	bh.version = ntohl(bh.version);
	bh.width = ntohl(bh.width);
	bh.height = ntohl(bh.height);
	bh.bytes = ntohl(bh.bytes);
	bh.magic_number = ntohl(bh.magic_number);

	if (bh.header_size > m_data.size() || bh.header_size == 0) {
		emit ioFailed(this);
		return;
	}

	name.resize(bh.header_size - sizeof(GimpPatternHeader));
	memcpy(&name[0], &m_data[sizeof(GimpPatternHeader)], name.size());

	if (name[name.size() - 1]) {
		emit ioFailed(this);
		return;
	}

	setName(&name[0]);

	if (bh.width == 0 || bh.height == 0 || !m_img.create(bh.width, bh.height, 32)) {
		emit ioFailed(this);
		return;
	}

	k = bh.header_size;

	if (bh.bytes == 1) {
// 		kdDebug() << "Loading grayscale pattern " << &name[0] << endl;
		// Grayscale
		Q_INT32 val;

		for (Q_UINT32 y = 0; y < bh.height; y++) {
			for (Q_UINT32 x = 0; x < bh.width; x++, k++) {
				if (static_cast<Q_UINT32>(k) > m_data.size()) {
					kdDebug() << "failed in gray\n";
					emit ioFailed(this);
					return;
				}

				val = m_data[k];
				m_img.setPixel(x, y, qRgb(val, val, val));
				m_img.setAlphaBuffer(false);
			}
		}
	} else if (bh.bytes == 2) {
// 		kdDebug() << "Loading grayscale + alpha pattern " << &name[0] << endl;
		// Grayscale + A
		Q_INT32 val;
		Q_INT32 alpha;
		for (Q_UINT32 y = 0; y < bh.height; y++) {
			for (Q_UINT32 x = 0; x < bh.width; x++, k++) {
				if (static_cast<Q_UINT32>(k + 2) > m_data.size()) {
					kdDebug() << "failed in grayA\n";
					emit ioFailed(this);
					return;
				}

				val = m_data[k];
				alpha = m_data[k++];
				m_img.setPixel(x, y, qRgba(val, val, val, alpha));
				m_img.setAlphaBuffer(true);
			}
		}
	} else if (bh.bytes == 3) {
// 		kdDebug() << "Loading rgb pattern " << &name[0] << endl;
		// RGB without alpha
		for (Q_UINT32 y = 0; y < bh.height; y++) {
			for (Q_UINT32 x = 0; x < bh.width; x++) {
				if (static_cast<Q_UINT32>(k + 3) > m_data.size()) {
					kdDebug() << "failed in RGB\n";
					emit ioFailed(this);
					return;
				}

				m_img.setPixel(x, y, qRgb(m_data[k],
							  m_data[k + 1],
							  m_data[k + 2]));
				k += 3;
				m_img.setAlphaBuffer(false);
			}
		}
	} else if (bh.bytes == 4) {
// 		kdDebug() << "Loading rgba pattern " << &name[0] << endl;
		// Has alpha
		for (Q_UINT32 y = 0; y < bh.height; y++) {
			for (Q_UINT32 x = 0; x < bh.width; x++) {
				if (static_cast<Q_UINT32>(k + 4) > m_data.size()) {
					kdDebug() << "failed in RGBA\n";
					emit ioFailed(this);
					return;
				}

				m_img.setPixel(x, y, qRgba(m_data[k],
							   m_data[k + 1],
							   m_data[k + 2],
							   m_data[k + 3]));
				k += 4;
				m_img.setAlphaBuffer(true);
			}
		}
	} else {
		emit ioFailed(this);
		return;
	}

	if (m_img.isNull()) {
		emit ioFailed(this);
		return;
	}

	setWidth(m_img.width());
	setHeight(m_img.height());

	setValid(true);

// 	kdDebug() << "pattern loaded\n";
	emit loadComplete(this);
}

KisLayerSP KisPattern::image(KisStrategyColorSpaceSP colorSpace) {
	QMap<QString, KisLayerSP>::const_iterator it = m_colorspaces.find(colorSpace->name());
	if (it != m_colorspaces.end())
		return (*it);
	Q_INT32 width = m_img.width();
	Q_INT32 height = m_img.height();
	KisLayerSP layer = new KisLayer(width, height, colorSpace, "pattern image");
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			QRgb pixel = m_img.pixel(x, y);
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
	m_colorspaces[colorSpace->name()] = layer;
	return layer;
}

#include "kis_pattern.moc"
