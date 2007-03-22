/*
 *  Copyright (c) 2005 Boudewijn Rempt <boud@valdyas.org>
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
#include <limits.h>

#include <QLabel>
#include <QLayout>
#include <QSlider>
#include <QString>
#include <QBitArray>
#include <q3valuevector.h>
#include <QGroupBox>
#include <QVBoxLayout>

#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <knuminput.h>

#include <KoChannelInfo.h>
#include <KoColorSpace.h>

#include "kis_global.h"
#include "squeezedcombobox.h"
#include "kis_dlg_layer_properties.h"
#include "kis_cmb_composite.h"
#include "kis_cmb_idlist.h"
#include "KoColorProfile.h"
#include "kis_int_spinbox.h"
#include "KoColorSpace.h"
#include "kis_channelflags_widget.h"

KisDlgLayerProperties::KisDlgLayerProperties(const QString& deviceName,
                                             qint32 opacity,
                                             const KoCompositeOp* compositeOp,
                                             const KoColorSpace * colorSpace,
                                             const QBitArray & channelFlags,
                                             QWidget *parent, const char *name, Qt::WFlags f)
    : super(parent)
{
    Q_UNUSED(f);
    setCaption( i18n("Layer Properties") );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );

    setObjectName(name);
    m_page = new WdgLayerProperties(this);
    m_page->layout()->setMargin(0);

    opacity = int((opacity * 100.0) / 255 + 0.5);

    setMainWidget(m_page);

    m_page->editName->setText(deviceName);
    connect( m_page->editName, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotNameChanged( const QString & ) ) );

    m_page->cmbColorSpaces->setCurrent(colorSpace->id());
    m_page->cmbColorSpaces->setEnabled(false);

    QString profilename;
    if (KoColorProfile* profile = const_cast<KoColorSpace *>(colorSpace)->profile())
        profilename = profile->productName();
    m_page->cmbProfile->addSqueezedItem(profilename);
    m_page->cmbProfile->setEnabled(false);

    m_page->intOpacity->setRange(0, 100, 13);
    m_page->intOpacity->setValue(opacity);

    m_page->cmbComposite->setCompositeOpList(colorSpace->userVisiblecompositeOps());
    m_page->cmbComposite->setCurrent(compositeOp);

    slotNameChanged( m_page->editName->text() );

    QVBoxLayout * vbox = new QVBoxLayout;
    m_channelFlags = new KisChannelFlagsWidget( colorSpace );
    vbox->addWidget( m_channelFlags );
    vbox->addStretch( 1 );
    m_page->grpActiveChannels->setLayout( vbox );
    m_channelFlags->setChannelFlags( channelFlags );

}

KisDlgLayerProperties::~KisDlgLayerProperties()
{
}

void KisDlgLayerProperties::slotNameChanged( const QString &_text )
{
    enableButtonOk( !_text.isEmpty() );
}

QString KisDlgLayerProperties::getName() const
{
    return m_page->editName->text();
}

int KisDlgLayerProperties::getOpacity() const
{
    qint32 opacity = m_page->intOpacity->value();

    if (!opacity)
        return 0;

    opacity = int((opacity * 255.0) / 100 + 0.5);
    if(opacity>255)
        opacity=255;
    return opacity;
}

KoCompositeOp * KisDlgLayerProperties::getCompositeOp() const
{
    return m_page->cmbComposite->currentItem();
}

QBitArray KisDlgLayerProperties::getChannelFlags() const
{
    return m_channelFlags->channelFlags();
}

#include "kis_dlg_layer_properties.moc"
