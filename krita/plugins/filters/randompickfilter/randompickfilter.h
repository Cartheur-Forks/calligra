/*
 * This file is part of Krita
 *
 * Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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

#ifndef RANDOMPICKFILTER_H
#define RANDOMPICKFILTER_H

#include <kparts/plugin.h>
#include "kis_filter.h"

class KisFilterConfigWidget;

class KritaRandomPickFilter : public KParts::Plugin
{
public:
    KritaRandomPickFilter(QObject *parent, const QStringList &);
    virtual ~KritaRandomPickFilter();
};

class KisFilterRandomPick : public KisFilter
{
    public:
        KisFilterRandomPick();
    public:
        virtual void process(const KisPaintDeviceSP src, const QPoint& srcTopLeft, KisPaintDeviceSP dst, const QPoint& dstTopLeft,  const QSize& size, const KisFilterConfiguration* config);
        virtual ColorSpaceIndependence colorSpaceIndependence() { return FULLY_INDEPENDENT; }
        static inline KoID id() { return KoID("randompick", i18n("Random Pick")); }
        virtual bool supportsPainting() { return true; }
        virtual bool supportsPreview() const { return true; }
        virtual bool supportsIncrementalPainting() const { return false; }
        virtual KisFilterConfiguration* factoryConfiguration(const KisPaintDeviceSP);
    public:
        virtual KisFilterConfigWidget * createConfigurationWidget(QWidget* parent, KisPaintDeviceSP dev);
};

#endif
