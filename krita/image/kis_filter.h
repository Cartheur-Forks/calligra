/*
 *  Copyright (c) 2004,2006-2007 Cyrille Berger <cberger@cberger.net>
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
#ifndef _KIS_FILTER_H_
#define _KIS_FILTER_H_

#include <list>

#include <QString>

#include <kis_shared.h>
#include <klocale.h>

#include <KoColorSpace.h>

#include "kis_types.h"
#include "KoID.h"
#include "krita_export.h"

#include "kis_filter_processing_information.h"

class KoColorSpace;
class KisBookmarkedConfigurationManager;
class KoUpdater;
class KisFilterConfigWidget;
class KisFilterConfiguration;
class QWidget;
class KoUpdater;

/**
 * Basic interface of a Krita filter.
 */
class KRITAIMAGE_EXPORT KisFilter : public KisShared {

public:
    static const KoID CategoryAdjust;
    static const KoID CategoryArtistic;
    static const KoID CategoryBlur;
    static const KoID CategoryColors;
    static const KoID CategoryEdgeDetection;
    static const KoID CategoryEmboss;
    static const KoID CategoryEnhance;
    static const KoID CategoryMap;
    static const KoID CategoryNonPhotorealistic;
    static const KoID CategoryOther;
public:

    /**
     * Construct a Krita filter
     */
    KisFilter(const KoID& id, const KoID & category, const QString & entry);
    virtual ~KisFilter() {}

public:

    /**
     * Override this function with the implementation of your filter.
     *
     * XXX: May the filter may assume that src and dst have the same
     * colorspace? (bsar)
     *
     * @param src the source paint device
     * @param srcTopLeft the top left coordinate where the filter starts to be applied
     * @param dst the destination paint device
     * @param dstTopLeft he top left coordinate for the destination paint device
     * @param size the size of the area that is filtered
     * @param channelFlags an array of bits that indicated which
     *        channels must be excluded and which channels must be included
     *        when filtering. Is _empty_ when all channels need to be filtered.
     * @param config the parameters of the filter
     */
    virtual void process(KisFilterConstantProcessingInformation src,
                         KisFilterProcessingInformation dst,
                         const QSize& size,
                         const KisFilterConfiguration* config,
                         KoUpdater* progressUpdater
        ) const = 0;

    /**
     * Provided for convenience when no progress reporting is needed.
     */
    virtual void process(KisFilterConstantProcessingInformation src,
                         KisFilterProcessingInformation dst,
                         const QSize& size,
                         const KisFilterConfiguration* config
        ) const;


    /**
     * Provided for convenience only when source and destination are the same
     */
    void process(KisPaintDeviceSP device, const QRect& rect, const KisFilterConfiguration* config,
                 KoUpdater* progressUpdater) const;

    /**
     * Provided for convenience only when source and destination are the same and no progress reporting
     * is necessary.
     */
    void process(KisPaintDeviceSP device, const QRect& rect, const KisFilterConfiguration* config) const;
    
public:
    /**
     * This function return the configuration set as the default by the user or the default configuration from
     * the filter writer as returned by factoryConfiguration.
     * This configuration is used by default for the configuration widget and to the process function if there is no
     * configuration widget.
     * @return the default configuration of this widget
     */
    virtual KisFilterConfiguration * defaultConfiguration(const KisPaintDeviceSP) const;


    /**
     * @return the bookmark manager for this filter
     */
    KisBookmarkedConfigurationManager* bookmarkManager();

    /**
     * @return the bookmark manager for this filter
     */
    const KisBookmarkedConfigurationManager* bookmarkManager() const;

    /**
     * Used when threading is used -- the overlap margin is passed to the
     * filter to use to compute pixels, but the margin is not pasted into the
     * resulting image. Use this for convolution filters, for instance.
     */
    virtual int overlapMarginNeeded( const KisFilterConfiguration* = 0 ) const { return 0; }

     /**
     * Similar to overlapMarginNeeded: some filters will alter a lot of pixels that are
     * near to each other at the same time. So when you changed a single rectangle
     * in a device, the actual rectangle that will feel the influence of this change
     * might be bigger. Use this function to determine that rect.
     * The default implementation makes a guess using overlapMarginNeeded.
      */
    virtual QRect enlargeRect(QRect rect, const KisFilterConfiguration* = 0) const;

    /**
     * Determine if this filter can work with this colorSpace. For instance, some
     * colorspaces don't depend on lcms, and cannot do certain tasks. The colorsfilters
     * are problems here.
     * BSAR: I'm still not convinced that this is the right approach. I think that every
     * colorspace should implement the api fully; and that the filter should simply call
     * that api. After all, you don't need lcms to desaturate.
     *
     * @param cs the colorspace that we want to know this filter works with
     */
    virtual bool workWith(const KoColorSpace* cs) const { Q_UNUSED(cs); return true; }

    /// @return Unique identification for this filter
    QString id() const;
    QString name() const;

    /// @return the submenu in the filters menu does filter want to go?
    KoID menuCategory() const;

    /// @return the i18n'ed string this filter wants to show itself in the menu
    QString menuEntry() const;

    /**
     * Create the configuration widget for this filter.
     *
     * @param parent the Qt owner widget of this widget
     * @param dev the paintdevice this filter will act on
     */
    virtual KisFilterConfigWidget * createConfigurationWidget(QWidget * parent, const KisPaintDeviceSP dev);
    // "Support" functions
public:
    /**
     * If true, this filter can be used in painting tools as a paint operation
     */
    bool supportsPainting() const;

    /// This filter can be displayed in a preview dialog
    bool supportsPreview() const;

    /// This filter can be used in adjustment layers
    bool supportsAdjustmentLayers() const;

    /**
     * Can this filter work incrementally when painting, or do we need to work
     * on the state as it was before painting started. The former is faster.
     */
    bool supportsIncrementalPainting() const;

    /**
     * This filter supports cutting up the work area and filtering
     * each chunk in a separate thread. Filters that need access to the
     * whole area for correct computations should return false.
     */
    bool supportsThreading() const;
    
    /**
     * Determine the colorspace independence of this filter.
     * @see ColorSpaceIndependence
     *
     * @return the degree of independence
     */
    ColorSpaceIndependence colorSpaceIndependence() const;
protected:
    void setSupportsPainting(bool v);
    void setSupportsPreview(bool v);
    void setSupportsAdjustmentLayers(bool v);
    void setSupportsIncrementalPainting(bool v);
    void setSupportsThreading(bool v);
    void setColorSpaceIndependence(ColorSpaceIndependence v);
    
protected:

    
    
    void setBookmarkManager(KisBookmarkedConfigurationManager* );
    /// @return the name of config group in KConfig
    QString configEntryGroup() const;

    /// @return the default configuration as defined by whoever wrote the plugin
    virtual KisFilterConfiguration* factoryConfiguration(const KisPaintDeviceSP) const;

private:
    struct Private;
    Private* const d;
};


#endif
