/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#ifndef KIS_FILTEROP_H_
#define KIS_FILTEROP_H_

#include "kis_paintop.h"
#include <QString>
#include <klocale.h>

class QPointF;
class KisPainter;
class KisFilterConfiguration;
class KisFilterConfigWidget;
class Ui_FilterOpOptions;
class KoID;

class KisFilterOpFactory  : public KisPaintOpFactory  {

public:
    KisFilterOpFactory() {}
    virtual ~KisFilterOpFactory() {}

    virtual KisPaintOp * createOp(const KisPaintOpSettings *settings, KisPainter * painter, KisImageSP image);
    virtual QString id() const { return "filter"; }
    virtual QString name() const { return i18n("Filter"); }
    virtual QString pixmap() { return "filterop.png"; }
    virtual KisPaintOpSettings *settings(QWidget * parent, const KoInputDevice& inputDevice, KisImageSP image);
};

class KisFilterOpSettings : public QObject, public KisPaintOpSettings {
    Q_OBJECT
    public:
        KisFilterOpSettings(QWidget* parent);
        virtual ~KisFilterOpSettings();
        virtual QWidget *widget() const { return m_optionsWidget; }
        const KisFilterSP filter() const;
        KisFilterConfiguration* filterConfig() const;
        virtual void setLayer( KisLayerSP layer );
        virtual void fromXML(const QDomElement&);
        virtual void toXML(QDomDocument&, QDomElement&) const;
    protected slots:
        void setCurrentFilter(const KoID &);
    private:
        QWidget* m_optionsWidget;
        Ui_FilterOpOptions* m_uiOptions;
        const KisFilterSP m_currentFilter;
        KisPaintDeviceSP m_paintDevice;
        KisFilterConfigWidget* m_currentFilterConfigWidget;
};

class KisFilterOp : public KisPaintOp {

public:

    KisFilterOp(const KisFilterOpSettings* settings, KisPainter * painter);
    virtual ~KisFilterOp();

    void paintAt(const KisPaintInformation& info);
private:
    const KisFilterOpSettings* m_settings;
};


#endif // KIS_FILTEROP_H_
