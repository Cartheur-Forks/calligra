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
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qtextedit.h>
#include <qcheckbox.h>
#include <qwidget.h>
#include <qcursor.h>

#include <klocale.h>
#include <kcolorcombo.h>
#include <kdebug.h>

#include <koUnitWidgets.h>

#include "kis_colorspace_registry.h"
#include "kis_dlg_create_img.h"
#include "wdgnewimage.h"
#include "kis_profile.h"
#include "kis_resource.h"
#include "kis_resourceserver.h"
#include "kis_factory.h"
#include "kis_id.h"
#include "kis_cmb_idlist.h"

KisDlgCreateImg::KisDlgCreateImg(Q_INT32 maxWidth, Q_INT32 defWidth, 
				 Q_INT32 maxHeight, Q_INT32 defHeight, 
				 QString colorStrategyName, QString imageName,
				 QWidget *parent, const char *name)
	: super(parent, name, true, "", Ok | Cancel)
{

	setCaption(i18n("New Image"));

	m_page = new WdgNewImage(this);

	setMainWidget(m_page);
	resize(m_page -> sizeHint());

	m_page -> txtName -> setText(imageName);

	m_page -> intWidth -> setValue(defWidth);
	m_page -> intWidth -> setMaxValue(maxWidth);
	m_page -> intHeight -> setValue(defHeight);
	m_page -> intHeight -> setMaxValue(maxHeight);
	m_page -> doubleResolution -> setValue(100.0); // XXX: Get this from settings?

	m_page -> cmbColorSpaces -> setIDList(KisColorSpaceRegistry::instance() -> listKeys());
	m_page -> cmbColorSpaces -> setCurrentText(colorStrategyName);

	connect(m_page -> cmbColorSpaces, SIGNAL(activated(const KisID &)), 
		this, SLOT(fillCmbProfiles(const KisID &)));

	// Temporary KisID; this will be matched to the translated ID in the current KisIDList.
	fillCmbProfiles(KisID(colorStrategyName, ""));

}

KisDlgCreateImg::~KisDlgCreateImg()
{
	delete m_page;
}

Q_INT32 KisDlgCreateImg::imgWidth() const
{
	return m_page -> intWidth -> value();
}

Q_INT32 KisDlgCreateImg::imgHeight() const
{
	return m_page -> intHeight -> value();
}

KisID KisDlgCreateImg::colorStrategyID() const
{
	return m_page -> cmbColorSpaces -> currentItem();
}

QColor KisDlgCreateImg::backgroundColor() const
{
	return QColor(m_page -> cmbColor -> color());
}

QUANTUM KisDlgCreateImg::backgroundOpacity() const
{
	// XXX: This widget is sizeof quantum dependent. Scale
	// to selected bit depth.
	Q_INT32 opacity = m_page -> sliderOpacity -> value();

	if (!opacity)
		return 0;

	opacity = opacity * 255 / 100;
	return upscale(opacity - 1);

}

QString KisDlgCreateImg::imgName() const
{
	return m_page -> txtName -> text();
}

double KisDlgCreateImg::imgResolution() const
{
	return m_page -> doubleResolution -> value();
 }

QString KisDlgCreateImg::imgDescription() const
{
	return m_page -> txtDescription -> text();
}

KisProfileSP KisDlgCreateImg::profile() const
{
	KisStrategyColorSpaceSP cs = KisColorSpaceRegistry::instance() -> get(m_page -> cmbColorSpaces -> currentItem());
	if (!cs) return 0;

	vKisProfileSP resourceslist = cs -> profiles();
	Q_UINT32 index = m_page -> cmbProfile -> currentItem();
	
	if (resourceslist.count() == 0 || 
	    index > resourceslist.count() ||
	    index == 0) { 
		return 0;
	}
	else {
		return resourceslist.at(index - 1);
	}
	
}

void KisDlgCreateImg::fillCmbProfiles(const KisID & s)
{


	m_page -> cmbProfile -> clear();
	m_page -> cmbProfile -> insertItem(i18n("None"));

	kdDebug() << "Colorspace: " << s.id() << "\n";
	KisStrategyColorSpaceSP cs = KisColorSpaceRegistry::instance() -> get(s);
	kdDebug() << "Instance: " << cs << "\n";

	if (cs == 0) return;

	vKisProfileSP profileList = cs -> profiles();
        vKisProfileSP::iterator it;
        for ( it = profileList.begin(); it != profileList.end(); ++it ) {
			m_page -> cmbProfile -> insertItem((*it) -> productName());
	}
	

}

#include "kis_dlg_create_img.moc"

