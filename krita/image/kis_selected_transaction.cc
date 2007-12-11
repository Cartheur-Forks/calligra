/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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
#include "kis_types.h"
#include "kis_global.h"
#include "kis_selected_transaction.h"
#include "kis_selection.h"
#include "kis_pixel_selection.h"
#include "kis_selection.h"

KisSelectedTransaction::KisSelectedTransaction(const QString& name, KisPaintDeviceSP device, QUndoCommand* parent) :
    KisTransaction(name, device, parent),
    m_device(device),
    m_hadSelection(false /*device->hasSelection()*/)
{
#if 0 // XXX_SELECTION
    m_selTransaction = new KisTransaction(name, KisPaintDeviceSP(device->selection()->getOrCreatePixelSelection().data()));
    if(! m_hadSelection) {
        m_device->deselect(); // let us not be the cause of select
    }
    m_firstRedo = true;
#endif
}

KisSelectedTransaction::~KisSelectedTransaction()
{
    //delete m_selTransaction;
}

void KisSelectedTransaction::redo()
{
#if 0
    //QUndoStack calls redo(), so the first call needs to be blocked
    if(m_firstRedo)
    {
        m_firstRedo = false;
        m_selTransaction->redo();
        return;
    }

    KisTransaction::redo();
    m_selTransaction->redo();
    if(m_redoHasSelection)
        m_device->selection();
    else
        m_device->deselect();

    m_device->setDirty(m_device->image()->bounds());
    m_device->emitSelectionChanged();
#endif
}

void KisSelectedTransaction::undo()
{
#if 0
    m_redoHasSelection = m_device->hasSelection();

    KisTransaction::undo();
    m_selTransaction->undo();
    if(m_hadSelection)
        m_device->selection();
    else
        m_device->deselect();

    m_device->setDirty(m_device->image()->bounds());
    m_device->emitSelectionChanged();
#endif
}

void KisSelectedTransaction::undoNoUpdate()
{
#if 0
    m_redoHasSelection = m_device->hasSelection();

    KisTransaction::undoNoUpdate();
    m_selTransaction->undoNoUpdate();
    if(m_hadSelection)
        m_device->selection();
    else
        m_device->deselect();
#endif
}
