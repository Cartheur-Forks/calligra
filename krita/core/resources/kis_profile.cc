/*
 *  kis_profile.cc - part of Krayon
 *
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 *                2001 John Califf
 *                2004 Boudewijn Rempt <boud@valdyas.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */



#include <cfloat>
#include <cmath>
#include <config.h>
#include LCMS_HEADER

#include <qimage.h>
#include <qtextstream.h>
#include <qfile.h>

#include <kdebug.h>

#include "kis_profile.h"
#include "kis_annotation.h"

#include "ksharedptr.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>


KisProfile::KisProfile(Q_UINT32 colorType)
	: super(QString()),
	  m_lcmsColorType(colorType)
{
}

KisProfile::KisProfile(QByteArray rawData, Q_UINT32 colorType)
	: super (QString()),
	  m_lcmsColorType(colorType),
	  m_rawData(rawData)
{
	m_profile = cmsOpenProfileFromMem(rawData.data(), (DWORD)rawData.size());
	init();
}

KisProfile::KisProfile(const QString& file, Q_UINT32 colorType) 
	: super(file),
	  m_lcmsColorType(colorType)
{
}

KisProfile::KisProfile(cmsHPROFILE profile, QByteArray rawData, Q_UINT32 colorType)
	: super (QString()),
	  m_profile(profile),
	  m_lcmsColorType(colorType),
	  m_rawData(rawData)
{
	init();
}

KisProfile::KisProfile(const cmsHPROFILE profile, Q_UINT32 colorType)
	: super (QString()),
	  m_profile(profile),
	  m_lcmsColorType(colorType)
{
	init();
}

KisProfile::~KisProfile()
{
	cmsCloseProfile(m_profile);
}


bool KisProfile::load()
{
	//cmsErrorAction(LCMS_ERROR_IGNORE);

 	//m_profile = cmsOpenProfileFromFile(filename().ascii(), "r");
	// XXX this should be more efficient: we load the file twice
	QFile file(filename());
	file.open(IO_ReadOnly);
	m_rawData = file.readAll();
	m_profile = cmsOpenProfileFromMem(m_rawData.data(), (DWORD)m_rawData.size());
	file.close();

	return init();

}

bool KisProfile::init() 
{
	if (m_profile) {
		m_colorSpaceSignature = cmsGetColorSpace(m_profile);
		m_deviceClass = cmsGetDeviceClass(m_profile);
		m_productName = cmsTakeProductName(m_profile);
		m_productDescription = cmsTakeProductDesc(m_profile);
		m_productInfo = cmsTakeProductInfo(m_profile);
		setValid(true);
		return true;
	}
	return false;
}

bool KisProfile::save()
{
	return false;
}

QImage KisProfile::img()
{
	return QImage();
}

KisAnnotationSP KisProfile::annotation() const
{
	// XXX we hardcode icc, this is correct for lcms?
	// XXX productName(), or just "ICC Profile"?
	if (!m_rawData.isEmpty())
		return new KisAnnotation("icc", productName(), m_rawData);
}

KisProfileSP KisProfile::getScreenProfile (int screen)
{

#ifdef Q_WS_X11

	Atom type;
	int format;
	unsigned long nitems;
	unsigned long bytes_after;
	Q_UINT8 * str;
	
	cmsHPROFILE profile = NULL;

	static Atom icc_atom = XInternAtom( qt_xdisplay(), "_ICC_PROFILE", False );

	if  ( XGetWindowProperty ( qt_xdisplay(),
					qt_xrootwin( screen ),
					icc_atom,
					0,
					INT_MAX,
					False,
					XA_CARDINAL,
					&type,
					&format,
					&nitems,
					&bytes_after,
					(unsigned char **) &str)
				) {
		if ( nitems )
 			profile =
				cmsOpenProfileFromMem(&str,
							nitems * sizeof(long));
		
		QByteArray bytes (nitems);
		bytes.assign((char*)str, (Q_UINT32)nitems);
		
		return new KisProfile(profile, bytes, TYPE_BGRA_8);
	} else {
		kdDebug(DBG_AREA_CMS) << "No profile set for X11, not correcting" << endl;
		return NULL;
	}
#else
	return NULL;
	
#endif
}

#include "kis_profile.moc"

