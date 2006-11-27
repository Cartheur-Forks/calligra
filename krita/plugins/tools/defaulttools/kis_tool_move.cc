/*
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *                1999 Michael Koch    <koch@kde.org>
 *                2002 Patrick Julien <freak@codepimps.org>
 *                2004 Boudewijn Rempt <boud@valdyas.org>
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

#include <stdlib.h>
#include <QPoint>
#include <QColor>
#include <QMouseEvent>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>

#include "kis_canvas_subject.h"
#include "kis_cursor.h"
#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_tool_move.h"
#include "kis_tool_move.moc"
#include "KoPointerEvent.h"
#include "KoPointerEvent.h"
#include "KoPointerEvent.h"
#include "kis_selection.h"
#include "kis_selection_manager.h"
#include "kis_layer.h"

KisToolMove::KisToolMove()
    : super(i18n("Move Tool"))
{
    setObjectName("tool_move");
    m_subject = 0;
    setCursor(KisCursor::moveCursor());
}

KisToolMove::~KisToolMove()
{
}

void KisToolMove::buttonPress(KoPointerEvent *e)
{
    if (m_subject && e->button() == Qt::LeftButton) {
        QPoint pos = e->pos().floorQPoint();
        
        KisLayerSP dev;

        if (!m_currentImage || !(dev = m_currentImage->activeLayer()))
            return;

        m_dragStart = pos;
        m_strategy.startDrag(pos);
    }
}

void KisToolMove::move(KoPointerEvent *e)
{
    if (m_subject) {
        QPoint pos = e->pos().floorQPoint();
        if((e->modifiers() & Qt::AltModifier) || (e->modifiers() & Qt::ControlModifier)) {
            if(fabs(pos.x() - m_dragStart.x()) > fabs(pos.y() - m_dragStart.y()))
                pos.setY(m_dragStart.y());
            else
                pos.setX(m_dragStart.x());
        }
        m_strategy.drag(pos);
    }
}

void KisToolMove::buttonRelease(KoPointerEvent *e)
{
    if (m_subject && e->button() == Qt::LeftButton) {
        m_strategy.endDrag(e->pos().floorQPoint());
    }
}

void KisToolMove::setup(KActionCollection *collection)
{
    m_action = collection->action(objectName());

    if (m_action == 0) {
        m_action = new KAction(KIcon("move"),
                               i18n("&Move"),
                               collection,
                               objectName());
        m_action->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_V));
        connect(m_action, SIGNAL(triggered()), this, SLOT(activate()));
        m_action->setToolTip(i18n("Move"));
        m_action->setActionGroup(actionGroup());
        m_ownAction = true;
    }
}

