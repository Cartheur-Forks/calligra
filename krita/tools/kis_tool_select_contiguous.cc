/*
 *  selecttool.h - part of Krayon
 *
 *  Copyright (c) 1999 Michael Koch <koch@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@ideasandassociates.com>
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

#include <qpainter.h>

#include <kaction.h>
#include <kdebug.h>

#include "kis_doc.h"
#include "kis_canvas.h"
#include "kis_cursor.h"
#include "kis_tool_select_contiguous.h"
#include "kis_view.h"

ContiguousSelectTool::ContiguousSelectTool(KisDoc *doc, KisCanvas *canvas) : KisTool(doc)
{
	m_dragging = false;
	m_canvas = canvas;
	m_drawn = false;
	m_init  = true;
	m_dragStart = QPoint(-1,-1);
	m_dragEnd = QPoint(-1,-1);
	m_cursor = KisCursor::selectCursor();
}

ContiguousSelectTool::~ContiguousSelectTool()
{
}

void ContiguousSelectTool::clearOld()
{
//   if (m_doc->isEmpty()) return;
        
   if(m_dragStart.x() != -1)
        drawRect( m_dragStart, m_dragEnd ); 

    QRect updateRect(0, 0, m_doc->current()->width(), 
        m_doc->current()->height());
    m_view->updateCanvas(updateRect);

    m_dragStart = QPoint(-1,-1);
    m_dragEnd =   QPoint(-1,-1);
}

void ContiguousSelectTool::mousePress( QMouseEvent* event )
{
 //   if ( m_doc->isEmpty() )
//        return;

    if( event->button() == LeftButton )
    {
        // erase old rectangle    
        if(m_drawn) 
        {
            m_drawn = false;
           
            if(m_dragStart.x() != -1)
                drawRect( m_dragStart, m_dragEnd ); 
        }
                
        m_init = false;
        m_dragging = true;
        m_dragStart = event->pos();
        m_dragEnd = event->pos();
    }
}


void ContiguousSelectTool::mouseMove( QMouseEvent* event )
{
//    if ( m_doc->isEmpty() )
//        return;

    if( m_dragging )
    {
        drawRect( m_dragStart, m_dragEnd );
        m_dragEnd = event->pos();
        drawRect( m_dragStart, m_dragEnd );
    }
}


void ContiguousSelectTool::mouseRelease( QMouseEvent* event )
{
//    if ( m_doc->isEmpty() )
//        return;

    if( ( m_dragging ) && ( event->button() == LeftButton ) )
    {
        m_dragging = false;
        m_drawn = true;
        
        QPoint zStart = zoomed(m_dragStart);
        QPoint zEnd   = zoomed(m_dragEnd);
                
        if(zStart.x() <= zEnd.x())
        {
            m_selectRect.setLeft(zStart.x());
            m_selectRect.setRight(zEnd.x());
        }    
        else 
        {
            m_selectRect.setLeft(zEnd.x());                   
            m_selectRect.setRight(zStart.x());
        }
        
        if(zStart.y() <= zEnd.y())
        {
            m_selectRect.setTop(zStart.y());
            m_selectRect.setBottom(zEnd.y());            
        }    
        else
        {
            m_selectRect.setTop(zEnd.y());
            m_selectRect.setBottom(zStart.y());            
        }
                    
        m_doc->getSelection()->setBounds(m_selectRect);

        kdDebug(0) << "selectRect" 
            << " left: "   << m_selectRect.left() 
            << " top: "    << m_selectRect.top()
            << " right: "  << m_selectRect.right() 
            << " bottom: " << m_selectRect.bottom()
            << endl;
    }
}


void ContiguousSelectTool::drawRect( const QPoint& start, const QPoint& end )
{
    QPainter p, pCanvas;

    p.begin( m_canvas );
    p.setRasterOp( Qt::NotROP );
    p.setPen( QPen( Qt::DotLine ) );

    float zF = m_view->zoomFactor();
    
    p.drawRect( QRect(start.x() + m_view->xPaintOffset() 
                                - (int)(zF * m_view->xScrollOffset()),
                      start.y() + m_view->yPaintOffset() 
                                - (int)(zF * m_view->yScrollOffset()), 
                      end.x() - start.x(), 
                      end.y() - start.y()) );
    p.end();
}

void ContiguousSelectTool::setupAction(QObject *collection)
{
	KToggleAction *toggle = new KToggleAction(i18n("&Contiguous select"), "contiguous" , 0, this, SLOT(toolSelect()), 
			collection, "tool_select_contiguous" );

	toggle -> setExclusiveGroup("tools");
}

bool ContiguousSelectTool::willModify() const
{
	return false;
}


