/*
 *  Copyright (c) 2007 Sven Langkamp <sven.langkamp@gmail.com>
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

#include "kis_shape_selection.h"


#include <QPainter>
#include <QTimer>

#include <KoLineBorder.h>
#include <KoPathShape.h>
#include <KoCompositeOp.h>
#include <KoColorSpaceRegistry.h>
#include <KoShapeManager.h>

#include "kis_painter.h"
#include "kis_paint_device.h"
#include "kis_shape_selection_model.h"
#include "kis_image.h"
#include "kis_selection.h"
#include "kis_shape_selection_canvas.h"

#include <kis_debug.h>

KisShapeSelection::KisShapeSelection(KisImageSP image, KisSelectionSP selection)
        : KoShapeContainer(new KisShapeSelectionModel(image, selection, this))
        , m_image(image)
{
    Q_ASSERT( m_image );
    setShapeId("KisShapeSelection");
    setSelectable(false);
    m_dirty = false;
    m_canvas = new KisShapeSelectionCanvas();
    m_canvas->shapeManager()->add(this);

}

KisShapeSelection::~KisShapeSelection()
{
    delete m_canvas;
}

KisShapeSelection::KisShapeSelection(const KisShapeSelection& rhs)
    : KoShapeContainer( rhs )
{
    m_dirty = rhs.m_dirty;
    m_image = rhs.m_image;
    m_canvas = new KisShapeSelectionCanvas();
    m_canvas->shapeManager()->add( this );
}

KisSelectionComponent* KisShapeSelection::clone()
{
    return new KisShapeSelection( *this );
}

bool KisShapeSelection::loadOdf(const KoXmlElement&, KoShapeLoadingContext&)
{

#ifdef __GNUC__
#warning "KisShapeSelection::loadOdf: Implement loading of shape selections"
#endif
    return false;

}

void KisShapeSelection::saveOdf(KoShapeSavingContext&) const
{

#ifdef __GNUC__
#warning "KisShapeSelection::saveOdf: Implement saving of shape selections"
#endif

}


bool KisShapeSelection::saveOdf( KoStore* ) const
{
#ifdef __GNUC__
#warning "KisShapeSelection::saveOdf: Implement saving of shape selections"
#endif
    return false;
}


void KisShapeSelection::addChild(KoShape *object)
{
    KoShapeContainer::addChild(object);
    m_canvas->shapeManager()->add(object);
}

void KisShapeSelection::removeChild(KoShape *object)
{
    m_canvas->shapeManager()->remove(object);
    KoShapeContainer::removeChild(object);
}

QPainterPath KisShapeSelection::selectionOutline()
{
    if (m_dirty) {
        QList<KoShape*> shapesList = iterator();

        QPainterPath outline;
        KoPathShape* pathShape;
        foreach(KoShape * shape, shapesList) {
            pathShape = dynamic_cast<KoPathShape*>(shape);
            if (pathShape) {
                QMatrix shapeMatrix = shape->absoluteTransformation(0);

                outline = outline.united(shapeMatrix.map(shape->outline()));
            }
        }
        m_outline = outline;
        m_dirty = false;
    }
    return m_outline;
}

void KisShapeSelection::paintComponent(QPainter& painter, const KoViewConverter& converter)
{
    Q_UNUSED(painter);
    Q_UNUSED(converter);
}

void KisShapeSelection::renderToProjection(KisSelection* projection)
{
    Q_ASSERT( projection );
    Q_ASSERT( m_image );
    QMatrix resolutionMatrix;
    resolutionMatrix.scale(m_image->xRes(), m_image->yRes());

    QRectF boundingRect = resolutionMatrix.mapRect(selectionOutline().boundingRect());
    renderSelection(projection, boundingRect.toAlignedRect());
}

void KisShapeSelection::renderToProjection(KisSelection* projection, const QRect& r)
{
    Q_ASSERT( projection );
    renderSelection(projection, r);
}

void KisShapeSelection::renderSelection(KisSelection* projection, const QRect& r)
{
    Q_ASSERT( projection );
    Q_ASSERT( m_image );

    QMatrix resolutionMatrix;
    resolutionMatrix.scale(m_image->xRes(), m_image->yRes());

    QTime t;
    t.start();

    KisPaintDeviceSP tmpMask = new KisPaintDevice(KoColorSpaceRegistry::instance()->alpha8(), "tmp");

    const qint32 MASK_IMAGE_WIDTH = 256;
    const qint32 MASK_IMAGE_HEIGHT = 256;

    QImage polygonMaskImage(MASK_IMAGE_WIDTH, MASK_IMAGE_HEIGHT, QImage::Format_ARGB32);
    QPainter maskPainter(&polygonMaskImage);
    maskPainter.setRenderHint(QPainter::Antialiasing, true);

    // Break the mask up into chunks so we don't have to allocate a potentially very large QImage.

    for (qint32 x = r.x(); x < r.x() + r.width(); x += MASK_IMAGE_WIDTH) {
        for (qint32 y = r.y(); y < r.y() + r.height(); y += MASK_IMAGE_HEIGHT) {

            maskPainter.fillRect(polygonMaskImage.rect(), QColor(OPACITY_TRANSPARENT, OPACITY_TRANSPARENT, OPACITY_TRANSPARENT, 255));
            maskPainter.translate(-x, -y);
            maskPainter.fillPath(resolutionMatrix.map(selectionOutline()), QColor(OPACITY_OPAQUE, OPACITY_OPAQUE, OPACITY_OPAQUE, 255));
            maskPainter.translate(x, y);

            qint32 rectWidth = qMin(r.x() + r.width() - x, MASK_IMAGE_WIDTH);
            qint32 rectHeight = qMin(r.y() + r.height() - y, MASK_IMAGE_HEIGHT);

            KisRectIterator rectIt = tmpMask->createRectIterator(x, y, rectWidth, rectHeight);

            while (!rectIt.isDone()) {
                (*rectIt.rawData()) = qRed(polygonMaskImage.pixel(rectIt.x() - x, rectIt.y() - y));
                ++rectIt;
            }
        }
    }
    KisPainter painter(projection);
    painter.bitBlt(r.x(), r.y(), COMPOSITE_OVER, tmpMask, r.x(), r.y(), r.width(), r.height());
    painter.end();
    dbgRender << "Shape selection rendering: " << t.elapsed();
}

void KisShapeSelection::setDirty()
{
    m_dirty = true;
}

KoShapeManager* KisShapeSelection::shapeManager() const
{
    return m_canvas->shapeManager();
}

KisShapeSelectionFactory::KisShapeSelectionFactory(QObject* parent)
        : KoShapeFactory(parent, "KisShapeSelection", "selection shape container")
{
}

KoShape* KisShapeSelectionFactory::createDefaultShape() const
{
    return 0;
}

KoShape* KisShapeSelectionFactory::createShape(const KoProperties* params) const
{
    Q_UNUSED(params);
    return 0;
}

#include "kis_shape_selection.moc"
