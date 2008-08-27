/*
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_paintop_settings.h"

#include <QWidget>

#include <KoPointerEvent.h>

#include "kis_node.h"

struct KisPaintOpSettings::Private {
    KisNodeSP node;
};

KisPaintOpSettings::KisPaintOpSettings() : d(new Private)
{
}

KisPaintOpSettings::~KisPaintOpSettings()
{
    delete d;
}

void KisPaintOpSettings::mousePressEvent(KoPointerEvent *e)
{
    e->ignore();
}

void KisPaintOpSettings::activate()
{
}

void KDE_DEPRECATED KisPaintOpSettings::setNode(KisNodeSP node)
{
    d->node = node;
}


