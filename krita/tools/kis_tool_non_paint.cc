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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "kis_tool_memento.h"
#include "kis_tool_non_paint.h"
#include "kis_view.h"

KisToolNonPaint::KisToolNonPaint(KisView *view, KisDoc *doc)
{
	m_view = view;
	m_doc = doc;
}

KisToolNonPaint::~KisToolNonPaint()
{
}

void KisToolNonPaint::paint(QPainter&)
{
}

void KisToolNonPaint::paint(QPainter&, const QRect&)
{
}

void KisToolNonPaint::clear()
{
}

void KisToolNonPaint::clear(const QRect&)
{
}

void KisToolNonPaint::enter(QEvent *)
{
}

void KisToolNonPaint::leave(QEvent *)
{
}

void KisToolNonPaint::mousePress(QMouseEvent *)
{
}

void KisToolNonPaint::mouseMove(QMouseEvent *)
{
}

void KisToolNonPaint::mouseRelease(QMouseEvent *)
{
}

void KisToolNonPaint::keyPress(QKeyEvent *)
{
}

void KisToolNonPaint::keyRelease(QKeyEvent *)
{
}

KDialog *KisToolNonPaint::options()
{
	return 0;
}

void KisToolNonPaint::save(KisToolMementoSP)
{
}

void KisToolNonPaint::restore(KisToolMementoSP)
{
}

void KisToolNonPaint::cursor(QWidget *w) const
{
	if (w)
		w -> setCursor(m_cursor);
}

void KisToolNonPaint::setCursor(const QCursor& cursor)
{
	m_cursor = cursor;
}

void KisToolNonPaint::activateSelf()
{
	if (m_view)
		m_view -> activateTool(this);
}

void KisToolNonPaint::setup()
{
}

void KisToolNonPaint::activate()
{
}

#include "kis_tool_non_paint.moc"

