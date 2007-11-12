/*
 * Copyright (C) 2007, Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_prescaled_projection.h"

#include <QImage>
#include <QPixmap>
#include <QColor>
#include <QRect>
#include <QPoint>
#include <QSize>
#include <QPainter>
#include <QTimer>

#include <qimageblitz.h>

#include <KoColorProfile.h>
#include <KoViewConverter.h>

#include "kis_config.h"
#include "kis_config_notifier.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_mask.h"
#include "kis_node.h"
#include "kis_paint_device.h"
#include "kis_paint_layer.h"
#include "kis_selection.h"
#include "kis_selection_mask.h"
#include "kis_types.h"

#define EPSILON 1e-6

// Casper's version that's fixed for jitter.
QImage sampleImage(const QImage& image, int columns, int rows, const QRect &dstRect)
{
    Q_ASSERT( image.format() == QImage::Format_ARGB32 );
    int *x_offset;
    int *y_offset;

    long j;
    long y;

    uchar *pixels;

    register const uchar *p;

    register long x;

    register uchar *q;

    /*
      Initialize sampled image attributes.
    */
    if ((columns == image.width()) && (rows == image.height()))
        return image.copy( dstRect );

    QImage sample_image( dstRect.width(), dstRect.height(), QImage::Format_ARGB32);
    /*
      Allocate scan line buffer and column offset buffers.
    */
    pixels= new uchar[ image.width() * 4 ];
    x_offset= new int[ sample_image.width() ];
    y_offset= new int[ sample_image.height() ];

    /*
      Initialize pixel offsets.
    */
// In the following several code 0.5 needs to be added, otherwise the image
// would be moved by half a pixel to bottom-right, just like
// with Qt's QImage::scale()

    for (x=0; x < (long) sample_image.width(); x++)
    {
        x_offset[x] = int((x + dstRect.left()) * image.width() / columns);
    }
    for (y=0; y < (long) sample_image.height(); y++)
    {
        y_offset[y] = int((y + dstRect.top()) * image.height() / rows);
    }
    /*
      Sample each row.
    */
    j = (-1);
    for (y = 0; y < (long) sample_image.height(); y++)
    {
        q = sample_image.scanLine( y );
        if (j != y_offset[y] )
        {
            /*
              Read a scan line.
            */
            j= y_offset[y];
            p= image.scanLine( j );
            (void) memcpy(pixels,p,image.width() * 4);
        }
        /*
          Sample each column.
        */
        for (x = 0; x < (long) sample_image.width(); x++)
        {
            *(QRgb*)q = ((QRgb*)pixels)[ x_offset[x] ];
            q += 4;
        }
    }

    delete[] y_offset;
    delete[] x_offset;
    delete[] pixels;
    return sample_image;
}

struct KisPrescaledProjection::Private
{
    Private()
        : updateAllOfQPainterCanvas( false )
        , usePixmapNotQImage( false )
        , useDeferredSmoothing( false )
        , useNearestNeighbour( false )
        , useQtScaling( false )
        , useSampling( false )
        , smoothBetween100And200Percent( true )
        , drawCheckers( false )
        , scrollCheckers( false )
        , drawMaskVisualisationOnUnscaledCanvasCache( false )
        , cacheKisImageAsQImage( true )
        , showMask( true )
        , checkSize( 32 )
        , documentOffset( 0, 0 )
        , canvasSize( 0, 0 )
        , imageSize( 0, 0 )
        , viewConverter( 0 )
        , monitorProfile( 0 )
        , exposure( 0.0 )
        {
            smoothingTimer.setSingleShot( true );
        }
    bool updateAllOfQPainterCanvas;
    bool usePixmapNotQImage;
    bool useDeferredSmoothing; // first sample, then smoothscale when
                               // zoom < 1.0
    bool useNearestNeighbour; // Use Krita to sample the image when
                              // zoom < 1.0
    bool useQtScaling; // Use Qt to smoothscale the image when zoom <
                       // 1.0
    bool useSampling; // use the above sample function instead
                      // qpainter's built-in scaling when zoom > 1.0
    bool smoothBetween100And200Percent; // if true, when zoom is
                                        // between 1.0 and 2.0,
                                        // smoothscale
    bool drawCheckers;
    bool scrollCheckers;
    bool drawMaskVisualisationOnUnscaledCanvasCache;
    bool cacheKisImageAsQImage;
    bool showMask;

    QColor checkersColor;
    qint32 checkSize;
    QImage unscaledCache;
    QImage prescaledQImage;
    QPixmap prescaledPixmap;
    QPoint documentOffset; // in view pixels
    QSize canvasSize; // in view pixels
    QSize imageSize; // in kisimage pixels
    KisImageSP image;
    KoViewConverter * viewConverter;
    const KoColorProfile * monitorProfile;
    float exposure;
    KisNodeSP currentNode;
    QTimer smoothingTimer;
    QRegion rectsToSmooth;
};

KisPrescaledProjection::KisPrescaledProjection()
    : QObject( 0 )
    , m_d( new Private() )
{
    updateSettings();
    connect( &m_d->smoothingTimer, SIGNAL( timeout() ), this, SLOT( slotDoSmoothScale() ) );
}

KisPrescaledProjection::~KisPrescaledProjection()
{
    delete m_d;
}


void KisPrescaledProjection::setImage( KisImageSP image )
{
    Q_ASSERT( image );
    m_d->image = image;
    setImageSize( image->width(), image->height() );
}

void KisPrescaledProjection::setImageSize(qint32 w, qint32 h)
{
    kDebug(41010) << "Setting image size from " << m_d->imageSize << " to " << w << ", " << h;
    // XXX: Make the limit of 50 megs configurable
    if ( ( w * h * 4 ) > ( 50 * 1024 * 1024 ) )
    {
        m_d->prescaledQImage = QImage();
        m_d->prescaledQImage.fill( QColor( 255, 0, 0, 128 ).rgba() );
        m_d->cacheKisImageAsQImage = false;
    }

    m_d->imageSize = QSize( w, h );
    if ( !m_d->useNearestNeighbour && m_d->cacheKisImageAsQImage ) {
        m_d->unscaledCache = QImage( w, h, QImage::Format_ARGB32 );
        m_d->prescaledQImage.fill( QColor( 255, 0, 0, 128 ).rgba() );
        updateCanvasProjection( QRect( 0, 0, w, h ) );
    }
}


bool KisPrescaledProjection::drawCheckers() const
{
    return m_d->drawCheckers;
}


void KisPrescaledProjection::setDrawCheckers( bool drawCheckers )
{
    m_d->drawCheckers = drawCheckers;
}

QPixmap KisPrescaledProjection::prescaledPixmap() const
{
    return m_d->prescaledPixmap;
}

QImage KisPrescaledProjection::prescaledQImage() const
{
    return m_d->prescaledQImage;
}

void KisPrescaledProjection::setViewConverter( KoViewConverter * viewConverter )
{
    m_d->viewConverter = viewConverter;
}

void KisPrescaledProjection::updateSettings()
{
    KisConfig cfg;
    m_d->updateAllOfQPainterCanvas = cfg.updateAllOfQPainterCanvas();
    m_d->usePixmapNotQImage = cfg.noXRender();
    m_d->useDeferredSmoothing = cfg.useDeferredSmoothing();
    m_d->useNearestNeighbour = cfg.useNearestNeigbour();
    m_d->useQtScaling = cfg.useQtSmoothScaling();
    m_d->useSampling = cfg.useSampling();
    // If any of the above are true, we don't use our own smooth scaling
    m_d->scrollCheckers = cfg.scrollCheckers();
    m_d->checkSize = cfg.checkSize();
    m_d->checkersColor = cfg.checkersColor();

    m_d->cacheKisImageAsQImage = cfg.cacheKisImageAsQImage();
    if ( !m_d->cacheKisImageAsQImage ) m_d->unscaledCache = QImage();

    m_d->drawMaskVisualisationOnUnscaledCanvasCache = cfg.drawMaskVisualisationOnUnscaledCanvasCache();

    // XXX: Make config setting
    m_d->smoothBetween100And200Percent = true;

    kDebug(41010) << "QPainter canvas will render with the following options:\n"
                  << "\t updateAllOfQPainterCanvas: " << m_d->updateAllOfQPainterCanvas << "\n"
                  << "\t useDeferredSmoothing: " << m_d->useDeferredSmoothing << "\n"
                  << "\t useNearestNeighbour: " << m_d->useNearestNeighbour << "\n"
                  << "\t useQtScaling: " << m_d->useQtScaling << "\n"
                  << "\t useSampling: " << m_d->useSampling << "\n"
                  << "\t smoothBetween100And200Percent: " << m_d->smoothBetween100And200Percent << "\n"
                  << "\t cacheKisImageAsQImage: " << m_d->cacheKisImageAsQImage ;
}

void KisPrescaledProjection::documentOffsetMoved( const QPoint &documentOffset )
{
    kDebug(41010) << "documentOffsetMoved " << m_d->documentOffset << ", to " << documentOffset;
    m_d->documentOffset = documentOffset;

    // We've called documentOffsetMoved before even updating the projection
    if ( m_d->prescaledQImage.isNull() ) {

        return;
    }

    preScale();

// Let someone else figure out the optimization where we copy the
// still visible part of the image after moving the offset and then
// only draw the newly visible parts
#if 0

    qint32 width = m_d->prescaledQImage.width();
    qint32 height = m_d->prescaledQImage.height();

    QRegion exposedRegion = QRect(0, 0, width, height);

    qint32 oldCanvasXOffset = m_d->documentOffset.x();
    qint32 oldCanvasYOffset = m_d->documentOffset.y();

    kDebug(41010) << "w: " << width << ", h" << height << ", oldCanvasXOffset " << oldCanvasXOffset << ", oldCanvasYOffset " << oldCanvasYOffset
             << ", new offset: " << documentOffset;

    m_d->documentOffset = documentOffset;

    QImage img = QImage( width, height, QImage::Format_ARGB32 );
    m_d->prescaledQImage.fill( QColor( 255, 0, 0, 128 ).rgba() );
    QPainter gc( &img );

    gc.setCompositionMode( QPainter::CompositionMode_Source );

    if (oldCanvasXOffset != m_d->documentOffset.x() || oldCanvasYOffset != m_d->documentOffset.y()) {

        qint32 deltaX =  oldCanvasXOffset - m_d->documentOffset.x();
        qint32 deltaY = oldCanvasYOffset - m_d->documentOffset.y();

        kDebug(41010) << "deltaX: " << deltaX << ", deltaY: " << deltaY;

        gc.drawImage( deltaX, deltaY, m_d->prescaledQImage );
        exposedRegion -= QRegion(QRect(0, 0, width - deltaX, height - deltaY));
    }


    if (!exposedRegion.isEmpty()) {

        QVector<QRect> rects = exposedRegion.rects();

        for (int i = 0; i < rects.count(); i++) {
            QRect r = rects[i];
            // Set the areas to empty. Who knows, there may be not
            // enough image to draw in them.
            gc.fillRect( r, QColor( 0, 0, 0, 0 ) );
            kDebug(41010) << "rect" << r;
            // And conver the rect to document pixels, because that's
            // what drawScaledImage expects.
            drawScaledImage( imageRectFromViewPortPixels( r ), gc);
        }
    }
    m_d->prescaledQImage = img;
#endif
}

void KisPrescaledProjection::updateCanvasProjection( const QRect & rc )
{
    kDebug(41010) << "updateCanvasProjection " << rc;
    if ( !m_d->image ) {
        kDebug(41010) << "Calling updateCanvasProjection without an image: " << kBacktrace() << endl;
        return;
    }

    // We cache the KisImage as a QImage
    if ( !m_d->useNearestNeighbour ) {

        if ( m_d->cacheKisImageAsQImage ) {

            updateUnscaledCache( rc );
        }
    }

    QRect vRect;

    if ( m_d->updateAllOfQPainterCanvas ) {
        vRect = QRect( m_d->documentOffset, m_d->canvasSize );
    }
    else {
        QRect vRect = viewRectFromImagePixels( rc );
    }

    if ( !vRect.isEmpty() ) {
        preScale( vRect );
    }

}


void KisPrescaledProjection::setMonitorProfile( const KoColorProfile * profile )
{
    m_d->monitorProfile = profile;
}

void KisPrescaledProjection::setHDRExposure( float exposure )
{
    m_d->exposure = exposure;
}

void KisPrescaledProjection::setCurrentNode( const KisNodeSP node )
{
    m_d->currentNode = node;
}

void KisPrescaledProjection::showCurrentMask( bool showMask )
{
    m_d->showMask = showMask;
}


void KisPrescaledProjection::preScale()
{
    Q_ASSERT( m_d->canvasSize.isValid() );
    preScale( QRect( QPoint( 0, 0 ), m_d->canvasSize ) );
}

void KisPrescaledProjection::preScale( const QRect & rc )
{
    kDebug(41010 ) << "Going to prescale rc " << rc;
    if ( !rc.isEmpty() ) {
        QPainter gc( &m_d->prescaledQImage );
        gc.setCompositionMode( QPainter::CompositionMode_Source );
        gc.fillRect( rc, QColor( 0, 0, 0, 0 ) );
        drawScaledImage( rc, gc);
    }

}

void KisPrescaledProjection::resizePrescaledImage( QSize newSize )
{

    QSize oldSize;

    if ( m_d->prescaledQImage.isNull() ) {
        oldSize = QSize( 0, 0 );
    }
    else {
        oldSize = m_d->prescaledQImage.size();
    }

    kDebug(41010) << "resizePrescaledImage from " << oldSize << " to " << newSize << endl;

    QImage img = QImage(newSize, QImage::Format_ARGB32);
    m_d->prescaledQImage.fill( QColor( 255, 0, 0, 128 ).rgba() );

// Let someone else figure out the optimization where we copy the
// still visible part of the image after moving the offset and then
// only draw the newly visible parts
#if 0
    QPainter gc( &img );
    gc.setCompositionMode( QPainter::CompositionMode_Source );


    if ( newSize.width() > oldSize.width() || newSize.height() > oldSize.height() ) {

        gc.drawImage( 0, 0,
                      m_d->prescaledQImage,
                      0, 0, m_d->prescaledQImage.width(), m_d->prescaledQImage.height() );

        QRegion r( QRect( 0, 0, newSize.width(), newSize.height() ) );
        r -= QRegion( QRect( 0, 0, m_d->prescaledQImage.width(), m_d->prescaledQImage.height() ) );
        foreach( QRect rc, r.rects() ) {
            drawScaledImage( rc, gc );
        }
    }
    else {
        gc.drawImage( 0, 0, m_d->prescaledQImage,
                      0, 0, m_d->prescaledQImage.width(), m_d->prescaledQImage.height() );
    }
#endif
    m_d->prescaledQImage = img;
    m_d->canvasSize = newSize;
    preScale();


}

void KisPrescaledProjection::drawScaledImage( const QRect & rc,  QPainter & gc, bool isDeferredAction )
{
    if ( !m_d->image )
        return;

    Q_ASSERT( m_d->viewConverter );

    // get the x and y zoom level
    double zoomX, zoomY;
    m_d->viewConverter->zoom(&zoomX, &zoomY);

    // Get the KisImage resolution
    double xRes = m_d->image->xRes();
    double yRes = m_d->image->yRes();

    // Compute the scale factors
    double scaleX = zoomX / xRes;
    double scaleY = zoomY / yRes;

    // Size of the image in KisImage pixels
    QSize imagePixelSize( m_d->image->width(), m_d->image->height() );

    // compute how large a fully scaled image is in viewpixels
    QSize dstSize = QSize(int(imagePixelSize.width() * scaleX ), int( imagePixelSize.height() * scaleY));

    // Don't go outside the image (will crash the sampleImage method below)
    QRect drawRect = rc.translated( m_d->documentOffset ).intersected(QRect( QPoint(0, 0), dstSize ) );

    // Go from the widget coordinates to points
    QRectF imageRect = m_d->viewConverter->viewToDocument( rc.translated( m_d->documentOffset ) );

    // Go from points to view pixels
    imageRect.setCoords( imageRect.left() * xRes, imageRect.top() * yRes,
                         imageRect.right() * xRes, imageRect.bottom() * yRes );

    // Don't go outside the image
    QRect alignedImageRect = imageRect.intersected( m_d->image->bounds() ).toAlignedRect();
    alignedImageRect.setBottom( alignedImageRect.bottom() + 1); // FIXME <= it's a work around in a Qt bug (bouuuh) (reported to them see http://trolltech.com/developer/task-tracker/index_html?method=entry&id=187008 ) the returned .toAlignedRect() is one pixel too small
    alignedImageRect.setRight( alignedImageRect.right() + 1);

    // the size of the rect after scaling
    QSize scaledSize = QSize( ( int )( alignedImageRect.width() * scaleX ), ( int )( alignedImageRect.height() * scaleY ));

    // Compute the corresponding area on the canvas
    QRectF rcFromAligned;
    rcFromAligned.setCoords( alignedImageRect.left(), alignedImageRect.top(),
                         alignedImageRect.right(), alignedImageRect.bottom() );
    rcFromAligned.setCoords( alignedImageRect.left() / xRes, alignedImageRect.top() / yRes,
                         (alignedImageRect.right() + 1.0 ) / xRes, (alignedImageRect.bottom() + 1.0 ) / yRes );
    m_d->viewConverter->documentToViewY( rcFromAligned.height() );
    rcFromAligned = m_d->viewConverter->documentToView( rcFromAligned ).translated( -m_d->documentOffset) ;
    
    // Apparently we have never before tried to draw the image
    if ( m_d->cacheKisImageAsQImage && m_d->unscaledCache.isNull() ) {
        updateUnscaledCache( alignedImageRect );
    }
    
#if 0
    kDebug(41010) << "#####################################";
    kDebug(41010) << " xRed = " << xRes << " yRes = " << yRes;
    kDebug(41010) << " m_d->viewConverter->documentToView( imageRect ) = " << m_d->viewConverter->documentToView( imageRect );
    kDebug(41010) << " rc.translated( m_d->documentOffset ) = " << rc.translated( m_d->documentOffset );
    kDebug(41010) << " m_d->viewConverter->documentToView( m_d->viewConverter->viewToDocument( rc ) ) = " << m_d->viewConverter->documentToView( m_d->viewConverter->viewToDocument( rc ) );
    kDebug(41010) << " imageRect = " << imageRect << " " << imageRect.bottomRight() << " " << imageRect.right() << " , " << imageRect.bottom();
    kDebug(41010) << " imageRect.toRect() = " << imageRect.toRect();
    kDebug(41010) << " alignedImageRect = " << alignedImageRect << " " << alignedImageRect.bottomRight();
    kDebug(41010) << " scaledSize = " << scaledSize;
    kDebug(41010) << " imageRect.intersected( m_d->image->bounds() ) = " << imageRect.intersected( m_d->image->bounds() );
    kDebug(41010) << " m_d->image->bounds() = " << m_d->image->bounds();
    kDebug(41010) << " rcFromAligned = " << rcFromAligned << " " << rcFromAligned.bottomRight();
    kDebug(41010) << " drawRect = " << drawRect;
    kDebug(41010) << " rc = " << rc;
    kDebug(41010) << " dstSize = " << dstSize;
    kDebug(41010) << "#####################################";
#endif
    
    QPointF rcTopLeftUnscaled( rcFromAligned.topLeft().x() / scaleX, rcFromAligned.topLeft().y() / scaleY );
    // And now for deciding what to do and when -- the complicated bit
    if ( scaleX > 1.0 - EPSILON && scaleY > 1.0 - EPSILON ) {

        // Between 1.0 and 2.0, a box filter often gives a much nicer
        // result, according to pippin. The default blitz filter is
        // called "blackman"
        if ( m_d->smoothBetween100And200Percent && scaleX < 2.0 - EPSILON && scaleY < 2.0 - EPSILON  ) {
            kDebug(41010) << "smoothBetween100And200Percent" << endl;
            QImage img;
            if ( !m_d->cacheKisImageAsQImage ) {
                img = m_d->image->convertToQImage( drawRect, 1.0 / scaleX, 1.0 / scaleY, m_d->monitorProfile, m_d->exposure );
                gc.setRenderHint(QPainter::SmoothPixmapTransform, true);
                gc.drawImage( rcFromAligned.topLeft(), img );
            }
            else {
                img = m_d->unscaledCache.copy( alignedImageRect );
                gc.save();
                gc.scale(scaleX, scaleY);
                gc.setRenderHint(QPainter::SmoothPixmapTransform, true);
                gc.drawImage(rcTopLeftUnscaled, img);
                gc.restore();
            }
        }
        else {
            QImage img;
            // Get the image directly from the KisImage
            if ( m_d->useNearestNeighbour || !m_d->cacheKisImageAsQImage ) {
                img = m_d->image->convertToQImage( alignedImageRect, 1.0, 1.0, m_d->monitorProfile, m_d->exposure );
            }
            else {
                // Crop the canvascache
                img = m_d->unscaledCache.copy( alignedImageRect );
            }

            // If so desired, use the sampleImage originally taken from
            // gwenview, which got it from mosfet, who got it from
            // ImageMagick
            if ( m_d->useSampling ) {
                kDebug(41010) << "useSampling" << endl;
                gc.drawImage( rcFromAligned.topLeft(), sampleImage(img, dstSize.width(), dstSize.height(), drawRect) );
            }
            else {
                // Else, let QPainter do the scaling, like we did in 1.6
                kDebug(41010) << "1.6 way" << endl;
                gc.save();
                gc.scale(scaleX, scaleY);
                gc.drawImage( rcTopLeftUnscaled, img);
                gc.restore();
            }
        }
    }
    else {
        QImage croppedImage = m_d->unscaledCache.copy( alignedImageRect );

        // Short circuit if we're in the deferred smoothing stage.
        if ( isDeferredAction ) {
            kDebug(41010) << " isDeferredAction ";
            gc.drawImage( rcTopLeftUnscaled, Blitz::smoothScale( croppedImage, dstSize ) );
            return;
        }
        // Use nearest neighbour interpolation from the raw KisImage
        if ( m_d->useNearestNeighbour || m_d->useDeferredSmoothing ) {
            kDebug(41010) << " useNearestNeighbour || m_d->useDeferredSmoothing";
            if ( m_d->useDeferredSmoothing ) {
                // XXX: Start smoothing job. The job will be replaced
                // by the next smoothing job if it is added before
                // this one is done.
                m_d->rectsToSmooth += rc;
                m_d->smoothingTimer.start(50);
            }
            QImage tmpImage = m_d->image->convertToQImage( drawRect, 1.0 / scaleX, 1.0 / scaleY, m_d->monitorProfile, m_d->exposure );
            gc.drawImage( rcTopLeftUnscaled, tmpImage );
        }
        else {
            // If we don't cache the image as an unscaled QImage, get
            // an unscaled QImage for this rect from KisImage.
            if ( !m_d->cacheKisImageAsQImage ) {
                kDebug(41010) << " !m_d->cacheKisImageAsQImage";
                croppedImage = m_d->image->convertToQImage( alignedImageRect.x(), alignedImageRect.y(), alignedImageRect.width(), alignedImageRect.height(), m_d->monitorProfile, m_d->exposure );
            }
            QSize s( ceil(rcFromAligned.size().width()), ceil(rcFromAligned.size().height()) );
            double scaleXbis = rcFromAligned.width() / s.width();
            double scaleYbis = rcFromAligned.height() / s.height();
            gc.save();
            gc.setRenderHint(QPainter::SmoothPixmapTransform, true);
            gc.scale( scaleXbis, scaleYbis);
            QPointF pt( rcFromAligned.topLeft().x() / scaleXbis, rcFromAligned.topLeft().y() / scaleYbis );
            if ( m_d->useQtScaling ) {
                kDebug(41010) << " m_d->useQtScaling";
                gc.drawImage( pt, croppedImage.scaled( s, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
            }
            else if ( m_d->useSampling ) {
                kDebug(41010) << " m_d->useSampling";
                gc.drawImage( pt, sampleImage(croppedImage, s.width(), s.height(), drawRect) );
            }
            else { // Smooth scaling using blitz
                kDebug(41010) << " else";
                gc.drawImage( pt, Blitz::smoothScale( croppedImage, s, Qt::IgnoreAspectRatio ) );
            }
            gc.restore();
        }
    }
}

QRect KisPrescaledProjection::viewRectFromImagePixels( const QRect & rc )
{
    double xRes = m_d->image->xRes();
    double yRes = m_d->image->yRes();

    QRectF docRect;
    docRect.setCoords((rc.left() - 2) / xRes, (rc.top() - 2) / yRes, (rc.right() + 2) / xRes, (rc.bottom() + 2) / yRes);

    QRect viewRect = m_d->viewConverter->documentToView(docRect).toAlignedRect();
    viewRect = viewRect.translated( -m_d->documentOffset );
    viewRect = viewRect.intersected( QRect( 0, 0, m_d->canvasSize.width(), m_d->canvasSize.width() ) );

    return viewRect;
}

QRect KisPrescaledProjection::imageRectFromViewPortPixels( const QRect & viewportRect )
{
    QRect intersectedRect = viewportRect.intersected( QRect( 0, 0, m_d->canvasSize.width(), m_d->canvasSize.width() ) );
    QRect translatedRect = intersectedRect.translated( -m_d->documentOffset );
    QRectF docRect = m_d->viewConverter->viewToDocument( translatedRect );

    return m_d->image->documentToIntPixel( docRect ).intersected( m_d->image->bounds() );
}

void KisPrescaledProjection::slotDoSmoothScale()
{
    QRect rc = m_d->rectsToSmooth.boundingRect();
    QPainter gc( &m_d->prescaledQImage );
    gc.setCompositionMode( QPainter::CompositionMode_Source );
    drawScaledImage( rc, gc, true);
    m_d->rectsToSmooth = QRegion();
    emit sigPrescaledProjectionUpdated( rc );
}

void KisPrescaledProjection::updateUnscaledCache( const QRect & rc )
{
    kDebug(41010) << rc;
    if ( m_d->unscaledCache.isNull() ) {
        m_d->unscaledCache = QImage( m_d->image->width(), m_d->image->height(), QImage::Format_ARGB32 );
        m_d->prescaledQImage.fill( QColor( 255, 0, 0, 128 ).rgba() );
    }

    QPainter p( &m_d->unscaledCache );
    p.setCompositionMode( QPainter::CompositionMode_Source );

    QImage updateImage = m_d->image->convertToQImage(rc.x(), rc.y(), rc.width(), rc.height(),
                                                     m_d->monitorProfile,
                                                     m_d->exposure);

    if ( m_d->showMask && m_d->drawMaskVisualisationOnUnscaledCanvasCache ) {

        // XXX: Also visualize the global selection

        KisSelectionSP selection = 0;

        if ( m_d->currentNode ) {
            if ( m_d->currentNode->inherits( "KisMask" ) ) {

                selection = dynamic_cast<const KisMask*>( m_d->currentNode.data() )->selection();
            }
            else if ( m_d->currentNode->inherits( "KisLayer" ) ) {

                KisLayerSP layer = dynamic_cast<KisLayer*>( m_d->currentNode.data() );
                if ( KisSelectionMaskSP selectionMask = layer->selectionMask() ) {
                    selection = selectionMask->selection();
                }
            }
        }
        selection->paint(&updateImage, rc);
    }

    p.drawImage( rc.x(), rc.y(), updateImage, 0, 0, rc.width(), rc.height() );
    p.end();

}

#include "kis_prescaled_projection.moc"
