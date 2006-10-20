/*
 * This file is part of the KDE project
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

#ifndef _KIS_OILPAINT_FILTER_H_
#define _KIS_OILPAINT_FILTER_H_

#include "kis_filter.h"
#include "kis_filter_config_widget.h"

class KisOilPaintFilterConfiguration : public KisFilterConfiguration
{

public:

    KisOilPaintFilterConfiguration(quint32 brushSize, quint32 smooth)
        : KisFilterConfiguration( "oilpaint", 1 )
        {
            setProperty("brushSize", brushSize);
            setProperty("smooth", smooth);
        };
public:

    inline quint32 brushSize() { return getInt("brushSize"); };
    inline quint32 smooth() {return getInt("smooth"); };

};


class KisOilPaintFilter : public KisFilter
{
public:
    KisOilPaintFilter();
public:
    virtual void process(const KisPaintDeviceSP src, const QPoint& srcTopLeft, KisPaintDeviceSP dst, const QPoint& dstTopLeft, const QSize& size, KisFilterConfiguration* config);
    static inline KoID id() { return KoID("oilpaint", i18n("Oilpaint")); };
    virtual bool supportsPainting() { return true; }
    virtual bool supportsPreview() { return true; }
    virtual std::list<KisFilterConfiguration*> listOfExamplesConfiguration(KisPaintDeviceSP dev);
    public:
    virtual KisFilterConfigWidget * createConfigurationWidget(QWidget* parent, KisPaintDeviceSP dev);
    virtual KisFilterConfiguration * configuration(QWidget*);
    virtual KisFilterConfiguration * configuration() { return new KisOilPaintFilterConfiguration( 1, 30); };
private:
    void OilPaint(const KisPaintDeviceSP src, KisPaintDeviceSP dst, const QPoint& srcTopLeft, const QPoint& dstTopLeft, int w, int h, int BrushSize, int Smoothness);
    uint MostFrequentColor(KisPaintDeviceSP, const QRect& bounds, int X, int Y, int Radius, int Intensity);
    // Function to calcule the color intensity and return the luminance (Y)
    // component of YIQ color model.
    inline uint GetIntensity(uint Red, uint Green, uint Blue) { return ((uint)(Red * 0.3 + Green * 0.59 + Blue * 0.11)); }
};

#endif
