/*
 *  main.cc - part of KImageShop
 *
 *  Copyright (c) 1999 Matthias Elter <me@kde.org>
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
 */

#include <dcopclient.h>

#include <klocale.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>

#include <koApplication.h>
#include <koDocument.h>
#include <koMainWindow.h>
#include <koQueryTypes.h>

static const char *description=I18N_NOOP("A KDE Image Processor");

static const KCmdLineOptions options[] =
{
	{ "+[file(s)]", I18N_NOOP( "File(s) or URL(s) to Open" ), 0 },
	{ 0, 0, 0 }
};

int main( int argc, char **argv )
{
    KAboutData aboutData( "kimageshop", I18N_NOOP("KImageShop"),
        "0.0.1", description, KAboutData::License_GPL, "(c) 1999, Michael Koch",
	0, "http://koffice.kde.org/kimageshop/" );
    aboutData.addAuthor("Matthias Elter", 0, "me@kde.org");
    aboutData.addAuthor("Carsten Pfeiffer", 0, "carpdjih@cetus.zrz.tu-berlin.de");
    aboutData.addAuthor("Michael Koch", 0, "koch@kde.org");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options );

    KoApplication app( argc, argv, "kimageshop" );
    //KoApplication app;

    app.dcopClient()->attach();
    app.dcopClient()->registerAs( "kimageshop" );

    app.start();
    app.exec();

    return 0;
}
