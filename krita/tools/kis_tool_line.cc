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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qpainter.h>

#include <kdebug.h>
#include <kaction.h>
#include <kcommand.h>
#include <klocale.h>

#include "kis_cursor.h"
#include "kis_painter.h"
#include "kis_tool_line.h"
#include "kis_button_press_event.h"
#include "kis_button_release_event.h"
#include "kis_move_event.h"
#include "kis_paintop_registry.h"
#include "kis_canvas_subject.h"
#include "kis_undo_adapter.h"

KisToolLine::KisToolLine()
	: super(i18n("Line")),
	  m_dragging( false )
{
	setName("tool_line");
	setCursor(KisCursor::arrowCursor());

	m_painter = 0;
	m_currentImage = 0;
	m_startPos = KisPoint(0, 0);
	m_endPos = KisPoint(0, 0);
}

KisToolLine::~KisToolLine()
{
}

void KisToolLine::update(KisCanvasSubject *subject)
{
	m_subject = subject;
	m_currentImage = subject -> currentImg();

	super::update(m_subject);
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

void KisToolLine::buttonPress(KisButtonPressEvent *e)
{
	if (!m_subject || !m_currentImage) return;

	if (!m_subject -> currentBrush()) return;

	if (e -> button() == QMouseEvent::LeftButton) {
		m_dragging = true;
		//KisCanvasControllerInterface *controller = m_subject -> canvasController();
		m_startPos = e -> pos(); //controller -> windowToView(e -> pos());
		m_endPos = e -> pos(); //controller -> windowToView(e -> pos());
	}
}

void KisToolLine::move(KisMoveEvent *e)
{
	if (m_dragging) {
		if (m_startPos != m_endPos)
			paintLine();
		//KisCanvasControllerInterface *controller = m_subject -> canvasController();

		if (e -> state() & Qt::AltButton) {
			KisPoint trans = e -> pos() - m_endPos;
			m_startPos += trans;
			m_endPos += trans;
		} else if (e -> state() & Qt::ShiftButton)
			m_endPos = straightLine(e -> pos());
		else
			m_endPos = e -> pos();//controller -> windowToView(e -> pos());
		paintLine();
	}
}

void KisToolLine::buttonRelease(KisButtonReleaseEvent *e)
{
	if (m_dragging && e -> button() == QMouseEvent::LeftButton) {
		m_dragging = false;
		KisCanvasControllerInterface *controller = m_subject -> canvasController();
		KisImageSP img = m_subject -> currentImg();

		if (m_startPos == m_endPos) {
			controller -> updateCanvas();
			m_dragging = false;
			return;
		}

		if ((e -> state() & Qt::ShiftButton) == Qt::ShiftButton) {
			m_endPos = straightLine(e -> pos());
		} else m_endPos = e -> pos();

		KisPaintDeviceSP device;
		if (m_currentImage &&
		    (device = m_currentImage -> activeDevice()) &&
		    m_subject &&
		    m_subject -> currentBrush()) {
			delete m_painter;
			m_painter = new KisPainter( device );
			Q_CHECK_PTR(m_painter);

			m_painter -> beginTransaction(i18n("Line"));

			m_painter -> setPaintColor(m_subject -> fgColor());
			m_painter -> setBrush(m_subject -> currentBrush());
			m_painter -> setOpacity(m_opacity);
			m_painter -> setCompositeOp(m_compositeOp);
			KisPaintOp * op = KisPaintOpRegistry::instance()->paintOp(m_subject->currentPaintop(), m_painter);
			m_painter -> setPaintOp(op); // Painter takes ownership
			m_painter -> paintLine(m_startPos, PRESSURE_DEFAULT, 0, 0, m_endPos, PRESSURE_DEFAULT, 0, 0);
			m_currentImage -> notify( m_painter -> dirtyRect() );
			notifyModified();

			/* remove remains of the line drawn while moving */
			if (controller -> canvas()) {
				controller -> canvas() -> update();
			}

			KisUndoAdapter *adapter = m_currentImage -> undoAdapter();
			if (adapter && m_painter) {
				adapter -> addCommand(m_painter->endTransaction());
			}
			delete m_painter;
			m_painter = 0;
		} else {
			// m_painter can be 0 here...!!!
			controller -> updateCanvas(m_painter -> dirtyRect()); // Removes the last remaining line.
		}
	}

}

KisPoint KisToolLine::straightLine(KisPoint point)
{
	KisPoint comparison = point - m_startPos;
	KisPoint result;
	
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
		KisCanvasControllerInterface *controller = m_subject -> canvasController();
		QWidget *canvas = controller -> canvas();
		QPainter gc(canvas);
		QRect rc;

		paintLine(gc, rc);
	}
}

void KisToolLine::paintLine(QPainter& gc, const QRect&)
{
	if (m_subject) {
		KisCanvasControllerInterface *controller = m_subject -> canvasController();
		RasterOp op = gc.rasterOp();
		QPen old = gc.pen();
		QPen pen(Qt::SolidLine);
		KisPoint start;
		KisPoint end;

//		Q_ASSERT(controller);
		start = controller -> windowToView(m_startPos);
		end = controller -> windowToView(m_endPos);
//  		start.setX(start.x() - controller -> horzValue());
//  		start.setY(start.y() - controller -> vertValue());
//  		end.setX(end.x() - controller -> horzValue());
//  		end.setY(end.y() - controller -> vertValue());
//  		end.setX((end.x() - start.x()));
//  		end.setY((end.y() - start.y()));
// 		start *= m_subject -> zoomFactor();
// 		end *= m_subject -> zoomFactor();
		gc.setRasterOp(Qt::NotROP);
		gc.setPen(pen);
		gc.drawLine(start.floorQPoint(), end.floorQPoint());
		gc.setRasterOp(op);
		gc.setPen(old);
	}
}

void KisToolLine::setup(KActionCollection *collection)
{
	m_action = static_cast<KRadioAction *>(collection -> action(name()));

	if (m_action == 0) {
		m_action = new KRadioAction(i18n("&Line"),
					    "line", Qt::Key_L, this,
					    SLOT(activate()), collection,
					    name());
		m_action -> setExclusiveGroup("tools");
		m_ownAction = true;
	}
}

#include "kis_tool_line.moc"

