/*
 * This file is part of Krita
 *
 * Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_PERCHANNEL_FILTER_H_
#define _KIS_PERCHANNEL_FILTER_H_

#include "kis_types.h"
#include "kis_filter.h"
#include "kis_strategy_colorspace.h"
#include "kis_multi_integer_filter_widget.h"
#include "kis_multi_double_filter_widget.h"

#include <kdebug.h>

template <typename Type>
class KisPerChannelFilterConfiguration
	: public KisFilterConfiguration
{
public:
	KisPerChannelFilterConfiguration(Q_INT32 nbchannels, vKisChannelInfoSP ci);

public:

	// This function return the value at index i
	inline Type& valueFor(Q_INT32 i) { return m_values[i]; };

	// This function return the channel number at index i
	inline Q_INT32 channel(Q_INT32 i) { return m_channels[i]; };

private:
	Type* m_values;
	Q_INT32* m_channels;
};


/**
 * This class is generic for filters that affect channel separately 
 */
template <typename Type, class ParamType, class WidgetClass>
class KisPerChannelFilter
	: public KisFilter
{
public:
	KisPerChannelFilter(const KisID& id, const QString & category, const QString & name, Type min, Type max, Type initvalue );
public:
	virtual KisFilterConfigWidget * createConfigurationWidget(QWidget* parent, KisPaintDeviceSP dev);
	virtual KisFilterConfiguration* configuration(QWidget*, KisPaintDeviceSP dev);
private:
	Type m_min;
	Type m_max;
	Type m_initvalue;
	Q_INT32 m_nbchannels;
};

typedef KisPerChannelFilterConfiguration<Q_INT32> KisIntegerPerChannelFilterConfiguration;
typedef KisPerChannelFilter<Q_INT32, KisIntegerWidgetParam, KisMultiIntegerFilterWidget> KisIntegerPerChannelFilter;

typedef KisPerChannelFilterConfiguration<double> KisDoublePerChannelFilterConfiguration;
typedef KisPerChannelFilter<double, KisDoubleWidgetParam, KisMultiDoubleFilterWidget> KisDoublePerChannelFilter;



// Implementation of the templatized functions

template <typename Type>
KisPerChannelFilterConfiguration<Type>::KisPerChannelFilterConfiguration(Q_INT32 nbintegers, vKisChannelInfoSP ci)
{
	m_values = new Type[ nbintegers ];
	Q_CHECK_PTR(m_values);

	m_channels = new Q_INT32[ nbintegers ];
	Q_CHECK_PTR(m_channels);


	for( Q_INT32 i = 0; i < nbintegers; i++ )
	{
		m_channels[ i ] = ci[ i ] -> pos();
		m_values[ i ] = Type(0);
	}
}

template <typename Type, class ParamType, class WidgetClass>
KisPerChannelFilter<Type, ParamType, WidgetClass>::KisPerChannelFilter(const KisID& id, const QString & category, const QString & name, Type min, Type max, Type initvalue )
	: KisFilter( id, category, name),
	  m_min (min),
	  m_max (max),
	  m_initvalue (initvalue),
	  m_nbchannels ( 0 )
{
}

template <typename Type, class ParamType, class WidgetClass>
KisFilterConfigWidget * KisPerChannelFilter<Type, ParamType, WidgetClass>::createConfigurationWidget(QWidget* parent, KisPaintDeviceSP dev)
{
	std::vector<ParamType> param;

	m_nbchannels = dev->colorStrategy()->nColorChannels();

	for(Q_INT32 i = 0; i < m_nbchannels; i++)
	{
		KisChannelInfoSP cI = dev->colorStrategy() -> channels()[i];
		param.push_back( ParamType( m_min, m_max, m_initvalue, cI->name() ) );
	}

	WidgetClass * w = new WidgetClass(parent, id().id().ascii(), id().id().ascii(), param );
	Q_CHECK_PTR(w);

	return w;
}

template <typename Type, class ParamType, class WidgetClass>
KisFilterConfiguration* KisPerChannelFilter<Type, ParamType, WidgetClass>::configuration(QWidget* nwidget, KisPaintDeviceSP dev)
{
	WidgetClass* widget = (WidgetClass*)nwidget;
	KisPerChannelFilterConfiguration<Type>* co = new KisPerChannelFilterConfiguration<Type>( m_nbchannels , dev->colorStrategy()->channels() );
	Q_CHECK_PTR(co);

	if( widget == 0 )
	{
		for(Q_INT32 i = 0; i < m_nbchannels; i++)
		{
			co->valueFor( i ) = 0;
		}
	} else {
		for(Q_INT32 i = 0; i < m_nbchannels; i++)
		{
			co->valueFor( i ) = widget->valueAt( i );
		}
	}
	return co;
}

#endif
