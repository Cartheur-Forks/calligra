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

#include <qpainter.h>

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>

#include "kis_button_press_event.h"
#include "kis_button_release_event.h"
#include "kis_canvas_controller.h"
#include "kis_canvas_subject.h"
#include "kis_move_event.h"
#include "kis_painter.h"
#include "kis_paintop_registry.h"
#include "kis_tool_rectangle.h"
#include "kis_undo_adapter.h"

KisToolRectangle::KisToolRectangle()
	: super(i18n ("Rectangle")),
          m_dragging (false),
          m_currentImage (0)
{
	setName("tool_rectangle");
}

KisToolRectangle::~KisToolRectangle()
{
}

void KisToolRectangle::update (KisCanvasSubject *subject)
{
        super::update (subject);
        if (m_subject)
            m_currentImage = m_subject->currentImg ();
}

void KisToolRectangle::buttonPress(KisButtonPressEvent *event)
{
	if (m_currentImage && event -> button() == LeftButton) {
		m_dragging = true;
		m_dragStart = m_dragCenter = m_dragEnd = event -> pos();
		draw(m_dragStart, m_dragEnd);
	}
}

void KisToolRectangle::move(KisMoveEvent *event)
{
	if (m_dragging) {
		// erase old lines on canvas
		draw(m_dragStart, m_dragEnd);
		// move (alt) or resize rectangle
		if (event -> state() & Qt::AltButton) {
			KisPoint trans = event -> pos() - m_dragEnd;
			m_dragStart += trans;
			m_dragEnd += trans;
		} else {
			KisPoint diag = event -> pos() - (event->state() & Qt::ControlButton
					? m_dragCenter : m_dragStart);
			// square?
			if (event -> state() & Qt::ShiftButton) {
				double size = QMAX(fabs(diag.x()), fabs(diag.y()));
				double w = diag.x() < 0 ? -size : size;
				double h = diag.y() < 0 ? -size : size;
				diag = KisPoint(w, h);
			}

			// resize around center point?
			if (event -> state() & Qt::ControlButton) {
				m_dragStart = m_dragCenter - diag;
				m_dragEnd = m_dragCenter + diag;
			} else {
				m_dragEnd = m_dragStart + diag;
			}
		}
		// draw new lines on canvas
		draw(m_dragStart, m_dragEnd);
		m_dragCenter = KisPoint((m_dragStart.x() + m_dragEnd.x()) / 2,
				(m_dragStart.y() + m_dragEnd.y()) / 2);
	}
}

void KisToolRectangle::buttonRelease(KisButtonReleaseEvent *event)
{
	if (!m_subject)
		return;

	if (m_dragging && event -> button() == LeftButton) {
		// erase old lines on canvas
		draw(m_dragStart, m_dragEnd);
		m_dragging = false;

		if (m_dragStart == m_dragEnd)
			return;

		if (!m_currentImage)
			return;

		KisPaintDeviceSP device = m_currentImage->activeDevice ();
		KisPainter painter (device);
		painter.beginTransaction (i18n ("Rectangle"));
		
		KisPaintOp * op = KisPaintOpRegistry::instance()->paintOp(m_subject->currentPaintop(), &painter);
		painter.setPaintOp(op);
		painter.setPaintColor(m_subject -> fgColor());
		painter.setBackgroundColor(m_subject -> bgColor());
		painter.setFillStyle(fillStyle());
		painter.setBrush(m_subject -> currentBrush());
		painter.setPattern(m_subject -> currentPattern());
		painter.setOpacity(m_opacity);
		painter.setCompositeOp(m_compositeOp);

		painter.paintRect(m_dragStart, m_dragEnd, PRESSURE_DEFAULT/*event -> pressure()*/, event -> xTilt(), event -> yTilt());
		m_currentImage -> notify( painter.dirtyRect() );
		notifyModified();

		KisUndoAdapter *adapter = m_currentImage -> undoAdapter();
		if (adapter) {
			adapter -> addCommand(painter.endTransaction());
		}
	}
}

void KisToolRectangle::draw(const KisPoint& start, const KisPoint& end )
{
	if (!m_subject)
		return;

	KisCanvasControllerInterface *controller = m_subject->canvasController ();
	QWidget *canvas = controller->canvas ();
	QPainter p (canvas);

	p.setRasterOp (Qt::NotROP);
	p.drawRect (QRect (controller->windowToView (start).floorQPoint(), controller->windowToView (end).floorQPoint()));
	p.end ();
}

void KisToolRectangle::setup(KActionCollection *collection)
{
	m_action = static_cast<KRadioAction *>(collection -> action(name()));

	if (m_action == 0) {
		m_action = new KRadioAction(i18n("&Rectangle"),
					    "rectangle",
					    Qt::Key_F6,
					    this,
					    SLOT(activate()),
					    collection,
					    name());
		m_action -> setToolTip(i18n("Draw a rectangle"));
		m_action -> setExclusiveGroup("tools");
		m_ownAction = true;
	}
}

#include "kis_tool_rectangle.moc"
