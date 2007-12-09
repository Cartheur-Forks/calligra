/*
 *  Copyright (c) 2003 Patrick Julien  <freak@codepimps.org>
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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
#include <QString>

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kparts/plugin.h>
#include <kservice.h>
#include <kservicetypetrader.h>

#include <kparts/componentfactory.h>
#include "kis_debug_areas.h"
#include <math.h>
#include "kis_types.h"
#include "kis_filter_registry.h"
#include "kis_paint_device.h"
#include "kis_filter.h"

KisFilterRegistry *KisFilterRegistry::m_singleton = 0;

KisFilterRegistry::KisFilterRegistry()
{
    Q_ASSERT(KisFilterRegistry::m_singleton == 0);
    KisFilterRegistry::m_singleton = this;

    KService::List  offers = KServiceTypeTrader::self()->query(QString::fromLatin1("Krita/Filter"),
                                                         QString::fromLatin1("(Type == 'Service') and "
                                                                             "([X-Krita-Version] == 3)"));

    KService::List::ConstIterator iter;

    for(iter = offers.begin(); iter != offers.end(); ++iter)
    {
        KService::Ptr service = *iter;
        int errCode = 0;
        KParts::Plugin* plugin =
             KService::createInstance<KParts::Plugin> ( service, this, QStringList(), &errCode);
        if ( !plugin ) {
            kDebug(41006) <<"found plugin" << service->property("Name").toString() <<"," << errCode <<"";
            if( errCode == KLibLoader::ErrNoLibrary)
            {
                kWarning(41006) <<" Error loading plugin was : ErrNoLibrary" << KLibLoader::self()->lastErrorMessage();
            }
        }

    }
}

KisFilterRegistry::~KisFilterRegistry()
{
}

KisFilterRegistry* KisFilterRegistry::instance()
{
    if(KisFilterRegistry::m_singleton == 0)
    {
        KisFilterRegistry::m_singleton = new KisFilterRegistry();
    }
    return KisFilterRegistry::m_singleton;
}

void KisFilterRegistry::add(KisFilterSP item)
{
    add(item->id(), item);
}

void KisFilterRegistry::add(const QString &id, KisFilterSP item)
{
    KoGenericRegistry<KisFilterSP>::add(id, item);
    emit(filterAdded(id));
}

#include "kis_filter_registry.moc"
