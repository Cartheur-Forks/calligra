/*
 *  selecttool.h - part of Krayon
 *
 *  Copyright (c) 2000 John Califf <jcaliff@compuzone.net>
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

#ifndef __selecttoolpolygonal_h__
#define __selecttoolpolygonal_h__

#include <qpoint.h>
#include <qpointarray.h>

#include "kis_tool.h"


class KisDoc;
class KisCanvas;
class KisView;

class PolygonalSelectTool : public KisTool {
public:
	PolygonalSelectTool(KisDoc *doc, KisCanvas *canvas);
	virtual ~PolygonalSelectTool();

	virtual void setupAction(QObject *collection);

	virtual void clearOld();
	virtual bool willModify() const;

	virtual void paintEvent(QPaintEvent *e);
	virtual void mousePress(QMouseEvent *event);
	virtual void mouseMove(QMouseEvent *event);
	virtual void mouseRelease(QMouseEvent *event);

	void start(QPoint p);
	void finish(QPoint p); 

protected:
	void drawLine(const QPoint& start, const QPoint& end); 
 
private:
	QPoint m_dragStart;
	QPoint m_dragEnd;

	QPoint m_start;
	QPoint m_finish;

	bool m_dragging;
	bool m_drawn;   

	QRect m_selectRect;
	QPointArray m_pointArray;
	int m_index;

	bool moveSelectArea;
	bool dragSelectArea;
	QPoint m_hotSpot;
	QPoint m_oldDragPoint;
	QRegion m_selectRegion;
	QRect m_imageRect;
	bool m_dragFirst;
	float m_dragdist;
};

#endif //__selecttoolpolygonal_h__

