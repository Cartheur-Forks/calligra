/*
 *  Copyright (c) 2003 Patrick Julien <freak@codepimps.org>
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

#ifndef KIS_CANVAS_CONTROLLER_H_
#define KIS_CANVAS_CONTROLLER_H_

#include <qglobal.h>
#include <qpoint.h>
#include <qrect.h>

class QWidget;
class KisTool;
class KisRect;
class KisPoint;

class KisCanvasControllerInterface {
public:
	KisCanvasControllerInterface();
	virtual ~KisCanvasControllerInterface();

public:
	virtual QWidget *canvas() const = 0;
	virtual Q_INT32 horzValue() const = 0;
	virtual Q_INT32 vertValue() const = 0;
	virtual void scrollTo(Q_INT32 x, Q_INT32 y) = 0;
	virtual void updateCanvas() = 0;
	virtual void updateCanvas(Q_INT32 x, Q_INT32 y, Q_INT32 w, Q_INT32 h) = 0;
	virtual void updateCanvas(const QRect& rc) = 0;
	virtual void zoomIn() = 0;
	virtual void zoomIn(Q_INT32 x, Q_INT32 y) = 0;
	virtual void zoomOut() = 0;
	virtual void zoomOut(Q_INT32 x, Q_INT32 y) = 0;
	virtual void zoomTo(Q_INT32 x, Q_INT32 y, Q_INT32 w, Q_INT32 h) = 0;
	virtual void zoomTo(const QRect& r) = 0;
	virtual void zoomTo(const KisRect& r) = 0;
	virtual QPoint viewToWindow(const QPoint& pt) = 0;
	virtual KisPoint viewToWindow(const KisPoint& pt) = 0;
	virtual QRect viewToWindow(const QRect& rc) = 0;
	virtual KisRect viewToWindow(const KisRect& rc) = 0;
	virtual void viewToWindow(Q_INT32 *x, Q_INT32 *y) = 0;
	virtual QPoint windowToView(const QPoint& pt) = 0;
	virtual KisPoint windowToView(const KisPoint& pt) = 0;
	virtual QRect windowToView(const QRect& rc) = 0;
	virtual KisRect windowToView(const KisRect& rc) = 0;
	virtual void windowToView(Q_INT32 *x, Q_INT32 *y) = 0;

private:
	KisCanvasControllerInterface(const KisCanvasControllerInterface&);
	KisCanvasControllerInterface& operator=(const KisCanvasControllerInterface&);
};

#endif // KIS_CANVAS_CONTROLLER_H_

