/*
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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

#include <kis_oasis_load_visitor.h>

#include <QDomElement>
#include <QDomNode>

#include <KoColorSpaceRegistry.h>
#include <KoStore.h>
#include <KoStoreDevice.h>

// Includes from krita/image
#include <kis_adjustment_layer.h>
#include <kis_filter.h>
#include <kis_filter_registry.h>
#include <kis_group_layer.h>
#include <kis_image.h>

#include <kis_paint_layer.h>
#include <kis_png_converter.h>
#include <kis_selection.h>

#include "kis_doc2.h"

void KisOasisLoadVisitor::loadImage(const KoXmlElement& elem)
{
    m_image = new KisImage(m_doc->undoAdapter(), 100, 100, KoColorSpaceRegistry::instance()->colorSpace("RGBA",""), "OpenRaster Image (name)"); // TODO: take into account width and height parameters, and metadata, when width = height = 0 use the new function from boud to get the size of the image after the layers have been loaded

    m_image->lock();
    for (KoXmlNode node = elem.firstChild(); !node.isNull(); node = node.nextSibling()) {
        if (node.isElement() && node.nodeName() == "image:stack") { // it's the root layer !
            KoXmlElement subelem = node.toElement();
            loadGroupLayer(subelem, m_image->rootLayer());
            return;
        }
    }
    m_image->unlock();
    m_image = KisImageSP(0);
}

void KisOasisLoadVisitor::loadLayerInfo(const KoXmlElement& elem, KisLayer* layer)
{
    layer->setName(elem.attribute("name"));
    layer->setX(elem.attribute("x").toInt());
    layer->setY(elem.attribute("y").toInt());
}

void KisOasisLoadVisitor::loadAdjustmentLayer(const KoXmlElement& elem, KisAdjustmentLayerSP aL)
{
    loadLayerInfo(elem, aL.data());
}

void KisOasisLoadVisitor::loadPaintLayer(const KoXmlElement& elem, KisPaintLayerSP pL)
{
    loadLayerInfo(elem, pL.data());

    QString filename = m_layerFilenames[pL.data()];
    dbgFile <<"Loading was unsuccessful";
}

void KisOasisLoadVisitor::loadGroupLayer(const KoXmlElement& elem, KisGroupLayerSP gL)
{
    loadLayerInfo(elem, gL.data());
    for (KoXmlNode node = elem.firstChild(); !node.isNull(); node = node.nextSibling()) {
        if (node.isElement())
        {
            KoXmlElement subelem = node.toElement();
            if(node.nodeName()== "image:stack")
            {
                quint8 opacity = 255;
                if( not subelem.attribute("opacity").isNull())
                {
                    opacity = subelem.attribute("opacity").toInt();
                }
                KisGroupLayerSP layer = new KisGroupLayer(m_image.data(), "", opacity);
                m_image->addNode(layer.data(), gL.data(), gL->childCount() );
                loadGroupLayer(subelem, layer);
            } else if(node.nodeName()== "image:layer")
            {
                QString filename = subelem.attribute("src");
                if( not filename.isNull() )
                {
                    quint8 opacity = 255;
                    if( not subelem.attribute("opacity").isNull())
                    {
                        opacity = subelem.attribute("opacity").toInt();
                    }
//                     KisPaintLayerSP layer = new KisPaintLayer( m_image.data(), "", opacity); // TODO: support of colorspacess
//                     m_layerFilenames[layer.data()] = srcAttr;
                    if (m_oasisStore->open(filename) ) {
                        KoStoreDevice io ( m_oasisStore );
                        if ( !io.open( QIODevice::ReadOnly ) )
                        {
                            dbgFile <<"Couldn't open for reading:" << filename;
                //             return false;
                        }
                        KisPNGConverter pngConv(0, gL->image()->undoAdapter() );
                        pngConv.buildImage( &io );
                        io.close();
                        m_oasisStore->close();
                        KisPaintLayerSP layer = new KisPaintLayer( gL->image() , "", opacity, pngConv.image()->projection());
                        m_image->addNode(layer.data(), gL.data(), gL->childCount() );
                        loadPaintLayer(subelem, layer);
                        dbgFile <<"Loading was successful";
                //         return true;
                    }
                }
            } else if(node.nodeName()== "image:filter")
            {

                QString filterType = subelem.attribute("type");
                QStringList filterTypeSplit = filterType.split(":");
                KisFilterSP f = 0;
                if(filterTypeSplit[0] == "applications" and filterTypeSplit[1] == "krita")
                {
                    f = KisFilterRegistry::instance()->value(filterTypeSplit[2]);
                }
                KisFilterConfiguration * kfc = f->defaultConfiguration(0);
                KisAdjustmentLayerSP layer = new KisAdjustmentLayer( gL->image() , "", kfc, KisSelectionSP(0));
                m_image->addNode(layer.data(), gL.data(), gL->childCount() );
                loadAdjustmentLayer(subelem, layer);

            }
        }
    }

}
