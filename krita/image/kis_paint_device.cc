/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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
#include <QRect>
#include <QMatrix>
#include <QImage>
#include <QApplication>
#include <QList>
#include <QTime>
#include <QTimer>
#include <QUndoCommand>
#include <QHash>


#include <klocale.h>
#include <kdebug.h>

#include "KoColorProfile.h"
#include "KoColor.h"
#include "KoColorSpace.h"
#include "KoColorSpaceRegistry.h"
#include "KoIntegerMaths.h"
#include <KoStore.h>

#include "kis_global.h"
#include "kis_paint_engine.h"
#include "kis_types.h"
#include "kis_painter.h"
#include "kis_fill_painter.h"
#include "kis_iterator.h"
#include "kis_iterators_pixel.h"
#include "kis_iteratorpixeltrait.h"
#include "kis_random_accessor.h"
#include "kis_random_sub_accessor.h"
#include "kis_selection.h"
#include "kis_layer.h"
#include "kis_paint_device.h"
#include "kis_datamanager.h"
#include "kis_memento.h"
#include "kis_undo_adapter.h"

//#define CACHE_EXACT_BOUNDS

class KisPaintDevice::Private {

public:

    KisLayerWSP parentLayer;
    qint32 x;
    qint32 y;
    KoColorSpace * colorSpace;
    qint32 pixelSize;
    qint32 nChannels;
    // The selection is a read/write mask. Pixels that are not selected should be skipped
    mutable bool hasSelection;
    mutable bool selectionDeselected;
    mutable KisSelectionSP selection;

    QList<KisFilter*> longRunningFilters;
    QTimer * longRunningFilterTimer;
    QPaintEngine * paintEngine;

    // XXX: Use shared pointers here?
    QHash<QString, KisMaskSP> painterlyChannels;
#ifdef CACHE_EXACT_BOUNDS
    QRect exactBounds;
#endif

};


namespace {

    class KisPaintDeviceCommand : public QUndoCommand {
        typedef QUndoCommand super;

    public:
        KisPaintDeviceCommand(const QString& name, KisPaintDeviceSP paintDevice);
        virtual ~KisPaintDeviceCommand() {}

    protected:
        void setUndo(bool undo);

        KisPaintDeviceSP m_paintDevice;
    };

    KisPaintDeviceCommand::KisPaintDeviceCommand(const QString& name, KisPaintDeviceSP paintDevice) :
        super(name), m_paintDevice(paintDevice)
    {
    }

    void KisPaintDeviceCommand::setUndo(bool undo)
    {
        if (m_paintDevice->undoAdapter()) {
            m_paintDevice->undoAdapter()->setUndo(undo);
        }
    }

    class KisConvertLayerTypeCmd : public KisPaintDeviceCommand {
        typedef KisPaintDeviceCommand super;

    public:
        KisConvertLayerTypeCmd(KisPaintDeviceSP paintDevice,
                       KisDataManagerSP beforeData, KoColorSpace * beforeColorSpace,
                       KisDataManagerSP afterData, KoColorSpace * afterColorSpace
                ) : super(i18n("Convert Layer Type"), paintDevice)
            {
                m_beforeData = beforeData;
                m_beforeColorSpace = beforeColorSpace;
                m_afterData = afterData;
                m_afterColorSpace = afterColorSpace;
            }

        virtual ~KisConvertLayerTypeCmd()
            {
            }

    public:
        virtual void redo()
            {
                setUndo(false);
                m_paintDevice->setData(m_afterData, m_afterColorSpace);
                setUndo(true);
            }

        virtual void undo()
            {
                setUndo(false);
                m_paintDevice->setData(m_beforeData, m_beforeColorSpace);
                setUndo(true);
            }

    private:
        KisDataManagerSP m_beforeData;
        KoColorSpace * m_beforeColorSpace;

        KisDataManagerSP m_afterData;
        KoColorSpace * m_afterColorSpace;
    };

}

KisPaintDevice::KisPaintDevice(KoColorSpace * colorSpace, const QString& name)
    : QObject(0)
    , m_d( new Private() )
{
    setObjectName(name);

    if (colorSpace == 0) {
        kWarning(41001) << "Cannot create paint device without colorspace!\n";
        return;
    }

    m_d->longRunningFilterTimer = 0;

    m_d->x = 0;
    m_d->y = 0;

    m_d->pixelSize = colorSpace->pixelSize();
    m_d->nChannels = colorSpace->channelCount();

    quint8 *defaultPixel = new quint8 [ m_d->pixelSize ];
    colorSpace->fromQColor(Qt::black, OPACITY_TRANSPARENT, defaultPixel);

    m_datamanager = new KisDataManager(m_d->pixelSize, defaultPixel);
    delete [] defaultPixel;

    Q_CHECK_PTR(m_datamanager);

    m_d->parentLayer = 0;

    m_d->colorSpace = colorSpace;

    m_d->hasSelection = false;
    m_d->selectionDeselected = false;
    m_d->selection = 0;
    m_d->paintEngine = new KisPaintEngine();

}

KisPaintDevice::KisPaintDevice(KisLayerWSP parent, KoColorSpace * colorSpace, const QString& name)
    : QObject(0)
    , m_d( new Private() )
{
    setObjectName(name);
    Q_ASSERT( colorSpace );
    m_d->longRunningFilterTimer = 0;

    m_d->x = 0;
    m_d->y = 0;

    m_d->hasSelection = false;
    m_d->selectionDeselected = false;
    m_d->selection = 0;

    m_d->parentLayer = parent;

    if (colorSpace == 0 && parent != 0 && parent->image() != 0) {
        m_d->colorSpace = parent->image()->colorSpace();
    }
    else {
        m_d->colorSpace = colorSpace;
    }

    m_d->pixelSize = m_d->colorSpace->pixelSize();
    m_d->nChannels = m_d->colorSpace->channelCount();

    quint8 *defaultPixel = new quint8[ m_d->pixelSize ];
    colorSpace->fromQColor(Qt::black, OPACITY_TRANSPARENT, defaultPixel);

    m_datamanager = new KisDataManager(m_d->pixelSize, defaultPixel);
    delete [] defaultPixel;
    Q_CHECK_PTR(m_datamanager);
    m_d->paintEngine = new KisPaintEngine();

}


KisPaintDevice::KisPaintDevice(const KisPaintDevice& rhs)
    : QObject()
    , KisShared(rhs)
    , QPaintDevice()
    , m_d( new Private() )
{
    if (this != &rhs) {
        m_d->longRunningFilterTimer = 0;
        m_d->parentLayer = 0;
        if (rhs.m_datamanager) {
            m_datamanager = new KisDataManager(*rhs.m_datamanager);
            Q_CHECK_PTR(m_datamanager);
        }
        else {
            kWarning() << "rhs " << rhs.objectName() << " has no datamanager\n";
        }
        m_d->x = rhs.m_d->x;
        m_d->y = rhs.m_d->y;
        m_d->colorSpace = rhs.m_d->colorSpace;
        m_d->hasSelection = false;
        m_d->selection = 0;

        m_d->pixelSize = rhs.m_d->pixelSize;

        m_d->nChannels = rhs.m_d->nChannels;
        m_d->paintEngine = rhs.m_d->paintEngine;
    }
}

KisPaintDevice::~KisPaintDevice()
{
    delete m_d->longRunningFilterTimer;
    QList<KisFilter*>::iterator it;
    QList<KisFilter*>::iterator end = m_d->longRunningFilters.end();
    for (it = m_d->longRunningFilters.begin(); it != end; ++it) {
        KisFilter * f = (*it);
        delete f;
    }
    m_d->longRunningFilters.clear();
    delete m_d->paintEngine;
    delete m_d;
}

QPaintEngine * KisPaintDevice::paintEngine () const
{
    return m_d->paintEngine;
}

int KisPaintDevice::metric( PaintDeviceMetric metric ) const
{
    QRect rc = exactBounds();
    int depth = colorSpace()->pixelSize() - 1;

    switch (metric) {
    case PdmWidth:
        return rc.width();
        break;

    case PdmHeight:
        return rc.height();
        break;

    case PdmWidthMM:
        return qRound( rc.width() * ( 72 / 25.4 ) );
        break;

    case PdmHeightMM:
        return qRound( rc.height() * ( 72 / 25.4 ) );
        break;

    case PdmNumColors:
        return depth * depth;
        break;

    case PdmDepth:
        return qRound( colorSpace()->pixelSize() * 8 ); // in bits
        break;

    case PdmDpiX:
        return 72;
        break;

    case PdmDpiY:
        return 72;
        break;

    case PdmPhysicalDpiX:
        return 72;
        break;

    case PdmPhysicalDpiY:
        return 72;
        break;
    default:
        qWarning("QImage::metric(): Unhandled metric type %d", metric);
        break;
    }
    return 0;
}

void KisPaintDevice::startBackgroundFilters()
{
    m_d->longRunningFilters = m_d->colorSpace->createBackgroundFilters();
    if (!m_d->longRunningFilters.isEmpty()) {
        m_d->longRunningFilterTimer = new QTimer(this);
        connect(m_d->longRunningFilterTimer, SIGNAL(timeout()), this, SLOT(runBackgroundFilters()));
        m_d->longRunningFilterTimer->start(800);
    }
}

void KisPaintDevice::setDirty(const QRect & rc)
{

#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds |= rc;
#endif
    emit dirtied( rc );
}

void KisPaintDevice::setDirty( const QRegion & region )
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds |= region.boundingRect();
#endif
    emit dirtied( region );
}

void KisPaintDevice::setDirty()
{
    emit dirtied();
}

KisImageSP KisPaintDevice::image() const
{
    if (m_d->parentLayer) {
        return m_d->parentLayer->image();
    } else {
        return 0;
    }
}


void KisPaintDevice::move(qint32 x, qint32 y)
{
    QRect dirtyRegion = extent();

    m_d->x = x;
    m_d->y = y;

    dirtyRegion |= extent();

    if(m_d->selection)
    {
        m_d->selection->setX(x);
        m_d->selection->setY(y);
    }

    setDirty(dirtyRegion);

    emit positionChanged(KisPaintDeviceSP(this));
}

void KisPaintDevice::move(const QPoint& pt)
{
    move(pt.x(), pt.y());
}

void KisPaintDevice::extent(qint32 &x, qint32 &y, qint32 &w, qint32 &h) const
{
    m_datamanager->extent(x, y, w, h);
    x += m_d->x;
    y += m_d->y;
}

QRect KisPaintDevice::extent() const
{
    qint32 x, y, w, h;
    extent(x, y, w, h);
    return QRect(x, y, w, h);
}

QRegion KisPaintDevice::region() const
{
    return m_datamanager->region();
}

void KisPaintDevice::exactBounds(qint32 &x, qint32 &y, qint32 &w, qint32 &h) const
{
    QRect r = exactBounds();
    x = r.x();
    y = r.y();
    w = r.width();
    h = r.height();
}

QRect KisPaintDevice::exactBoundsImprovedOldMethod() const
{
    // Solution n°2
    Q_INT32  x, y, w, h, boundX2, boundY2, boundW2, boundH2;
    extent(x, y, w, h);
    extent(boundX2, boundY2, boundW2, boundH2);

    const Q_UINT8* defaultPixel = m_datamanager->defaultPixel();
    bool found = false;
    {
        KisHLineConstIterator it = const_cast<KisPaintDevice *>(this)->createHLineConstIterator(x, y, w);
        for (Q_INT32 y2 = y; y2 < y + h ; ++y2) {
            while (!it.isDone() && found == false) {
                if (memcmp(it.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                    boundY2 = y2;
                    found = true;
                    break;
                }
                ++it;
            }
            if (found) break;
            it.nextRow();
        }
    }

    found = false;

    for (Q_INT32 y2 = y + h; y2 > y ; --y2) {
        KisHLineConstIterator it = const_cast<KisPaintDevice *>(this)->createHLineConstIterator(x, y2, w);
        while (!it.isDone() && found == false) {
            if (memcmp(it.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                boundH2 = y2 - boundY2 + 1;
                found = true;
                break;
            }
            ++it;
        }
        if (found) break;
    }
    found = false;

    {
        KisVLineConstIterator it = const_cast<KisPaintDevice *>(this)->createVLineConstIterator(x, boundY2, boundH2);
        for (Q_INT32 x2 = x; x2 < x + w ; ++x2) {
            while (!it.isDone() && found == false) {
                if (memcmp(it.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                    boundX2 = x2;
                    found = true;
                    break;
                }
                ++it;
            }
            if (found) break;
            it.nextCol();
        }
    }

    found = false;

    // Look for right edge )
    {
        for (Q_INT32 x2 = x + w; x2 > x ; --x2) {
            KisVLineConstIterator it = const_cast<KisPaintDevice *>(this)->createVLineConstIterator(/*x + w*/ x2, boundY2, boundH2);
            while (!it.isDone() && found == false) {
                if (memcmp(it.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                    boundW2 = x2 - boundX2 + 1; // XXX: I commented this
                                            // +1 out, but why? It
                                            // should be correct, since
                                            // we've found the first
                                            // pixel that should be
                                            // included, and it should
                                            // be added to the width.
                    found = true;
                    break;
                }
                ++it;
            }
            if (found) break;
        }
    }
    return QRect(boundX2, boundY2, boundW2, boundH2);
}

QRect KisPaintDevice::exactBoundsOldMethod() const
{
//    QTime t;
//    t.start();

    qint32 x, y, w, h, boundX, boundY, boundW, boundH;

    extent(x, y, w, h);
    extent(boundX, boundY, boundW, boundH);

    const quint8* defaultPixel = m_datamanager->defaultPixel();

    bool found = false;

    KisHLineConstIterator it = this->createHLineConstIterator(x, y, w);
    for (qint32 y2 = y; y2 < y + h ; ++y2) {
        while (!it.isDone() && found == false) {
            if (memcmp(it.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                boundY = y2;
                found = true;
                break;
            }
            ++it;
        }
        it.nextRow();
        if (found) break;
    }

    found = false;

    for (qint32 y2 = y + h; y2 > y ; --y2) {
        it = this->createHLineConstIterator(x, y2, w);
        while (!it.isDone() && found == false) {
            if (memcmp(it.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                boundH = y2 - boundY + 1;
                found = true;
                break;
            }
            ++it;
        }
        if (found) break;
    }

    found = false;

    KisVLineConstIterator vit = createVLineConstIterator(x, boundY, boundH);
    for (qint32 x2 = x; x2 < x + w ; ++x2) {
        while (!vit.isDone() && found == false) {
            if (memcmp(vit.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                boundX = x2;
                found = true;
                break;
            }
            ++vit;
        }
        vit.nextCol();
        if (found) break;
    }

    found = false;

    // Look for right edge )
    for (qint32 x2 = x + w; x2 > x ; --x2) {
        vit = this->createVLineConstIterator(x2, y, h);
        while (!vit.isDone() && found == false) {
            if (memcmp(vit.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                boundW = x2 - boundX + 1; // XXX: I commented this
                                          // +1 out, but why? It
                                          // should be correct, since
                                          // we've found the first
                                          // pixel that should be
                                          // included, and it should
                                          // be added to the width.
                found = true;
                break;
            }
            ++vit;
        }
        if (found) break;
    }
//    kDebug() << "Exactbounds " << boundX << ", " << boundY << ", " << boundW << ", " << boundH << " took " << t.elapsed() << endl;
    return QRect(boundX, boundY, boundW, boundH);


}

QRect KisPaintDevice::exactBounds() const
{
#ifdef CACHE_EXACT_BOUNDS
    return m_d->exactBounds;
#else
    return exactBoundsImprovedOldMethod();
#endif
}


void KisPaintDevice::crop(qint32 x, qint32 y, qint32 w, qint32 h)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds = QRect( x, y, w, h );
#endif
    m_datamanager->setExtent(x - m_d->x, y - m_d->y, w, h);
}


void KisPaintDevice::crop(const QRect & r)
{
    QRect rc( r );
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds = rc;
#endif
    rc.translate(-m_d->x, -m_d->y);
    m_datamanager->setExtent(rc);
}

void KisPaintDevice::clear()
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds = QRect( 0, 0, 0, 0 );
#endif
    m_datamanager->clear();
}

void KisPaintDevice::fill(qint32 x, qint32 y, qint32 w, qint32 h, const quint8 *fillPixel)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds &= QRect( x, y, w, h );
#endif
    m_datamanager->clear(x, y, w, h, fillPixel);
}


void KisPaintDevice::clear( const QRect & rc )
{
#ifdef CACHE_EXACT_BOUNDS
    if ( rc.contains( m_d->exactBounds) )
         m_d->exactBounds = QRect();
#endif
    m_datamanager->clear( rc.x(), rc.y(), rc.width(), rc.height(), m_datamanager->defaultPixel() );
}

void KisPaintDevice::mirrorX()
{
    QRect r;
    if (hasSelection()) {
        r = selection()->selectedExactRect();
    }
    else {
        r = exactBounds();
    }

    KisHLineConstIteratorPixel srcIt = createHLineConstIterator(r.x(), r.top(), r.width());
    KisHLineIteratorPixel dstIt = createHLineIterator(r.x(), r.top(), r.width());

    for (qint32 y = r.top(); y <= r.bottom(); ++y) {

        dstIt += r.width() - 1;

        while (!srcIt.isDone()) {
            if (srcIt.isSelected()) {
                memcpy(dstIt.rawData(), srcIt.oldRawData(), m_d->pixelSize);
            }
            ++srcIt;
            --dstIt;

        }
        srcIt.nextRow();
        dstIt.nextRow();
    }
    if (m_d->parentLayer) {
        m_d->parentLayer->setDirty(r);
    }
}

void KisPaintDevice::mirrorY()
{
    /* Read a line from bottom to top and and from top to bottom and write their values to each other */
    QRect r;
    if (hasSelection()) {
        r = selection()->selectedExactRect();
    }
    else {
        r = exactBounds();
    }


    qint32 y1, y2;
    for (y1 = r.top(), y2 = r.bottom(); y1 <= r.bottom(); ++y1, --y2) {
        KisHLineIteratorPixel itTop = createHLineIterator(r.x(), y1, r.width());
        KisHLineConstIteratorPixel itBottom = createHLineConstIterator(r.x(), y2, r.width());
        while (!itTop.isDone() && !itBottom.isDone()) {
            if (itBottom.isSelected()) {
                memcpy(itTop.rawData(), itBottom.oldRawData(), m_d->pixelSize);
            }
            ++itBottom;
            ++itTop;
        }
    }

    if (m_d->parentLayer) {
        m_d->parentLayer->setDirty(r);
    }
}

KisMementoSP KisPaintDevice::getMemento()
{
    return m_datamanager->getMemento();
}

void KisPaintDevice::rollback(KisMementoSP memento) { m_datamanager->rollback(memento); }

void KisPaintDevice::rollforward(KisMementoSP memento) { m_datamanager->rollforward(memento); }

bool KisPaintDevice::write(KoStore *store)
{
    bool retval = m_datamanager->write(store);
    emit ioProgress(100);

        return retval;
}

bool KisPaintDevice::read(KoStore *store)
{
    bool retval = m_datamanager->read(store);
    emit ioProgress(100);

        return retval;
}

void KisPaintDevice::convertTo(KoColorSpace * dstColorSpace, qint32 renderingIntent)
{
    if ( (colorSpace()->id() == dstColorSpace->id()) )
    {
        return;
    }

    KisPaintDevice dst(dstColorSpace);
    dst.setX(getX());
    dst.setY(getY());

    qint32 x, y, w, h;
    extent(x, y, w, h);

    for (qint32 row = y; row < y + h; ++row) {

        qint32 column = x;
        qint32 columnsRemaining = w;

        while (columnsRemaining > 0) {

            qint32 numContiguousDstColumns = dst.numContiguousColumns(column, row, row);
            qint32 numContiguousSrcColumns = numContiguousColumns(column, row, row);

            qint32 columns = qMin(numContiguousDstColumns, numContiguousSrcColumns);
            columns = qMin(columns, columnsRemaining);

            KisHLineConstIteratorPixel srcIt = createHLineConstIterator(column, row, columns);
            KisHLineIteratorPixel dstIt = dst.createHLineIterator(column, row, columns);

            const quint8 *srcData = srcIt.rawData();
            quint8 *dstData = dstIt.rawData();

            m_d->colorSpace->convertPixelsTo(srcData, dstData, dstColorSpace, columns, renderingIntent);

            column += columns;
            columnsRemaining -= columns;
        }
    }

    KisDataManagerSP oldData = m_datamanager;
    KoColorSpace *oldColorSpace = m_d->colorSpace;

    setData(dst.m_datamanager, dstColorSpace);

    if (undoAdapter() && undoAdapter()->undo()) {
        undoAdapter()->addCommand(new KisConvertLayerTypeCmd(KisPaintDeviceSP(this), oldData, oldColorSpace, m_datamanager, m_d->colorSpace));
    }
    // XXX: emit colorSpaceChanged(dstColorSpace);
}

void KisPaintDevice::setProfile(KoColorProfile * profile)
{
    if (profile == 0) return;

    KoColorSpace * dstSpace =
            KoColorSpaceRegistry::instance()->colorSpace( colorSpace()->id(),
                                                                      profile);
    if (dstSpace)
        m_d->colorSpace = dstSpace;
    emit profileChanged(profile);
}

void KisPaintDevice::setData(KisDataManagerSP data, KoColorSpace * colorSpace)
{
    m_datamanager = data;
    m_d->colorSpace = colorSpace;
    m_d->pixelSize = m_d->colorSpace->pixelSize();
    m_d->nChannels = m_d->colorSpace->channelCount();

    if (m_d->parentLayer) {
        m_d->parentLayer->setDirty(extent());
        m_d->parentLayer->notifyPropertyChanged();
    }
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds = extent();
#endif

}

KisUndoAdapter *KisPaintDevice::undoAdapter() const
{
    if (m_d->parentLayer && m_d->parentLayer->image()) {
        return m_d->parentLayer->image()->undoAdapter();
    }
    return 0;
}

void KisPaintDevice::convertFromQImage(const QImage& image, const QString &srcProfileName,
                                       qint32 offsetX, qint32 offsetY)
{
    Q_UNUSED( srcProfileName );

    QImage img = image;

    if (img.format() != QImage::Format_ARGB32) {
        img = img.convertToFormat(QImage::Format_ARGB32);
    }
#ifdef __GNUC__
#warning "KisPaintDevice::convertFromQImage. re-enable use of srcProfileName"
#endif
#if 0

    // XXX: Apply import profile
    if (colorSpace() == KoColorSpaceRegistry::instance() ->colorSpace("RGBA",0)) {
        writeBytes(img.bits(), 0, 0, img.width(), img.height());
    }
    else {
#endif
#if 0
        quint8 * dstData = new quint8[img.width() * img.height() * pixelSize()];
        KoColorSpaceRegistry::instance()
                ->colorSpace("RGBA", srcProfileName)->
                        convertPixelsTo(img.bits(), dstData, colorSpace(), img.width() * img.height());
        writeBytes(dstData, offsetX, offsetY, img.width(), img.height());
#endif
        KisPainter p( this );
        p.bitBlt(offsetX, offsetY, colorSpace()->compositeOp( COMPOSITE_OVER ),
                 &image, OPACITY_OPAQUE,
                 0, 0, image.width(), image.height());
        p.end();
#ifdef CACHE_EXACT_BOUNDS
        m_d->exactBounds = image.rect();
#endif
//    }
}

QImage KisPaintDevice::convertToQImage(KoColorProfile *  dstProfile, float exposure)
{
    qint32 x1;
    qint32 y1;
    qint32 w;
    qint32 h;

    x1 = - getX();
    y1 = - getY();

    if (image()) {
        w = image()->width();
        h = image()->height();
    }
    else {
        extent(x1, y1, w, h);
    }

    return convertToQImage(dstProfile, x1, y1, w, h, exposure);
}

QImage KisPaintDevice::convertToQImage(KoColorProfile *  dstProfile, qint32 x1, qint32 y1, qint32 w, qint32 h, float exposure)
{
    if (w < 0)
        return QImage();

    if (h < 0)
        return QImage();

    quint8 * data;
    try {
        data = new quint8 [w * h * m_d->pixelSize];
    } catch(std::bad_alloc)
    {
        kWarning() << "KisPaintDevice::convertToQImage std::bad_alloc for " << w << " * " << h << " * " << m_d->pixelSize << endl;
        //delete[] data; // data is not allocated, so don't free it
        return QImage();
    }
    Q_CHECK_PTR(data);

    // XXX: Is this really faster than converting line by line and building the QImage directly?
    //      This copies potentially a lot of data.
    readBytes(data, x1, y1, w, h);
    QImage image = colorSpace()->convertToQImage(data, w, h, dstProfile, INTENT_PERCEPTUAL, exposure);
    delete[] data;

    return image;
}

KisPaintDeviceSP KisPaintDevice::createThumbnailDevice(qint32 w, qint32 h) const
{
    KisPaintDeviceSP thumbnail = KisPaintDeviceSP(new KisPaintDevice(colorSpace(), "thumbnail"));
    thumbnail->clear();

    int srcw, srch;
    if( image() )
    {
        srcw = image()->width();
        srch = image()->height();
    }
    else
    {
        const QRect e = exactBounds();
        srcw = e.width();
        srch = e.height();
    }

    if (w > srcw)
    {
        w = srcw;
        h = qint32(double(srcw) / w * h);
    }
    if (h > srch)
    {
        h = srch;
        w = qint32(double(srch) / h * w);
    }

    if (srcw > srch)
        h = qint32(double(srch) / srcw * w);
    else if (srch > srcw)
        w = qint32(double(srcw) / srch * h);

    KisRandomConstAccessorPixel iter = createRandomConstAccessor(0,0);
    for (qint32 y = 0; y < h; ++y) {
        qint32 iY = (y * srch ) / h;
        for (qint32 x = 0; x < w; ++x) {
            qint32 iX = (x * srcw ) / w;
            iter.moveTo(iX, iY);
            thumbnail->setPixel(x, y, KoColor(iter.rawData(), m_d->colorSpace));
        }
    }

    return thumbnail;

}


QImage KisPaintDevice::createThumbnail(qint32 w, qint32 h)
{
    KisPaintDeviceSP dev = createThumbnailDevice( w, h );
    return dev->convertToQImage( KoColorSpaceRegistry::instance()->rgb8()->profile() );
}

KisRectIteratorPixel KisPaintDevice::createRectIterator(qint32 left, qint32 top, qint32 w, qint32 h)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds &= QRect( left, top, w, h );
#endif
    KisDataManager* selectionDm = 0;

    if(hasSelection())
        selectionDm = m_d->selection->m_datamanager.data();

    return KisRectIteratorPixel(m_datamanager.data(), selectionDm, left, top, w, h, m_d->x, m_d->y);
}

KisRectConstIteratorPixel KisPaintDevice::createRectConstIterator(qint32 left, qint32 top, qint32 w, qint32 h) const
{
    KisDataManager* dm = const_cast< KisDataManager*>(m_datamanager.data()); // TODO: don't do this
    KisDataManager* selectionDm = 0;

    if(hasSelection())
        selectionDm = const_cast< KisDataManager*>(m_d->selection->m_datamanager.data());

    return KisRectConstIteratorPixel(dm, selectionDm, left, top, w, h, m_d->x, m_d->y);
}

KisHLineIteratorPixel  KisPaintDevice::createHLineIterator(qint32 x, qint32 y, qint32 w)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds &= QRect( x, y, w, 1 );
#endif

    KisDataManager* selectionDm = 0;

    if(hasSelection())
        selectionDm = m_d->selection->m_datamanager.data();

    return KisHLineIteratorPixel(m_datamanager.data(), selectionDm, x, y, w, m_d->x, m_d->y);
}

KisHLineConstIteratorPixel  KisPaintDevice::createHLineConstIterator(qint32 x, qint32 y, qint32 w) const
{
    KisDataManager* dm = const_cast< KisDataManager*>(m_datamanager.data()); // TODO: don't do this
    KisDataManager* selectionDm = 0;

    if(hasSelection())
        selectionDm = const_cast< KisDataManager*>(m_d->selection->m_datamanager.data());

    return KisHLineConstIteratorPixel(dm, selectionDm, x, y, w, m_d->x, m_d->y);
}

KisVLineIteratorPixel  KisPaintDevice::createVLineIterator(qint32 x, qint32 y, qint32 h)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds &= QRect( x, y, 1, h );
#endif

    KisDataManager* selectionDm = 0;

    if(hasSelection())
        selectionDm = m_d->selection->m_datamanager.data();

    return KisVLineIteratorPixel(m_datamanager.data(), selectionDm, x, y, h, m_d->x, m_d->y);
}

KisVLineConstIteratorPixel  KisPaintDevice::createVLineConstIterator(qint32 x, qint32 y, qint32 h) const
{
    KisDataManager* dm = const_cast< KisDataManager*>(m_datamanager.data()); // TODO: don't do this
    KisDataManager* selectionDm = 0;

    if(hasSelection())
        selectionDm = const_cast< KisDataManager*>(m_d->selection->m_datamanager.data());

    return KisVLineConstIteratorPixel(dm, selectionDm, x, y, h, m_d->x, m_d->y);
}

KisRandomAccessorPixel KisPaintDevice::createRandomAccessor(qint32 x, qint32 y)
{
    KisDataManager* selectionDm = 0;

    if(hasSelection())
        selectionDm = m_d->selection->m_datamanager.data();

    return KisRandomAccessorPixel(m_datamanager.data(), selectionDm, x, y, m_d->x, m_d->y);
}

KisRandomConstAccessorPixel KisPaintDevice::createRandomConstAccessor(qint32 x, qint32 y) const {
    KisDataManager* dm = const_cast< KisDataManager*>(m_datamanager.data()); // TODO: don't do this
    KisDataManager* selectionDm = 0;

    if(hasSelection())
        selectionDm = const_cast< KisDataManager*>(m_d->selection->m_datamanager.data());

    return KisRandomConstAccessorPixel(dm, selectionDm, x, y, m_d->x, m_d->y);
}

KisRandomSubAccessorPixel KisPaintDevice::createRandomSubAccessor() const
{
    KisPaintDevice* pd = const_cast<KisPaintDevice*>(this);
    return KisRandomSubAccessorPixel(pd);
}

void KisPaintDevice::emitSelectionChanged()
{
    if (m_d->parentLayer && m_d->parentLayer->image()) {
        m_d->parentLayer->image()->slotSelectionChanged();
    }
}

void KisPaintDevice::emitSelectionChanged(const QRect& r)
{
    if (m_d->parentLayer && m_d->parentLayer->image()) {
        m_d->parentLayer->image()->slotSelectionChanged(r);
    }
}

KisSelectionSP KisPaintDevice::selection()
{
    // XXX: Fix when we fix the API

    if ( m_d->selectionDeselected && m_d->selection ) {
        m_d->selectionDeselected = false;
    }
    else if (!m_d->selection) {
        m_d->selection = KisSelectionSP(new KisSelection(KisPaintDeviceSP(this)));
        Q_CHECK_PTR(m_d->selection);
        m_d->selection->setX(m_d->x);
        m_d->selection->setY(m_d->y);
    }
    m_d->hasSelection = true;

    //TODO find a better place for this call
    m_d->selection->updateProjection();
    return m_d->selection;
}

const KisSelectionSP KisPaintDevice::selection() const
{
    if ( m_d->selectionDeselected && m_d->selection ) {
        m_d->selectionDeselected = false;
    }
    else if (!m_d->selection) {
        m_d->selection = KisSelectionSP(new KisSelection(const_cast<KisPaintDevice*>(this)));
        Q_CHECK_PTR(m_d->selection);
        m_d->selection->setX(m_d->x);
        m_d->selection->setY(m_d->y);
    }
    m_d->hasSelection = true;

    return m_d->selection;
}


bool KisPaintDevice::hasSelection() const
{
    return m_d->hasSelection;
}

bool KisPaintDevice::selectionDeselected()
{
    return m_d->selectionDeselected;
}


void KisPaintDevice::deselect()
{
    if (m_d->selection && m_d->hasSelection) {
        m_d->hasSelection = false;
        m_d->selectionDeselected = true;
    }
}

void KisPaintDevice::reselect()
{
    m_d->hasSelection = true;
    m_d->selectionDeselected = false;
}

void KisPaintDevice::addSelection(KisSelectionSP selection) {

    KisPainter painter(this->selection());
    QRect r = selection->selectedExactRect();
    painter.bitBlt(r.x(), r.y(), COMPOSITE_OVER, KisPaintDeviceSP(selection.data()), r.x(), r.y(), r.width(), r.height());
    painter.end();
}

void KisPaintDevice::subtractSelection(KisSelectionSP selection) {
    KisPainter painter(KisPaintDeviceSP(this->selection().data()));
    selection->invert();

    QRect r = selection->selectedExactRect();
    painter.bitBlt(r.x(), r.y(), COMPOSITE_ERASE, KisPaintDeviceSP(selection.data()), r.x(), r.y(), r.width(), r.height());

    selection->invert();
    painter.end();
}

void KisPaintDevice::clearSelection()
{
    if (!hasSelection()) return;

    QRect r = m_d->selection->selectedExactRect();

    if (r.isValid()) {

        KisHLineIterator devIt = createHLineIterator(r.x(), r.y(), r.width());
        KisHLineConstIterator selectionIt = m_d->selection->createHLineIterator(r.x(), r.y(), r.width());

        for (qint32 y = 0; y < r.height(); y++) {

            while (!devIt.isDone()) {
                // XXX: Optimize by using stretches

                m_d->colorSpace->applyInverseAlphaU8Mask( devIt.rawData(), selectionIt.rawData(), 1);

                ++devIt;
                ++selectionIt;
            }
            devIt.nextRow();
            selectionIt.nextRow();
        }

        if (m_d->parentLayer) {
            m_d->parentLayer->setDirty(r);
        }
    }
}

void KisPaintDevice::applySelectionMask(KisSelectionSP mask)
{
    QRect r = mask->selectedExactRect();
    crop(r);

    KisHLineIterator pixelIt = createHLineIterator(r.x(), r.top(), r.width());
    KisHLineConstIterator maskIt = mask->createHLineIterator(r.x(), r.top(), r.width());

    for (qint32 y = r.top(); y <= r.bottom(); ++y) {

        while (!pixelIt.isDone()) {
            // XXX: Optimize by using stretches

            m_d->colorSpace->applyAlphaU8Mask( pixelIt.rawData(), maskIt.rawData(), 1);

            ++pixelIt;
            ++maskIt;
        }
        pixelIt.nextRow();
        maskIt.nextRow();
    }
}

KisSelectionSP KisPaintDevice::setSelection( KisSelectionSP selection)
{
    if (selection) {
        KisSelectionSP oldSelection = m_d->selection;
        m_d->selection = selection;
        m_d->hasSelection = true;
        return oldSelection;
    }
    else return KisSelectionSP(0);
}

bool KisPaintDevice::pixel(qint32 x, qint32 y, QColor *c, quint8 *opacity)
{
    KisHLineConstIteratorPixel iter = createHLineIterator(x, y, 1);

    const quint8 *pix = iter.rawData();

    if (!pix) return false;

    colorSpace()->toQColor(pix, c, opacity);

    return true;
}


bool KisPaintDevice::pixel(qint32 x, qint32 y, KoColor * kc)
{
    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1);

    quint8 *pix = iter.rawData();

    if (!pix) return false;

    kc->setColor(pix, m_d->colorSpace);

    return true;
}

KoColor KisPaintDevice::colorAt(qint32 x, qint32 y) const
{
    KisRandomConstAccessorPixel iter = createRandomConstAccessor(x, y);
    return KoColor(iter.rawData(), m_d->colorSpace);
}

bool KisPaintDevice::setPixel(qint32 x, qint32 y, const QColor& c, quint8  opacity)
{
    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1);

    colorSpace()->fromQColor(c, opacity, iter.rawData());

    return true;
}

bool KisPaintDevice::setPixel(qint32 x, qint32 y, const KoColor& kc)
{
    quint8 * pix;
    if (kc.colorSpace() != m_d->colorSpace) {
        KoColor kc2 (kc, m_d->colorSpace);
        pix = kc2.data();
    }
    else {
        pix = kc.data();
    }

    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1);
    memcpy(iter.rawData(), pix, m_d->colorSpace->pixelSize());

    return true;
}


qint32 KisPaintDevice::numContiguousColumns(qint32 x, qint32 minY, qint32 maxY) const
{
    return m_datamanager->numContiguousColumns(x - m_d->x, minY - m_d->y, maxY - m_d->y);
}

qint32 KisPaintDevice::numContiguousRows(qint32 y, qint32 minX, qint32 maxX) const
{
    return m_datamanager->numContiguousRows(y - m_d->y, minX - m_d->x, maxX - m_d->x);
}

qint32 KisPaintDevice::rowStride(qint32 x, qint32 y) const
{
    return m_datamanager->rowStride(x - m_d->x, y - m_d->y);
}

void KisPaintDevice::setX(qint32 x)
{
    m_d->x = x;
    if(m_d->selection && m_d->selection.data() != this)
        m_d->selection->setX(x);
}

void KisPaintDevice::setY(qint32 y)
{
    m_d->y = y;
    if(m_d->selection && m_d->selection.data() != this)
        m_d->selection->setY(y);
}


void KisPaintDevice::readBytes(quint8 * data, qint32 x, qint32 y, qint32 w, qint32 h)
{
    m_datamanager->readBytes(data, x - m_d->x, y - m_d->y, w, h);
}

void KisPaintDevice::readBytes(quint8 * data, const QRect &rect)
{
    readBytes(data, rect.x(), rect.y(), rect.width(), rect.height());
}

void KisPaintDevice::writeBytes(const quint8 * data, qint32 x, qint32 y, qint32 w, qint32 h)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds &= QRect( x, y, w, h );
#endif
    m_datamanager->writeBytes( data, x - m_d->x, y - m_d->y, w, h);
}

void KisPaintDevice::writeBytes(const quint8 * data, const QRect &rect)
{
    writeBytes(data, rect.x(), rect.y(), rect.width(), rect.height());
}

KisDataManagerSP KisPaintDevice::dataManager() const
{
    return m_datamanager;
}

void KisPaintDevice::runBackgroundFilters()
{
    QRect rc = exactBounds();
    if (!m_d->longRunningFilters.isEmpty()) {
        QList<KisFilter*>::iterator it;
        QList<KisFilter*>::iterator end = m_d->longRunningFilters.end();
        for (it = m_d->longRunningFilters.begin(); it != end; ++it) {
            (*it)->process(this, rc, 0);
        }
    }
    if (m_d->parentLayer) m_d->parentLayer->setDirty(rc);
}


qint32 KisPaintDevice::pixelSize() const
{
    Q_ASSERT(m_d->pixelSize > 0);
    return m_d->pixelSize;
}

qint32 KisPaintDevice::channelCount() const
{
    Q_ASSERT(m_d->nChannels > 0);
    return m_d->nChannels;
;
}

KoColorSpace * KisPaintDevice::colorSpace() const
{
    Q_ASSERT(m_d->colorSpace != 0);
    return m_d->colorSpace;
}


qint32 KisPaintDevice::getX() const
{
    return m_d->x;
}

qint32 KisPaintDevice::getY() const
{
    return m_d->y;
}

KisMaskSP KisPaintDevice::painterlyChannel( const QString & channelId )
{
    if (m_d->painterlyChannels.contains(channelId))
        return m_d->painterlyChannels.value(channelId);

    return 0;
}

const QHash<QString,KisMaskSP> KisPaintDevice::painterlyChannels()
{
    return m_d->painterlyChannels;
}

void KisPaintDevice::addPainterlyChannel( KisMaskSP painterlyChannel )
{
    m_d->painterlyChannels[painterlyChannel->id()] = painterlyChannel;
}

KisMaskSP KisPaintDevice::removePainterlyChannel( const QString & channelId )
{
    if ( m_d->painterlyChannels.contains( channelId ) ) {
        return m_d->painterlyChannels.take( channelId );
    }
    return 0;
}


#include "kis_paint_device.moc"
