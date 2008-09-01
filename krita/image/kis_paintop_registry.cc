/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_paintop_registry.h"
#include <QPixmap>
#include <QWidget>

#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kparts/plugin.h>
#include <kservice.h>
#include <kparts/componentfactory.h>
#include <kservicetypetrader.h>

#include <KoGenericRegistry.h>
#include <KoPluginLoader.h>
#include <KoColorSpace.h>
#include <KoCompositeOp.h>
#include <KoID.h>

#include "kis_types.h"

#include "kis_paint_device.h"
#include "kis_paintop.h"
#include "kis_painter.h"
#include "kis_debug.h"
#include "kis_layer.h"
#include "kis_image.h"
#include "kis_config_widget.h"

KisPaintOpRegistry * KisPaintOpRegistry::m_singleton = 0;

KisPaintOpRegistry::KisPaintOpRegistry()
{
    Q_ASSERT(KisPaintOpRegistry::m_singleton == 0);
}

KisPaintOpRegistry::~KisPaintOpRegistry()
{
}

KisPaintOpRegistry* KisPaintOpRegistry::instance()
{
    if (KisPaintOpRegistry::m_singleton == 0) {
        KisPaintOpRegistry::m_singleton = new KisPaintOpRegistry();
        KoPluginLoader::instance()->load("Krita/Paintop", "(Type == 'Service') and ([X-Krita-Version] == 3)");
        Q_CHECK_PTR(KisPaintOpRegistry::m_singleton);
    }
    return KisPaintOpRegistry::m_singleton;
}

KisPaintOp * KisPaintOpRegistry::paintOp(const QString & id, const KisPaintOpSettingsSP settings, KisPainter * painter, KisImageSP image) const
{
    if (painter == 0) {
        kWarning() << " KisPaintOpRegistry::paintOp painter is null";
        return 0;
    }

    if (!painter->bounds().isValid() && image) {
        painter->setBounds(image->bounds());
    }

    KisPaintOpFactory* f = value(id);
    if (f) {
        return f->createOp(settings, painter, image);
    }
    return 0;
}

KisPaintOp * KisPaintOpRegistry::paintOp(const KisPaintOpPresetSP preset, KisPainter * painter, KisImageSP image) const
{
    Q_ASSERT(preset);
    if (!preset) return 0;
    return paintOp(preset->paintOp().id(), preset->settings(), painter, image);
}

KisPaintOpSettingsSP KisPaintOpRegistry::settings(const KoID& id, QWidget * parent, const KoInputDevice& inputDevice, KisImageSP image) const
{
    KisPaintOpFactory* f = value(id.id());
    if (f)
        return f->settings(parent, inputDevice, image);

    return 0;
}

KisPaintOpPresetSP KisPaintOpRegistry::defaultPreset(const KoID& id, QWidget * parent, const KoInputDevice& inputDevice, KisImageSP image) const
{
    KisPaintOpPresetSP preset = new KisPaintOpPreset();
    preset->setName(i18n("default"));
    preset->setSettings(settings(id, parent, inputDevice, image));
    if (preset->settings() && preset->settings()->widget()) {
        preset->settings()->widget()->hide();
    }
    preset->setPaintOp(id);
    preset->setValid(true);
    return preset;
}

bool KisPaintOpRegistry::userVisible(const KoID & id, const KoColorSpace* cs) const
{

    KisPaintOpFactory* f = value(id.id());
    if (!f) {
        dbgRegistry << "No paintop" << id.id() << "";
        return false;
    }
    return f->userVisible(cs);

}

QString KisPaintOpRegistry::pixmap(const KoID & id) const
{
    KisPaintOpFactory* f = value(id.id());

    if (!f) {
        dbgRegistry << "No paintop" << id.id() << "";
        return "";
    }

    return f->pixmap();
}

#include "kis_paintop_registry.moc"
