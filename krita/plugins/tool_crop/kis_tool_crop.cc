/*
 *  kis_tool_crop.cc -- part of Krita
 *
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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
#include <qpen.h>
#include <qpushbutton.h>
#include <qobject.h>
#include <qcombobox.h>

#include <kdebug.h>
#include <kaction.h>
#include <kcommand.h>
#include <klocale.h>
#include <knuminput.h>

#include <kis_global.h>
#include <kis_painter.h>
#include <kis_canvas_controller.h>
#include <kis_canvas_subject.h>
#include <kis_cursor.h>
#include <kis_image.h>
#include <kis_undo_adapter.h>
#include <kis_button_press_event.h>
#include <kis_button_release_event.h>
#include <kis_move_event.h>

#include "kis_tool_crop.h"
#include "wdg_tool_crop.h"

namespace {
	class CropCmd : public KNamedCommand {
		typedef KNamedCommand super;

	public:
		CropCmd();
		virtual ~CropCmd();

	public:
		virtual void execute();
		virtual void unexecute();

	private:
	};

	CropCmd::CropCmd() : super(i18n("Crop"))
	{
	}

	CropCmd::~CropCmd()
	{
	}

	void CropCmd::execute()
	{
	}

	void CropCmd::unexecute()
	{
	}
}

KisToolCrop::KisToolCrop()
{
	setName("tool_crop");
	setCursor(KisCursor::selectCursor());
	m_subject = 0;
	m_selecting = false;
	m_startPos = QPoint(0, 0);
	m_endPos = QPoint(0, 0);
}

KisToolCrop::~KisToolCrop()
{
}

void KisToolCrop::update(KisCanvasSubject *subject)
{
	m_subject = subject;
	super::update(m_subject);
}

void KisToolCrop::paint(QPainter& gc)
{
	if (m_selecting)
		paintOutline(gc, QRect());
}

void KisToolCrop::paint(QPainter& gc, const QRect& rc)
{
	if (m_selecting)
		paintOutline(gc, rc);
}

void KisToolCrop::clearRect()
{
	if (m_subject) {
		KisCanvasControllerInterface *controller = m_subject -> canvasController();
		KisImageSP img = m_subject -> currentImg();

		Q_ASSERT(controller);

		controller -> canvas() -> update();
		
		m_startPos = QPoint(0, 0);
		m_endPos = QPoint(0, 0);


		((WdgToolCrop*)m_optWidget) -> intStartX -> setValue(m_startPos.x());
		((WdgToolCrop*)m_optWidget) -> intStartY -> setValue(m_startPos.y());
		((WdgToolCrop*)m_optWidget) -> intEndX -> setValue(m_endPos.x());
		((WdgToolCrop*)m_optWidget) -> intEndY -> setValue(m_endPos.y());

		m_selecting = false;
	}
}

void KisToolCrop::buttonPress(KisButtonPressEvent *e)
{
	if (m_subject) {
		KisImageSP img = m_subject -> currentImg();

		if (img && img -> activeDevice() && e -> button() == LeftButton) {
			clearRect();
			m_startPos = e -> pos().floorQPoint();
			m_endPos = e -> pos().floorQPoint();

			((WdgToolCrop*)m_optWidget) -> intStartX -> setValue(m_startPos.x());
			((WdgToolCrop*)m_optWidget) -> intStartY -> setValue(m_startPos.y());
			((WdgToolCrop*)m_optWidget) -> intEndX -> setValue(m_endPos.x());
			((WdgToolCrop*)m_optWidget) -> intEndY -> setValue(m_endPos.y());

			m_selecting = true;
		}
	}
}

void KisToolCrop::move(KisMoveEvent *e)
{
	if (m_subject && m_selecting) {

		if (m_startPos != m_endPos)
			paintOutline();

		m_endPos = e -> pos().floorQPoint();
		((WdgToolCrop*)m_optWidget) -> intEndX -> setValue(m_endPos.x());
		((WdgToolCrop*)m_optWidget) -> intEndY -> setValue(m_endPos.y());

		paintOutline();
	}
}

void KisToolCrop::buttonRelease(KisButtonReleaseEvent *e)
{
	KisImageSP img = m_subject -> currentImg();

	if (!img)
		return;

	if (m_subject && m_selecting) {
		if (m_startPos == m_endPos) {
			clearRect();
		}
		m_endPos = e -> pos().floorQPoint();
		((WdgToolCrop*)m_optWidget) -> intEndX -> setValue(m_endPos.x());
		((WdgToolCrop*)m_optWidget) -> intEndY -> setValue(m_endPos.y());

		m_selecting = false;
	}
}

void KisToolCrop::paintOutline()
{
	if (m_subject) {
		KisCanvasControllerInterface *controller = m_subject -> canvasController();
		QWidget *canvas = controller -> canvas();
		QPainter gc(canvas);
		QRect rc;

		paintOutline(gc, rc);
	}
}

void KisToolCrop::paintOutline(QPainter& gc, const QRect&)
{
	if (m_subject) {
		KisCanvasControllerInterface *controller = m_subject -> canvasController();
		RasterOp op = gc.rasterOp();
		QPen old = gc.pen();
		QPen pen(Qt::SolidLine);
		pen.setWidth(2);
		QPoint start;
		QPoint end;

		Q_ASSERT(controller);
		start = controller -> windowToView(m_startPos);
		end = controller -> windowToView(m_endPos);

		gc.setRasterOp(Qt::NotROP);
		gc.setPen(pen);
		gc.drawRect(QRect(start, end));
		gc.setRasterOp(op);
		gc.setPen(old);
	}
}

void KisToolCrop::crop() {
	// XXX: Should cropping be part of KisImage/KisPaintDevice's API?

	KisImageSP img = m_subject -> currentImg();

	if (!img)
		return;

	if (m_endPos.y() < 0)
		m_endPos.setY(0);
	
	if (m_endPos.y() > img -> height())
		m_endPos.setY(img -> height());
	
	if (m_endPos.x() < 0)
		m_endPos.setX(0);
	
	if (m_endPos.x() > img -> width())
				m_endPos.setX(img -> width());
	
	QRect rc(m_startPos, m_endPos);
	rc = rc.normalize();
	
	
	if (((WdgToolCrop *)m_optWidget) -> cmbType -> currentItem() == 0) {
		KisLayerSP layer = img -> activeLayer();
		cropLayer(layer, rc);
		layer -> move(rc.x(), rc.y());
		
	}
	else {
		vKisLayerSP layers = img -> layers();
		vKisLayerSP_it it;
		for ( it = layers.begin(); it != layers.end(); ++it ) {
			KisLayerSP layer = (*it);
			cropLayer(layer, rc);
		}
		img -> resize(rc);

	}
	img -> notify(rc);

	if (img -> undoAdapter())
		img -> undoAdapter() -> addCommand(new CropCmd());

	((WdgToolCrop*)m_optWidget) -> intStartX -> setValue(0);
	((WdgToolCrop*)m_optWidget) -> intStartY -> setValue(0);
	((WdgToolCrop*)m_optWidget) -> intEndX -> setValue(0);
	((WdgToolCrop*)m_optWidget) -> intEndY -> setValue(0);

}

void KisToolCrop::cropLayer(KisLayerSP layer, QRect rc) 
{
	KisPaintDeviceSP tmp = new KisPaintDevice(layer -> colorStrategy(), "temp");
	
	KisPainter p(tmp.data());
	p.bitBlt(0, 0, COMPOSITE_COPY,
		 layer.data(),
		 rc.x(),
		 rc.y(),
		 rc.width(),
		 rc.height());
	p.end();
	
	
	p.begin(layer.data());
	p.bitBlt(0, 0, COMPOSITE_COPY, tmp.data(), rc.x(), rc.y(), rc.width(), rc.height());
}

QWidget* KisToolCrop::createOptionWidget(QWidget* parent)
{
	WdgToolCrop * w = new WdgToolCrop(parent);
	
	connect(w -> bnCrop, SIGNAL(clicked()), this, SLOT(crop()));

	m_optWidget = w;
	return m_optWidget;
}

QWidget* KisToolCrop::optionWidget()
{
	return m_optWidget;
}

void KisToolCrop::setup(KActionCollection *collection)
{
	m_action = static_cast<KRadioAction *>(collection -> action(name()));

	if (m_action == 0) {
		m_action = new KRadioAction(i18n("Tool &Crop"), 
					    "crop", 
					    0, 
					    this,
					    SLOT(activate()), 
					    collection, 
					    name());
		m_action -> setExclusiveGroup("tools");
		m_ownAction = true;
	}
}

#include "kis_tool_crop.moc"
