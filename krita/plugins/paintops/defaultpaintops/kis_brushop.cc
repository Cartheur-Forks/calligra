/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
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

#include <string.h>

#include <QRect>
#include <QWidget>
#include <QLayout>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <qtoolbutton.h>

#include <kdebug.h>

#include "KoColorTransformation.h"

#include "kcurve.h"
#include "kis_brush.h"
#include "kis_global.h"
#include "kis_paint_device.h"

#include "kis_painter.h"
#include "kis_types.h"
#include "kis_paintop.h"
#include "KoInputDevice.h"
#include "kis_selection.h"
#include "kis_brushop.h"
#include "ui_kis_dlgbrushcurvecontrol.h"

KisPaintOp * KisBrushOpFactory::createOp(const KisPaintOpSettings *settings,
                                         KisPainter * painter, KisImageSP image)
{
    Q_UNUSED( image );
    const KisBrushOpSettings *brushopSettings = dynamic_cast<const KisBrushOpSettings *>(settings);
    Q_ASSERT(settings == 0 || brushopSettings != 0);

    KisPaintOp * op = new KisBrushOp(brushopSettings, painter);
    Q_CHECK_PTR(op);
    return op;
}

KisBrushOpSettings::KisBrushOpSettings(QWidget *parent)
    : KisPaintOpSettings(parent)
{
    m_optionsWidget = new QWidget(parent);
    m_optionsWidget->setObjectName("brush option widget");

    QHBoxLayout * l = new QHBoxLayout(m_optionsWidget);

    m_pressureVariation = new QLabel(i18n("Pressure variation: "), m_optionsWidget);
    l->addWidget(m_pressureVariation);

    m_size =  new QCheckBox(i18n("size"), m_optionsWidget);
    m_size->setChecked(true);
    l->addWidget(m_size);

    m_opacity = new QCheckBox(i18n("opacity"), m_optionsWidget);
    l->addWidget(m_opacity);

    m_darken = new QCheckBox(i18n("darken"), m_optionsWidget);
    l->addWidget(m_darken);

    m_curveControl = new Ui::WdgBrushCurveControl();
    m_curveControlWidget = new QDialog(m_optionsWidget);
    m_curveControl->setupUi(m_curveControlWidget);

    QToolButton* moreButton = new QToolButton(m_optionsWidget);
    moreButton->setArrowType(Qt::UpArrow);
    moreButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    moreButton->setMinimumSize(QSize(24,24)); // Bah, I had hoped the above line would make this unneeded
    connect(moreButton, SIGNAL(clicked()), this, SLOT(slotCustomCurves()));

    m_customSize = false;
    m_customOpacity = false;
    m_customDarken = false;
    // the curves will get filled in when the slot gets accepted
}

void KisBrushOpSettings::slotCustomCurves() {
    if (m_curveControlWidget->exec() == QDialog::Accepted) {
        m_customSize = m_curveControl->sizeCheckbox->isChecked();
        m_customOpacity = m_curveControl->opacityCheckbox->isChecked();
        m_customDarken = m_curveControl->darkenCheckbox->isChecked();

        if (m_customSize) {
            transferCurve(m_curveControl->sizeCurve, m_sizeCurve);
        }
        if (m_customOpacity) {
            transferCurve(m_curveControl->opacityCurve, m_opacityCurve);
        }
        if (m_customDarken) {
            transferCurve(m_curveControl->darkenCurve, m_darkenCurve);
        }
    }
}

void KisBrushOpSettings::transferCurve(KCurve* curve, double* target) {
    double value;
    for (int i = 0; i < 256; i++) {
        value = curve->getCurveValue( i / 255.0);
        if (value < PRESSURE_MIN)
            target[i] = PRESSURE_MIN;
        else if (value > PRESSURE_MAX)
            target[i] = PRESSURE_MAX;
        else
            target[i] = value;
    }
}

bool KisBrushOpSettings::varySize() const
{
    return m_size->isChecked();
}

bool KisBrushOpSettings::varyOpacity() const
{
    return m_opacity->isChecked();
}

bool KisBrushOpSettings::varyDarken() const
{
    return m_darken->isChecked();
}

KisPaintOpSettings* KisBrushOpFactory::settings(QWidget * parent, const KoInputDevice& inputDevice, KisImageSP /*image*/)
{
    if (inputDevice == KoInputDevice::mouse()) {
        // No options for mouse, only tablet devices
        return 0;
    } else {
        return new KisBrushOpSettings(parent);
    }
}

KisBrushOp::KisBrushOp(const KisBrushOpSettings *settings, KisPainter *painter)
    : KisPaintOp(painter)
    , m_pressureSize(true)
    , m_pressureOpacity(false)
    , m_pressureDarken(false)
    , m_customSize(false)
    , m_customOpacity(false)
    , m_customDarken(false)
{
    if (settings != 0) {
        m_pressureSize = settings->varySize();
        m_pressureOpacity = settings->varyOpacity();
        m_pressureDarken = settings->varyDarken();
        m_customSize = settings->customSize();
        m_customOpacity = settings->customOpacity();
        m_customDarken = settings->customDarken();
        if (m_customSize) {
            memcpy(m_sizeCurve, settings->sizeCurve(), 256 * sizeof(double));
        }
        if (m_customOpacity) {
            memcpy(m_opacityCurve, settings->opacityCurve(), 256 * sizeof(double));
        }
        if (m_customDarken) {
            memcpy(m_darkenCurve, settings->darkenCurve(), 256 * sizeof(double));
        }
    }
}

KisBrushOp::~KisBrushOp()
{
}

void KisBrushOp::paintAt(const KisPaintInformation& info)
{
    KisPaintInformation adjustedInfo(info);
    if (!m_pressureSize)
        adjustedInfo.setPressure( PRESSURE_DEFAULT );


    // Painting should be implemented according to the following algorithm:
    // retrieve brush
    // if brush == mask
    //          retrieve mask
    // else if brush == image
    //          retrieve image
    // subsample (mask | image) for position -- pos should be double!
    // apply filters to mask (color | gradient | pattern | etc.
    // composite filtered mask into temporary layer
    // composite temporary layer into target layer
    // @see: doc/brush.txt

    if (!painter()->device()) return;

    KisBrush *brush = painter()->brush();

    Q_ASSERT(brush);
    if (!brush) return;
    if (! brush->canPaintFor(adjustedInfo) )
        return;

    KisPaintDeviceSP device = painter()->device();

    QPointF hotSpot = brush->hotSpot( KisPaintOp::scaleForPressure( info.pressure() ) );
    QPointF pt = info.pos() - hotSpot;

    // Split the coordinates into integer plus fractional parts. The integer
    // is where the dab will be positioned and the fractional part determines
    // the sub-pixel positioning.
    qint32 x;
    double xFraction;
    qint32 y;
    double yFraction;

    splitCoordinate(pt.x(), &x, &xFraction);
    splitCoordinate(pt.y(), &y, &yFraction);

    KisPaintDeviceSP dab = KisPaintDeviceSP(0);

    quint8 origOpacity = painter()->opacity();
    KoColor origColor = painter()->paintColor();

    if (m_pressureOpacity) {
        if (!m_customOpacity)
            painter()->setOpacity((qint8)(origOpacity * info.pressure()));
        else
            painter()->setOpacity((qint8)(origOpacity * scaleToCurve(info.pressure(), m_opacityCurve)));
    }

    if (m_pressureDarken) {
        KoColor darkened = origColor;
        // Darken docs aren't really clear about what exactly the amount param can have as value...
        quint32 darkenAmount;
        if (!m_customDarken)
            darkenAmount = (qint32)(255  - 75 * info.pressure());
        else
            darkenAmount = (qint32)(255  - 75 * scaleToCurve(info.pressure(), m_darkenCurve));

        KoColorTransformation* transfo = darkened.colorSpace()->createDarkenAdjustement(darkenAmount, false, 0.0);
        transfo->transform(origColor.data(), darkened.data(), 1);
        painter()->setPaintColor(darkened);
        delete transfo;
    }

    painter()->setPressure(adjustedInfo.pressure());

    double scale = KisPaintOp::scaleForPressure( adjustedInfo.pressure() );
    
    QRect dabRect = QRect(0, 0, brush->maskWidth(scale),
                          brush->maskHeight(scale));
    QRect dstRect = QRect(x, y, dabRect.width(), dabRect.height());


    if ( painter()->bounds().isValid() ) {
        dstRect &= painter()->bounds();
    }

    if (dstRect.isNull() || dstRect.isEmpty() || !dstRect.isValid()) return;

    qint32 sx = dstRect.x() - x;
    qint32 sy = dstRect.y() - y;
    qint32 sw = dstRect.width();
    qint32 sh = dstRect.height();

    if (brush->brushType() == IMAGE || brush->brushType() == PIPE_IMAGE) {
        dab = brush->image(device->colorSpace(), scale, 0.0, adjustedInfo, xFraction, yFraction);
    }
    else {
        dab = cachedDab( );
        KoColor color = painter()->paintColor();
        color.convertTo( dab->colorSpace() );
        brush->mask(dab, color, scale, 0.0, info, xFraction, yFraction);
    }

    painter()->bltSelection(dstRect.x(), dstRect.y(), painter()->compositeOp(), dab, painter()->opacity(), sx, sy, sw, sh);

    painter()->setOpacity(origOpacity);
    painter()->setPaintColor(origColor);

}

double KisBrushOp::paintLine(const KisPaintInformation &pi1,
                             const KisPaintInformation &pi2,
                             double savedDist )
{
    KisPaintInformation adjustedInfo1(pi1);
    KisPaintInformation adjustedInfo2(pi2);
    if ( !m_pressureSize ) {
        adjustedInfo1.setPressure( PRESSURE_DEFAULT );
        adjustedInfo2.setPressure( PRESSURE_DEFAULT );
    }
    return KisPaintOp::paintLine( adjustedInfo1, adjustedInfo2, savedDist );
}

#include "kis_brushop.moc"
