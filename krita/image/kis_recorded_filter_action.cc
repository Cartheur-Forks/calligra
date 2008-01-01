/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
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

#include "kis_recorded_filter_action.h"

#include <QDomElement>
#include <QString>


#include "kis_filter.h"
#include "kis_filter_configuration.h"
#include "kis_filter_registry.h"
#include "kis_layer.h"
#include "kis_selection.h"
#include "kis_transaction.h"
#include "kis_undo_adapter.h"

struct KisRecordedFilterAction::Private {
    KisLayerSP layer;
    const KisFilter* filter;
    QString config;
    QRect rect;
};

KisRecordedFilterAction::KisRecordedFilterAction(QString name, KisLayerSP layer, const KisFilter* filter, KisFilterConfiguration* fc) : KisRecordedAction(name, "FilterAction"), d(new Private)
{
    d->layer = layer;
    d->filter = filter;
    if(fc)
    {
        d->config = fc->toXML();
    }
}

KisRecordedFilterAction::KisRecordedFilterAction(const KisRecordedFilterAction& rhs) : KisRecordedAction(rhs), d(new Private(*rhs.d))
{
}

KisRecordedFilterAction::~KisRecordedFilterAction()
{
}

void KisRecordedFilterAction::play(KisUndoAdapter* adapter) const
{
    
    KisFilterConfiguration * kfc = d->filter->defaultConfiguration(0);
    if(kfc)
    {
        kfc->fromXML(d->config);
    }
    KisPaintDeviceSP dev = d->layer->paintDevice();
    KisTransaction * cmd = 0;
    if (adapter) cmd = new KisTransaction(d->filter->name(), dev);

    QRect r1 = dev->extent();
    QRect r2 = d->layer->image()->bounds();

    // Filters should work only on the visible part of an image.
    QRect rect = r1.intersect(r2);

    if (KisSelectionSP selection = d->layer->selection()) {
        QRect r3 = selection->selectedExactRect();
        rect = rect.intersect(r3);
    }

    d->filter->process( dev, rect, kfc);
    dev->setDirty( rect );
    if (adapter) adapter->addCommand( cmd );
}

void KisRecordedFilterAction::toXML(QDomDocument& doc, QDomElement& elt) const
{
    KisRecordedAction::toXML(doc,elt);
    elt.setAttribute("layer", KisRecordedAction::layerToIndexPath(d->layer));
    elt.setAttribute("filter", d->filter->id());
    // Save configuration
    KisFilterConfiguration * kfc = d->filter->defaultConfiguration(d->layer->paintDevice());
    if(kfc)
    {
        kfc->fromXML( d->config );
        QDomElement filterConfigElt = doc.createElement( "Params");
        kfc->toXML(doc, filterConfigElt);
        elt.appendChild(filterConfigElt);
    }
}

KisRecordedAction* KisRecordedFilterAction::clone() const
{
    return new KisRecordedFilterAction(*this);
}

KisRecordedFilterActionFactory::KisRecordedFilterActionFactory() :
        KisRecordedActionFactory("FilterAction")
{
}

KisRecordedFilterActionFactory::~KisRecordedFilterActionFactory()
{

}

KisRecordedAction* KisRecordedFilterActionFactory::fromXML(KisImageSP img, const QDomElement& elt)
{
    QString name = elt.attribute("name");
    KisLayerSP layer = KisRecordedActionFactory::indexPathToLayer(img, elt.attribute("layer"));
    const KisFilterSP filter = KisFilterRegistry::instance()->get(elt.attribute("filter"));
    if(filter)
    {
        KisFilterConfiguration* config = filter->defaultConfiguration(layer->paintDevice());
        QDomElement paramsElt = elt.firstChildElement("Params");
        if(config and not paramsElt.isNull())
        {
            config->fromXML(paramsElt);
        }
        KisRecordedFilterAction* rfa = new KisRecordedFilterAction(name, layer, filter, config);
        delete config;
        return rfa;
    } else {
        return 0;
    }
}
