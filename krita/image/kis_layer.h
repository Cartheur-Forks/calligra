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

#include "krita_export.h"

#include "kis_shared.h"
#include "kis_types.h"
#include "kis_layer_visitor.h"
#include "KoCompositeOp.h"
#include "KoDocumentSectionModel.h"
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
class KRITAIMAGE_EXPORT KisLayer: public KoDocumentSectionModel, public KisShared
{
    typedef KoDocumentSectionModel super;
    Q_OBJECT

public:
    KisLayer(KisImageSP img, const QString &name, quint8 opacity);
    KisLayer(const KisLayer& rhs);
    virtual ~KisLayer();

    virtual KoColorSpace * colorSpace();

    /// Whether this layer is the active one in its image
    bool isActive() const;

    /// Sets this layer as the active one
    void setActive();

    virtual QIcon icon() const = 0;

    virtual PropertyList properties() const;
    virtual void setProperties( const PropertyList &properties  );

    virtual void setActiveChannels( QBitArray & activeChannels );
    QBitArray & activeChannels();

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

    /**
       @return true if the given rect overlapts with the dirty region
       of this adjustement layer
    */
    bool isDirty( const QRect & rect );

    /**
       Mark the specified area as clean
    */
    void setClean( QRect rc );

    /// Return a copy of this layer
    virtual KisLayerSP clone() const = 0;

    /// Returns the ID of the layer, which is guaranteed to be unique among all KisLayers.
    int id() const;

    /* Returns the index of the layer in its parent's list of child layers. Indices
     * increase from 0, which is the topmost layer in the list, to the bottommost.
     */
    virtual int index() const;

    /// Moves this layer to the specified index within its parent's list of child layers.
    virtual void setIndex(int index);

    /**
     * Returns the parent layer of a layer. This is 0 only for a root layer; otherwise
     * this will be an actual GroupLayer */
    virtual KisGroupLayerSP parent() const;

    /**
     * Returns the previous sibling of this layer in the parent's list. This is the layer
     * *above* this layer. 0 is returned if there is no parent, or if this child has no more
     * previous siblings (== firstChild())
     */
    virtual KisLayerSP prevSibling() const;

    /**
     * Returns the next sibling of this layer in the parent's list. This is the layer *below*
     * this layer. 0 is returned if there is no parent, or if this child has no more next
     * siblings (== lastChild())
     */
    virtual KisLayerSP nextSibling() const;

    /**
     * Returns the sibling above this layer in its parent's list. 0 is returned if there is no parent,
     * or if this layer is the topmost layer in its group. This is the same as calling prevSibling().
     */
    KisLayerSP siblingAbove() const { return prevSibling(); }

    /**
     * Returns the sibling below this layer in its parent's list. 0 is returned if there is no parent,
     * or if this layer is the bottommost layer in its group.  This is the same as calling nextSibling().
     */
    KisLayerSP siblingBelow() const { return nextSibling(); }

    /// Returns how many direct child layers this layer has (not recursive).
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

    KisLayerSP layerFromIndex(const QModelIndex &index);
    vKisLayerSP layersFromIndexes(const QModelIndexList &list);

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

//     KisUndoAdapter *undoAdapter() const;

    /// paints a mask where the selection on this layer resides
    virtual void paint(QImage &img, qint32 x, qint32 y, qint32 w, qint32 h);
    virtual void paint(QImage &img, const QRect& scaledImageRect, const QSize& scaledImageSize, const QSize& imageSize);

    /// paints where no data is on this layer. Useful when it is a transparent layer stacked on top of another one
    virtual void paintMaskInactiveLayers(QImage &img, qint32 x, qint32 y, qint32 w, qint32 h);

    /// Returns a thumbnail in requested size. The QImage may have transparent parts.
    /// May also return 0
    virtual QImage createThumbnail(qint32 w, qint32 h);

    /// Accept the KisLayerVisitor (for the Visitor design pattern), should call the correct function on the KisLayerVisitor for this layer type
    virtual bool accept(KisLayerVisitor &) = 0;

public: // from QAbstractItemModel
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

protected:
    QModelIndex indexFromLayer(KisLayer *layer) const;

    /// causes QAbstractItemModel::dataChanged() to be emitted and percolated up the tree
    void notifyPropertyChanged(KisLayer *layer);
private:

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
class KRITAIMAGE_EXPORT KisLayerSupportsIndirectPainting {

public:

    KisLayerSupportsIndirectPainting() : m_compositeOp(0) { }
    virtual ~KisLayerSupportsIndirectPainting() {}

    // Indirect painting
    void setTemporaryTarget(KisPaintDeviceSP t);
    void setTemporaryCompositeOp(const KoCompositeOp* c);
    void setTemporaryOpacity(Q_UINT8 o);
    KisPaintDeviceSP temporaryTarget();
    const KoCompositeOp* temporaryCompositeOp() const;
    Q_UINT8 temporaryOpacity() const;

    // Or I could make KisLayer a virtual base of KisLayerSupportsIndirectPainting and so, but
    // I'm sure virtual diamond inheritance isn't as appreciated as this

    virtual KisLayer* layer() = 0;

private:

    // To simulate the indirect painting
    KisPaintDeviceSP m_temporaryTarget;
    const KoCompositeOp* m_compositeOp;
    Q_UINT8 m_compositeOpacity;

};

Q_DECLARE_METATYPE( KisLayerSP )

#endif // KIS_LAYER_H_

