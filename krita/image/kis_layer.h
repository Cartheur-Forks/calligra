/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef KIS_LAYER_H_
#define KIS_LAYER_H_

#include <QRect>
#include <QRegion>
#include <QMetaType>
#include <QObject>

#include "krita_export.h"

#include "KoCompositeOp.h"
#include "KoDocumentSectionModel.h"

#include "kis_shared.h"
#include "kis_types.h"
#include "kis_layer_visitor.h"

#include "kis_paint_device.h"


class QIcon;
class QPainter;
class QBitArray;
class KisGroupLayer;
class KoColorSpace;

/**
 * Abstract class that represents the concept of a Layer in Krita. This is not related
 * to the paint devices: this is merely an abstraction of how layers can be stacked and
 * rendered differently.
 * Regarding the previous-, first-, next- and lastChild() calls, first means that it the layer
 * is at the top of the group in the layerlist, using next will iterate to the bottom to last,
 * whereas previous will go up to first again.
 **/
class KRITAIMAGE_EXPORT KisLayer: public QObject, public KisShared
{

    Q_OBJECT

public:
    KisLayer(KisImageWSP img, const QString &name, quint8 opacity);
    KisLayer(const KisLayer& rhs);
    virtual ~KisLayer();

    virtual KoColorSpace * colorSpace();

    virtual void updateProjection(const QRect& r) = 0;

    virtual KisPaintDeviceSP projection() = 0;


    /// Whether this layer is the active one in its image
    bool isActive() const;

    /// Sets this layer as the active one
    void setActive();

    virtual QIcon icon() const = 0;

    virtual KoDocumentSectionModel::PropertyList properties() const;
    virtual void setProperties( const KoDocumentSectionModel::PropertyList &properties  );

    virtual void setChannelFlags( QBitArray & channelFlags );
    QBitArray & channelFlags();

public slots:
    /**
     * Set the entire layer extent dirty; this percolates up to parent layers all the
     * way to the root layer.
     */
    virtual void setDirty();

    /**
     * Add the given rect to the set of dirty rects for this layer;
     * this percolates up to parent layers all the way to the root
     * layer.
     */
    virtual void setDirty(const QRect & rect);

    /**
      Add the given region to the set of dirty rects for this layer;
      this percolates up to parent layers all the way to the root
      layer, if propagate is true;
    */
    virtual void setDirty( const QRegion & region);

public:

#ifdef DIRTY_AND_PROJECTION
    /**
       @return true if the given rect overlapts with the dirty region
       of this adjustement layer
    */
    bool isDirty( const QRect & rect );

    /**
       Mark the specified area as clean
    */
    void setClean( QRect rc );
#endif
    /// Return a copy of this layer
    virtual KisLayerSP clone() const = 0;

    /// Returns the ID of the layer, which is guaranteed to be unique among all KisLayers.
    int id() const;

    /**
     * Returns the index of the layer in its parent's list of child
     * layers. Indices increase from 0, which is the topmost layer in
     * the list, to the bottommost.
     */
    virtual int index() const;

    /**
     * Returns the parent layer of a layer. This is 0 only for a root
     * layer; otherwise this will be an actual GroupLayer
     */
    virtual KisGroupLayerSP parentLayer();

    virtual const KisGroupLayerSP parentLayer() const;

    /**
     * Returns the previous sibling of this layer in the parent's
     * list. This is the layer *above* this layer. 0 is returned if
     * there is no parent, or if this child has no more previous
     * siblings (== firstChild())
     */
    virtual KisLayerSP prevSibling() const;

    /**
     * Returns the next sibling of this layer in the parent's list.
     * This is the layer *below* this layer. 0 is returned if there is
     * no parent, or if this child has no more next siblings (==
     * lastChild())
     */
    virtual KisLayerSP nextSibling() const;

    /// Returns how many direct child layers this layer has (not
    /// recursive). The childcount can include masks.
    virtual uint childCount() const { return 0; }

    virtual KisLayerSP at(int /*index*/) const { return KisLayerSP(0); }

    /// Returns the first child layer of this layer (if it supports that).
    virtual KisLayerSP firstChild() const { return KisLayerSP(0); }

    /// Returns the last child layer of this layer (if it supports that).
    virtual KisLayerSP lastChild() const { return KisLayerSP(0); }

    /// Recursively searches this layer and any child layers for a layer with the specified name.
    virtual KisLayerSP findLayer(const QString& name) const;

    /// Recursively searches this layer and any child layers for a layer with the specified ID.
    virtual KisLayerSP findLayer(int id) const;

    enum { Visible = 1, Hidden = 2, Locked = 4, Unlocked = 8 };

    /// Returns the total number of layers in this layer, its child layers, and their child layers recursively, optionally ones with the specified properties Visible or Locked, which you can OR together.
    virtual int numLayers(int type = 0) const;

public:
    /**
     * Called when the layer is made active.
     * Be sure to call the superclass's implementation when reimplementing.
     */
    virtual void activate();

    /**
     * Called when the layer is made active.
     * Be sure to call the superclass's implementation when reimplementing.
     */
    virtual void deactivate();

public:
    virtual qint32 x() const = 0;
    virtual void setX(qint32) = 0;

    virtual qint32 y() const = 0;
    virtual void setY(qint32) = 0;

    /// Returns an approximation of where the bounds on actual data are in this layer
    virtual QRect extent() const = 0;
    /// Returns the exact bounds of where the actual data resides in this layer
    virtual QRect exactBounds() const = 0;

    virtual const bool visible() const;
    virtual void setVisible(bool v);
    QUndoCommand *setVisibleCommand(bool visiblel);

    quint8 opacity() const; //0-255
    void setOpacity(quint8 val); //0-255
    quint8 percentOpacity() const; //0-100
    void setPercentOpacity(quint8 val); //0-100
    QUndoCommand *setOpacityCommand(quint8 val);
    QUndoCommand *setOpacityCommand(quint8 prevOpacity, quint8 newOpacity);

    bool locked() const;
    void setLocked(bool l);
    QUndoCommand *setLockedCommand(bool locked);

    void notifyPropertyChanged();
    void notifyCommandExecuted();

    bool temporary() const;
    void setTemporary(bool t);

    virtual QString name() const;
    virtual void setName(const QString& name);

    const KoCompositeOp * compositeOp() const;
    void setCompositeOp(const KoCompositeOp * compositeOp);
    QUndoCommand * setCompositeOpCommand(const KoCompositeOp * compositeOp);

    KisImageSP image() const;
    virtual void setImage(KisImageSP image);

    /// paints a mask where the selection on this layer resides
    virtual void paint(QImage &img, qint32 x, qint32 y, qint32 w, qint32 h);
    virtual void paint(QImage &img, const QRect& scaledImageRect, const QSize& scaledImageSize, const QSize& imageSize);

    /// Returns a thumbnail in requested size. The QImage may have transparent parts.
    /// May also return 0
    virtual QImage createThumbnail(qint32 w, qint32 h);

    /// Accept the KisLayerVisitor (for the Visitor design pattern), should call the correct function on the KisLayerVisitor for this layer type
    virtual bool accept(KisLayerVisitor &) = 0;


    /// Returns the list of effect masks
    QList<KisEffectMask*> effectMasks();

    /// Add an effect mask at the specified position
    void addEffectMask( KisEffectMask* mask, int index = 0 );

    /// Add an effect mask on top of the specified mask
    void addEffectMask( KisEffectMask * mask,  KisEffectMask * aboveThis );

    /// Remove the given effect mask
    void removeEffectMask( KisEffectMask* mask );

    /// Remove the mask at the speficied index
    void removeEffectMask( int index );

protected:

    bool hasEffectMasks();

    /**
     * Apply the effect masks to the given paint device, producing
     * finally the dst paint device.
     */
    void applyEffectMasks( const KisPaintDeviceSP src,  KisPaintDeviceSP dst, const QRect & rc );

private:

    /// ???
    bool matchesFlags(int flags) const;

private:
    friend class KisGroupLayer;

    // For KoGroupLayer. XXX: make the base KisLayer a group layer and
    // remove these. (BSAR)
    void setIndexPrivate( int index );
    void setCompositeOpPrivate( const KoCompositeOp * op );
    void setParentPrivate( KisGroupLayerSP parent );

    class Private;
    Private * const m_d;

};

/**
 * For classes that support indirect painting.
 *
 * XXX: Name doesn't suggest an object -- is KisIndirectPaintingLayer
 * a better name? (BSAR)
 */
class KRITAIMAGE_EXPORT KisIndirectPaintingSupport {

public:

    KisIndirectPaintingSupport() : m_compositeOp(0) { }
    virtual ~KisIndirectPaintingSupport() {}

    // Indirect painting
    void setTemporaryTarget(KisPaintDeviceSP t);
    void setTemporaryCompositeOp(const KoCompositeOp* c);
    void setTemporaryOpacity(quint8 o);
    KisPaintDeviceSP temporaryTarget();
    const KoCompositeOp* temporaryCompositeOp() const;
    quint8 temporaryOpacity() const;

    // Or I could make KisLayer a virtual base of KisIndirectPaintingSupport and so, but
    // I'm sure virtual diamond inheritance isn't as appreciated as this

    virtual KisLayer* layer() = 0;

private:

    // To simulate the indirect painting
    KisPaintDeviceSP m_temporaryTarget;
    const KoCompositeOp* m_compositeOp;
    quint8 m_compositeOpacity;

};

Q_DECLARE_METATYPE( KisLayerSP )

#endif // KIS_LAYER_H_

