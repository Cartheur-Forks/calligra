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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <limits.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qslider.h>
#include <qstring.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <knuminput.h>
#include "kis_global.h"
#include "kis_dlg_paint_properties.h"
#include "integerwidget.h"

KisPaintPropertyDlg::KisPaintPropertyDlg(const QString& deviceName, const QPoint& pos, Q_INT32 opacity, QWidget *parent, const char *name, WFlags f)
	: super(parent, name, f, name, Ok | Cancel)
{
	QWidget *page = new QWidget(this);
	QVBoxLayout *layout;
	QGridLayout *grid;
	QGridLayout *gridInBox;
	QGroupBox *grp;
	QLabel *lbl;

	opacity = downscale(opacity);
	opacity = opacity * 100 / 255;

	if (opacity)
		opacity++;

	setCaption(i18n("Device Properties"));
	setMainWidget(page);
	layout = new QVBoxLayout(page, 3);
	grid = new QGridLayout(layout, 5, 2, 3);

	lbl = new QLabel(i18n("Name:"), page);
	m_name = new KLineEdit(deviceName, page);
	grid -> addWidget(lbl, 0, 0);
	grid -> addWidget(m_name, 0, 1);

	lbl = new QLabel(i18n("Opacity:"), page);
#if 0
	IntegerWidget *m_opacity = new IntegerWidget(0, 100, page);
#else
	m_opacity = new KIntNumInput(page);
	m_opacity -> setRange(0, 100, 13, true);
	m_opacity -> setValue(opacity);
#endif
	grid -> addWidget(lbl, 1, 0);
	grid -> addWidget(m_opacity, 1, 1);

	grp = new QGroupBox(i18n("Position"), page);
	gridInBox = new QGridLayout(grp, 2, 2, 12);

	lbl = new QLabel(i18n("X axis:"), grp);
	m_x = new KIntSpinBox(SHRT_MIN, SHRT_MAX, 10, pos.x(), 10, grp);
	gridInBox -> addWidget(lbl, 0, 0);
	gridInBox -> addWidget(m_x, 0, 1);

	lbl = new QLabel(i18n("Y axis:"), grp);
	m_y = new KIntSpinBox(SHRT_MIN, SHRT_MAX, 10, pos.y(), 10, grp);
	gridInBox -> addWidget(lbl, 1, 0);
	gridInBox -> addWidget(m_y, 1, 1);

	grid -> addMultiCellWidget(grp, 3, 4, 0, 1);
}

KisPaintPropertyDlg::~KisPaintPropertyDlg()
{
}

QString KisPaintPropertyDlg::getName() const
{
	return m_name -> text();
}

int KisPaintPropertyDlg::getOpacity() const
{
	Q_INT32 opacity = m_opacity -> value();

	if (!opacity)
		return 0;

	opacity = opacity * 255 / 100;
	return upscale(opacity - 1);
}

QPoint KisPaintPropertyDlg::getPosition() const
{
	return QPoint(m_x -> value(), m_y -> value());
}

