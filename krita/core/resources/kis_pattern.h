/*
 *  kis_pattern.h - part of Krayon
 *
 *  Copyright (c) 2000 Matthias Elter  <elter@kde.org>
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

#ifndef __kis_pattern_h__
#define __kis_pattern_h__

#include <kio/job.h>

#include "kis_resource.h"
#include "kis_alpha_mask.h"
#include "kis_global.h"
#include "kis_layer.h"
#include "kis_point.h"

class QPoint;
class QImage;

class KisPattern : public KisResource {
	typedef KisResource super;
	Q_OBJECT

public:
	KisPattern(const QString& file);
	virtual ~KisPattern();

	virtual bool load();
	virtual bool save();
	virtual QImage img();

	/**
	 * returns a KisLayerSP made with colorSpace as the Colorspace strategy
	 * for use in the fill painter.
	 **/
	KisLayerSP image(KisStrategyColorSpace * colorSpace);

	Q_INT32 width() const;
	Q_INT32 height() const;

protected:
	void setWidth(Q_INT32 w);
	void setHeight(Q_INT32 h);

private:
	bool init();

private:
	QByteArray m_data;
	QImage m_img;
	QMap<QString, KisLayerSP> m_colorspaces;

	Q_INT32 m_width;
	Q_INT32 m_height;
};

#endif

