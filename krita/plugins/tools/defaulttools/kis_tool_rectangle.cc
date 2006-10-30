/*
 *  kis_tool_rectangle.cc - part of Krita
 *
 *  Copyright (c) 2000 John Califf <jcaliff@compuzone.net>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@k.org>
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

#include <QPainter>

#include <kaction.h>
#include <kdebug.h>
#include <kactioncollection.h>
#include <klocale.h>

#include "KoPointerEvent.h"
#include "KoPointerEvent.h"
#include "kis_canvas_controller.h"
#include "kis_canvas_subject.h"
#include "KoPointerEvent.h"
#include "kis_painter.h"
#include "kis_paintop_registry.h"
#include "kis_tool_rectangle.h"
#include "kis_undo_adapter.h"
#include "kis_canvas.h"
#include "QPainter"
#include "kis_cursor.h"
#include "kis_layer.h"

KisToolRectangle::KisToolRectangle()
    : super(i18n ("Rectangle")),
          m_dragging (false),
          m_currentImage (0)
{
    setObjectName("tool_rectangle");
    setCursor(KisCursor::load("tool_rectangle_cursor.png", 6, 6));
}

KisToolRectangle::~KisToolRectangle()
{
}

void KisToolRectangle::buttonPress(KoPointerEvent *event)
{
    if (m_currentImage && event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStart = m_dragCenter = m_dragEnd = event->pos();
        draw(m_dragStart, m_dragEnd);
    }
}

void KisToolRectangle::move(KoPointerEvent *event)
{
    if (m_dragging) {
        // erase old lines on canvas
        draw(m_dragStart, m_dragEnd);
        // move (alt) or resize rectangle
        if (event->modifiers() & Qt::AltModifier) {
            KoPoint trans = event->pos() - m_dragEnd;
            m_dragStart += trans;
            m_dragEnd += trans;
        } else {
            KoPoint diag = event->pos() - (event->modifiers() & Qt::ControlModifier
                    ? m_dragCenter : m_dragStart);
            // square?
            if (event->modifiers() & Qt::ShiftModifier) {
                double size = qMax(fabs(diag.x()), fabs(diag.y()));
                double w = diag.x() < 0 ? -size : size;
                double h = diag.y() < 0 ? -size : size;
                diag = KoPoint(w, h);
            }

            // resize around center point?
            if (event->modifiers() & Qt::ControlModifier) {
                m_dragStart = m_dragCenter - diag;
                m_dragEnd = m_dragCenter + diag;
            } else {
                m_dragEnd = m_dragStart + diag;
            }
        }
        // draw new lines on canvas
        draw(m_dragStart, m_dragEnd);
        m_dragCenter = KoPoint((m_dragStart.x() + m_dragEnd.x()) / 2,
                (m_dragStart.y() + m_dragEnd.y()) / 2);
    }
}

void KisToolRectangle::buttonRelease(KoPointerEvent *event)
{
    if (!m_subject)
        return;

    if (!m_currentImage)
        return;

    KisPaintDeviceSP device = m_currentImage->activeDevice ();
    if (!device) return;

    if (m_dragging && event->button() == Qt::LeftButton) {
        // erase old lines on canvas
        draw(m_dragStart, m_dragEnd);
        m_dragging = false;

        if (m_dragStart == m_dragEnd)
            return;

        if (!m_currentImage)
            return;


        KisPainter painter (device);
        if (m_currentImage->undo()) painter.beginTransaction (i18n ("Rectangle"));

        painter.setPaintColor(m_subject->fgColor());
        painter.setBackgroundColor(m_subject->bgColor());
        painter.setFillStyle(fillStyle());
        painter.setBrush(m_subject->currentBrush());
        painter.setPattern(m_subject->currentPattern());
        painter.setOpacity(m_opacity);
        painter.setCompositeOp(m_compositeOp);
        KisPaintOp * op = KisPaintOpRegistry::instance()->paintOp(m_subject->currentPaintop(), m_subject->currentPaintopSettings(), &painter);
        painter.setPaintOp(op);

        painter.paintRect(m_dragStart, m_dragEnd, PRESSURE_DEFAULT/*event->pressure()*/, event->xTilt(), event->yTilt());
        device->setDirty( painter.dirtyRect() );
        notifyModified();

        if (m_currentImage->undo()) {
            m_currentImage->undoAdapter()->addCommand(painter.endTransaction());
        }
    }
}

void KisToolRectangle::draw(const KoPoint& start, const KoPoint& end )
{
    if (!m_subject)
        return;

    KisCanvasController *controller = m_subject->canvasController ();
    KisCanvas *canvas = controller->kiscanvas();
    QPainter p (canvas->canvasWidget());

    //p.setRasterOp (Qt::NotROP);
    p.drawRect (QRect (controller->windowToView (start).floorQPoint(), controller->windowToView (end).floorQPoint()));
    p.end ();
}

void KisToolRectangle::setup(KActionCollection *collection)
{
    m_action = collection->action(objectName());

    if (m_action == 0) {
        m_action = new KAction(KIcon("tool_rectangle"),
                               i18n("&Rectangle"),
                               collection,
                               objectName());
        m_action->setShortcut(Qt::Key_F6);
        connect(m_action, SIGNAL(triggered()), this, SLOT(activate()));
        m_action->setToolTip(i18n("Draw a rectangle"));
        m_action->setActionGroup(actionGroup());
        m_ownAction = true;
    }
}

#include "kis_tool_rectangle.moc"
