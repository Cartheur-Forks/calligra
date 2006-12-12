/*
 *  kis_tool_polygon.cc -- part of Krita
 *
 *  Copyright (c) 2004 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
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


#include <math.h>

#include <QPainter>
#include <QSpinBox>
//Added by qt3to4:
#include <QKeyEvent>

#include <kaction.h>
#include <kdebug.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kdebug.h>
#include <knuminput.h>

#include "kis_view.h" // XXX: Something that kis view includes is needed here, but I don't know what
#include "kis_painter.h"
#include "kis_canvas_subject.h"
#include "kis_canvas_controller.h"
#include "KoPointerEvent.h"
#include "KoPointerEvent.h"
#include "KoPointerEvent.h"
#include "kis_paintop_registry.h"
#include "kis_canvas.h"
#include "QPainter"
#include "kis_cursor.h"
#include "kis_undo_adapter.h"

#include "kis_tool_polygon.h"

KisToolPolygon::KisToolPolygon()
        : super(i18n ("Polygon")),
          m_dragging (false),
          m_currentImage (0)
{
    setObjectName("tool_polygon");
    setCursor(KisCursor::load("tool_polygon_cursor.png", 6, 6));
}

KisToolPolygon::~KisToolPolygon()
{
}

void KisToolPolygon::update (KisCanvasSubject *subject)
{
        super::update (subject);
        if (m_subject)
            m_currentImage = m_subject->currentImg ();
}

void KisToolPolygon::buttonPress(KoPointerEvent *event)
{
    if (m_currentImage) {
        if (event->button() == Qt::LeftButton && event->modifiers() != Qt::ShiftModifier) {

            m_dragging = true;

            if (m_points.isEmpty())
            {
                m_dragStart = event->pos();
                m_dragEnd = event->pos();
                m_points.append(m_dragStart);
            } else {
                m_dragStart = m_dragEnd;
                m_dragEnd = event->pos();
                draw();
            }
        } else if (event->button() == Qt::LeftButton && event->modifiers() == Qt::ShiftModifier) {
            finish();
        }
    }
}

void KisToolPolygon::finish()
{
    // erase old lines on canvas
    draw();
    m_dragging = false;

    KisPaintDeviceSP device = m_currentImage->activeDevice ();
    if (!device) return;

    KisPainter painter (device);
    if (m_currentImage->undo()) painter.beginTransaction (i18n ("Polygon"));

    painter.setPaintColor(m_subject->fgColor());
    painter.setBackgroundColor(m_subject->bgColor());
    painter.setFillStyle(fillStyle());
    painter.setBrush(m_subject->currentBrush());
    painter.setPattern(m_subject->currentPattern());
    painter.setOpacity(m_opacity);
    painter.setCompositeOp(m_compositeOp);
    KisPaintOp * op = KisPaintOpRegistry::instance()->paintOp(m_subject->currentPaintOp(), m_subject->currentPaintOpSettings(), &painter);
    painter.setPaintOp(op); // Painter takes ownership

    painter.paintPolygon(m_points);

    m_points.clear();

    device->setDirty( painter.dirtyRect() );
    notifyModified();

    if (m_currentImage->undo()) {
        m_currentImage->undoAdapter()->addCommand(painter.endTransaction());
    }
}

void KisToolPolygon::doubleClick( KoPointerEvent * )
{
    finish();
}

void KisToolPolygon::move(KoPointerEvent *event)
{
    if (m_dragging) {
        // erase old lines on canvas
        draw();
        // get current mouse position
        m_dragEnd = event->pos();
        // draw new lines on canvas
        draw();
    }
}

void KisToolPolygon::buttonRelease(KoPointerEvent *event)
{
        if (!m_subject || !m_currentImage)
            return;

        if (m_dragging && event->button() == Qt::LeftButton)  {
                m_dragging = false;
                m_points.append (m_dragEnd);
    }

    if (m_dragging && event->button() == Qt::RightButton) {

        }
}

void KisToolPolygon::paint(QPainter& gc)
{
    draw(gc);
}

void KisToolPolygon::paint(QPainter& gc, const QRect&)
{
    draw(gc);
}

void KisToolPolygon::draw()
{
    if (m_subject) {
        KisCanvasController *controller = m_subject->canvasController();
        KisCanvas *canvas = controller->kiscanvas();
        QPainter gc(canvas->canvasWidget());

        draw(gc);
    }
}

void KisToolPolygon::draw(QPainter& gc)
{
        if (!m_subject || !m_currentImage)
            return;

        QPen pen(Qt::white, 0, Qt::SolidLine);

    gc.setPen(pen);
    //gc.setRasterOp(Qt::XorROP);

    KisCanvasController *controller = m_subject->canvasController();
    QPointF start, end;
    QPoint startPos;
    QPoint endPos;

    if (m_dragging) {
        startPos = controller->windowToView(m_dragStart.floorQPoint());
        endPos = controller->windowToView(m_dragEnd.floorQPoint());
        gc.drawLine(startPos, endPos);
    } else {
        for (KoPointVector::iterator it = m_points.begin(); it != m_points.end(); ++it) {

            if (it == m_points.begin())
            {
                start = (*it);
            } else {
                end = (*it);

                startPos = controller->windowToView(start.floorQPoint());
                endPos = controller->windowToView(end.floorQPoint());

                gc.drawLine(startPos, endPos);

                start = end;
            }
        }
    }
}



void KisToolPolygon::setup(KActionCollection *collection)
{
    m_action = collection->action(objectName());

    if (m_action == 0) {
        KShortcut shortcut(Qt::Key_Plus);
        shortcut.append(Qt::Key_F9);

        m_action = new KAction(KIcon("tool_polygon"),
                               i18n("&Polygon"),
                               collection,
                               objectName());
        Q_CHECK_PTR(m_action);
        m_action->setShortcut(shortcut);
        connect(m_action, SIGNAL(triggered()), this, SLOT(activate()));
        m_action->setActionGroup(actionGroup());
        m_action->setToolTip(i18n("Draw a polygon. Shift-mouseclick ends the polygon."));
        m_ownAction = true;
    }
}

void KisToolPolygon::keyPress(QKeyEvent *e)
{
    if (e->key()==Qt::Key_Escape);
    // erase old lines on canvas
    draw();
    m_dragging = false;
    m_points.clear();
}


#include "kis_tool_polygon.moc"
