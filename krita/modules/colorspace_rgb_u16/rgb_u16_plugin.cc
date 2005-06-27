/*
* rgb_u16_plugin.cc -- Part of Krita
*
* Copyright (c) 2004 Boudewijn Rempt (boud@valdyas.org)
* Copyright (c) 2005 Adrian Page <adrian@pagenet.plus.com>
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

#include <kinstance.h>
#include <kgenericfactory.h>
#include <kdebug.h>

#include <kis_global.h>
#include <kis_colorspace_registry.h>
#include <kis_factory.h>

#include "rgb_u16_plugin.h"
#include "kis_strategy_colorspace_rgb_u16.h"

typedef KGenericFactory<RGBU16Plugin> RGBU16PluginFactory;
K_EXPORT_COMPONENT_FACTORY( krita_rgb_u16_plugin, RGBU16PluginFactory( "krita" ) )


RGBU16Plugin::RGBU16Plugin(QObject *parent, const char *name, const QStringList &)
	: KParts::Plugin(parent, name)
{
		setInstance(RGBU16PluginFactory::instance());

	kdDebug(DBG_AREA_PLUGINS) << "16-bit integer RGB Color model plugin. Class: "
		<< className()
		<< ", Parent: "
		<< parent -> className()
		<< "\n";

	if ( parent->inherits("KisFactory") )
	{
		m_StrategyColorSpaceRGBU16 = new KisStrategyColorSpaceRGBU16();
		Q_CHECK_PTR(m_StrategyColorSpaceRGBU16);
		KisColorSpaceRegistry::instance() -> add(m_StrategyColorSpaceRGBU16);
	}

}

RGBU16Plugin::~RGBU16Plugin()
{
}

#include "rgb_u16_plugin.moc"
