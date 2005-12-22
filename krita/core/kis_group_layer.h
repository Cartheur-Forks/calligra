/*
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
#ifndef KIS_GROUP_LAYER_H_
#define KIS_GROUP_LAYER_H_

#include "kis_layer.h"
#include "kis_types.h"


class KisGroupLayer : public KisLayer {
    typedef KisLayer super;

    Q_OBJECT

public:
    KisGroupLayer(KisImage *img, const QString &name, Q_UINT8 opacity);
    KisGroupLayer(const KisGroupLayer& rhs);
    virtual ~KisGroupLayer();

    virtual KisLayerSP clone() const;
public:

    // Called when the layer is made active
    virtual void activate() {};

    // Called when another layer is made active
    virtual void deactivate() {};

    virtual Q_INT32 x() const {return 0;};
    virtual void setX(Q_INT32) {};

    virtual Q_INT32 y() const {return 0;};
    virtual void setY(Q_INT32) {};

    virtual QRect extent() const {return QRect();};
    virtual QRect exactBounds() const {return QRect();};

    virtual void insertLayer(KisLayerSP newLayer, KisLayerSP belowLayer);
    virtual void removeLayer(KisLayerSP layer);

    virtual void accept(KisLayerVisitor &v) { v.visit(this); }

    virtual KisLayerSP firstChild() const;
    virtual KisLayerSP lastChild() const;

private:
    vKisLayerSP m_layers; // Contains the list of all layers
};

#endif // KIS_GROUP_LAYER_H_

