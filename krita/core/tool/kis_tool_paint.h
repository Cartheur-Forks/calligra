/*
 *  Copyright (c) 2003 Boudewijn Rempt
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

#ifndef KIS_TOOL_PAINT_H_
#define KIS_TOOL_PAINT_H_

#include <qcursor.h>

#include <koffice_export.h>

#include "kis_tool.h"
#include "kis_composite_op.h"

class QEvent;
class QKeyEvent;
class QPaintEvent;
class QRect;
class QGridLayout;
class KDialog;
class KisCanvasSubject;
class QLabel;
class KIntNumInput;
class KisCmbComposite;

enum enumBrushMode {
	PAINT,
	PAINT_STYLUS,
	ERASE,
	ERASE_STYLUS,
	HOVER
};

class KRITACORE_EXPORT KisToolPaint : public KisTool {

	Q_OBJECT
	typedef KisTool super;

public:
	KisToolPaint(const QString& UIName);
	virtual ~KisToolPaint();

public:
	virtual void update(KisCanvasSubject *subject);

	virtual void paint(QPainter& gc);
	virtual void paint(QPainter& gc, const QRect& rc);
	virtual void clear();
	virtual void clear(const QRect& rc);

	virtual void enter(QEvent *e);
	virtual void leave(QEvent *e);
	virtual void buttonPress(KisButtonPressEvent *e);
	virtual void move(KisMoveEvent *e);
	virtual void buttonRelease(KisButtonReleaseEvent *e);
	virtual void keyPress(QKeyEvent *e);
	virtual void keyRelease(QKeyEvent *e);

	virtual void cursor(QWidget *w) const;
	virtual void setCursor(const QCursor& cursor);
	virtual QWidget* createOptionWidget(QWidget* parent);
	virtual QWidget* optionWidget();

public slots:
	virtual void activate();
	void slotSetOpacity(int opacityPerCent);
	void slotSetCompositeMode(const KisCompositeOp& compositeOp);

protected:
	void notifyModified() const;

	// Add the tool-specific layout to the default option widget's layout.
	void addOptionWidgetLayout(QLayout *layout);

private:
	void updateCompositeOpComboBox();

protected:
	KisCanvasSubject *m_subject;
	QRect m_dirtyRect;
	QUANTUM m_opacity;
	KisCompositeOp m_compositeOp;

private:
	QString m_UIName;

	QCursor m_cursor;
	QCursor m_toolCursor;

	QWidget *m_optionWidget;
	QGridLayout *m_optionWidgetLayout;

	QLabel *m_lbOpacity;
	KIntNumInput *m_slOpacity;
	QLabel *m_lbComposite;
	KisCmbComposite *m_cmbComposite;
};

#endif // KIS_TOOL_PAINT_H_

