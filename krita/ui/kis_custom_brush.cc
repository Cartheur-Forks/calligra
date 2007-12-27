/*
 *  Copyright (c) 2005 Bart Coppens <kde@bartcoppens.be>
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

#include <KoImageResource.h>
#include <kdebug.h>
#include <QLabel>
#include <QImage>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>

#include <QPixmap>
#include <QShowEvent>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>

#include "kis_view2.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_paint_device.h"
#include "kis_brush.h"
#include "kis_imagepipe_brush.h"
#include "kis_custom_brush.h"
#include "kis_resource_mediator.h"
#include "kis_resourceserverprovider.h"
#include "kis_paint_layer.h"
#include "kis_group_layer.h"

KisCustomBrush::KisCustomBrush(QWidget *parent, const char* name, const QString& caption, KisView2* view)
    : KisWdgCustomBrush(parent, name), m_view(view)
{
    Q_ASSERT(m_view);
    setWindowTitle(caption);

    m_brush = 0;

    preview->setScaledContents(true);

    KoResourceServer<KisBrush>* rServer = KisResourceServerProvider::instance()->brushServer();
    m_rServerAdapter = new KoResourceServerAdapter<KisBrush>(rServer);

    connect(addButton, SIGNAL(pressed()), this, SLOT(slotAddPredefined()));
    connect(brushButton, SIGNAL(pressed()), this, SLOT(slotUseBrush()));
//    connect(exportButton, SIGNAL(pressed()), this, SLOT(slotExport()));
    connect(brushStyle, SIGNAL(activated(int)), this, SLOT(slotUpdateCurrentBrush(int)));
    connect(colorAsMask, SIGNAL(stateChanged(int)), this, SLOT(slotUpdateCurrentBrush(int)));
}

KisCustomBrush::~KisCustomBrush() {
    delete m_brush;
    delete m_rServerAdapter;
}

void KisCustomBrush::showEvent(QShowEvent *) {
    slotUpdateCurrentBrush(0);
}

void KisCustomBrush::slotUpdateCurrentBrush(int) {
    delete m_brush;
    m_brush = 0;
    if (m_view->image()) {
        createBrush();
        if (m_brush)
            preview->setPixmap(QPixmap::fromImage(m_brush->img()));
    }
}

void KisCustomBrush::slotExport() {
    ;
}

void KisCustomBrush::slotAddPredefined() {
    // Save in the directory that is likely to be: ~/.kde/share/apps/krita/brushes
    // a unique file with this brushname
    QString dir = KGlobal::dirs()->saveLocation("data", "krita/brushes");
    QString extension;

    if (brushStyle->currentIndex() == 0) {
        extension = ".gbr";
    } else {
        extension = ".gih";
    }

    QString tempFileName;
    {
        KTemporaryFile file;
        file.setPrefix(dir);
        file.setSuffix(extension);
        file.setAutoRemove(false);
        file.open();
        tempFileName = file.fileName();
    }

    // Save it to that file
    m_brush->setFilename(tempFileName);

    // Add it to the brush server, so that it automatically gets to the mediators, and
    // so to the other brush choosers can pick it up, if they want to
    if (m_rServerAdapter)
        m_rServerAdapter->addResource(m_brush->clone());
}

void KisCustomBrush::slotUseBrush() {
    KisBrush* copy = m_brush->clone();

    Q_CHECK_PTR(copy);

    emit(activatedResource(copy));
}

void KisCustomBrush::createBrush() {
    KisImageSP img = m_view->image();

    if (!img)
        return;

    if (brushStyle->currentIndex() == 0) {
        m_brush = new KisBrush(img->mergedImage().data(), 0, 0, img->width(), img->height());
        if (colorAsMask->isChecked())
            m_brush->makeMaskImage();
        return;
    }

    // For each layer in the current image, create a new image, and add it to the list
    QVector< QVector<KisPaintDevice*> > devices;
    devices.push_back(QVector<KisPaintDevice*>());
    int w = img->width();
    int h = img->height();

    // We only loop over the rootLayer. Since we actually should have a layer selection
    // list, no need to elaborate on that here and now
    KisLayer* layer = dynamic_cast<KisLayer*>( img->rootLayer()->firstChild().data() );
    while (layer) {
        KisPaintLayer* paint = 0;
        if (layer->visible() && (paint = dynamic_cast<KisPaintLayer*>(layer)))
            devices[0].push_back(paint->paintDevice().data());
        layer = dynamic_cast<KisLayer*>( layer->nextSibling().data() );
    }
    QVector<KisPipeBrushParasite::SelectionMode> modes;

    switch(comboBox2->currentIndex()) {
        case 0: modes.push_back(KisPipeBrushParasite::Constant); break;
        case 1: modes.push_back(KisPipeBrushParasite::Random); break;
        case 2: modes.push_back(KisPipeBrushParasite::Incremental); break;
        case 3: modes.push_back(KisPipeBrushParasite::Pressure); break;
        case 4: modes.push_back(KisPipeBrushParasite::Angular); break;
        default: modes.push_back(KisPipeBrushParasite::Incremental);
    }

    m_brush = new KisImagePipeBrush(img->objectName(), w, h, devices, modes);
    if (colorAsMask->isChecked())
        m_brush->makeMaskImage();
}


#include "kis_custom_brush.moc"
