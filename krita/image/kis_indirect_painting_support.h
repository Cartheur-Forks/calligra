/*
 *  Copyright (c) 2004 Bart Coppens <kde@bartcoppens.be>
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef KIS_INDIRECT_PAINTING_SUPPORT_H_
#define KIS_INDIRECT_PAINTING_SUPPORT_H_

#include "krita_export.h"
#include "kis_types.h"
#include "kis_node.h"

class QBitArray;

/**
 * For classes that support indirect painting.
 *
 * XXX: Name doesn't suggest an object -- is KisIndirectPaintingLayer
 * a better name? (BSAR)
 */
class KRITAIMAGE_EXPORT KisIndirectPaintingSupport
{
    KisIndirectPaintingSupport(const KisIndirectPaintingSupport&);
    KisIndirectPaintingSupport& operator=(const KisIndirectPaintingSupport&);
public:

    KisIndirectPaintingSupport();
    virtual ~KisIndirectPaintingSupport();

    bool hasTemporaryTarget() const;

    void setTemporaryTarget(KisPaintDeviceSP t);
    void setTemporaryCompositeOp(const KoCompositeOp* c);
    void setTemporaryOpacity(quint8 o);
    void setTemporaryChannelFlags(const QBitArray& channelFlags);

    void setDirty(const QRect &rect);

    /**
     * Writes the temporary target into the paint device of the layer.
     * This action will lock the temporary target itself.
     */
    void mergeToLayer(KisLayerSP layer, const QString &transactionText);

    /**
     * Lock the temporary target.
     * It should be done for guarding every access to
     * temporaryTarget() or original()
     * NOTE: well, not "every", but...
     */
    void lockTemporaryTarget() const;

    /**
     * Unlock the temporary target
     * 
     * \see lockTemporaryTarget()
     */
    void unlockTemporaryTarget() const;

    KisPaintDeviceSP temporaryTarget();
    const KisPaintDeviceSP temporaryTarget() const;
    const KoCompositeOp* temporaryCompositeOp() const;
    quint8 temporaryOpacity() const;
    const QBitArray& temporaryChannelFlags() const;

private:
    /**
     * Adds a dirty rect to the list of rects those should be
     * merged to the layer after the indirect painting is finished
     * WARNING: should be called with the lock held
     *
     * \see lockTemporaryTarget()
     */
    void addIndirectlyDirtyRect(const QRect &rect);
    QRegion indirectlyDirtyRegion();

private:
    struct Private;
    Private* const d;
};


#endif /* KIS_INDIRECT_PAINTING_SUPPORT_H_ */
