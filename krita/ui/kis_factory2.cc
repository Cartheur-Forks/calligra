/*
 *  kis_factory.cc - part of Krayon
 *
 *  Copyright (c) 1999 Matthias Elter <elter@kde.org>
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
#include <lcms.h>

#include <QStringList>
#include <QDir>

#include <kdebug.h>
#include <kcomponentdata.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kparts/plugin.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <kparts/componentfactory.h>


#include <KoPluginLoader.h>
#include <KoShapeRegistry.h>

#include "kis_aboutdata.h"

#include "kis_shape_selection.h"
#include "kis_doc2.h"
#include "kis_factory2.h"

KAboutData* KisFactory2::s_aboutData = 0;
KComponentData* KisFactory2::s_instance = 0;



KisFactory2::KisFactory2( QObject* parent )
    : KoFactory( parent )
{
    s_aboutData = newKritaAboutData();

    (void)componentData();
}

KisFactory2::~KisFactory2()
{
    delete s_aboutData;
    s_aboutData = 0;
    delete s_instance;
    s_instance = 0;
}

/**
 * Create the document
 */
KParts::Part* KisFactory2::createPartObject( QWidget *parentWidget,
                        QObject* parent,
                        const char* classname, const QStringList & )
{
    bool bWantKoDocument = ( strcmp( classname, "KoDocument" ) == 0 );

    KisDoc2 *doc = new KisDoc2( parentWidget, parent, !bWantKoDocument );
    Q_CHECK_PTR(doc);

    if ( !bWantKoDocument )
        doc->setReadWrite( false );

    return doc;
}


KAboutData* KisFactory2::aboutData()
{
    return s_aboutData;
}

const KComponentData &KisFactory2::componentData()
{
    QString homedir = getenv("HOME");

    if ( !s_instance )
    {
        if ( s_aboutData )
            s_instance = new KComponentData(s_aboutData);
        else
            s_instance = new KComponentData( newKritaAboutData() );
        Q_CHECK_PTR(s_instance);

        KoShapeRegistry* r = KoShapeRegistry::instance();
        r->add( new KisShapeSelectionFactory(r) );

        // Load the krita-specific tools
        KoPluginLoader::instance()->load(QString::fromLatin1("Krita/Tool"),
                                         QString::fromLatin1("[X-Krita-Version] == 3"));


        s_instance->dirs()->addResourceType("krita_template", "data", "krita/templates");

        // XXX: Are these obsolete?
        s_instance->dirs()->addResourceType("kis", "data", "krita/");
        s_instance->dirs()->addResourceType("kis_pics", "data", "krita/pics/");
        s_instance->dirs()->addResourceType("kis_images", "data", "krita/images/");
        s_instance->dirs()->addResourceType("toolbars", "data", "koffice/toolbar/");

        // Create spec

        s_instance->dirs()->addResourceType("kis_brushes", "data", "krita/brushes/");
        s_instance->dirs()->addResourceDir("kis_brushes", "/usr/share/create/brushes/gimp");
        s_instance->dirs()->addResourceDir("kis_brushes", QDir::homePath() + QString("/.create/brushes/gimp"));

        s_instance->dirs()->addResourceType("kis_patterns", "data", "krita/patterns/");
        s_instance->dirs()->addResourceDir("kis_patterns", "/usr/share/create/patterns/gimp");
        s_instance->dirs()->addResourceDir("kis_patterns", QDir::homePath() + QString("/.create/patterns/gimp"));

        s_instance->dirs()->addResourceType("kis_gradients", "data", "krita/gradients/");
        s_instance->dirs()->addResourceDir("kis_gradients", "/usr/share/create/gradients/gimp");
        s_instance->dirs()->addResourceDir("kis_gradients", QDir::homePath() + QString("/.create/gradients/gimp"));

        s_instance->dirs()->addResourceType("kis_profiles", "data", "krita/profiles/");

        s_instance->dirs()->addResourceType("kis_palettes", "data", "krita/palettes/");
        s_instance->dirs()->addResourceDir("kis_palettes", "/usr/share/create/swatches");
        s_instance->dirs()->addResourceDir("kis_palettes", QDir::homePath() + QString("/.create/swatches"));

        s_instance->dirs()->addResourceType("kis_shaders", "data", "krita/shaders/");

        // Tell the iconloader about share/apps/koffice/icons
        KIconLoader::global()->addAppDir("koffice");



    }

    return *s_instance;
}

#include "kis_factory2.moc"
