/*
 *  kis_tool_fill.h - part of Krayon^Krita
 *
 *  Copyright (c) 2004 Bart Coppens <kde@bartcoppens.be>
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

#ifndef __filltool_h__
#define __filltool_h__

#include <qpoint.h>

#include "kis_tool.h"
#include "kis_tool_paint.h"

class KisPainter;
class QWidget;
class QLabel;
class QCheckBox;
class KIntNumInput;
class KisCmbComposite;
class KActionCollection;

class KisToolFill : public KisToolPaint {

	typedef KisToolPaint super;
	Q_OBJECT

public:

	KisToolFill();
	virtual ~KisToolFill();
  
	virtual void setup(KActionCollection *collection);
	virtual void update(KisCanvasSubject *subject);

	virtual void buttonPress(KisButtonPressEvent*); 

	bool flood(int startX, int startY);
      
	virtual QWidget* createOptionWidget(QWidget* parent);
	virtual QWidget* optionWidget();

public slots:
	virtual void slotSetThreshold(int);
	virtual void slotSetCompositeMode(int);
	virtual void slotSetUsePattern(int);

private:
	int m_threshold;
	Q_INT32 m_depth;
	KisLayerSP m_lay;
	QUANTUM* m_oldColor, *m_color;
	KisPainter *m_painter;
	KisCanvasSubject *m_subject;
	KisImageSP m_currentImage;
	bool *m_map, m_samplemerged, m_usePattern, m_useSelection;
	KisSelectionSP m_selection;

	QWidget *m_optWidget;
	QLabel *m_lbThreshold;
	KIntNumInput *m_slThreshold;
	QLabel *m_lbComposite;
	KisCmbComposite *m_cmbComposite;
	QCheckBox *m_checkUsePattern;
	CompositeOp m_compositeOp;
};


#include "kis_tool_factory.h"

class KisToolFillFactory : public KisToolFactory {
	typedef KisToolFactory super;
public:
	KisToolFillFactory(KActionCollection * ac) : super(ac) {};
	virtual ~KisToolFillFactory(){};
	
	virtual KisTool * createTool() { KisToolFill * t = new KisToolFill(); t -> setup(m_ac); return t; }
	virtual QString name() { return i18n("Fill tool"); }

};




#endif //__filltool_h__

