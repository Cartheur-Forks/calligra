/*
 *  Copyright (c) 2004-2006 Cyrille Berger <cberger@cberger.net>
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
#ifndef _KIS_FILTER_CONFIG_WIDGET_H_
#define _KIS_FILTER_CONFIG_WIDGET_H_

#include <QWidget>
#include "kis_filter_configuration.h"
#include <krita_export.h>

/**
 * Empty base class. Filters can build their own configuration widgets that
 * inherit this class. The configuration widget can emit sigPleaseUpdatePreview
 * when it wants the preview in the filter dialog to be updated.
 */
class KRITAIMAGE_EXPORT KisFilterConfigWidget : public QWidget {

    Q_OBJECT

protected:

    KisFilterConfigWidget(QWidget * parent, Qt::WFlags f = 0 );

public:
    virtual ~KisFilterConfigWidget();

    /**
     * @param config the configuration for this filter widget.
     */
    virtual void setConfiguration(KisFilterConfiguration * config) = 0;

    /**
     * @return the filter configuration
     */
    virtual KisFilterConfiguration* configuration() const = 0;

signals:

    /**
     * Subclasses should emit this signal whenever the preview should be
     * be recalculated.
     */
    void sigPleaseUpdatePreview();
};


#endif
