/*
 * This file is part of Krita
 *
 * Copyright (c) Michael Thaler <michael.thaler@physik.tu-muenchen.de>
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

#ifndef _KIS_RAINDROPS_FILTER_H_
#define _KIS_RAINDROPS_FILTER_H_

#include "kis_filter.h"
#include "kis_view.h"
#include <kdebug.h>

class KisRainDropsFilterConfiguration : public KisFilterConfiguration
{
public:
	KisRainDropsFilterConfiguration(Q_UINT32 dropSize, Q_UINT32 number, Q_UINT32 fishEyes) : m_dropSize(dropSize), m_number(number), m_fishEyes(fishEyes) {};
public:
	inline Q_UINT32 dropSize() { return m_dropSize; };
	inline Q_UINT32 number() {return m_number; };
	inline Q_UINT32 fishEyes() {return m_fishEyes; };
private:
	Q_UINT32 m_dropSize;
	Q_UINT32 m_number;
	Q_UINT32 m_fishEyes;
};

class KisRainDropsFilter : public KisFilter
{
public:
	KisRainDropsFilter();
public:
	virtual void process(KisPaintDeviceSP,KisPaintDeviceSP, KisFilterConfiguration* , const QRect&);
	static inline KisID id() { return KisID("raindrops", i18n("Raindrops")); };
	virtual bool supportsPainting() { return false; }
public:
	virtual KisFilterConfigWidget * createConfigurationWidget(QWidget* parent, KisPaintDeviceSP dev);
	virtual KisFilterConfiguration* configuration(QWidget*, KisPaintDeviceSP dev);
private:
	void   rainDrops(KisPaintDeviceSP src, const QRect& rect, int DropSize, int Amount, int Coeff);
	bool** CreateBoolArray (uint Columns, uint Rows);
	void   FreeBoolArray (bool** lpbArray, uint Columns);
	uchar  LimitValues (int ColorValue);
};

#endif
