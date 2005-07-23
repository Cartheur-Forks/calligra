/*
 * tool_polygon.cc -- Part of Krita
 *
 * Copyright (c) 2004 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
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
#include <stdlib.h>
#include <vector>

#include <qpoint.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kgenericfactory.h>

#include <kis_global.h>
#include <kis_types.h>
#include <kis_tool_registry.h>
#include <kis_factory.h>
#include <kis_view.h>

#include "tool_polygon.h"
#include "tool_polygon.moc"
#include "kis_tool_polygon.h"


typedef KGenericFactory<ToolPolygon> ToolPolygonFactory;
K_EXPORT_COMPONENT_FACTORY( kritatoolpolygon, ToolPolygonFactory( "krita" ) )


ToolPolygon::ToolPolygon(QObject *parent, const char *name, const QStringList &)
	: KParts::Plugin(parent, name)
{
	setInstance(ToolPolygonFactory::instance());

	kdDebug(DBG_AREA_PLUGINS) << "Polygon tool plugin. Class: "
		<< className()
		<< ", Parent: "
		<< parent -> className()
		<< "\n";

	if ( parent->inherits("KisFactory") )
	{
		KisToolRegistry * r = KisToolRegistry::instance();

		r -> add(new KisToolPolygonFactory(actionCollection()));
	}

}

ToolPolygon::~ToolPolygon()
{
}

//#include "tool_polygon.moc"
