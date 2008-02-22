/*
* lms_f32_plugin.cc -- Part of Krita
*
* Copyright (c) 2004 Boudewijn Rempt (boud@valdyas.org)
* Copyright (c) 2005 Adrian Page <adrian@pagenet.plus.com>
*  Copyright (c) 2005 Cyrille Berger <cberger@cberger.net>
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
*  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "lms_f32_plugin.h"

#include <kcomponentdata.h>
#include <kgenericfactory.h>
#include <KoColorSpaceRegistry.h>
#include <kis_basic_histogram_producers.h>


#include "kis_lms_f32_colorspace.h"

typedef KGenericFactory<LMSF32Plugin> LMSF32PluginFactory;
K_EXPORT_COMPONENT_FACTORY( krita_lms_f32_plugin, LMSF32PluginFactory( "krita" ) )


LMSF32Plugin::LMSF32Plugin(QObject *parent, const QStringList &)
    : KParts::Plugin(parent)
{
    setComponentData(LMSF32PluginFactory::componentData());

    if ( parent->inherits("KoColorSpaceRegistry") )
    {
	KoColorSpaceRegistry * f = dynamic_cast<KoColorSpaceRegistry*>(parent);

        KoColorSpace * colorSpaceLMSF32  = new KisLmsF32ColorSpace(f, 0);

        KoColorSpaceFactory * csf  = new KisLmsF32ColorSpaceFactory();
        f->add(csf);

        KisHistogramProducerFactoryRegistry::instance()->add(
            new KisBasicHistogramProducerFactory<KisBasicF32HistogramProducer>
            (KoID("LMSF32HISTO", i18n("Float32 Histogram")), colorSpaceLMSF32) );
    }

}

LMSF32Plugin::~LMSF32Plugin()
{
}

#include "lms_f32_plugin.moc"
