/*
 *  kis_tool_brush.cc - part of Krita
 *
 *  Copyright (c) 2003-2004 Boudewijn Rempt <boud@valdyas.org>
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
#include <qlabel.h>
#include <qlayout.h>
#include <qwidget.h>

#include <kdebug.h>
#include <kaction.h>
#include <kcommand.h>
#include <klocale.h>

#include "integerwidget.h"
#include "kis_brush.h"
#include "kis_paintop.h"
#include "kis_paintop_registry.h"
#include "kis_cmb_composite.h"
#include "kis_cursor.h"
#include "kis_doc.h"
#include "kis_painter.h"
#include "kis_tool_brush.h"
#include "kis_view.h"


KisToolBrush::KisToolBrush()
        : super(i18n("Brush"))
{
	setName("tool_brush");
	setCursor(KisCursor::brushCursor());
}

KisToolBrush::~KisToolBrush()
{
}

void KisToolBrush::update(KisCanvasSubject *subject)
{
	super::update(subject);
	setCursor(KisCursor::brushCursor());
}

void KisToolBrush::initPaint(KisEvent *e) 
{
	super::initPaint(e);

	KisPaintOp * op = KisPaintOpRegistry::singleton() -> paintOp("paintbrush", painter());
	painter() -> setPaintOp(op); // And now the painter owns the op and will destroy it.
}

void KisToolBrush::setup(KActionCollection *collection)
{
	m_action = static_cast<KRadioAction *>(collection -> action(name()));

	if (m_action == 0) {
		m_action = new KRadioAction(i18n("Tool &Brush"),
					    "paintbrush", 0, this,
					    SLOT(activate()), collection,
					    name());
		m_action -> setExclusiveGroup("tools");
		m_ownAction = true;
	}
}


#include "kis_tool_brush.moc"

