/*
 *  kis_tool_line.cc - part of Krayon
 *
 *  Copyright (c) 2000 John Califf
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2003 Boudewijn Rempt <boud@valdyas.org>
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
#include <QLayout>
#include <QWidget>

#include <kdebug.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kcommand.h>
#include <klocale.h>

#include "kis_cursor.h"
#include "kis_painter.h"
#include "kis_tool_line.h"
#include "KoPointerEvent.h"
#include "KoPointerEvent.h"
#include "KoPointerEvent.h"
#include "kis_paintop_registry.h"
#include "kis_canvas_subject.h"
#include "kis_undo_adapter.h"
#include "kis_canvas.h"
#include "QPainter"
#include "kis_cursor.h"
#include "kis_layer.h"

KisToolLine::KisToolLine()
    : super(i18n("Line")),
      m_dragging( false )
{
    setObjectName("tool_line");
    setCursor(KisCursor::load("tool_line_cursor.png", 6, 6));

    m_painter = 0;
    m_currentImage = 0;
    m_startPos = KoPoint(0, 0);
    m_endPos = KoPoint(0, 0);
}

KisToolLine::~KisToolLine()
{
}

void KisToolLine::paint(QPainter& gc)
{
    if (m_dragging)
        paintLine(gc, QRect());
}

void KisToolLine::paint(QPainter& gc, const QRect& rc)
{
    if (m_dragging)
        paintLine(gc, rc);
}

void KisToolLine::buttonPress(KoPointerEvent *e)
{
    if (!m_subject || !m_currentImage) return;

    if (!m_subject->currentBrush()) return;

    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        //KisCanvasController *controller = m_subject->canvasController();
        m_startPos = e->pos(); //controller->windowToView(e->pos());
        m_endPos = e->pos(); //controller->windowToView(e->pos());
    }
}

void KisToolLine::move(KoPointerEvent *e)
{
    if (m_dragging) {
        if (m_startPos != m_endPos)
            paintLine();
        //KisCanvasController *controller = m_subject->canvasController();

        if (e->modifiers() & Qt::AltModifier) {
            KoPoint trans = e->pos() - m_endPos;
            m_startPos += trans;
            m_endPos += trans;
        } else if (e->modifiers() & Qt::ShiftModifier)
            m_endPos = straightLine(e->pos());
        else
            m_endPos = e->pos();//controller->windowToView(e->pos());
        paintLine();
    }
}

void KisToolLine::buttonRelease(KoPointerEvent *e)
{
    if (m_dragging && e->button() == Qt::LeftButton) {
        m_dragging = false;
        if(m_subject) {
            KisCanvasController *controller = m_subject->canvasController();
            KisImageSP img = m_subject->currentImg();

            if (m_startPos == m_endPos) {
                controller->updateCanvas();
                m_dragging = false;
                return;
            }

            if ((e->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) {
                m_endPos = straightLine(e->pos());
            } else m_endPos = e->pos();

            KisPaintDeviceSP device;
            if (m_currentImage &&
                (device = m_currentImage->activeDevice()) &&
                m_subject->currentBrush()) {
                delete m_painter;
                m_painter = new KisPainter( device );
                Q_CHECK_PTR(m_painter);

                if (m_currentImage->undo()) m_painter->beginTransaction(i18n("Line"));

                m_painter->setPaintColor(m_subject->fgColor());
                m_painter->setBrush(m_subject->currentBrush());
                m_painter->setOpacity(m_opacity);
                m_painter->setCompositeOp(m_compositeOp);
                KisPaintOp * op = KisPaintOpRegistry::instance()->paintOp(m_subject->currentPaintop(), m_subject->currentPaintopSettings(), m_painter);
                m_painter->setPaintOp(op); // Painter takes ownership
                m_painter->paintLine(m_startPos, PRESSURE_DEFAULT, 0, 0, m_endPos, PRESSURE_DEFAULT, 0, 0);
                device->setDirty( m_painter->dirtyRect() );
                notifyModified();

                /* remove remains of the line drawn while moving */
                if (controller->kiscanvas()) {
                    controller->kiscanvas()->update();
                }

                if (m_currentImage->undo() && m_painter) {
                    m_currentImage->undoAdapter()->addCommand(m_painter->endTransaction());
                }
                delete m_painter;
                m_painter = 0;
            } else {
                // m_painter can be 0 here...!!!
                controller->updateCanvas(m_painter->dirtyRect()); // Removes the last remaining line.
            }
        }
    }

}

KoPoint KisToolLine::straightLine(KoPoint point)
{
    KoPoint comparison = point - m_startPos;
    KoPoint result;

    if ( fabs(comparison.x()) > fabs(comparison.y())) {
        result.setX(point.x());
        result.setY(m_startPos.y());
    } else {
        result.setX( m_startPos.x() );
        result.setY( point.y() );
    }

    return result;
}

void KisToolLine::paintLine()
{
    if (m_subject) {
        KisCanvasController *controller = m_subject->canvasController();
        KisCanvas *canvas = controller->kiscanvas();
        QPainter gc(canvas->canvasWidget());
        QRect rc;

        paintLine(gc, rc);
    }
}

void KisToolLine::paintLine(QPainter& gc, const QRect&)
{
    if (m_subject) {
        KisCanvasController *controller = m_subject->canvasController();
        //RasterOp op = gc.rasterOp();
        QPen old = gc.pen();
        QPen pen(Qt::SolidLine);
        KoPoint start;
        KoPoint end;

//        Q_ASSERT(controller);
        start = controller->windowToView(m_startPos);
        end = controller->windowToView(m_endPos);
//          start.setX(start.x() - controller->horzValue());
//          start.setY(start.y() - controller->vertValue());
//          end.setX(end.x() - controller->horzValue());
//          end.setY(end.y() - controller->vertValue());
//          end.setX((end.x() - start.x()));
//          end.setY((end.y() - start.y()));
//         start *= m_subject->zoomFactor();
//         end *= m_subject->zoomFactor();
        //gc.setRasterOp(Qt::NotROP);
        gc.setPen(pen);
        gc.drawLine(start.floorQPoint(), end.floorQPoint());
        //gc.setRasterOp(op);
        gc.setPen(old);
    }
}

void KisToolLine::setup(KActionCollection *collection)
{
    m_action = collection->action(objectName());

    if (m_action == 0) {
        m_action = new KAction(KIcon("tool_line"),
                               i18n("&Line"),
                               collection,
                               objectName());
        m_action->setShortcut(Qt::Key_L);
        connect(m_action, SIGNAL(triggered()), this, SLOT(activate()));
        m_action->setToolTip(i18n("Draw a line"));
        m_action->setActionGroup(actionGroup());
        m_ownAction = true;
    }
}

QString KisToolLine::quickHelp() const {
    return i18n("Alt+Drag will move the origin of the currently displayed line around, Shift+Drag will force you to draw straight lines");
}

#include "kis_tool_line.moc"

