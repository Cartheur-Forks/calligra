/*
 *  kis_paintop_box.cc - part of KImageShop/Krayon/Krita
 *
 *  Copyright (c) 2004 Boudewijn Rempt (boud@valdyas.org)
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
#include <qwidget.h>
#include <qstring.h>
#include <qvaluelist.h>
#include <qpixmap.h>

#include <klocale.h>

#include <kis_id.h>
#include <kis_paintop_registry.h>
#include <kis_view.h>
#include <kis_painter.h>
#include <kis_paintop.h>

#include "kis_paintop_box.h"

KisPaintopBox::KisPaintopBox (KisView * view, QWidget *parent, const char * name)
    : super (parent, name),
      m_view(view)
{
    setCaption(i18n("Painter's Toolchest"));
    m_paintops = new QValueList<KisID>();

    connect(this, SIGNAL(selected(const KisID &)), m_view, SLOT(paintopActivated(const KisID &)));
    connect(this, SIGNAL(highlighted(int)), this, SLOT(slotItemSelected(int)));

    // XXX: Let's see... Are all paintops loaded and ready?
    KisIDList keys = KisPaintOpRegistry::instance()->listKeys();
    for ( KisIDList::Iterator it = keys.begin(); it != keys.end(); ++it ) {
        if (KisPaintOpRegistry::instance()->userVisible(*it)) {
            addItem(*it);
        }
    }
    setCurrentItem( m_paintops->findIndex(KisID("paintbrush","")));

}

KisPaintopBox::~KisPaintopBox()
{
    delete m_paintops;
}

void KisPaintopBox::addItem(const KisID & paintop, const QString & /*category*/)
{
    m_paintops->append(paintop);
    QPixmap pm = KisPaintOpRegistry::instance()->getPixmap(paintop);
    if (pm.isNull()) {
        QPixmap p = QPixmap( 16, 16 );
        p.fill();
        insertItem(p,  paintop.name());
    }
    else {
        insertItem(pm, paintop.name());
    }

}

void KisPaintopBox::slotItemSelected(int index)
{
    KisID id = *m_paintops->at(index);
    emit selected(id);
}

#include "kis_paintop_box.moc"
