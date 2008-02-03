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

#include "filters_gallery.h"

#include <QApplication>

#include <kis_debug.h>
#include <kgenericfactory.h>
#include <kstandarddirs.h>
#include <kactioncollection.h>

#include "KoColorSpaceRegistry.h"

#include "kis_undo_adapter.h"
#include "KoProgressUpdater.h"
#include "kis_dlg_filtersgallery.h"
#include "kis_filter.h"
#include "kis_filters_listview.h"

#include "kis_paint_device.h"
#include "kis_selection.h"
#include "kis_view2.h"
#include "kis_statusbar.h"
#include "kis_transaction.h"
#include <kis_image.h>

namespace Krita {
namespace Plugins {
namespace FiltersGallery {

typedef KGenericFactory<KritaFiltersGallery> KritaFiltersGalleryFactory;
K_EXPORT_COMPONENT_FACTORY( kritafiltersgallery, KritaFiltersGalleryFactory( "krita" ) )

KritaFiltersGallery::KritaFiltersGallery(QObject *parent, const QStringList &)
        : KParts::Plugin(parent)
{
    if ( parent->inherits("KisView2") )
    {
        setComponentData(KritaFiltersGallery::componentData());

        setXMLFile(KStandardDirs::locate("data","kritaplugins/kritafiltersgallery.rc"),
        true);

        m_view = (KisView2*) parent;

        KAction *action  = new KAction(i18n("&Filters Gallery"), this);
        actionCollection()->addAction("krita_filters_gallery", action );
        connect(action, SIGNAL(triggered()), this, SLOT(showFiltersGalleryDialog()));

        // Add a docker with the list of filters
//         QImage img;
//         if(img.load(locate("data","krita/images/previewfilter.png")))
//         {
//            KisPaintDeviceSP preview = new KisPaintDevice(KoColorSpaceRegistry::instance()->colorSpace(KoID("RGBA",""),""));
//            preview->convertFromQImage(img,"");
//            m_view->canvasSubject()->paletteManager()->addWidget(new KisFiltersListView(preview,m_view),"filterslist",krita::EFFECTSBOX, 0);
//         }

    }


}

KritaFiltersGallery::~KritaFiltersGallery()
{
}

void KritaFiltersGallery::showFiltersGalleryDialog()
{
    KisDlgFiltersGallery dlg(m_view, m_view);
    if (dlg.exec())
    {
        QApplication::setOverrideCursor( Qt::WaitCursor );

        KisFilter* filter = dlg.currentFilter();
        if(filter )
        {
            KisImageSP img = m_view->image();
            if (!img) return;

            KisPaintDeviceSP dev = m_view->activeDevice();
            if (!dev) return;
            QRect r1 = dev->exactBounds();
            QRect r2 = img->bounds();

            QRect rect = r1.intersect(r2);

            KisSelectionSP selection = m_view->selection();
            if ( selection ) {
                QRect r3 = selection->selectedExactRect();
                rect = rect.intersect(r3);
            }
            KisFilterConfiguration* config = dlg.currentConfigWidget()->configuration();
// XXX_PROGRESS
//            m_view->statusBar()->progress()->setSubject(filter, true, true);
//             filter->setProgressDisplay(m_view->statusBar()->progress()); // TODO: port to KoUpdater

            KisTransaction * cmd = new KisTransaction(filter->name(), dev);

            filter->process(dev, rect, config);

            delete config;
//             if (filter->cancelRequested()) { // TODO: port to
//                 cmd->undo();
//                 delete cmd;
//             } else {
                dev->setDirty(rect);
                if (img->undo())
                    img->undoAdapter()->addCommand(cmd);
                else
                    delete cmd;
//             }
            QApplication::restoreOverrideCursor();

        }
    }
}

}
}
}

#include "filters_gallery.moc"
