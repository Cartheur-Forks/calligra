/*
 *  kis_tool_select_elliptical.cc -- part of Krita
 *
 *  Copyright (c) 2004 Boudewijn Rempt (boud@valdyas.org)
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

#include <QApplication>
#include <QPainter>
#include <QPen>
#include <QLayout>
#include <QVBoxLayout>

#include <kdebug.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kcommand.h>
#include <klocale.h>

#include "kis_autobrush_resource.h"
#include "kis_canvas_controller.h"
#include "kis_canvas_subject.h"
#include "kis_cursor.h"
#include "kis_image.h"
#include "kis_painter.h"
#include "kis_tool_select_elliptical.h"
#include "kis_layer.h"
#include "kis_undo_adapter.h"
#include "KoPointerEvent.h"
#include "KoPointerEvent.h"
#include "KoPointerEvent.h"
#include "kis_selection.h"
#include "kis_selection_options.h"
#include "kis_selected_transaction.h"
#include "kis_canvas.h"
#include "QPainter"

KisToolSelectElliptical::KisToolSelectElliptical()
    : super(i18n("Elliptical Select"))
{
    setObjectName("tool_select_elliptical");
    setCursor(KisCursor::load("tool_elliptical_selection_cursor.png", 6, 6));

    m_subject = 0;
    m_selecting = false;
    m_centerPos = KoPoint(0, 0);
    m_startPos = KoPoint(0, 0);
    m_endPos = KoPoint(0, 0);
    m_optWidget = 0;
    m_selectAction = SELECTION_ADD;
}

KisToolSelectElliptical::~KisToolSelectElliptical()
{
}

void KisToolSelectElliptical::activate()
{
    super::activate();

    if (!m_optWidget)
        return;

    m_optWidget->slotActivated();
}

void KisToolSelectElliptical::paint(QPainter& gc)
{
    if (m_selecting)
        paintOutline(gc, QRect());
}

void KisToolSelectElliptical::paint(QPainter& gc, const QRect& rc)
{
    if (m_selecting)
        paintOutline(gc, rc);
}

void KisToolSelectElliptical::clearSelection()
{
    if (m_subject) {
        KisCanvasController *controller = m_subject->canvasController();
        

        Q_ASSERT(controller);

//         if (m_currentImage && m_currentImage->floatingSelection().data() != 0) {
//             m_currentImage->unsetFloatingSelection();
//                         controller->canvas()->update();
//         }

        m_startPos = KoPoint(0, 0);
        m_endPos = KoPoint(0, 0);
        m_selecting = false;
    }
}

void KisToolSelectElliptical::buttonPress(KoPointerEvent *e)
{
    if (m_subject) {
        

        if (m_currentImage && m_currentImage->activeDevice() && e->button() == Qt::LeftButton) {
            clearSelection();
            m_startPos = m_endPos = m_centerPos = e->pos();
            m_selecting = true;
            paintOutline();
        }
    }
}

void KisToolSelectElliptical::move(KoPointerEvent *e)
{
    if (m_subject && m_selecting) {
        paintOutline();
        // move (alt) or resize ellipse
        if (e->modifiers() & Qt::AltModifier) {
            KoPoint trans = e->pos() - m_endPos;
            m_startPos += trans;
            m_endPos += trans;
        } else {
            KoPoint diag = e->pos() - (e->modifiers() & Qt::ControlModifier
                    ? m_centerPos : m_startPos);
            // circle?
            if (e->modifiers() & Qt::ShiftModifier) {
                double size = qMax(fabs(diag.x()), fabs(diag.y()));
                double w = diag.x() < 0 ? -size : size;
                double h = diag.y() < 0 ? -size : size;
                diag = KoPoint(w, h);
            }

            // resize around center point?
            if (e->modifiers() & Qt::ControlModifier) {
                m_startPos = m_centerPos - diag;
                m_endPos = m_centerPos + diag;
            } else {
                m_endPos = m_startPos + diag;
            }
        }
        paintOutline();
        m_centerPos = KoPoint((m_startPos.x() + m_endPos.x()) / 2,
                (m_startPos.y() + m_endPos.y()) / 2);
    }
}

void KisToolSelectElliptical::buttonRelease(KoPointerEvent *e)
{
     if (m_subject && m_selecting && e->button() == Qt::LeftButton) {

        paintOutline();

        if (m_startPos == m_endPos) {
            clearSelection();
        } else {
            QApplication::setOverrideCursor(KisCursor::waitCursor());
            

            if (!m_currentImage)
                return;

            if (m_endPos.y() < 0)
                m_endPos.setY(0);

            if (m_endPos.y() > m_currentImage->height())
                m_endPos.setY(m_currentImage->height());

            if (m_endPos.x() < 0)
                m_endPos.setX(0);

            if (m_endPos.x() > m_currentImage->width())
                m_endPos.setX(m_currentImage->width());

            if (m_currentImage && m_currentImage->activeDevice()) {
                KisPaintDeviceSP dev = m_currentImage->activeDevice();
                KisSelectedTransaction *t = 0;
                if (m_currentImage->undo()) t = new KisSelectedTransaction(i18n("Elliptical Selection"), dev);

                bool hasSelection = dev->hasSelection();
                if(! hasSelection)
                {
                    dev->selection()->clear();
                    if(m_selectAction==SELECTION_SUBTRACT)
                        dev->selection()->invert();
                }
                QRect rc( m_startPos.floorQPoint(), m_endPos.floorQPoint());
                rc = rc.normalized();

                KisSelectionSP tmpSel = KisSelectionSP(new KisSelection(dev));
                KisAutobrushCircleShape shape(rc.width(),rc.height(), 1, 1);
                quint8 value;
                for (int y = 0; y <= rc.height(); y++)
                    for (int x = 0; x <= rc.width(); x++)
                    {
                        value = MAX_SELECTED - shape.valueAt(x,y);
                        tmpSel->setSelected( x+rc.x(), y+rc.y(), value);
                    }
                switch(m_selectAction)
                {
                    case SELECTION_ADD:
                        dev->addSelection(tmpSel);
                        break;
                    case SELECTION_SUBTRACT:
                        dev->subtractSelection(tmpSel);
                        break;
                }

                if(hasSelection) {
                    dev->setDirty(rc);
                    dev->emitSelectionChanged(rc);
                } else {
                    dev->setDirty();
                    dev->emitSelectionChanged();
                }

                if (m_currentImage->undo())
                    m_currentImage->undoAdapter()->addCommand(t);

                QApplication::restoreOverrideCursor();
            }
        }
        m_selecting = false;
    }
}

void KisToolSelectElliptical::paintOutline()
{
    if (m_subject) {
        KisCanvasController *controller = m_subject->canvasController();
        KisCanvas *canvas = controller->kiscanvas();
        QPainter gc(canvas->canvasWidget());
        QRect rc;

        paintOutline(gc, rc);
    }
}

void KisToolSelectElliptical::paintOutline(QPainter& gc, const QRect&)
{
    if (m_subject) {
        KisCanvasController *controller = m_subject->canvasController();
        //RasterOp op = gc.rasterOp();
        QPen old = gc.pen();
        QPen pen(Qt::DotLine);
        QPoint start;
        QPoint end;

        Q_ASSERT(controller);
        start = controller->windowToView(m_startPos).floorQPoint();
        end = controller->windowToView(m_endPos).floorQPoint();

        //gc.setRasterOp(Qt::NotROP);
        gc.setPen(pen);
        gc.drawEllipse(QRect(start, end));
        //gc.setRasterOp(op);
        gc.setPen(old);
    }
}

void KisToolSelectElliptical::slotSetAction(int action) {
    if (action >= SELECTION_ADD && action <= SELECTION_SUBTRACT)
        m_selectAction =(enumSelectionMode)action;
}

void KisToolSelectElliptical::setup(KActionCollection *collection)
{
    m_action = collection->action(objectName());

    if (m_action == 0) {
        m_action = new KAction(KIcon("tool_elliptical_selection"),
                               i18n("&Elliptical Selection"),
                               collection,
                               objectName());
        Q_CHECK_PTR(m_action);
        m_action->setShortcut(Qt::Key_J);
        connect(m_action, SIGNAL(triggered()), this, SLOT(activate()));
        m_action->setToolTip(i18n("Select an elliptical area"));
        m_action->setActionGroup(actionGroup());
        m_ownAction = true;
    }
}

QWidget* KisToolSelectElliptical::createOptionWidget()
{
    m_optWidget = new KisSelectionOptions(parent, m_subject);
    Q_CHECK_PTR(m_optWidget);
    m_optWidget->setWindowTitle(i18n("Elliptical Selection"));

    connect (m_optWidget, SIGNAL(actionChanged(int)), this, SLOT(slotSetAction(int)));

    QVBoxLayout * l = dynamic_cast<QVBoxLayout*>(m_optWidget->layout());
    Q_ASSERT(l);
    if (l) {
        l->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding));
    }

    return m_optWidget;
}

QWidget* KisToolSelectElliptical::optionWidget()
{
        return m_optWidget;
}



#include "kis_tool_select_elliptical.moc"
