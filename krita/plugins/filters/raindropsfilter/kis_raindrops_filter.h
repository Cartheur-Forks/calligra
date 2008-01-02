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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _KIS_RAINDROPS_FILTER_H_
#define _KIS_RAINDROPS_FILTER_H_

#include "kis_filter.h"
#include "kis_filter_config_widget.h"
#include "kis_paint_device.h"

class KisRainDropsFilterConfiguration : public KisFilterConfiguration
{
public:
    KisRainDropsFilterConfiguration(quint32 dropSize, quint32 number, quint32 fishEyes)
        : KisFilterConfiguration( "raindrops", 1 )
        {
            setProperty("dropsize", dropSize);
            setProperty("number", number);
            setProperty("fishEyes", fishEyes);
        }
public:
    inline quint32 dropSize() { return getInt("dropsize"); }
    inline quint32 number() {return getInt("number"); }
    inline quint32 fishEyes() {return getInt("fishEyes"); }

};

class KisRainDropsFilter : public KisFilter
{
public:
    KisRainDropsFilter();
public:
    using KisFilter::process;
    
    void process(KisFilterConstProcessingInformation src,
                 KisFilterProcessingInformation dst,
                 const QSize& size,
                 const KisFilterConfiguration* config,
                 KoUpdater* progressUpdater
        ) const;
    static inline KoID id() { return KoID("raindrops", i18n("Raindrops")); }
    
    virtual std::list<KisFilterConfiguration*> listOfExamplesConfiguration(KisPaintDeviceSP )
    { std::list<KisFilterConfiguration*> list; list.insert(list.begin(), new KisRainDropsFilterConfiguration( 30, 80, 20)); return list; }

public:
    virtual KisFilterConfigWidget * createConfigurationWidget(QWidget* parent, const KisPaintDeviceSP dev) const;
    virtual KisFilterConfiguration* configuration(QWidget*);
private:
    bool** CreateBoolArray (uint Columns, uint Rows);
    void   FreeBoolArray (bool** lpbArray, uint Columns);
    uchar  LimitValues (int ColorValue);
};

#endif
