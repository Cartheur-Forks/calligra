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

#include "kis_filter.h"

#include <qlayout.h>
#include <qframe.h>

#include "kis_filter_registry.h"
#include "kis_tile_command.h"
#include "kis_undo_adapter.h"
#include "kis_filter_configuration_widget.h"
#include "kis_preview_dialog.h"
#include "kis_previewwidget.h"

KisFilter::KisFilter(const QString& name, KisView * view) :
	m_name(name),
	m_view(view),
	m_dialog(0)
{
}

KisFilterConfiguration* KisFilter::configuration(KisFilterConfigurationWidget*)
{
	return 0;
}

void KisFilter::refreshPreview( )
{
	if( m_dialog == 0 )
		return;
	m_dialog -> previewWidget -> slotRenewLayer();
	KisLayerSP layer = m_dialog -> previewWidget -> getLayer();
	KisFilterConfiguration* config = configuration(m_widget);
	QRect rect(0, 0, layer -> width(), layer -> height());
	process((KisPaintDeviceSP) layer, config, rect, 0 );
	m_dialog->previewWidget -> slotUpdate();
}

KisFilterConfigurationWidget* KisFilter::createConfigurationWidget(QWidget* )
{
	return 0;
}

void KisFilter::slotActivated()
{
	KisImageSP img = m_view -> currentImg();
	if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	// Create the config dialog
	m_dialog = new KisPreviewDialog( (QWidget*) m_view, name().ascii(), true);
	m_widget = createConfigurationWidget( (QWidget*)m_dialog->container );

	if( m_widget != 0)
	{
		m_dialog->previewWidget->slotSetLayer( layer );
		connect(m_dialog->previewWidget, SIGNAL(updated()), this, SLOT(refreshPreview()));
		QGridLayout *widgetLayout = new QGridLayout((QWidget *)m_dialog->container, 1, 1);
		widgetLayout -> addWidget(m_widget, 0 , 0);
		m_dialog->container->setMinimumSize(m_widget->minimumSize());
		refreshPreview();
		m_dialog -> setFixedSize(m_dialog -> minimumSize());
		if(m_dialog->exec() == QDialog::Rejected )
		{
			delete m_dialog;
			return;
		}
	}

	//Apply the filter
	KisFilterConfiguration* config = configuration(m_widget);
	KisTileCommand* ktc = new KisTileCommand(name(), (KisPaintDeviceSP)layer ); // Create a command
	QRect rect(0, 0, layer->width(), layer->height());

	process((KisPaintDeviceSP)layer, config, rect, ktc);

	img->undoAdapter()->addCommand( ktc );
	img->notify();
	delete m_dialog;
	m_dialog = 0;
	delete config;
}
#include "kis_filter.moc"
