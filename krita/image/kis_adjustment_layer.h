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
#ifndef KIS_ADJUSTMENT_LAYER_H_
#define KIS_ADJUSTMENT_LAYER_H_

#include <QObject>
#include "kis_types.h"
#include "kis_layer.h"
#include <krita_export.h>
#include "kis_node_filter_interface.h"

class KisFilterConfiguration;
class KisNodeVisitor;

/**
 * Class that contains a KisFilter and optionally a KisSelection. The combination
 * is used by to influence the rendering of the layers under this layer in the
 * layerstack.
 *
 * AdjustmentLayers also function as a kind of "fixating layers".
 *
 * XXX_NODE: implement prepareForRemoval with:
         // Adjustment layers should mark the layers underneath them, whose rendering
        // they have cached, dirty on removal. Otherwise, the group won't be re-rendered.
        KisAdjustmentLayer * al = dynamic_cast<KisAdjustmentLayer*>(layer.data());
        if (al) {
            QRect r = al->extent();
            lock(); // Lock the image, because we are going to dirty a lot of layers
            KisLayerSP l = layer->nextSibling();
            while (l) {
                KisAdjustmentLayer * al2 = dynamic_cast<KisAdjustmentLayer*>(l.data());
                if (al2 != 0) break;
                l = l->nextSibling();
            }
            unlock();
        }

 * XXX_NODE: also implement masks modifying the adj. layer's selection.
 */
class KRITAIMAGE_EXPORT KisAdjustmentLayer : public KisLayer, public KisIndirectPaintingSupport, public KisNodeFilterInterface
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

    KisNodeSP clone() const {
        return KisNodeSP(new KisAdjustmentLayer(*this));
    }

    bool allowAsChild(KisNodeSP) const;

    void updateProjection(const QRect& r);

    /**
     * return the final result of the layer and all masks
     */
    KisPaintDeviceSP projection() const;

    /**
     * return paint device that the user can paint on. For paint layers,
     * this is the basic, wet painting device, for adjustment layers it's
     * the selection.
     */
    KisPaintDeviceSP paintDevice() const;

    QIcon icon() const;
    KoDocumentSectionModel::PropertyList sectionModelProperties() const;
    using KisLayer::setDirty;
    virtual void setDirty();
public:

    KisFilterConfiguration * filter() const;
    void setFilter(KisFilterConfiguration * filterConfig);

    KisSelectionSP selection() const;

    /// Set the selection of this adjustment layer to a copy of selection.
    void setSelection(KisSelectionSP selection);

    /**
     * overridden from KisBaseNode
     */
    qint32 x() const;

    /**
     * overridden from KisBaseNode
     */
    void setX(qint32 x);

    /**
     * overridden from KisBaseNode
     */
    qint32 y() const;

    /**
     * overridden from KisBaseNode
     */
    void setY(qint32 y);

public:

    /// Returns an approximation of where the bounds on actual data are in this layer
    QRect extent() const;

    /// Returns the exact bounds of where the actual data resides in this layer
    QRect exactBounds() const;

    bool accept(KisNodeVisitor &);

    void resetCache();

    KisPaintDeviceSP cachedPaintDevice();

    bool showSelection() const;
    void setSelection(bool b);

    QImage createThumbnail(qint32 w, qint32 h);

    // KisIndirectPaintingSupport
    KisLayer* layer() {
        return this;
    }

private:

    class Private;
    Private * const m_d;
};

#endif // KIS_ADJUSTMENT_LAYER_H_

