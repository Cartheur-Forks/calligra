/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004,2006-2007 Cyrille Berger <cberger@cberger.net>
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

#include "kis_duplicateop.h"

#include <QDomElement>
#include <QRect>

#include <kis_debug.h>

#include <QWidget>

#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoPointerEvent.h>
#include <KoInputDevice.h>
#include <KoCompositeOp.h>

#include <kis_config_widget.h>
#include <kis_image.h>
#include <kis_brush.h>
#include <kis_global.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_types.h>

#include <kis_paintop.h>
#include <kis_iterators_pixel.h>
#include <kis_selection.h>
#include <kis_perspective_grid.h>
#include <kis_random_sub_accessor.h>

#include "ui_wdgduplicateop.h"

class KisDuplicateOpWidget : public KisConfigWidget
{
public:
    KisDuplicateOpWidget()
        : KisConfigWidget()
    {
    }

    virtual void setConfiguration(KisPropertiesConfiguration * config)
    {
    }

    virtual KisPropertiesConfiguration* configuration() const
    {
        return 0;
    }

};


class KisDuplicateOpSettings : public QObject, public KisPaintOpSettings
{

    Q_OBJECT

public:

    KisDuplicateOpSettings(QWidget* parent, KisImageSP image) :
            QObject(parent),
            KisPaintOpSettings(),
            m_optionsWidget(new KisDuplicateOpWidget()),
            m_uiOptions( new Ui_DuplicateOpOptionsWidget()),
            m_image(image),
            m_isOffsetNotUptodate(true)
    {
        m_uiOptions->setupUi(m_optionsWidget);
    }

    ~KisDuplicateOpSettings() {
        delete m_uiOptions;
    }

    virtual KisConfigWidget *widget() const {
        return m_optionsWidget;
    }

    QPointF offset() const {
        return m_offset;
    }

    void mousePressEvent(KoPointerEvent *e) {
        if (e->modifiers() == Qt::ShiftModifier) {
            m_position = m_image->documentToPixel(e->point);
            m_isOffsetNotUptodate = true;
            e->accept();
        } else {
            if (m_isOffsetNotUptodate) {
                m_offset = m_image->documentToPixel(e->point) - m_position;
                m_isOffsetNotUptodate = false;
            }
            e->ignore();
        }
    }

    void activate() {
        if (m_image->perspectiveGrid()->countSubGrids() != 1) {
            m_uiOptions->cbPerspective->setEnabled(false);
            m_uiOptions->cbPerspective->setChecked(false);
        } else {
            m_uiOptions->cbPerspective->setEnabled(true);
        }
    }

    bool healing() const {
        return m_uiOptions->cbHealing->isChecked();
    }

    bool perspectiveCorrection() const {
        return m_uiOptions->cbPerspective->isChecked();
    }

    void fromXML(const QDomElement& elt) {
        m_uiOptions->cbHealing->setChecked(elt.attribute("Healing", "0").toInt());
        m_uiOptions->cbPerspective->setChecked(elt.attribute("PerspectiveCorrection", "0").toInt());
        m_offset.setX(elt.attribute("OffsetX", "0.0").toDouble());
        m_offset.setY(elt.attribute("OffsetY", "0.0").toDouble());
        m_isOffsetNotUptodate = false;
    }

    void toXML(QDomDocument&, QDomElement& elt) const {
        elt.setAttribute("OffsetX", QString::number(m_offset.x()));
        elt.setAttribute("OffsetY", QString::number(m_offset.y()));
        elt.setAttribute("Healing", QString::number(m_uiOptions->cbHealing->isChecked()));
        elt.setAttribute("PerspectiveCorrection", QString::number(m_uiOptions->cbPerspective->isChecked()));
    }

    KisPaintOpSettingsSP clone() const {
        KisDuplicateOpSettings* s = new KisDuplicateOpSettings(0, m_image);
        s->m_uiOptions->cbHealing->setChecked(healing());
        s->m_uiOptions->cbPerspective->setChecked(perspectiveCorrection());
        s->m_offset = m_offset;
        s->m_isOffsetNotUptodate = m_isOffsetNotUptodate;
        s->m_position = m_position;
        return s;
    }

private:
    KisConfigWidget* m_optionsWidget;
    Ui_DuplicateOpOptionsWidget* m_uiOptions;
    KisImageSP m_image;
    bool m_isOffsetNotUptodate;
    QPointF m_position; // Give the position of the last alt-click
    QPointF m_offset;
};



KisPaintOp * KisDuplicateOpFactory::createOp(const KisPaintOpSettingsSP _settings, KisPainter * _painter, KisImageSP _image)
{
    const KisDuplicateOpSettings* settings = dynamic_cast<const KisDuplicateOpSettings *>(_settings.data());
    assert(settings);
    KisPaintOp * op = new KisDuplicateOp(settings, _painter, _image);
    Q_CHECK_PTR(op);
    return op;
}

KisPaintOpSettingsSP KisDuplicateOpFactory::settings(QWidget * parent, const KoInputDevice& inputDevice, KisImageSP image)
{
    Q_UNUSED(inputDevice);
    return new KisDuplicateOpSettings(parent, image);
}

KisPaintOpSettingsSP KisDuplicateOpFactory::settings(KisImageSP image)
{
    return new KisDuplicateOpSettings(0, image);
}


KisDuplicateOp::KisDuplicateOp(const KisDuplicateOpSettings* settings, KisPainter * painter, KisImageSP image)
        : KisBrushBasedPaintOp(painter), m_srcdev(0), m_image(image)
        , m_settings(settings)
        , m_duplicateStartIsSet(false)
{
}

KisDuplicateOp::~KisDuplicateOp()
{
}


double KisDuplicateOp::minimizeEnergy(const double* m, double* sol, int w, int h)
{
    int rowstride = 3 * w;
    double err = 0;
    memcpy(sol, m, 3* sizeof(double) * w);
    m += rowstride;
    sol += rowstride;
    for (int i = 1; i < h - 1; i++) {
        memcpy(sol, m, 3* sizeof(double));
        m += 3; sol += 3;
        for (int j = 3; j < rowstride - 3; j++) {
            double tmp = *sol;
            *sol = ((*(m - 3) + *(m + 3) + *(m - rowstride) + *(m + rowstride)) + 2 * *m) / 6;
            double diff = *sol - tmp;
            err += diff * diff;
            m ++; sol ++;
        }
        memcpy(sol, m, 3* sizeof(double));
        m += 3; sol += 3;
    }
    memcpy(sol, m, 3* sizeof(double) * w);
    return err;
}

#define CLAMP(x,l,u) ((x)<(l)?(l):((x)>(u)?(u):(x)))

void KisDuplicateOp::paintAt(const KisPaintInformation& info)
{
    if (!painter()) return;
    if (!m_duplicateStartIsSet) {
        m_duplicateStartIsSet = true;
        m_duplicateStart = info.pos();
    }

    bool heal = m_settings->healing();

    if (!source()) return;

    KisBrushSP brush = m_brush;
    if (!brush) return;
    if (! brush->canPaintFor(info))
        return;

    double scale = KisPaintOp::scaleForPressure(info.pressure());
    QPointF hotSpot = brush->hotSpot(scale, scale);
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
    xFraction = yFraction = 0.0;

    QPointF srcPointF = pt - m_settings->offset();
    QPoint srcPoint = QPoint(x - static_cast<qint32>(m_settings->offset().x()),
                             y - static_cast<qint32>(m_settings->offset().y()));


    qint32 sw = brush->maskWidth(scale, 0.0);
    qint32 sh = brush->maskHeight(scale, 0.0);

    if (srcPoint.x() < 0)
        srcPoint.setX(0);

    if (srcPoint.y() < 0)
        srcPoint.setY(0);
    if (!(m_srcdev && !(*m_srcdev->colorSpace() == *source()->colorSpace()))) {
        m_srcdev = new KisPaintDevice(source()->colorSpace(), "duplicate source dev");
    }
    Q_CHECK_PTR(m_srcdev);

    // Perspective correction ?
    KisPainter copyPainter(m_srcdev);
    if (m_settings->perspectiveCorrection()) {
        Matrix3qreal startM = Matrix3qreal::Identity();
        Matrix3qreal endM = Matrix3qreal::Identity();

        // First look for the grid corresponding to the start point
        KisSubPerspectiveGrid* subGridStart = *m_image->perspectiveGrid()->begin();
        QRect r = QRect(0, 0, m_image->width(), m_image->height());

#if 1
        if (subGridStart) {
            startM = KisPerspectiveMath::computeMatrixTransfoFromPerspective(r, *subGridStart->topLeft(), *subGridStart->topRight(), *subGridStart->bottomLeft(), *subGridStart->bottomRight());
        }
#endif
#if 1
        // Second look for the grid corresponding to the end point
        KisSubPerspectiveGrid* subGridEnd = *m_image->perspectiveGrid()->begin();
        if (subGridEnd) {
            endM = KisPerspectiveMath::computeMatrixTransfoToPerspective(*subGridEnd->topLeft(), *subGridEnd->topRight(), *subGridEnd->bottomLeft(), *subGridEnd->bottomRight(), r);
        }
#endif

        // Compute the translation in the perspective transformation space:
        QPointF positionStartPaintingT = KisPerspectiveMath::matProd(endM, QPointF(m_duplicateStart));
        QPointF duplicateStartPositionT = KisPerspectiveMath::matProd(endM, QPointF(m_duplicateStart) - QPointF(m_settings->offset()));
        QPointF translat = duplicateStartPositionT - positionStartPaintingT;
        KisRectIteratorPixel dstIt = m_srcdev->createRectIterator(0, 0, sw, sh);
        KisRandomSubAccessorPixel srcAcc = source()->createRandomSubAccessor();
        //Action
        while (!dstIt.isDone()) {
            if (dstIt.isSelected()) {
                QPointF p =  KisPerspectiveMath::matProd(startM, KisPerspectiveMath::matProd(endM, QPointF(dstIt.x() + x, dstIt.y() + y)) + translat);
                srcAcc.moveTo(p);
                srcAcc.sampledOldRawData(dstIt.rawData());
            }
            ++dstIt;
        }


    } else {
        // Or, copy the source data on the temporary device:
        copyPainter.bitBlt(0, 0, COMPOSITE_COPY, source(), srcPoint.x(), srcPoint.y(), sw, sh);
        copyPainter.end();
    }

    // heal ?

    if (heal) {
        quint16 dataDevice[4];
        quint16 dataSrcDev[4];
        double* matrix = new double[ 3 * sw * sh ];
        // First divide
        const KoColorSpace* deviceCs = source()->colorSpace();
        KisHLineConstIteratorPixel deviceIt = source()->createHLineConstIterator(x, y, sw);
        KisHLineIteratorPixel srcDevIt = m_srcdev->createHLineIterator(0, 0, sw);
        double* matrixIt = &matrix[0];
        for (int j = 0; j < sh; j++) {
            for (int i = 0; !srcDevIt.isDone(); i++) {
                deviceCs->toLabA16(deviceIt.rawData(), (quint8*)dataDevice, 1);
                deviceCs->toLabA16(srcDevIt.rawData(), (quint8*)dataSrcDev, 1);
                // Division
                for (int k = 0; k < 3; k++) {
                    matrixIt[k] = dataDevice[k] / (double)qMax((int)dataSrcDev [k], 1);
                }
                ++deviceIt;
                ++srcDevIt;
                matrixIt += 3;
            }
            deviceIt.nextRow();
            srcDevIt.nextRow();
        }
        // Minimize energy
        {
            int iter = 0;
            double err;
            double* solution = new double [ 3 * sw * sh ];
            do {
                err = minimizeEnergy(&matrix[0], &solution[0], sw, sh);
                memcpy(&matrix[0], &solution[0], sw * sh * 3 * sizeof(double));
                iter++;
            } while (err < 0.00001 && iter < 100);
            delete [] solution;
        }

        // Finaly multiply
        deviceIt = source()->createHLineIterator(x, y, sw);
        srcDevIt = m_srcdev->createHLineIterator(0, 0, sw);
        matrixIt = &matrix[0];
        for (int j = 0; j < sh; j++) {
            for (int i = 0; !srcDevIt.isDone(); i++) {
                deviceCs->toLabA16(deviceIt.rawData(), (quint8*)dataDevice, 1);
                deviceCs->toLabA16(srcDevIt.rawData(), (quint8*)dataSrcDev, 1);
                // Multiplication
                for (int k = 0; k < 3; k++) {
                    dataSrcDev[k] = (int)CLAMP(matrixIt[k] * qMax((int) dataSrcDev[k], 1), 0, 65535);
                }
                deviceCs->fromLabA16((quint8*)dataSrcDev, srcDevIt.rawData(), 1);
                ++deviceIt;
                ++srcDevIt;
                matrixIt += 3;
            }
            deviceIt.nextRow();
            srcDevIt.nextRow();
        }
        delete [] matrix;
    }


    // Add the dab as selection to the srcdev
//     KisPainter copySelection(srcdev->selection().data());
//     copySelection.bitBlt(0, 0, COMPOSITE_OVER, dab, 0, 0, sw, sh);
//     copySelection.end();

    brush->mask(m_srcdev, scale, scale, 0.0, info, xFraction, yFraction);

    QRect dabRect = QRect(0, 0, brush->maskWidth(scale, 0.0), brush->maskHeight(scale, 0.0));
    QRect dstRect = QRect(x, y, dabRect.width(), dabRect.height());

    if (painter()->bounds().isValid()) {
        dstRect &= painter()->bounds();
    }

    if (dstRect.isNull() || dstRect.isEmpty() || !dstRect.isValid()) return;

    qint32 sx = dstRect.x() - x;
    qint32 sy = dstRect.y() - y;
    sw = dstRect.width();
    sh = dstRect.height();

    painter()->bltSelection(dstRect.x(), dstRect.y(), painter()->compositeOp(), m_srcdev, painter()->opacity(), sx, sy, sw, sh);
}
#include "kis_duplicateop.moc"

