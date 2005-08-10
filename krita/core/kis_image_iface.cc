/*
 *  This file is part of the KDE project
 *
 *  Copyright (C) 2002 Laurent Montel <lmontel@mandrakesoft.com>
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
#include <kapplication.h>

#include "kis_image_iface.h"
#include "kis_types.h"
#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_paint_device_iface.h"
#include <dcopclient.h>

KisImageIface::KisImageIface( KisImage *img_ )
    : DCOPObject()
{
    m_img = img_;
}

QString KisImageIface::name()const
{
    return m_img->name();
}

int KisImageIface::height() const
{
    return m_img->height();
}

int KisImageIface::width() const
{
    return m_img->width();
}

bool KisImageIface::empty() const
{
    return m_img->empty();
}

void KisImageIface::setName(const QString& name)
{
    m_img->setName( name );
}

DCOPRef KisImageIface::activeDevice()
{
    KisPaintDeviceSP dev = m_img->activeDevice();

    if( !dev )
        return DCOPRef();
    else
        return DCOPRef( kapp->dcopClient()->appId(),
                dev->dcopObject()->objId(),
                "KisPaintDeviceIface");

}
