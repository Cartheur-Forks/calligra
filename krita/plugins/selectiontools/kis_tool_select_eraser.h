/*
 *  kis_tool_select_eraser.h - part of Krita
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

#ifndef KIS_TOOL_SELECT_ERASER_H_
#define KIS_TOOL_SELECT_ERASER_H_

#include <kis_tool_factory.h>
#include <kis_tool_freehand.h>

class KisPoint;
class KisSelectionOptions;


/**
 * The selection eraser makes a selection smaller by painting with the
 * current eraser shape. Not sure what kind of an icon could represent
 * this... Depends a bit on how we're going to visualize selections.
 */
class KisToolSelectEraser : public KisToolFreehand {
	Q_OBJECT
	typedef KisToolFreehand super;

public:
	KisToolSelectEraser();
	virtual ~KisToolSelectEraser();

	virtual void setup(KActionCollection *collection);
	virtual QWidget* createOptionWidget(QWidget* parent);
	virtual QWidget* optionWidget();

public slots:
	virtual void activate();

protected:

	virtual void initPaint(KisEvent *e);
private:
	KisSelectionOptions * m_optWidget;

};


class KisToolSelectEraserFactory : public KisToolFactory {
	typedef KisToolFactory super;
public:
	KisToolSelectEraserFactory(KActionCollection * ac) : super(ac) {};
	virtual ~KisToolSelectEraserFactory(){};
	
	virtual KisTool * createTool() { 
		KisTool * t =  new KisToolSelectEraser(); 
		Q_CHECK_PTR(t);
		t -> setup(m_ac); 
		return t; 
	}
	virtual KisID id() { return KisID("eraserselect", i18n("Eraser select tool")); }
};



#endif // KIS_TOOL_SELECT_ERASER_H_

