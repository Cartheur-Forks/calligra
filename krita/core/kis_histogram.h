/*
 *  Copyright (c) 2004 Boudewijn Rempt
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
#ifndef KIS_HISTOGRAM_
#define KIS_HISTOGRAM_

#include <qvaluevector.h>

#include "kis_types.h"
#include "color_strategy/kis_abstract_colorspace.h"
/**
 * The histogram class computes the histogram data from the specified layer
 * for the specified channel.
 *
 * XXX: also from complete image? In which case, how can we handle the 
 * the situation where different layers have a different color model?
 */
enum enumHistogramType {
    LINEAR,
    LOGARITHMIC
};

typedef QValueVector<Q_UINT32> vBins;

class KisHistogram : public KShared {

public:
    KisHistogram(KisLayerSP layer, 
             const KisChannelInfo & initialChannel, 
             const enumHistogramType type);

    virtual ~KisHistogram();

    void computeHistogramFor(const KisChannelInfo & channel);

    Q_UINT32 getValue(Q_UINT8 i) { return m_values[i]; }
    QUANTUM getMax() { return m_max; }
    QUANTUM getMin() { return m_min; }
    Q_UINT32 getHighest() { return m_high; }
    Q_UINT32 getLowest() { return m_low; }
    double getMean() { return m_mean; }
    double getMedian() { return m_median; }
    double getStandardDeviation() { return m_stddev; }
    Q_UINT32 getPixels () { return m_pixels; }
    Q_UINT32 getCount() { return m_count; }
    Q_UINT8 getPercentile() { return m_percentile; }
    enumHistogramType getHistogramType() { return m_type; }
    void setHistogramType(enumHistogramType type) { m_type = type; }


private:
    // Dump the histogram to debug.
    void dump(); 

    KisLayerSP m_layer;

    enumHistogramType m_type;
    
    Q_UINT32 m_values[256];
    Q_UINT8 m_max, m_min;
    Q_UINT32 m_high, m_low;
    double m_mean, m_median, m_stddev;
    Q_UINT32 m_pixels, m_count;
    Q_UINT8 m_percentile;

};


#endif // KIS_HISTOGRAM_WIDGET_
