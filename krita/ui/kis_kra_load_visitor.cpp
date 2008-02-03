/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
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

#include "kis_kra_load_visitor.h"

#include <QRect>

#include <KoColorSpaceRegistry.h>
#include <colorprofiles/KoIccColorProfile.h>
#include <KoStore.h>
#include <KoColorSpace.h>

// kritaimage
#include "kis_types.h"
#include "kis_node_visitor.h"
#include "kis_image.h"
#include "kis_selection.h"
#include "kis_layer.h"
#include "kis_paint_layer.h"
#include "kis_group_layer.h"
#include "kis_adjustment_layer.h"
#include "kis_filter_configuration.h"
#include "kis_datamanager.h"

KisKraLoadVisitor::KisKraLoadVisitor(KisImageSP img, KoStore *store, QMap<KisLayer *, QString> &layerFilenames, QString name) :
    KisNodeVisitor(),
    m_layerFilenames(layerFilenames)
{
    m_external = false;
    m_img = img;
    m_store = store;
    m_name = name;
}

void KisKraLoadVisitor::setExternalUri(const QString &uri)
{
    m_external = true;
    m_uri = uri;
}

bool KisKraLoadVisitor::visit( KisExternalLayer * )
{
    return true;
}

bool KisKraLoadVisitor::visit(KisPaintLayer *layer)
{        //connect(*layer->paintDevice(), SIGNAL(ioProgress(qint8)), m_img, SLOT(slotIOProgress(qint8)));

    QString location = m_external ? QString::null : m_uri;
    location += m_name + "/layers/" + m_layerFilenames[layer];
    
    // Layer data
    if (m_store->open(location)) {
        if (!layer->paintDevice()->read(m_store)) {
            layer->paintDevice()->disconnect();
            m_store->close();
            //IODone();
            return false;
        }

        m_store->close();
    }
    else {
        kError() << "No image data: that's an error!";
        return false;
    }

    // icc profile
    location = m_external ? QString::null : m_uri;
    location += m_name + "/layers/" + m_layerFilenames[layer] + ".icc";

    if (m_store->hasFile(location)) {
        QByteArray data;
        m_store->open(location);
        data = m_store->read(m_store->size());
        m_store->close();
        // Create a colorspace with the embedded profile
        const KoColorSpace * cs = KoColorSpaceRegistry::instance()->colorSpace(layer->paintDevice()->colorSpace()->id(), new KoIccColorProfile(data));
        // replace the old colorspace
        layer->paintDevice()->setDataManager(layer->paintDevice()->dataManager(), cs);

    }

//     // mask
//     if (layer->hasMask()) { // We set this in KisDoc::loadPaintLayer
//         KisPaintDeviceSP mask = layer->getMask();
//         location = m_external ? QString::null : m_uri;
//         location += m_img->name() + "/layers/" + m_layerFilenames[layer] + ".mask";

//         // Layer data
//         if (m_store->open(location)) {
//             if (!mask->read(m_store)) {
//                 mask->disconnect();
//                 m_store->close();
//                 //IODone();
//                 return false;
//             }

//             m_store->close();
//         }
//         layer->setDirty(); // Update the entire layer
//     }
    layer->setDirty(m_img->bounds());
    return true;

}

bool KisKraLoadVisitor::visit(KisGroupLayer *layer)
{
    KisKraLoadVisitor visitor(m_img, m_store, m_layerFilenames, m_name);

    if(m_external)
        visitor.setExternalUri(m_uri);

    visitAll( layer );

    layer->setDirty(m_img->bounds());
    return true;
}

bool KisKraLoadVisitor::visit(KisAdjustmentLayer* layer)
{
    //connect(*layer->paintDevice(), SIGNAL(ioProgress(qint8)), m_img, SLOT(slotIOProgress(qint8)));

    // The selection -- if present. If not, we simply cannot open the dratted thing.
    QString location = m_external ? QString::null : m_uri;
    location += m_name + "/layers/" + m_layerFilenames[layer] + ".selection";
    if (m_store->hasFile(location)) {
        m_store->open(location);
        KisSelectionSP selection = KisSelectionSP(new KisSelection());
        if (!selection->read(m_store)) {
            selection->disconnect();
            m_store->close();
        }
        else {
            layer->setSelection( selection );
        }
        m_store->close();

    }

    // filter configuration
    location = m_external ? QString::null : m_uri;
    location += m_name + "/layers/" + m_layerFilenames[layer] + ".filterconfig";

    if (m_store->hasFile(location) && layer->filter()) {
        QByteArray data;
        m_store->open(location);
        data = m_store->read(m_store->size());
        m_store->close();
        if (!data.isEmpty()) {
            KisFilterConfiguration * kfc = layer->filter();
            kfc->fromLegacyXML(QString(data));
        }
    }

    layer->setDirty( m_img->bounds() );
    return true;


}

