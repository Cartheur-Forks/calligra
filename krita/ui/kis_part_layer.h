/*
 *  Copyright (c) 2005 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef _KIS_PART_LAYER_
#define _KIS_PART_LAYER_

#include <QObject>
#include <QRect>

#include <KoDocument.h>
#include <KoDocumentChild.h>

#include "kis_paint_layer.h"
#include "kis_types.h"
#include "kis_doc2.h"
#include "kis_part_layer_iface.h"
#include "kis_layer_visitor.h"

class KoFrame;
class KisView2;
class KisDoc2;


/**
 * The child document is responsible for saving and loading the embedded layers.
 */
class KisChildDoc : public KoDocumentChild
{

public:

    KisChildDoc ( KisDoc2 * KisDoc2, const QRect& rect, KoDocument * childDoc );
    KisChildDoc ( KisDoc2 * kisDdoc );

    virtual ~KisChildDoc();

    KisDoc2 * parent() const { return m_doc; }

    void setPartLayer (KisPartLayerSP layer) { m_partLayer = layer; }

    KisPartLayerSP partLayer() const { return m_partLayer; }
protected:

    KisDoc2 * m_doc;
    KisPartLayerSP m_partLayer;
};


/**
 * A PartLayer is a layer that contains a KOffice Part like a KWord document
 * or a KSpread spreadsheet. Or whatever. A Karbon drawing.
 *
 * The part is rendered into an RBGA8 paint device so we can composite it with
 * the other layers.
 *
 * When it is activated (see activate()), it draws a rectangle around itself on the KisDoc2,
 * whereas when it is deactivated (deactivate()), it removes that rectangle and commits
 * the child to the paint device.
 *
 * Embedded parts should get loaded and saved to the Native Krita Fileformat natively.
 */
class KisPartLayerImpl : public KisPartLayer {
    Q_OBJECT
    typedef KisPartLayer super;
public:
    KisPartLayerImpl(KisImageSP img, KisChildDoc * doc);
    virtual ~KisPartLayerImpl();

    virtual PropertyList properties() const;
    virtual KisLayerSP clone() const;

    /// Returns the childDoc so that we can access the doc from other places, if need be (KisDoc2)
    virtual KisChildDoc* childDoc() const { return m_doc; }

    void setDocType(const QString& type) { m_docType = type; }
    QString docType() const { return m_docType; }

    virtual void setX(qint32 x);
    virtual void setY(qint32 y);
    virtual qint32 x() const { return m_doc->geometry() . x(); }
    virtual qint32 y() const { return m_doc->geometry() . y(); } //m_paintLayer->y(); }
    virtual QRect extent() const { return m_doc->geometry(); }
    virtual QRect exactBounds() const { return m_doc->geometry(); }

    virtual QImage createThumbnail(qint32 w, qint32 h);

    virtual bool accept(KisLayerVisitor& visitor) {
        return visitor.visit(this);
    }

    virtual KisPaintDeviceSP prepareProjection(KisPaintDeviceSP projection, const QRect& r);

    virtual void paintSelection(QImage &img, qint32 x, qint32 y, qint32 w, qint32 h);

    virtual bool saveToXML(QDomDocument doc, QDomElement elem);
private slots:
    /// Repaints our device with the data from the embedded part
    //void repaint();
    /// When we activate the embedding, we clear ourselves
    void childActivated(KoDocumentChild* child);
    void childDeactivated(bool activated);


private:
    // KisPaintLayerSP m_paintLayer;
    KisPaintDeviceSP m_cache;
    KoFrame * m_frame; // The widget that holds the editable view of the embedded part
    KisChildDoc * m_doc; // The sub-document
    QString m_docType;
    bool m_activated;
};

/**
 * Visitor that connects all partlayers in an image to a KisView2's signals
 */
class KisConnectPartLayerVisitor : public KisLayerVisitor {
    KisImageSP m_img;
    KisView2* m_view;
    bool m_connect; // connects, or disconnects signals
public:
    KisConnectPartLayerVisitor(KisImageSP img, KisView2* view, bool mode);
    virtual ~KisConnectPartLayerVisitor() {}

    virtual bool visit(KisPaintLayer *layer);
    virtual bool visit(KisGroupLayer *layer);
    virtual bool visit(KisPartLayer *layer);
    virtual bool visit(KisAdjustmentLayer *layer);
};

#endif // _KIS_PART_LAYER_
