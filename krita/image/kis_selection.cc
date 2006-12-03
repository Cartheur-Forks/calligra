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


#include <QImage>
#include <QVector>

#include <kdebug.h>
#include <klocale.h>
#include <QColor>

#include "kis_layer.h"
#include "kis_debug_areas.h"
#include "kis_types.h"
#include "KoColorSpaceRegistry.h"
#include "kis_fill_painter.h"
#include "kis_iterators_pixel.h"
#include "KoIntegerMaths.h"
#include "kis_image.h"
#include "kis_datamanager.h"
#include "kis_fill_painter.h"
#include "kis_selection.h"

KisSelection::KisSelection(KisPaintDeviceSP dev)
    : super(dev,
            QString("selection for ") + dev->objectName())
    , m_parentPaintDevice(dev)
    , m_dirty(false)
{
    Q_ASSERT(dev);
}

KisSelection::KisSelection()
    : super("anonymous selection")
    , m_parentPaintDevice(0), m_dirty(false)
{
}

KisSelection::KisSelection(const KisSelection& rhs)
    : super(rhs)
    , m_dirty(false)
{
}

KisSelection::~KisSelection()
{
}

quint8 KisSelection::selected(qint32 x, qint32 y) const
{
    KisHLineConstIteratorPixel iter = createHLineConstIterator(x, y, 1);

    const quint8 *pix = iter.rawData();

    return *pix;
}

void KisSelection::setSelected(qint32 x, qint32 y, quint8 s)
{
    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1);

    quint8 *pix = iter.rawData();

    *pix = s;
}

QImage KisSelection::maskImage() const
{
    // If part of a KisAdjustmentLayer, there may be no parent device.
    QImage img;
    QRect bounds;
    if (m_parentPaintDevice) {

        bounds = m_parentPaintDevice->exactBounds();
        bounds = bounds.intersect( m_parentPaintDevice->image()->bounds() );
        img = QImage(bounds.width(), bounds.height(), 32);
    }
    else {
        bounds = QRect( 0, 0, image()->width(), image()->height());
        img = QImage(bounds.width(), bounds.height(), 32);
    }

    KisHLineConstIteratorPixel it = createHLineConstIterator(bounds.x(), bounds.y(), bounds.width());
    for (int y2 = bounds.y(); y2 < bounds.height() - bounds.y(); ++y2) {
            int x2 = 0;
            while (!it.isDone()) {
                    quint8 s = MAX_SELECTED - *(it.rawData());
                    qint32 c = qRgb(s, s, s);
                    img.setPixel(x2, y2, c);
                    ++x2;
                    ++it;
            }
    }
    return img;
}
void KisSelection::select(QRect r)
{
    KisFillPainter painter(KisPaintDeviceSP(this));
    KoColorSpace * cs = KisMetaRegistry::instance()->csRegistry()->rgb8();
    painter.fillRect(r, KoColor(Qt::white, cs), MAX_SELECTED);
    qint32 x, y, w, h;
    extent(x, y, w, h);
}

void KisSelection::clear(QRect r)
{
    KisFillPainter painter(KisPaintDeviceSP(this));
    KoColorSpace * cs = KisMetaRegistry::instance()->csRegistry()->rgb8();
    painter.fillRect(r, KoColor(Qt::white, cs), MIN_SELECTED);
}

void KisSelection::clear()
{
    quint8 defPixel = MIN_SELECTED;
    m_datamanager->setDefaultPixel(&defPixel);
    m_datamanager->clear();
}

void KisSelection::invert()
{
    qint32 x,y,w,h;

    extent(x, y, w, h);
    KisRectIterator it = createRectIterator(x, y, w, h);
    while ( ! it.isDone() )
    {
        // CBR this is wrong only first byte is inverted
        // BSAR: But we have always only one byte in this color model :-).
        *(it.rawData()) = MAX_SELECTED - *(it.rawData());
        ++it;
    }
    quint8 defPixel = MAX_SELECTED - *(m_datamanager->defaultPixel());
    m_datamanager->setDefaultPixel(&defPixel);
}

bool KisSelection::isTotallyUnselected(QRect r) const
{
    if(*(m_datamanager->defaultPixel()) != MIN_SELECTED)
        return false;
    QRect sr = selectedExactRect();
    return ! r.intersects(sr);
}

bool KisSelection::isProbablyTotallyUnselected(QRect r) const
{
    if(*(m_datamanager->defaultPixel()) != MIN_SELECTED)
        return false;
    QRect sr = selectedRect();
    return ! r.intersects(sr);
}



QRect KisSelection::selectedRect() const
{
    if(*(m_datamanager->defaultPixel()) == MIN_SELECTED || !m_parentPaintDevice)
        return extent();
    else
        return extent().unite(m_parentPaintDevice->extent());
}

QRect KisSelection::selectedExactRect() const
{
    if(*(m_datamanager->defaultPixel()) == MIN_SELECTED || !m_parentPaintDevice)
        return exactBounds();
    else
        return exactBounds().unite(m_parentPaintDevice->exactBounds());
}

void KisSelection::paintUniformSelectionRegion(QImage img, const QRect& imageRect, const QRegion& uniformRegion)
{
    Q_ASSERT(img.size() == imageRect.size());
    Q_ASSERT(imageRect.contains(uniformRegion.boundingRect()));

    if (img.isNull() || img.size() != imageRect.size() || !imageRect.contains(uniformRegion.boundingRect())) {
        return;
    }

    if (*m_datamanager->defaultPixel() == MIN_SELECTED) {

        QRegion region = uniformRegion & QRegion(imageRect);

        if (!region.isEmpty()) {
            QVector<QRect> rects = region.rects();

            for (int i = 0; i < rects.count(); i++) {
                QRect r = rects[i];

                for (qint32 y = 0; y < r.height(); ++y) {

                    QRgb *imagePixel = reinterpret_cast<QRgb *>(img.scanLine(r.y() - imageRect.y() + y));
                    imagePixel += r.x() - imageRect.x();

                    qint32 numPixels = r.width();

                    while (numPixels > 0) {

                        QRgb srcPixel = *imagePixel;
                        quint8 srcGrey = (qRed(srcPixel) + qGreen(srcPixel) + qBlue(srcPixel)) / 9;
                        quint8 srcAlpha = qAlpha(srcPixel);

                        srcGrey = UINT8_MULT(srcGrey, srcAlpha);
                        quint8 dstAlpha = qMax(srcAlpha, quint8(192));

                        QRgb dstPixel = qRgba(128 + srcGrey, 128 + srcGrey, 165 + srcGrey, dstAlpha);
                        *imagePixel = dstPixel;

                        ++imagePixel;
                        --numPixels;
                    }
                }
            }
        }
    }
}

void KisSelection::paint(QImage img, qint32 imageRectX, qint32 imageRectY, qint32 imageRectWidth, qint32 imageRectHeight)
{
    Q_ASSERT(img.size() == QSize(imageRectWidth, imageRectHeight));

    if (img.isNull() || img.size() != QSize(imageRectWidth, imageRectHeight)) {
        return;
    }

    QRect imageRect(imageRectX, imageRectY, imageRectWidth, imageRectHeight);
    QRect selectionExtent = extent();

    selectionExtent.setLeft(selectionExtent.left() - 1);
    selectionExtent.setTop(selectionExtent.top() - 1);
    selectionExtent.setWidth(selectionExtent.width() + 2);
    selectionExtent.setHeight(selectionExtent.height() + 2);

    QRegion uniformRegion = QRegion(imageRect);
    uniformRegion -= QRegion(selectionExtent);

    if (!uniformRegion.isEmpty()) {
        paintUniformSelectionRegion(img, imageRect, uniformRegion);
    }

    QRect nonuniformRect = imageRect & selectionExtent;

    if (!nonuniformRect.isEmpty()) {

        const qint32 imageRectOffsetX = nonuniformRect.x() - imageRectX;
        const qint32 imageRectOffsetY = nonuniformRect.y() - imageRectY;

        imageRectX = nonuniformRect.x();
        imageRectY = nonuniformRect.y();
        imageRectWidth = nonuniformRect.width();
        imageRectHeight = nonuniformRect.height();

        const qint32 NUM_SELECTION_ROWS = 3;

        quint8 *selectionRow[NUM_SELECTION_ROWS];

        qint32 aboveRowIndex = 0;
        qint32 centerRowIndex = 1;
        qint32 belowRowIndex = 2;

        selectionRow[aboveRowIndex] = new quint8[imageRectWidth + 2];
        selectionRow[centerRowIndex] = new quint8[imageRectWidth + 2];
        selectionRow[belowRowIndex] = new quint8[imageRectWidth + 2];

        readBytes(selectionRow[centerRowIndex], imageRectX - 1, imageRectY - 1, imageRectWidth + 2, 1);
        readBytes(selectionRow[belowRowIndex], imageRectX - 1, imageRectY, imageRectWidth + 2, 1);

        for (qint32 y = 0; y < imageRectHeight; ++y) {

            qint32 oldAboveRowIndex = aboveRowIndex;
            aboveRowIndex = centerRowIndex;
            centerRowIndex = belowRowIndex;
            belowRowIndex = oldAboveRowIndex;

            readBytes(selectionRow[belowRowIndex], imageRectX - 1, imageRectY + y + 1, imageRectWidth + 2, 1);

            const quint8 *aboveRow = selectionRow[aboveRowIndex] + 1;
            const quint8 *centerRow = selectionRow[centerRowIndex] + 1;
            const quint8 *belowRow = selectionRow[belowRowIndex] + 1;

            QRgb *imagePixel = reinterpret_cast<QRgb *>(img.scanLine(imageRectOffsetY + y));
            imagePixel += imageRectOffsetX;

            for (qint32 x = 0; x < imageRectWidth; ++x) {

                quint8 center = *centerRow;

                if (center != MAX_SELECTED) {

                    // this is where we come if the pixels should be blue or bluish

                    QRgb srcPixel = *imagePixel;
                    quint8 srcGrey = (qRed(srcPixel) + qGreen(srcPixel) + qBlue(srcPixel)) / 9;
                    quint8 srcAlpha = qAlpha(srcPixel);

                    // Color influence is proportional to alphaPixel.
                    srcGrey = UINT8_MULT(srcGrey, srcAlpha);

                    QRgb dstPixel;

                    if (center == MIN_SELECTED) {
                        //this is where we come if the pixels should be blue (or red outline)

                        quint8 left = *(centerRow - 1);
                        quint8 right = *(centerRow + 1);
                        quint8 above = *aboveRow;
                        quint8 below = *belowRow;

                        // Stop unselected transparent areas from appearing the same
                        // as selected transparent areas.
                        quint8 dstAlpha = qMax(srcAlpha, quint8(192));

                        // now for a simple outline based on 4-connectivity
                        if (left != MIN_SELECTED || right != MIN_SELECTED || above != MIN_SELECTED || below != MIN_SELECTED) {
                            dstPixel = qRgba(255, 0, 0, dstAlpha);
                        } else {
                            dstPixel = qRgba(128 + srcGrey, 128 + srcGrey, 165 + srcGrey, dstAlpha);
                        }
                    } else {
                        dstPixel = qRgba(UINT8_BLEND(qRed(srcPixel), srcGrey + 128, center),
                                         UINT8_BLEND(qGreen(srcPixel), srcGrey + 128, center),
                                         UINT8_BLEND(qBlue(srcPixel), srcGrey + 165, center),
                                         srcAlpha);
                    }

                    *imagePixel = dstPixel;
                }

                aboveRow++;
                centerRow++;
                belowRow++;
                imagePixel++;
            }
        }

        delete [] selectionRow[aboveRowIndex];
        delete [] selectionRow[centerRowIndex];
        delete [] selectionRow[belowRowIndex];
    }
}

void KisSelection::paint(QImage img, const QRect& scaledImageRect, const QSize& scaledImageSize, const QSize& imageSize)
{
    if (img.isNull() || scaledImageRect.isEmpty() || scaledImageSize.isEmpty() || imageSize.isEmpty()) {
        return;
    }

    Q_ASSERT(img.size() == scaledImageRect.size());

    if (img.size() != scaledImageRect.size()) {
        return;
    }

    qint32 imageWidth = imageSize.width();
    qint32 imageHeight = imageSize.height();

    QRect selectionExtent = extent();

    selectionExtent.setLeft(selectionExtent.left() - 1);
    selectionExtent.setTop(selectionExtent.top() - 1);
    selectionExtent.setWidth(selectionExtent.width() + 2);
    selectionExtent.setHeight(selectionExtent.height() + 2);

    double xScale = static_cast<double>(scaledImageSize.width()) / imageWidth;
    double yScale = static_cast<double>(scaledImageSize.height()) / imageHeight;

    QRect scaledSelectionExtent;

    scaledSelectionExtent.setLeft(static_cast<int>(selectionExtent.left() * xScale));
    scaledSelectionExtent.setRight(static_cast<int>(ceil((selectionExtent.right() + 1) * xScale)) - 1);
    scaledSelectionExtent.setTop(static_cast<int>(selectionExtent.top() * yScale));
    scaledSelectionExtent.setBottom(static_cast<int>(ceil((selectionExtent.bottom() + 1) * yScale)) - 1);

    QRegion uniformRegion = QRegion(scaledImageRect);
    uniformRegion -= QRegion(scaledSelectionExtent);

    if (!uniformRegion.isEmpty()) {
        paintUniformSelectionRegion(img, scaledImageRect, uniformRegion);
    }

    QRect nonuniformRect = scaledImageRect & scaledSelectionExtent;

    if (!nonuniformRect.isEmpty()) {

        const qint32 scaledImageRectXOffset = nonuniformRect.x() - scaledImageRect.x();
        const qint32 scaledImageRectYOffset = nonuniformRect.y() - scaledImageRect.y();

        const qint32 scaledImageRectX = nonuniformRect.x();
        const qint32 scaledImageRectY = nonuniformRect.y();
        const qint32 scaledImageRectWidth = nonuniformRect.width();
        const qint32 scaledImageRectHeight = nonuniformRect.height();

        const qint32 imageRowLeft = static_cast<qint32>(scaledImageRectX / xScale);
        const qint32 imageRowRight = static_cast<qint32>((ceil((scaledImageRectX + scaledImageRectWidth - 1 + 1) / xScale)) - 1);

        const qint32 imageRowWidth = imageRowRight - imageRowLeft + 1;
        const qint32 imageRowStride = imageRowWidth + 2;

        const qint32 NUM_SELECTION_ROWS = 3;

        qint32 aboveRowIndex = 0;
        qint32 centerRowIndex = 1;
        qint32 belowRowIndex = 2;

        qint32 aboveRowSrcY = -3;
        qint32 centerRowSrcY = -3;
        qint32 belowRowSrcY = -3;

        quint8 *selectionRows = new quint8[imageRowStride * NUM_SELECTION_ROWS];
        quint8 *selectionRow[NUM_SELECTION_ROWS];

        selectionRow[0] = selectionRows + 1;
        selectionRow[1] = selectionRow[0] + imageRowStride;
        selectionRow[2] = selectionRow[0] + (2 * imageRowStride);

        for (qint32 y = 0; y < scaledImageRectHeight; ++y) {

            qint32 scaledY = scaledImageRectY + y;
            qint32 srcY = (scaledY * imageHeight) / scaledImageSize.height();

            quint8 *aboveRow;
            quint8 *centerRow;
            quint8 *belowRow;

            if (srcY - 1 == aboveRowSrcY) {
                aboveRow = selectionRow[aboveRowIndex];
                centerRow = selectionRow[centerRowIndex];
                belowRow = selectionRow[belowRowIndex];
            } else if (srcY - 1 == centerRowSrcY) {

                qint32 oldAboveRowIndex = aboveRowIndex;

                aboveRowIndex = centerRowIndex;
                centerRowIndex = belowRowIndex;
                belowRowIndex = oldAboveRowIndex;

                aboveRow = selectionRow[aboveRowIndex];
                centerRow = selectionRow[centerRowIndex];
                belowRow = selectionRow[belowRowIndex];

                readBytes(belowRow - 1, imageRowLeft - 1, srcY + 1, imageRowStride, 1);

            } else if (srcY - 1 == belowRowSrcY) {

                qint32 oldAboveRowIndex = aboveRowIndex;
                qint32 oldCenterRowIndex = centerRowIndex;

                aboveRowIndex = belowRowIndex;
                centerRowIndex = oldAboveRowIndex;
                belowRowIndex = oldCenterRowIndex;

                aboveRow = selectionRow[aboveRowIndex];
                centerRow = selectionRow[centerRowIndex];
                belowRow = selectionRow[belowRowIndex];

                if (belowRowIndex == centerRowIndex + 1) {
                    readBytes(centerRow - 1, imageRowLeft - 1, srcY, imageRowStride, 2);
                } else {
                    readBytes(centerRow - 1, imageRowLeft - 1, srcY, imageRowStride, 1);
                    readBytes(belowRow - 1, imageRowLeft - 1, srcY + 1, imageRowStride, 1);
                }

            } else {

                aboveRowIndex = 0;
                centerRowIndex = 1;
                belowRowIndex = 2;

                aboveRow = selectionRow[aboveRowIndex];
                centerRow = selectionRow[centerRowIndex];
                belowRow = selectionRow[belowRowIndex];

                readBytes(selectionRows, imageRowLeft - 1, srcY - 1, imageRowStride, NUM_SELECTION_ROWS);
            }

            aboveRowSrcY = srcY - 1;
            centerRowSrcY = aboveRowSrcY + 1;
            belowRowSrcY = centerRowSrcY + 1;

            QRgb *imagePixel = reinterpret_cast<QRgb *>(img.scanLine(scaledImageRectYOffset + y));
            imagePixel += scaledImageRectXOffset;

            for (qint32 x = 0; x < scaledImageRectWidth; ++x) {

                qint32 scaledX = scaledImageRectX + x;
                qint32 srcX = (scaledX * imageWidth) / scaledImageSize.width();

                quint8 center = *(centerRow + srcX - imageRowLeft);

                if (center != MAX_SELECTED) {

                    // this is where we come if the pixels should be blue or bluish

                    QRgb srcPixel = *imagePixel;
                    quint8 srcGrey = (qRed(srcPixel) + qGreen(srcPixel) + qBlue(srcPixel)) / 9;
                    quint8 srcAlpha = qAlpha(srcPixel);

                    // Color influence is proportional to alphaPixel.
                    srcGrey = UINT8_MULT(srcGrey, srcAlpha);

                    QRgb dstPixel;

                    if (center == MIN_SELECTED) {
                        //this is where we come if the pixels should be blue (or red outline)

                        quint8 left = *(centerRow + (srcX - imageRowLeft) - 1);
                        quint8 right = *(centerRow + (srcX - imageRowLeft) + 1);
                        quint8 above = *(aboveRow + (srcX - imageRowLeft));
                        quint8 below = *(belowRow + (srcX - imageRowLeft));

                        // Stop unselected transparent areas from appearing the same
                        // as selected transparent areas.
                        quint8 dstAlpha = qMax(srcAlpha, quint8(192));

                        // now for a simple outline based on 4-connectivity
                        if (left != MIN_SELECTED || right != MIN_SELECTED || above != MIN_SELECTED || below != MIN_SELECTED) {
                            dstPixel = qRgba(255, 0, 0, dstAlpha);
                        } else {
                            dstPixel = qRgba(128 + srcGrey, 128 + srcGrey, 165 + srcGrey, dstAlpha);
                        }
                    } else {
                        dstPixel = qRgba(UINT8_BLEND(qRed(srcPixel), srcGrey + 128, center),
                                         UINT8_BLEND(qGreen(srcPixel), srcGrey + 128, center),
                                         UINT8_BLEND(qBlue(srcPixel), srcGrey + 165, center),
                                         srcAlpha);
                    }

                    *imagePixel = dstPixel;
                }

                imagePixel++;
            }
        }

        delete [] selectionRows;
    }
}

void KisSelection::setDirty(const QRect& rc)
{
    if (m_dirty)
        super::setDirty(rc);
}

            void KisSelection::setDirty()
{
    if (m_dirty)
        super::setDirty();
}

