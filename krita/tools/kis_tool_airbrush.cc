/*
 *  kis_tool_airbrush.cc - part of KimageShop^WKrayon^WKrita
 *
 *  Copyright (c) 1999 Matthias Elter <me@kde.org>
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
#include <qevent.h>
#include <qtimer.h>

#include <kaction.h>
#include <kcommand.h>
#include <kdebug.h>
#include <klocale.h>

#include "kis_brush.h"
#include "kis_cursor.h"
#include "kis_doc.h"
#include "kis_painter.h"
#include "kis_tool_airbrush.h"
#include "kis_view.h"
#include "kis_event.h"
#include "kis_paintop.h"
#include "kis_paintop_registry.h"

namespace {
	Q_INT32 RATE = 100;
}

KisToolAirbrush::KisToolAirbrush()
	: super(i18n("Airbrush"))
{
	setName("tool_airbrush");
	setCursor(KisCursor::airbrushCursor());

	m_timer = new QTimer(this);
	Q_CHECK_PTR(m_timer);

	connect(m_timer, SIGNAL(timeout()), this, SLOT(timeoutPaint()));
}

KisToolAirbrush::~KisToolAirbrush()
{
	delete m_timer;
	m_timer = 0;
}

void KisToolAirbrush::timeoutPaint()
{
	if (currentImage() && painter()) {
		painter() -> paintAt(m_prevPos, m_prevPressure, m_prevXTilt, m_prevYTilt);
		currentImage() -> notify(painter() -> dirtyRect());
	}
}

void KisToolAirbrush::initPaint(KisEvent *e)
{
	super::initPaint(e);
	m_timer -> start( RATE );
	KisPaintOp * op = KisPaintOpRegistry::instance() -> paintOp("airbrush", painter());
	painter() -> setPaintOp(op); // Painter takes over ownership of paintop
}

void KisToolAirbrush::endPaint()
{
	m_timer -> stop();
	super::endPaint();
}

void KisToolAirbrush::setup(KActionCollection *collection)
{
	m_action = static_cast<KRadioAction *>(collection -> action(name()));

	if (m_action == 0) {
		m_action = new KRadioAction(i18n("&Airbrush"),
					    "airbrush", Qt::Key_I, this,
					    SLOT(activate()), collection,
					    name());
		m_action -> setExclusiveGroup("tools");
		m_ownAction = true;
	}
}

#include "kis_tool_airbrush.moc"

