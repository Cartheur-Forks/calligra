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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <stdlib.h>
#include <qpoint.h>
#include <kaction.h>
#include <kcommand.h>
#include <klocale.h>
#include <koColor.h>
#include "kis_doc.h"
#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_view.h"
#include "kis_tool_memento.h"
#include "kis_strategy_move.h"
#include "kis_undo_adapter.h"

namespace {
	class MoveCommand : public KNamedCommand {
		typedef KNamedCommand super;

	public:
		MoveCommand(KisView *view, KisImageSP img, KisPaintDeviceSP device, const QPoint& oldpos, const QPoint& newpos);
		virtual ~MoveCommand();

		virtual void execute();
		virtual void unexecute();

	private:
		void moveTo(const QPoint& pos);

	private:
		KisView *m_view;
		KisPaintDeviceSP m_device;
		QPoint m_oldPos;
		QPoint m_newPos;
		KisImageSP m_img;
	};

	MoveCommand::MoveCommand(KisView *view, KisImageSP img, KisPaintDeviceSP device, const QPoint& oldpos, const QPoint& newpos) :
		super(i18n("Move Painting Device"))
	{
		m_view = view;
		m_img = img;
		m_device = device;
		m_oldPos = oldpos;
		m_newPos = newpos;
	}

	MoveCommand::~MoveCommand()
	{
	}

	void MoveCommand::execute()
	{
		moveTo(m_newPos);
	}

	void MoveCommand::unexecute()
	{
		moveTo(m_oldPos);
	}

	void MoveCommand::moveTo(const QPoint& pos)
	{
		QRect rc;

		rc.setRect(m_device -> x(), m_device -> y(), m_device -> width(), m_device -> height());
		m_device -> move(pos.x(), pos.y());
		rc |= QRect(m_device -> x(), m_device -> y(), m_device -> width(), m_device -> height());
		m_img -> invalidate(); //rc);
		m_view -> updateCanvas(); //rc);
	}
}

KisStrategyMove::KisStrategyMove(KisView *view, KisDoc *doc)
{
	m_view = view;
	m_doc = doc;
	m_dragging = false;
}

KisStrategyMove::~KisStrategyMove()
{
}

void KisStrategyMove::startDrag(const QPoint& pos)
{
	KisImageSP img;
	KisPaintDeviceSP dev;

	if (!(img = m_view -> currentImg()))
		return;

	dev = img -> activeDevice();

	if (!dev || !dev -> visible())
		return;

	m_dragging = true;
	m_doc -> setModified(true);
	m_dragStart.setX(pos.x());
	m_dragStart.setY(pos.y());
	m_layerStart.setX(dev -> x());
	m_layerStart.setY(dev -> y());
	m_layerPosition = m_layerStart;
}

void KisStrategyMove::drag(const QPoint& original)
{
	if (m_dragging) {
		KisImageSP img = m_view -> currentImg();
		KisPaintDeviceSP dev;

		if (img && (dev = img -> activeDevice())) {
			QPoint pos = original;
			QRect rc;

			if (pos.x() >= img -> width() || pos.y() >= img -> height())
				return;

			pos -= m_dragStart;
			rc.setRect(dev -> x(), dev -> y(), dev -> width(), dev -> height());
			dev -> move(dev -> x() + pos.x(), dev -> y() + pos.y());
			rc = rc.unite(QRect(dev -> x(), dev -> y(), dev -> width(), dev -> height()));
			rc.setX(QMAX(0, rc.x()));
			rc.setY(QMAX(0, rc.y()));
			img -> invalidate(rc);
			m_layerPosition = QPoint(dev -> x(), dev -> y());
 			m_dragStart = original;
#if 0
			rc.setX(static_cast<Q_INT32>(rc.x() * m_view -> zoom()));
			rc.setY(static_cast<Q_INT32>(rc.y() * m_view -> zoom()));
			rc.setWidth(static_cast<Q_INT32>(rc.width() * m_view -> zoom()));
			rc.setHeight(static_cast<Q_INT32>(rc.height() * m_view -> zoom()));
#endif
			m_view -> updateCanvas(); //rc);
		}
	}
}

void KisStrategyMove::endDrag(const QPoint& pos, bool undo)
{
	if (m_dragging) {
		KisImageSP img = m_view -> currentImg();
		KisPaintDeviceSP dev;

		if (img && (dev = img -> activeDevice())) {
			drag(pos);
			m_dragging = false;

			if (undo) {
				KCommand *cmd = new MoveCommand(m_view, img, img -> activeDevice(), m_layerStart, m_layerPosition);
				KisUndoAdapter *adapter = img -> undoAdapter();

				if (adapter)
					adapter -> addCommand(cmd);
			}

			dev -> anchor();
		}
	}
}

void KisStrategyMove::simpleMove(const QPoint& pt1, const QPoint& pt2)
{
	startDrag(pt1);
	endDrag(pt2);
}

