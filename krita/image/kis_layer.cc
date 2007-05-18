/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
 *
 *  this program is free software; you can redistribute it and/or modify
 *  it under the terms of the gnu general public license as published by
 *  the free software foundation; either version 2 of the license, or
 *  (at your option) any later version.
 *
 *  this program is distributed in the hope that it will be useful,
 *  but without any warranty; without even the implied warranty of
 *  merchantability or fitness for a particular purpose.  see the
 *  gnu general public license for more details.
 *
 *  you should have received a copy of the gnu general public license
 *  along with this program; if not, write to the free software
 *  foundation, inc., 675 mass ave, cambridge, ma 02139, usa.
 */

#include <kdebug.h>
#include <kicon.h>
#include <QIcon>
#include <QImage>
#include <QBitArray>

#include "kis_debug_areas.h"
#include "kis_group_layer.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_painter.h"

#include "kis_mask.h"
#include "kis_effect_mask.h"
#include "kis_transparency_mask.h"

static int getID()
{
    static int id = 1;
    return id++;
}

class KisLayer::Private {

public:

    int id;
    int index;
    quint8 opacity;
    bool locked;
    bool visible;
    bool temporary;

    QString name;
    KisGroupLayerWSP parent;
    KisImageWSP image;
    QBitArray channelFlags;

    // XXX: Make these (weak) shared pointers?
    KisTransparencyMask* transparencyMask;
    KisEffectMaskSP previewMask;
    QList<KisEffectMask*> effectMasks;

    // Operation used to composite this layer with the projection of
    // the layers _under_ this layer
    const KoCompositeOp * compositeOp;
#ifdef DIRTY_AND_PROJECTION
    QRegion dirtyRegion; // XXX: this should be part of the data manager!
#endif
};


KisLayer::KisLayer(KisImageWSP img, const QString &name, quint8 opacity)
    : QObject(0)
    , m_d( new Private )
{
    m_d->id = getID();
    m_d->index = -1;
    m_d->opacity = opacity;
    m_d->locked = false;
    m_d->visible = true;
    m_d->temporary = false;
    m_d->name = name;
    m_d->parent = 0;
    m_d->image = img;
    m_d->compositeOp = const_cast<KoCompositeOp*>( img->colorSpace()->compositeOp( COMPOSITE_OVER ) );
    setObjectName(name);
}

KisLayer::KisLayer(const KisLayer& rhs)
    : QObject( 0 )
    , KisShared(rhs)
    , m_d( new Private() )
{
    if (this != &rhs) {
        m_d->id = getID();
        m_d->index = -1;
        m_d->opacity = rhs.m_d->opacity;
        m_d->locked = rhs.m_d->locked;
        m_d->visible = rhs.m_d->visible;
        m_d->temporary = rhs.m_d->temporary;
        m_d->name = rhs.m_d->name;
        m_d->image = rhs.m_d->image;
        m_d->parent = 0;
        m_d->compositeOp = rhs.m_d->compositeOp;
    }
}

KisLayer::~KisLayer()
{
}

KoColorSpace * KisLayer::colorSpace()
{
    return m_d->image->colorSpace();
}

KoDocumentSectionModel::PropertyList KisLayer::properties() const
{
    KoDocumentSectionModel::PropertyList l;
    l << KoDocumentSectionModel::Property(i18n("Visible"), KIcon("visible"), KIcon("novisible"), visible());
    l << KoDocumentSectionModel::Property(i18n("Locked"), KIcon("locked"), KIcon("unlocked"), locked());
    l << KoDocumentSectionModel::Property(i18n("Opacity"), i18n("%1%", percentOpacity()));
    l << KoDocumentSectionModel::Property(i18n("Composite Mode"), compositeOp()->id());
    return l;
}

void KisLayer::setProperties( const KoDocumentSectionModel::PropertyList &properties )
{
    setVisible( properties.at( 0 ).state.toBool() );
    setLocked( properties.at( 1 ).state.toBool() );
}

void KisLayer::setChannelFlags( QBitArray & channelFlags )
{
    Q_ASSERT( ( ( quint32 )channelFlags.count() == colorSpace()->channelCount() || channelFlags.isEmpty()) );
    m_d->channelFlags = channelFlags;
}

QBitArray & KisLayer::channelFlags()
{
    return m_d->channelFlags;
}


void KisLayer::activate()
{
    notifyPropertyChanged();
}

void KisLayer::deactivate()
{
    /*notifyPropertyChanged(this); not necessary, and causes scrolling issues */
}

bool KisLayer::isActive() const
{
    if (image())
        return this == image()->activeLayer().data();
    else
        return false;
}

void KisLayer::setActive()
{
    if (image())
        image()->activateLayer(KisLayerSP(this));
}



void KisLayer::setDirty()
{
    QRect rc = extent();
    setDirty( QRegion( rc ) );
}

void KisLayer::setDirty(const QRect & rc)
{
    setDirty( QRegion( rc ) );
}

void KisLayer::setDirty( const QRegion & region)
{
    // If we're dirty, our parent is dirty, if we've got a parent
    if ( region.isEmpty() ) return;

    if (m_d->parent) {
        m_d->parent->setDirty(region);
    }
    if (m_d->image.data()) {
        m_d->image->notifyLayerUpdated(KisLayerSP(this));
    }
#ifdef DIRTY_AND_PROJECTION
    m_d->dirtyRegion += region;
#endif
}

#ifdef DIRTY_AND_PROJECTION
bool KisLayer::isDirty( const QRect & rect )
{
    return m_d->dirtyRegion.intersects( rect );
}

void KisLayer::setClean( QRect rc )
{
    m_d->dirtyRegion -= QRegion( rc );
}
#endif

int KisLayer::id() const { return m_d->id; }

KisGroupLayerSP KisLayer::parentLayer()
{
    return m_d->parent;
}

const KisGroupLayerSP KisLayer::parentLayer() const
{
    return m_d->parent;
}

KisLayerSP KisLayer::prevSibling() const
{
    if (!parentLayer())
        return KisLayerSP(0);
    return parentLayer()->at(index() - 1);
}

KisLayerSP KisLayer::nextSibling() const
{
    if (!parentLayer())
        return KisLayerSP(0);
    return parentLayer()->at(index() + 1);
}

int KisLayer::index() const
{
    return m_d->index;
}

KisLayerSP KisLayer::findLayer(const QString& n) const
{
    if (name() == n)
        return KisLayerSP(const_cast<KisLayer*>(this)); //HACK any less ugly way? findLayer() is conceptually const...
    for (KisLayerSP layer = firstChild(); layer; layer = layer->nextSibling())
        if (KisLayerSP found = layer->findLayer(n))
            return found;
    return KisLayerSP(0);
}

KisLayerSP KisLayer::findLayer(int i) const
{
    if (id() == i)
        return KisLayerSP(const_cast<KisLayer*>(this)); //HACK
    for (KisLayerSP layer = firstChild(); layer; layer = layer->nextSibling())
        if (KisLayerSP found = layer->findLayer(i))
            return found;
    return KisLayerSP(0);
}

int KisLayer::numLayers(int flags) const
{
    int num = 0;
    if (matchesFlags(flags)) num++;
    for (KisLayer* layer = firstChild().data(); layer; layer = layer->nextSibling().data())
        num += layer->numLayers(flags);
    return num;
}

bool KisLayer::matchesFlags(int flags) const
{
    if ((flags & Visible) && !visible())
        return false;
    if ((flags & Hidden) && visible())
        return false;
    if ((flags & Locked) && !locked())
        return false;
    if ((flags & Unlocked) && locked())
        return false;
    return true;
}

quint8 KisLayer::opacity() const
{
    return m_d->opacity;
}

void KisLayer::setOpacity(quint8 val)
{
    if (m_d->opacity != val)
    {
        m_d->opacity = val;
        setDirty();
        notifyPropertyChanged();
    }
}

quint8 KisLayer::percentOpacity() const
{
    return int(float(opacity() * 100) / 255 + 0.5);
}

void KisLayer::setPercentOpacity(quint8 val)
{
    setOpacity(int(float(val * 255) / 100 + 0.5));
}

const bool KisLayer::visible() const
{
    return m_d->visible;
}

void KisLayer::setVisible(bool v)
{
    if (m_d->visible != v) {

        m_d->visible = v;
        notifyPropertyChanged();
        setDirty();
    }
}

bool KisLayer::locked() const
{
    return m_d->locked;
}

void KisLayer::setLocked(bool l)
{
    if (m_d->locked != l) {
        m_d->locked = l;
        notifyPropertyChanged();
    }
}

bool KisLayer::temporary() const
{
    return m_d->temporary;
}

void KisLayer::setTemporary(bool t)
{
    m_d->temporary = t;
}

QString KisLayer::name() const
{
        return m_d->name;
}

void KisLayer::setName(const QString& name)
{
    if (!name.isEmpty() && m_d->name != name)
    {
        m_d->name = name;
        notifyPropertyChanged();
    }
}

const KoCompositeOp * KisLayer::compositeOp() const
{
    return m_d->compositeOp;
}

void KisLayer::setCompositeOp(const KoCompositeOp* compositeOp)
{
    if (m_d->compositeOp != compositeOp)
    {
       m_d->compositeOp = const_cast<KoCompositeOp*>( compositeOp );
       notifyPropertyChanged();
       setDirty();
    }
}

KisImageSP KisLayer::image() const { return m_d->image; }

void KisLayer::setImage(KisImageSP image)
{
    m_d->image = image;
}

void KisLayer::paint(QImage &, qint32, qint32, qint32, qint32)
{
}

void KisLayer::paint(QImage &, const QRect&, const QSize&, const QSize&)
{
}

QImage KisLayer::createThumbnail(qint32, qint32)
{
    return QImage();
}

void KisLayer::notifyPropertyChanged()
{
    if(image() && !signalsBlocked())
        image()->notifyPropertyChanged(KisLayerSP(this));
}

void KisLayer::notifyCommandExecuted()
{
    notifyPropertyChanged();
}


void KisLayer::setIndexPrivate( int index )
{
    m_d->index = index;
}

void KisLayer::setCompositeOpPrivate( const KoCompositeOp * op )
{
    m_d->compositeOp = op;
}

void KisLayer::setParentPrivate( KisGroupLayerSP parent )
{
    m_d->parent = parent;
}

bool KisLayer::hasEffectMasks()
{
    // If all these things don't exist, we have no effectMasks.
    return !( m_d->transparencyMask == 0 && m_d->previewMask == 0 && m_d->effectMasks.isEmpty() );
}

void KisLayer::applyEffectMasks( const KisPaintDeviceSP src,  KisPaintDeviceSP dst, const QRect & rc )
{
    Q_ASSERT( src != dst );


    // First copy the src onto the dst
    KisPainter gc( dst );
    gc.setCompositeOp(colorSpace()->compositeOp(COMPOSITE_COPY));
    gc.bitBlt( rc.topLeft(), src, rc );

    // Then loop through the effect masks and apply them

    // Then apply the preview mask

    // Then apply the transparency mask
}


void KisIndirectPaintingSupport::setTemporaryTarget(KisPaintDeviceSP t) {
    m_temporaryTarget = t;
}

void KisIndirectPaintingSupport::setTemporaryCompositeOp(const KoCompositeOp* c) {
    m_compositeOp = c;
}

void KisIndirectPaintingSupport::setTemporaryOpacity(quint8 o) {
    m_compositeOpacity = o;
}

KisPaintDeviceSP KisIndirectPaintingSupport::temporaryTarget() {
    return m_temporaryTarget;
}

const KoCompositeOp* KisIndirectPaintingSupport::temporaryCompositeOp() const {
    return m_compositeOp;
}

quint8 KisIndirectPaintingSupport::temporaryOpacity() const {
    return m_compositeOpacity;
}


#include "kis_layer.moc"
