/* 
 * This file is part of Krita
 *
 * Copyright (c) 1999 Matthias Elter (me@kde.org)
 * Copyright (c) 2001-2002 Igor Jansen (rm@kde.org)
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

#include "kis_rgb_widget.h"

#include <qlayout.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qspinbox.h>

#include <koFrameButton.h>
#include <koColorSlider.h>
#include <kcolordialog.h>
#include <kdualcolorbutton.h>
#include "kis_color.h"
#include <qcolor.h>

KisRGBWidget::KisRGBWidget(QWidget *parent, const char *name) : super(parent, name)
{
	m_subject = 0;

	m_ColorButton = new KDualColorButton(this);
	m_ColorButton ->  setFixedSize(m_ColorButton->sizeHint());
	QGridLayout *mGrid = new QGridLayout(this, 3, 5, 5, 2);

	/* setup color sliders */
	mRSlider = new KoColorSlider(this);
	mRSlider->setMaximumHeight(20);
	mRSlider->slotSetRange(0, 255);

	mGSlider = new KoColorSlider(this);
	mGSlider->setMaximumHeight(20);
	mGSlider->slotSetRange(0, 255);

	mBSlider = new KoColorSlider(this);
	mBSlider->setMaximumHeight(20);
	mBSlider->slotSetRange(0, 255);

	/* setup slider labels */
	mRLabel = new QLabel("R", this);
	mRLabel->setFixedWidth(12);
	mRLabel->setFixedHeight(20);
	mGLabel = new QLabel("G", this);
	mGLabel->setFixedWidth(12);
	mGLabel->setFixedHeight(20);
	mBLabel = new QLabel("B", this);
	mBLabel->setFixedWidth(12);
	mBLabel->setFixedHeight(20);

	/* setup spin box */
	mRIn = new QSpinBox(0, 255, 1, this);
	mRIn->setFixedWidth(50);
	mRIn->setFixedHeight(20);
	mGIn = new QSpinBox(0, 255, 1, this);
	mGIn->setFixedWidth(50);
	mGIn->setFixedHeight(20);
	mBIn = new QSpinBox(0, 255, 1, this);
	mBIn->setFixedWidth(50);
	mBIn->setFixedHeight(20);

	mGrid->addMultiCellWidget(m_ColorButton, 0, 3, 0, 0, Qt::AlignTop);
	mGrid->addWidget(mRLabel, 0, 1);
	mGrid->addWidget(mGLabel, 1, 1);
	mGrid->addWidget(mBLabel, 2, 1);
	mGrid->addMultiCellWidget(mRSlider, 0, 0, 2, 3);
	mGrid->addMultiCellWidget(mGSlider, 1, 1, 2, 3);
	mGrid->addMultiCellWidget(mBSlider, 2, 2, 2, 3);
	mGrid->addWidget(mRIn, 0, 4);
	mGrid->addWidget(mGIn, 1, 4);
	mGrid->addWidget(mBIn, 2, 4);

	connect(m_ColorButton, SIGNAL(fgChanged(const QColor &)), this, SLOT(slotFGColorSelected(const QColor &)));
	connect(m_ColorButton, SIGNAL(bgChanged(const QColor &)), this, SLOT(slotBGColorSelected(const QColor &)));

	/* connect color sliders */
	connect(mRSlider, SIGNAL(valueChanged(int)), this, SLOT(slotRChanged(int)));
	connect(mGSlider, SIGNAL(valueChanged(int)), this, SLOT(slotGChanged(int)));
	connect(mBSlider, SIGNAL(valueChanged(int)), this, SLOT(slotBChanged(int)));

	/* connect spin box */
	connect(mRIn, SIGNAL(valueChanged(int)), this, SLOT(slotRChanged(int)));
	connect(mGIn, SIGNAL(valueChanged(int)), this, SLOT(slotGChanged(int)));
	connect(mBIn, SIGNAL(valueChanged(int)), this, SLOT(slotBChanged(int)));
}

void KisRGBWidget::slotRChanged(int r)
{
	if (m_ColorButton->current() == KDualColorButton::Foreground){
		m_fgColor.setRgb(r, m_fgColor.green(), m_fgColor.blue());
		m_ColorButton->setCurrent(KDualColorButton::Foreground);
		if(m_subject)
			m_subject->setFGColor(KisColor(m_fgColor));
	}
	else{
		m_bgColor.setRgb(r, m_bgColor.green(), m_bgColor.blue());
		m_ColorButton->setCurrent(KDualColorButton::Background);
		if(m_subject)
			m_subject->setBGColor(KisColor(m_bgColor));
	}
}

void KisRGBWidget::slotGChanged(int g)
{
	if (m_ColorButton->current() == KDualColorButton::Foreground){
		m_fgColor.setRgb(m_fgColor.red(), g, m_fgColor.blue());
		m_ColorButton->setCurrent(KDualColorButton::Foreground);
		if(m_subject)
			m_subject->setFGColor(m_fgColor);
	}
	else{
		m_bgColor.setRgb(m_bgColor.red(), g, m_bgColor.blue());
		m_ColorButton->setCurrent(KDualColorButton::Background);
		if(m_subject)
			m_subject->setBGColor(m_bgColor);
	}
}

void KisRGBWidget::slotBChanged(int b)
{
	if (m_ColorButton->current() == KDualColorButton::Foreground){
		m_fgColor.setRgb(m_fgColor.red(), m_fgColor.green(), b);
		m_ColorButton->setCurrent(KDualColorButton::Foreground);
		if(m_subject)
			m_subject->setFGColor(m_fgColor);
	}
	else{
		m_bgColor.setRgb(m_bgColor.red(), m_bgColor.green(), b);
		m_ColorButton->setCurrent(KDualColorButton::Background);
		if(m_subject)
			m_subject->setBGColor(m_bgColor);
	}
}

void KisRGBWidget::update(KisCanvasSubject *subject)
{
	m_subject = subject;
	m_fgColor = subject->fgColor().toQColor();
	m_bgColor = subject->bgColor().toQColor();

	QColor color = (m_ColorButton->current() == KDualColorButton::Foreground)? m_fgColor : m_bgColor;

	int r = color.red();
	int g = color.green();
	int b = color.blue();

	disconnect(m_ColorButton, SIGNAL(fgChanged(const QColor &)), this, SLOT(slotFGColorSelected(const QColor &)));
	disconnect(m_ColorButton, SIGNAL(bgChanged(const QColor &)), this, SLOT(slotBGColorSelected(const QColor &)));

	m_ColorButton->setForeground( m_fgColor );
	m_ColorButton->setBackground( m_bgColor );

	connect(m_ColorButton, SIGNAL(fgChanged(const QColor &)), this, SLOT(slotFGColorSelected(const QColor &)));
	connect(m_ColorButton, SIGNAL(bgChanged(const QColor &)), this, SLOT(slotBGColorSelected(const QColor &)));

	mRSlider->blockSignals(true);
	mRIn->blockSignals(true);
	mGSlider->blockSignals(true);
	mGIn->blockSignals(true);
	mBSlider->blockSignals(true);
	mBIn->blockSignals(true);
	
	mRSlider->slotSetColor1(QColor(0, g, b));
	mRSlider->slotSetColor2(QColor(255, g, b));
	mRSlider->slotSetValue(r);
	mRIn->setValue(r);

	mGSlider->slotSetColor1(QColor(r, 0, b));
	mGSlider->slotSetColor2(QColor(r, 255, b));
	mGSlider->slotSetValue(g);
	mGIn->setValue(g);

	mBSlider->slotSetColor1(QColor(r, g, 0));
	mBSlider->slotSetColor2(QColor(r, g, 255));
	mBSlider->slotSetValue(b);
	mBIn->setValue(b);
	
	mRSlider->blockSignals(false);
	mRIn->blockSignals(false);
	mGSlider->blockSignals(false);
	mGIn->blockSignals(false);
	mBSlider->blockSignals(false);
	mBIn->blockSignals(false);
}

void KisRGBWidget::slotFGColorSelected(const QColor& c)
{
	m_fgColor = QColor(c);
	if(m_subject)
	{
		QColor bgColor = m_ColorButton -> background();
		m_subject->setFGColor(m_fgColor);
		//Background signal could be blocked so do that manually 
		//see bug 106919
		m_subject->setBGColor(bgColor);
	}
}

void KisRGBWidget::slotBGColorSelected(const QColor& c)
{
	m_bgColor = QColor(c);
	if(m_subject)
		m_subject->setBGColor(m_bgColor);
}

#include "kis_rgb_widget.moc"
