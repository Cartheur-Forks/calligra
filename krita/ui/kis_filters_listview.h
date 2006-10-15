/*
 * This file is part of Krita
 *
 * Copyright (c) 2005 Cyrille Berger <cberger@cberger.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef _KIS_FILTERS_LIST_VIEW_H_
#define _KIS_FILTERS_LIST_VIEW_H_

#include <QEvent>
#include <QPixmap>

#include <k3iconview.h>
#include <krita_export.h>
#include "KoID.h"
#include "kis_types.h"
#include "kis_layer.h"
#include "kis_paint_device.h"
#include "kis_thread.h"

class KisView;
class KisFilter;
class KisFilterConfiguration;
class KisPreviewView;
class QTimer;
class KisFiltersIconViewItem;
class KisFiltersListView;
class KisThreadPool;

class KisThumbnailDoneEvent : public QEvent
{
public:

    KisThumbnailDoneEvent(KisFiltersIconViewItem * iconItem, const QImage & img)
        : QEvent(static_cast<QEvent::Type>(QEvent::User + 1969))
        , m_iconItem(iconItem)
        , m_image(img) {}

    KisFiltersIconViewItem * m_iconItem;
    QImage m_image;
};


class KisFiltersThumbnailThread : public KisThread
{
public:

    KisFiltersThumbnailThread(Q3IconView * parent,
                              KisFiltersIconViewItem * iconItem,
                              KisFilterConfiguration * config, KisFilter * filter,
                              KisPaintDeviceSP dev, const QRect & bounds,
                              KoColorProfile * profile);

    ~KisFiltersThumbnailThread();

    virtual void run();
    QPixmap pixmap();
    void cancel();

private:
    Q3IconView * m_parent;
    KisFiltersIconViewItem * m_iconItem;
    KisFilterConfiguration * m_config;
    KisFilter * m_filter;
    KisPaintDeviceSP m_dev;
    const QRect m_bounds;
    KoColorProfile * m_profile;
    QImage m_image;
};

class KisFiltersIconViewItem : public Q3IconViewItem {
public:
    KisFiltersIconViewItem( Q3IconView * parent, const QString & text, const QPixmap & icon,
                            KoID id, KisFilter* filter, KisFilterConfiguration* filterConfig,
                            KisPaintDeviceSP thumb, const QRect & bounds, KoColorProfile * profile);

    virtual ~KisFiltersIconViewItem();
    KoID id() { return m_id; }
    KisFilter* filter() { return m_filter; }
    void setFilterConfiguration(KisFilterConfiguration* fc) { m_filterconfig = fc; }

    void resetThread() { m_thread = 0; };
    KisThread * thread() { return m_thread; }

private:
    KoID m_id;
    KisFilter* m_filter;
    KisFilterConfiguration* m_filterconfig;
    KisFiltersThumbnailThread * m_thread;
};

class KRITAUI_EXPORT KisFiltersListView : public K3IconView {

public:
    explicit KisFiltersListView(QWidget* parent = 0, Qt::WFlags f = 0, bool filterForAdjustmentLayers = false);
    KisFiltersListView(QWidget* parent, bool filterForAdjustmentLayers = false, const char* name = 0);
    KisFiltersListView(KisLayerSP layer, QWidget* parent, bool filterForAdjustmentLayers = false, const char * name = 0) KDE_DEPRECATED;
    KisFiltersListView(KisPaintDeviceSP layer, QWidget* parent, bool filterForAdjustmentLayers = false, const char * name = 0);

    virtual void customEvent(QCustomEvent *);

    private:

    void init();

public:
    void setLayer(KisLayerSP layer) KDE_DEPRECATED;
    void setProfile(KoColorProfile * profile) { m_profile = profile; };

    inline void setPaintDevice(KisPaintDeviceSP pd) {
        if( pd != m_original)
        {
            m_original = pd;
            buildPreview();
        }
    }
    void buildPreview();
    void setCurrentFilter(KoID filter);

private:

    KisPaintDeviceSP m_original;
    KisImageSP m_imgthumb;
    KisPaintDeviceSP m_thumb;
    KoColorProfile * m_profile;
    KisThreadPool * threadPool;
    bool m_filterForAdjustmentLayers;

};

#endif
