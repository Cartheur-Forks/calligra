/* This file is part of the KDE project
   Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kptfactory.h"
#include "kptpart.h"
#include "kptaboutdata.h"
#include <kinstance.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>

using namespace KPlato;

extern "C"
{
    void* init_libkplatopart()
    {
        return new KPTFactory;
    }
};

KInstance* KPTFactory::s_global = 0L;
KAboutData* KPTFactory::s_aboutData = 0L;

KPTFactory::KPTFactory( QObject* parent, const char* name )
    : KoFactory( parent, name )
{
    global();
}

KPTFactory::~KPTFactory()
{
    delete s_aboutData;
    s_aboutData = 0L;
    delete s_global;
    s_global = 0L;
}

KParts::Part *KPTFactory::createPartObject(QWidget *parentWidget,
					   const char *widgetName,
					   QObject* parent, const char* name,
					   const char* classname,
					   const QStringList &)
{
    // If classname is "KoDocument", our host is a koffice application
    // otherwise, the host wants us as a simple part, so switch to readonly
    // and single view.
    bool bWantKoDocument = (strcmp(classname, "KoDocument") == 0);

    // parentWidget and widgetName are used by KoDocument for the
    // "readonly+singleView" case.
    KPTPart *part = new KPTPart(parentWidget, widgetName, parent, name,
				!bWantKoDocument);

    if (!bWantKoDocument)
      part->setReadWrite(false);

    return part;
}

KAboutData* KPTFactory::aboutData()
{
    if ( !s_aboutData )
        s_aboutData = newKPTAboutData();
    return s_aboutData;
}

KInstance* KPTFactory::global()
{
    if ( !s_global )
    {
        s_global = new KInstance( aboutData() );

        // Add any application-specific resource directories here
	s_global->dirs()->addResourceType("kplato_template",
					  KStandardDirs::kde_default("data") + "kplato/templates/");
	s_global->dirs()->addResourceType( "expression", KStandardDirs::kde_default("data") + "kplato/expression/");
	s_global->dirs()->addResourceType("toolbar",
					  KStandardDirs::kde_default("data") + "koffice/toolbar/");

        // Tell the iconloader about share/apps/koffice/icons
        s_global->iconLoader()->addAppDir("koffice");
    }
    return s_global;
}

#include "kptfactory.moc"
