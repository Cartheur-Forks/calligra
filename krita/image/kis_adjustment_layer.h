/*
 *  Copyright (c) 2006 Boudewijn Rempt
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
#ifndef KIS_ADJUSTMENT_LAYER_H_
#define KIS_ADJUSTMENT_LAYER_H_

#include <QObject>
#include "kis_types.h"
#include "kis_paint_device.h"
#include "kis_layer_visitor.h"
#include "kis_layer.h"
#include "KoCompositeOp.h"
#include <krita_export.h>

class K3NamedCommand;
class QPainter;
class KisGroupLayer;
class KisFilterConfiguration;

/**
 * Class that contains a KisFilter and optionally a KisSelection. The combination
 * is used by to influence the rendering of the layers under this layer in the
 * layerstack.
 *
 * AdjustmentLayers also function as a kind of "fixating layers" == as
 * long as they
 **/
class KRITAIMAGE_EXPORT KisAdjustmentLayer : public KisLayer, public KisIndirectPaintingSupport
{
    Q_OBJECT

public:
    /**
     * Create a new adjustment layer with the given configuration and selection.
     * Note that the selection will be _copied_.
     */
    KisAdjustmentLayer(KisImageSP img, const QString &name, KisFilterConfiguration * kfc, KisSelectionSP selection);
    KisAdjustmentLayer(const KisAdjustmentLayer& rhs);
    virtual ~KisAdjustmentLayer();

    void updateProjection(const QRect& r);
    void resetProjection( KisPaintDeviceSP to);
    KisPaintDeviceSP projection();

    QIcon icon() const;
    KoDocumentSectionModel::PropertyList properties() const;

    /// Return a copy of this layer
    KisLayerSP clone() const;

public:

    KisFilterConfiguration * filter() const;
    void setFilter(KisFilterConfiguration * filterConfig);

    KisSelectionSP selection();

    /// Set the selection of this adjustment layer to a copy of selection.
    void setSelection(KisSelectionSP selection);

    void paint(QImage &img, qint32 x, qint32 y, qint32 w, qint32 h);
    void paint(QImage &img, const QRect& scaledImageRect, const QSize& scaledImageSize, const QSize& imageSize);

public:

    qint32 x() const;
    void setX(qint32);

    qint32 y() const;
    void setY(qint32);

    /// Returns an approximation of where the bounds on actual data are in this layer
    QRect extent() const;

    /// Returns the exact bounds of where the actual data resides in this layer
    QRect exactBounds() const;

    bool accept(KisLayerVisitor &);

    void resetCache();
    KisPaintDeviceSP cachedPaintDevice() { return m_cachedPaintDev; }

    bool showSelection() const { return m_showSelection; }
    void setSelection(bool b) { m_showSelection = b; }

    QImage createThumbnail(qint32 w, qint32 h);

    // KisIndirectPaintingSupport
    KisLayer* layer() { return this; }

private:
    bool m_showSelection;
    KisFilterConfiguration * m_filterConfig;
    KisSelectionSP m_selection;
    KisPaintDeviceSP m_cachedPaintDev;
    QRegion m_dirtyRegion;

private slots:

    void slotSelectionChanged(KisImageSP image);

};

#endif // KIS_ADJUSTMENT_LAYER_H_

