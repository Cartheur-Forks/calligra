/*
 *  kis_tool.cc - part of KImageShop
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
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

#include <qmessagebox.h>

#include "kis_tool.h"
#include "kis_cursor.h"

KisTool::KisTool(KisDoc *doc, KisView *view)
{
  m_pDoc = doc;
  m_pView = view;
  m_Cursor = KisCursor::arrowCursor();
}

KisTool::~KisTool() {}

QString KisTool::toolName()
{
  return "BaseTool";
}

void KisTool::optionsDialog()
{
  QMessageBox::information (0L, "KimageShop", "No Options available for this tool.", 1);
}

void KisTool::setCursor( const QCursor& c )
{
  m_Cursor = c;
}

QCursor KisTool::cursor()
{
  return m_Cursor;
}

void KisTool::mousePress(QMouseEvent*){}
void KisTool::mouseMove(QMouseEvent*){}
void KisTool::mouseRelease(QMouseEvent*){}

#include "kis_tool.moc"
