/*
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
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
#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <klocale.h>
#include <koIconChooser.h>

#include "integerwidget.h"
#include "kis_brush_chooser.h"
#include "kis_global.h"
#include "kis_icon_item.h"
#include "kis_brush.h"

KisBrushChooser::KisBrushChooser(QWidget *parent, const char *name) : super(parent, name)
{
	m_lbSpacing = new QLabel(i18n("Spacing: "), this);
	m_slSpacing = new IntegerWidget( 1, 100, this, "int_widget" );
	m_slSpacing -> setTickmarks(QSlider::Below);
	m_slSpacing -> setTickInterval(10);
	QObject::connect(m_slSpacing, SIGNAL(valueChanged(int)), this, SLOT(slotSetItemSpacing(int)));

	m_chkColorMask = new QCheckBox(i18n("Use color as mask"), this);
	QObject::connect(m_chkColorMask, SIGNAL(toggled(bool)), this, SLOT(slotSetItemUseColorAsMask(bool)));

	m_lbName = new QLabel(this);

	QVBoxLayout *mainLayout = new QVBoxLayout(this, 2, -1, "main layout");

	mainLayout -> addWidget(m_lbName);
	mainLayout -> addWidget(chooserWidget(), 10);

	QGridLayout *spacingLayout = new QGridLayout( 2, 2);

	mainLayout -> addLayout(spacingLayout, 1);

	spacingLayout -> addWidget(m_lbSpacing, 0, 0);
	spacingLayout -> addWidget(m_slSpacing, 0, 1);

	spacingLayout -> addMultiCellWidget(m_chkColorMask, 1, 1, 0, 1);
}

KisBrushChooser::~KisBrushChooser()
{
}

void KisBrushChooser::slotSetItemSpacing(int spacingValue)
{
	KisIconItem *item = static_cast<KisIconItem *>(currentItem());

	if (item) {
		KisBrush *brush = static_cast<KisBrush *>(item -> resource());
		brush -> setSpacing(spacingValue * 10);
	}
}

void KisBrushChooser::slotSetItemUseColorAsMask(bool useColorAsMask)
{
	KisIconItem *item = static_cast<KisIconItem *>(currentItem());

	if (item) {
		KisBrush *brush = static_cast<KisBrush *>(item -> resource());
		brush -> setUseColorAsMask(useColorAsMask);
		item -> updatePixmaps();
		// The item's pixmaps may have changed so get observers to update
		// their display.
		notify();
	}
}

void KisBrushChooser::update(KoIconItem *item)
{
	KisIconItem *kisItem = static_cast<KisIconItem *>(item);

	if (kisItem) {
		KisBrush *brush = static_cast<KisBrush *>(kisItem -> resource());

		QString text = QString("%1 (%2 x %3)").arg(brush -> name()).arg(brush -> width()).arg(brush -> height());

		m_lbName -> setText(text);
		m_slSpacing -> setValue(brush -> spacing() / 10);
		m_chkColorMask -> setChecked(brush -> useColorAsMask());
		m_chkColorMask -> setEnabled(brush -> hasColor());
	}
}

#include "kis_brush_chooser.moc"

