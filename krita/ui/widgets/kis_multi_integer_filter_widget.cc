/*
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#include "kis_multi_integer_filter_widget.h"

#include <qlabel.h>
#include <qlayout.h>

#include <knuminput.h>
#include <klocale.h>

#include "kis_filter.h"

KisIntegerWidgetParam::KisIntegerWidgetParam(  Q_INT32 nmin, Q_INT32 nmax, Q_INT32 ninitvalue, QString nname) :
	min(nmin),
	max(nmax),
	initvalue(ninitvalue),
	name(nname)
{

}

KisMultiIntegerFilterWidget::KisMultiIntegerFilterWidget( KisFilter* nfilter, QWidget * parent, const char * name, const char * caption, vKisIntegerWidgetParam iwparam) : QWidget( parent, name )
{
	Q_INT32 m_nbintegerWidgets = iwparam.size();

	this->setCaption(caption);

	QGridLayout *widgetLayout = new QGridLayout(this, m_nbintegerWidgets + 1, 3);
	widgetLayout -> setColStretch ( 1, 1 );

	m_integerWidgets = new KIntNumInput*[ m_nbintegerWidgets ];

	for( Q_INT32 i = 0; i < m_nbintegerWidgets; ++i)
	{
		m_integerWidgets[i] = new KIntNumInput( this, iwparam[i].name.ascii());
		m_integerWidgets[i] -> setRange( iwparam[i].min, iwparam[i].max);
		m_integerWidgets[i] -> setValue( iwparam[i].initvalue );

		connect(m_integerWidgets[i], SIGNAL(valueChanged( int )), nfilter, SLOT(refreshPreview()));

		QLabel* lbl = new QLabel(iwparam[i].name+":", this);
		widgetLayout -> addWidget( lbl, i , 0);

		widgetLayout -> addWidget( m_integerWidgets[i], i , 1);
	}
	QSpacerItem * sp = new QSpacerItem(1, 1);
	widgetLayout -> addItem(sp, m_nbintegerWidgets, 0);

}

#include "kis_multi_integer_filter_widget.moc"
