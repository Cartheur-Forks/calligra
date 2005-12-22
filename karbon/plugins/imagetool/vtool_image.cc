/* This file is part of the KDE project
   Copyright (C) 2002, The Karbon Developers

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.

*/

#include <kgenericfactory.h>

#include "karbon_factory.h"
#include "karbon_tool_factory.h"
#include "karbon_tool_registry.h"

#include "vtool_image.h"

#include "imagetoolplugin.h"

typedef KGenericFactory<VToolImage> VToolImageFactory;
K_EXPORT_COMPONENT_FACTORY( karbonvtoolimage, VToolImageFactory( "karbonimagetoolplugin" ) )

VToolImage::VToolImage(QObject *parent, const char *name, const QStringList &) : KParts::Plugin(parent, name)
{
	setInstance(VToolImageFactory::instance());

	kdDebug() << "VToolImage. Class: "
		<< className()
		<< ", Parent: "
		<< parent -> className()
		<< "\n";

	if ( parent->inherits("KarbonFactory") )
	{
		KarbonToolRegistry* r = KarbonToolRegistry::instance();
		r->add(new KarbonToolFactory<VImageTool>());
	}
}

VToolImage::~VToolImage()
{
}

#include "vtool_image.moc"

