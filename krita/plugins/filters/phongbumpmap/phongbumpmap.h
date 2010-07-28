/*
 *  Copyright (c) 2010 José Luis Vergara <pentalis@gmail.com>
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

#ifndef PHONGBUMPMAP_H
#define PHONGBUMPMAP_H

#include <QWidget>

#include <QObject>
#include <QVariant>

#include <kis_types.h>
#include <filter/kis_filter.h>
#include "kis_config_widget.h"

#include "ui_wdgphongbumpmap.h"

class KisNodeModel;

class PhongBumpmapWidget : public QWidget, public Ui::WdgPhongBumpmap
{
    Q_OBJECT

public:
    PhongBumpmapWidget(QWidget *parent) : QWidget(parent) {
        setupUi(this);
    }
};

class KritaPhongBumpmap : public QObject
{
public:
    KritaPhongBumpmap(QObject *parent, const QVariantList &);
    virtual ~KritaPhongBumpmap();
};


/**
 * Implementation of the phong illumination model on top of a 
 * heightmap-based mesh to achieve a bumpmapping effect with
 * multiple illumination sources.
 *
 * TODO: actually make it support multiple illumination sources.
 */
class KisFilterPhongBumpmap : public KisFilter
{
public:
    KisFilterPhongBumpmap();
    
public:

    using KisFilter::process;

    void process(KisConstProcessingInformation src,
                 KisProcessingInformation dst,
                 const QSize& size,
                 const KisFilterConfiguration* config,
                 KoUpdater* progressUpdater
                ) const;

    qreal inline pixelProcess (quint8* heightmap, quint32 posup, quint32 posdown, quint32 posleft, quint32 posright);
    
    bool supportsAdjustmentLayers() const {
        return false;
    }

    virtual KisConfigWidget * createConfigurationWidget(QWidget* parent, const KisPaintDeviceSP dev, const KisImageWSP image = 0) const;
    virtual KisFilterConfiguration* factoryConfiguration(const KisPaintDeviceSP) const;
    

};

class KisPhongBumpmapConfigWidget : public KisConfigWidget
{

    Q_OBJECT

public:
    KisPhongBumpmapConfigWidget(const KisPaintDeviceSP dev, const KisImageWSP image, QWidget * parent, Qt::WFlags f = 0);
    virtual ~KisPhongBumpmapConfigWidget() {}

    void setConfiguration(const KisPropertiesConfiguration* config);
    KisPropertiesConfiguration* configuration() const;

    PhongBumpmapWidget * m_page;

private:

    KisPaintDeviceSP m_device;
    KisImageWSP m_image;
    KisNodeModel * m_model;
};

#endif
