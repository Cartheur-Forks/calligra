/*
 *  kis_aboutdata.h - part of Krayon
 *
 *  Copyright (c) 1999-2000 Matthias Elter  <me@kde.org>
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
#if !defined KIS_ABOUT_DATA_H_
#define KIS_ABOUT_DATA_H_

#include <kaboutdata.h>
#include <klocale.h>
#include <config.h>

KAboutData * newKrayonAboutData()
{
	KAboutData * aboutData = new KAboutData( "krita",
			I18N_NOOP("Krita"),
			VERSION,
			I18N_NOOP("KOffice image manipulation application."),
			KAboutData::License_GPL,
			I18N_NOOP("(c) 1999-2002 The Krita team.\n(c) 2002-2003 Patrick Julien."),
			"",
			"http://www.koffice.org/krita",
			"submit@bugs.kde.org");
	aboutData->addAuthor("Matthias Elter", 0, "me@kde.org");
	aboutData->addAuthor("Carsten Pfeiffer", 0, "carpdjih@cetus.zrz.tu-berlin.de");
	aboutData->addAuthor("Michael Koch", 0, "koch@kde.org");
	aboutData->addAuthor("John Califf",0, "jcaliff@compuzone.net");
	aboutData->addAuthor("Laurent Montel",0, "lmontel@mandrakesoft.com");
	aboutData->addAuthor("Toshitaka Fujioka", 0, "fujioka@kde.org");
	aboutData->addAuthor("Patrick Julien", 0, "freak@codepimps.org");
	aboutData->addAuthor("Boudewijn Rempt", 0, "boud@valdyas.org");
	return aboutData;
}

#endif // KRAYON_ABOUTDATA
