/*
 *  kis_factory.h - part of Krayon
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __kis_factory_h__
#define __kis_factory_h__

#include <koFactory.h>

class KInstance;
class KisPluginServer;
class KisResourceServer;
class KAboutData;

class KisFactory : public KoFactory
{
    Q_OBJECT
public:
    KisFactory( QObject* parent = 0, const char* name = 0 );
    ~KisFactory();

    virtual KParts::Part *createPartObject( 
        QWidget *parentWidget = 0, 
        const char *widgetName = 0, 
        QObject *parent = 0, 
        const char *name = 0, 
        const char *classname = "KoDocument", 
        const QStringList &args = QStringList() ); 

    static KAboutData        *aboutData();
    static KInstance         *global();
    static KisPluginServer   *pServer();
    static KisResourceServer *rServer();

private:
    static KInstance           *s_global;
    static KisPluginServer     *s_pserver;
    static KisResourceServer   *s_rserver;
    static KAboutData          *s_aboutData;
};

#endif
