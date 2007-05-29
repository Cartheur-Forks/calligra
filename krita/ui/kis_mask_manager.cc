/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2006
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
#include "kis_mask_manager.h"

#include <klocale.h>
#include <kstandardaction.h>
#include <kaction.h>
#include <ktoggleaction.h>
#include <kactioncollection.h>

#include <kis_undo_adapter.h>
#include <kis_paint_layer.h>
#include "kis_doc2.h"
#include "kis_view2.h"

KisMaskManager::KisMaskManager( KisView2 * view)
    : m_view( view )
    , m_createMask( 0 )
    , m_maskFromSelection( 0 )
    , m_maskToSelection( 0 )
    , m_applyMask( 0 )
    , m_removeMask( 0 )
    , m_showMask( 0 )
{
}

void KisMaskManager::setup( KActionCollection * actionCollection )
{
    m_createMask  = new KAction(i18n("Create Mask"), this);
    actionCollection->addAction("create_mask", m_createMask );
    connect(m_createMask, SIGNAL(triggered()), this, SLOT(slotCreateMask()));

    m_maskFromSelection  = new KAction(i18n("Mask From Selection"), this);
    actionCollection->addAction("create_mask_from_selection", m_maskFromSelection );
    connect(m_maskFromSelection, SIGNAL(triggered()), this, SLOT(slotMaskFromSelection()));

    m_maskToSelection  = new KAction(i18n("Mask To Selection"), this);
    actionCollection->addAction("create_selection_from_mask", m_maskToSelection );
    connect(m_maskToSelection, SIGNAL(triggered()), this, SLOT(slotMaskToSelection()));

    m_applyMask  = new KAction(i18n("Apply Mask"), this);
    actionCollection->addAction("apply_mask", m_applyMask );
    connect(m_applyMask, SIGNAL(triggered()), this, SLOT(slotApplyMask()));

    m_removeMask  = new KAction(i18n("Remove Mask"), this);
    actionCollection->addAction("remove_mask", m_removeMask );
    connect(m_removeMask, SIGNAL(triggered()), this, SLOT(slotRemoveMask()));

    m_showMask  = new KToggleAction(i18n("Show Mask"), this);
    actionCollection->addAction("show_mask", m_showMask );
    connect(m_showMask, SIGNAL(triggered()), this, SLOT(slotShowMask()));
}

void KisMaskManager::updateGUI()
{

}



void KisMaskManager::slotCreateMask() {
//     KisPaintLayer* layer = dynamic_cast<KisPaintLayer*>(m_view->image()->activeLayer().data());
//     if (!layer)
//         return;
//     QUndoCommand *cmd = layer->createMaskCommand();
//     m_view->document()->addCommand(cmd);
}
void KisMaskManager::slotMaskFromSelection() {
//     KisPaintLayer* layer = dynamic_cast<KisPaintLayer*>(m_view->image()->activeLayer().data());
//     if (!layer)
//         return;

//     QUndoCommand *cmd = layer->maskFromSelectionCommand();
//     m_view->document()->addCommand(cmd);
}


void KisMaskManager::slotMaskToSelection() {
//     KisPaintLayer* layer = dynamic_cast<KisPaintLayer*>(m_view->image()->activeLayer().data());
//     if (!layer)
//         return;

//     QUndoCommand *cmd = layer->maskToSelectionCommand();
//     m_view->document()->addCommand(cmd);
}

void KisMaskManager::slotApplyMask() {
//     KisPaintLayer* layer = dynamic_cast<KisPaintLayer*>(m_view->image()->activeLayer().data());
//     if (!layer)
//         return;

//     QUndoCommand *cmd = layer->applyMaskCommand();
//     m_view->document()->addCommand(cmd);
}

void KisMaskManager::slotRemoveMask() {
//     KisPaintLayer* layer = dynamic_cast<KisPaintLayer*>(m_view->image()->activeLayer().data());
//     if (!layer)
//         return;

//     QUndoCommand *cmd = layer->removeMaskCommand();
//     m_view->document()->addCommand(cmd);
}

void KisMaskManager::slotShowMask() {
//     KisPaintLayer* layer = dynamic_cast<KisPaintLayer*>(m_view->image()->activeLayer().data());
//     if (!layer)
//         return;

//     layer->setRenderMask(m_showMask->isChecked());
}

void KisMaskManager::maskUpdated() {
//     KisPaintLayer* layer = dynamic_cast<KisPaintLayer*>(m_view->image()->activeLayer().data());
//     if (!layer) {
//         m_createMask->setEnabled(false);
//         m_applyMask->setEnabled(false);
//         m_removeMask->setEnabled(false);
//         m_showMask->setEnabled(false);
//         return;
//     }
//     m_createMask->setEnabled(!layer->hasMask());
//     m_maskFromSelection->setEnabled(true); // Perhaps also update this to false when no selection?
//     m_maskToSelection->setEnabled(layer->hasMask());
//     m_applyMask->setEnabled(layer->hasMask());
//     m_removeMask->setEnabled(layer->hasMask());

//     m_editMask->setChecked(layer->editMask());
//     m_showMask->setEnabled(layer->hasMask());
//     m_showMask->setChecked(layer->renderMask());
}

#include "kis_mask_manager.moc"
