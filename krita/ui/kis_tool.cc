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
#include <QCursor>

#include <KoCanvasBase.h>
#include <KoShapeManager.h>
#include <KoTool.h>
#include <KoColor.h>
#include <KoID.h>
#include <KoPointerEvent.h>
#include <KoViewConverter.h>
#include <KoSelection.h>

#include "kis_layer_shape.h"
#include "kis_layer_container_shape.h"
#include "kis_mask_shape.h"
#include "kis_shape_layer.h"

#include <kis_image.h>
#include <kis_layer.h>
#include <kis_group_layer.h>
#include <kis_adjustment_layer.h>
#include <kis_mask.h>
#include <kis_paint_layer.h>
#include <kis_brush.h>
#include <kis_pattern.h>
#include <kis_gradient.h>
#include "kis_resource_provider.h"

#include "kis_canvas2.h"
#include "kis_tool.h"

KisTool::KisTool( KoCanvasBase * canvas, const QCursor & cursor )
    : KoTool( canvas )
    , m_cursor( cursor )
{
}

KisTool::~KisTool()
{
}

void KisTool::activate(bool )
{
    useCursor(m_cursor, true);

    m_currentFgColor = m_canvas->resourceProvider()->resource( KoCanvasResource::ForegroundColor ).value<KoColor>();
    m_currentBgColor = m_canvas->resourceProvider()->resource( KoCanvasResource::BackgroundColor ).value<KoColor>();
    m_currentBrush = static_cast<KisBrush *>( m_canvas->resourceProvider()->resource( KisResourceProvider::CurrentBrush ).value<void *>() );
    m_currentPattern = static_cast<KisPattern *>( m_canvas->resourceProvider()->resource( KisResourceProvider::CurrentPattern).value<void *>() );
    m_currentGradient = static_cast<KisGradient *>( m_canvas->resourceProvider()->resource( KisResourceProvider::CurrentGradient ).value<void *>() );
    m_currentPaintOp = m_canvas->resourceProvider()->resource( KisResourceProvider::CurrentPaintop ).value<KoID >();
    m_currentPaintOpSettings = static_cast<KisPaintOpSettings*>( m_canvas->resourceProvider()->resource( KisResourceProvider::CurrentPaintopSettings ).value<void *>() );
    m_currentLayer = m_canvas->resourceProvider()->resource( KisResourceProvider::CurrentKritaLayer ).value<KisLayerSP>();
    m_currentExposure = static_cast<float>( m_canvas->resourceProvider()->resource( KisResourceProvider::HdrExposure ).toDouble() );
    m_currentImage = image();
}



void KisTool::deactivate()
{
}

void KisTool::resourceChanged( int key, const QVariant & v )
{
    switch ( key ) {
    case ( KoCanvasResource::ForegroundColor ):
        m_currentFgColor = v.value<KoColor>();
        break;
    case ( KoCanvasResource::BackgroundColor ):
        m_currentBgColor = v.value<KoColor>();
        break;
    case ( KisResourceProvider::CurrentBrush ):
        m_currentBrush = static_cast<KisBrush *>( v.value<void *>() );
        break;
    case ( KisResourceProvider::CurrentPattern ):
        m_currentPattern = static_cast<KisPattern *>( v.value<void *>() );
        break;
    case ( KisResourceProvider::CurrentGradient ):
        m_currentGradient = static_cast<KisGradient *>( v.value<void *>() );
        break;
    case ( KisResourceProvider::CurrentPaintop ):
        m_currentPaintOp = v.value<KoID >();
        break;
    case ( KisResourceProvider::CurrentPaintopSettings ):
        m_currentPaintOpSettings = static_cast<KisPaintOpSettings*>( v.value<void *>() );
        break;
    case ( KisResourceProvider::HdrExposure ):
        m_currentExposure = static_cast<float>( v.toDouble() );
    case ( KisResourceProvider::CurrentKritaLayer ):
        m_currentLayer = v.value<KisLayerSP>();
    default:
        ;
        // Do nothing
    };
}

QPointF KisTool::convertToPixelCoord( KoPointerEvent *e )
{
    return image()->documentToPixel(e->point);
}

QPoint KisTool::convertToIntPixelCoord( KoPointerEvent *e )
{
    return image()->documentToIntPixel(e->point);
}

QRectF KisTool::convertToPt( const QRectF &rect )
{
    QRectF r;
    //We add 1 in the following to the extreme coords because a pixel always has size
    r.setCoords(int(rect.left()) / image()->xRes(), int(rect.top()) / image()->yRes(),
             int(1 + rect.right()) / image()->xRes(), int(1 + rect.bottom()) / image()->yRes());
    return r;
}

QPointF KisTool::pixelToView(const QPoint &pixelCoord)
{
    QPointF documentCoord = image()->pixelToDocument(pixelCoord);
    return m_canvas->viewConverter()->documentToView(documentCoord);
}

QPointF KisTool::pixelToView(const QPointF &pixelCoord)
{
    QPointF documentCoord = image()->pixelToDocument(pixelCoord);
    return m_canvas->viewConverter()->documentToView(documentCoord);
}

QRectF KisTool::pixelToView(const QRectF &pixelRect)
{
    QPointF topLeft = pixelToView(pixelRect.topLeft());
    QPointF bottomRight = pixelToView(pixelRect.bottomRight());
    return QRectF(topLeft, bottomRight);
}

void KisTool::updateCanvasPixelRect(const QRectF &pixelRect)
{
    m_canvas->updateCanvas(convertToPt(pixelRect));
}

void KisTool::updateCanvasViewRect(const QRectF &viewRect)
{
    m_canvas->updateCanvas(m_canvas->viewConverter()->viewToDocument(viewRect));
}

KisImageSP KisTool::image() const
{
#if 0
    KisCanvas2 * kisCanvas = dynamic_cast<KisCanvas2*> ( m_canvas );
    if ( !kisCanvas ) {
        kDebug(41007) << "The current canvas is not a kis canvas!\n";
        return 0;
    }

    KisImageSP img = kisCanvas->currentImage();

    return img;
#endif

    KoShape * shape = m_canvas->shapeManager()->selection()->firstSelectedShape();

    if ( !shape ) return 0;

    if ( shape->shapeId() == KIS_LAYER_CONTAINER_ID ) {
        return static_cast<KisLayerContainerShape*>( shape )->groupLayer()->image();
    } else if ( shape->shapeId() ==  KIS_LAYER_SHAPE_ID) {
        return static_cast<KisLayerShape*>( shape )->layer()->image();
    } else if ( shape->shapeId() == KIS_MASK_SHAPE_ID ) {
        // XXX
        return 0;
    } else if ( shape->shapeId() == KIS_SHAPE_LAYER_ID ) {
        return static_cast<KisShapeLayer*>( shape )->image();
    } else {
        // First selected shape is not a krita layer type shape
        return 0;
    }

}

void KisTool::notifyModified() const
{
    KisImageSP img = image();
    if ( img ) {
        img->setModified();
    }
}

#include "kis_tool.moc"
