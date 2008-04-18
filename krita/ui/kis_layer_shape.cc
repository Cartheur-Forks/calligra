/*
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_layer_shape.h"
#include <kis_types.h>
#include <kis_layer.h>
#include <kis_image.h>

#include <kis_paint_device.h>

#include "kis_mask_shape.h"

class KisLayerShape::Private {
public:
    KisLayerSP layer;
};

KisLayerShape::KisLayerShape( KoShapeContainer * parent, KisLayerSP layer)
    : KoShapeContainer()
    , m_d ( new Private() )
{

    m_d->layer = layer;
    Q_ASSERT( layer->image() );

    setShapeId( KIS_LAYER_SHAPE_ID );
    setParent( parent );
    parent->addChild(this);
}

KisLayerShape::~KisLayerShape()
{
    delete m_d;
}

KoShape * KisLayerShape::cloneShape() const
{
    // TODO implement cloning
    return 0;
}

KisLayerSP KisLayerShape::layer()
{
    return m_d->layer;
}

void KisLayerShape::paint(QPainter &painter, const KoViewConverter &converter)
{
    Q_UNUSED(painter);
    Q_UNUSED(converter);
}


void KisLayerShape::paintComponent(QPainter &painter, const KoViewConverter &converter)
{
    Q_UNUSED(painter);
    Q_UNUSED(converter);
}

QSizeF KisLayerShape::size() const
{
    Q_ASSERT( m_d );
    Q_ASSERT( m_d->layer );

    QRect br = m_d->layer->extent();
    KisImageSP image = m_d->layer->image();

    if ( !image ) return QSizeF( 0.0, 0.0 );

    dbgUI <<"KisLayerShape::size extent:" << br <<", x res:" << image->xRes() <<", y res:" << image->yRes();

    return QSizeF( br.width() / image->xRes(), br.height() / image->yRes() );
}

QRectF KisLayerShape::boundingRect() const
{
    QRect br = m_d->layer->extent();
    dbgUI <<"KisLayerShape::boundingRect extent:" << br <<", x res:" << m_d->layer->image()->xRes() <<", y res:" << m_d->layer->image()->yRes();
    return QRectF(int(br.left()) / m_d->layer->image()->xRes(), int(br.top()) / m_d->layer->image()->yRes(),
                  int(1 + br.right()) / m_d->layer->image()->xRes(), int(1 + br.bottom()) / m_d->layer->image()->yRes());

}

void KisLayerShape::setPosition( const QPointF & position )
{
    Q_ASSERT( m_d );
    Q_ASSERT( m_d->layer );

    KisImageSP image = m_d->layer->image();
    if ( image ) {
        // XXX: Does flake handle undo for us?
        QPointF pf( position.x() / image->xRes(), position.y() / image->yRes() );
        QPoint p = pf.toPoint();
        m_d->layer->setX( p.x() );
        m_d->layer->setY( p.y() );
    }
}


void KisLayerShape::addChild( KoShape * shape )
{
    if ( shape->shapeId() != KIS_MASK_SHAPE_ID ) {
        dbgUI <<"Can only add mask shapes as children to layer shapes!";
        return;
    }
    KoShapeContainer::addChild(shape);
}

void KisLayerShape::saveOdf( KoShapeSavingContext & /*context*/ ) const
{
    
    // TODO
}

bool KisLayerShape::loadOdf( const KoXmlElement & /*element*/, KoShapeLoadingContext &/*context*/ )
{
    return false; // TODO
}
