/*
 * This file is part of Krita
 *
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

#include "kis_perspective_grid_manager.h"

#include <kaction.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktoggleaction.h>
#include <kactioncollection.h>

#include "kis_canvas2.h"
#include "kis_image.h"
#include "kis_grid_drawer.h"
#include "kis_perspective_grid.h"
#include "kis_view2.h"
#include "kis_resource_provider.h"

KisPerspectiveGridManager::KisPerspectiveGridManager(KisView2 * parent)
    : QObject()
    , m_toggleEdition(false)
    , m_view(parent)
{
}


KisPerspectiveGridManager::~KisPerspectiveGridManager()
{

}

void KisPerspectiveGridManager::updateGUI()
{
    KisImageSP image = m_view->image();


    if (image ) {
        KisPerspectiveGrid* pGrid = image->perspectiveGrid();
        m_toggleGrid->setEnabled( pGrid->hasSubGrids());
    }
}

void KisPerspectiveGridManager::setup(KActionCollection * collection)
{


    m_toggleGrid  = new KToggleAction(i18n("Show Perspective Grid"), this);
    collection->addAction("view_toggle_perspective_grid", m_toggleGrid );
    connect(m_toggleGrid, SIGNAL(triggered()), this, SLOT(toggleGrid()));

    m_toggleGrid->setCheckedState(KGuiItem(i18n("Hide Perspective Grid")));
    m_toggleGrid->setChecked(false);
    m_gridClear  = new KAction(i18n("Clear Perspective Grid"), this);
    collection->addAction("view_clear_perspective_grid", m_gridClear );
    connect(m_gridClear, SIGNAL(triggered()), this, SLOT(clearPerspectiveGrid()));
}

void KisPerspectiveGridManager::setGridVisible(bool t)
{
    KisImageSP image = m_view->image();


    if (t && image ) {
        KisPerspectiveGrid* pGrid = image->perspectiveGrid();
        if( pGrid->hasSubGrids())
        {
            m_toggleGrid->setChecked(true);
        }
    } else {
        m_toggleGrid->setChecked(false);
    }
//    m_view->updateCanvas();
}


void KisPerspectiveGridManager::toggleGrid()
{
    KisImageSP image = m_view->image();


    if (image && m_toggleGrid->isChecked()) {
        KisPerspectiveGrid* pGrid = image->perspectiveGrid();

        if(!pGrid->hasSubGrids())
        {
            KMessageBox::error(0, i18n("Before displaying the perspective grid, you need to initialize it with the perspective grid tool"), i18n("No Perspective Grid to Display") );
            m_toggleGrid->setChecked(false);
        }
    }
//    m_view->updateCanvas();
}

void KisPerspectiveGridManager::clearPerspectiveGrid()
{
    KisImageSP image = m_view->image();
    if (image ) {
        image->perspectiveGrid()->clearSubGrids();
//        m_view->updateCanvas();
        m_toggleGrid->setChecked(false);
        m_toggleGrid->setEnabled(false);
    }
}

void KisPerspectiveGridManager::startEdition()
{
    m_toggleEdition = true;
    m_toggleGrid->setEnabled( false );
//    if( m_toggleGrid->isChecked() )
//        m_view->updateCanvas();
}

void KisPerspectiveGridManager::stopEdition()
{
    m_toggleEdition = false;
    m_toggleGrid->setEnabled( true );
//    if( m_toggleGrid->isChecked() )
//       m_view->updateCanvas();
}

void KisPerspectiveGridManager::drawGrid(const QRect & wr, QPainter *p, bool openGL )
{
    Q_UNUSED( wr );
    Q_UNUSED( p );
    Q_UNUSED( openGL );

    KisImageSP image = m_view->resourceProvider()->currentImage();


    if (image && m_toggleGrid->isChecked() && !m_toggleEdition) {
        KisPerspectiveGrid* pGrid = image->perspectiveGrid();

        KisGridDrawer *gridDrawer = 0;

        if (openGL) {
            gridDrawer = new OpenGLGridDrawer( m_view->document(), m_view->canvasBase()->viewConverter() );
        } else {
            Q_ASSERT(p);

            if (p) {
                QPainterGridDrawer* painterGridDrawer = new QPainterGridDrawer(m_view->document(), m_view->canvasBase()->viewConverter() );
                painterGridDrawer->setPainter( p );
                gridDrawer = painterGridDrawer;
            }
        }
        
        Q_ASSERT(gridDrawer != 0);

        for( QList<KisSubPerspectiveGrid*>::const_iterator it = pGrid->begin(); it != pGrid->end(); ++it)
        {
            gridDrawer->drawPerspectiveGrid(image, wr, *it );
        }
        delete gridDrawer;
    }
}


#include "kis_perspective_grid_manager.moc"
