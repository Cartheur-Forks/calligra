/*
 *  Copyright (c) 1999-2000 Matthias Elter  <me@kde.org>
 *  Copyright (c) 2001 Toshitaka Fujioka  <fujioka@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004-2007 Boudewijn Rempt <boud@valdyas.org<
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
#ifndef KIS_DOC_2_H_
#define KIS_DOC_2_H_

#include <kdebug.h>

#include <KoDocument.h>

#include "kis_types.h"

#include <krita_export.h>

class QString;

class KoColorSpace;
class KoColor;
class KoShape;

class KoShapeControllerBase;
class KisView2;
class KisChildDoc;
class KisUndoAdapter;
class KisLayerModel;

/**
 * The class that represents a Krita document containing content and
   settings.

   KisDoc2 also keeps track of the correspondence between the layer
   structure of the KisImage and the shape tree that is used by the
   tools.


   XXX: File format todos

   * load/save the channel flags setting for layers

 */
class KRITAUI_EXPORT KisDoc2 : public KoDocument
{

    Q_OBJECT

public:
    KisDoc2(QWidget *parentWidget = 0, QObject* parent = 0, bool singleViewMode = false );
    virtual ~KisDoc2();

public:
    // Overide KoDocument
    virtual bool wantExportConfirmation() const { return false; }
    virtual bool completeLoading(KoStore *store);
    virtual bool completeSaving(KoStore*);
    virtual bool loadOasis( const KoXmlDocument&, KoOasisStyles&, const KoXmlDocument&, KoStore* );
    virtual bool saveOasis( KoStore*, KoXmlWriter* );
    virtual bool loadChildren( KoStore* store);
    virtual bool loadXML(QIODevice *, const KoXmlDocument& doc);
    virtual QByteArray mimeType() const;
    virtual QWidget* createCustomDocumentWidget(QWidget *parent);
    virtual KoDocument* hitTest(const QPoint &pos, KoView* view, const QMatrix& matrix = QMatrix());

    /**
     * Draw the image embedded in another KOffice document
     */
    virtual void paintContent(QPainter& painter, const QRect& rect);

    virtual QDomDocument saveXML();

    virtual void setUndo(bool undo);
    virtual bool undo() const;

public slots:

    /**
     * Initialize an empty document using default values
     * @since 1.5
     */
     virtual void initEmpty();

public:

    /**
     * Create a new image that has this document as a parent and
     * replace the current image with this image.
     */
    bool newImage(const QString& name, qint32 width, qint32 height, KoColorSpace * cs, const KoColor &bgColor, const QString &imgDescription, const double imgResolution);

    /**
     * Create a new image that has this document as a parent and
     * replace the current image with this image.
     */
    KisImageSP newImage(const QString& name, qint32 width, qint32 height, KoColorSpace * colorspace);

    KisImageSP image();

    /**
     * Adds the specified child document to this document; this
     * is not done with KoDocument::insertChild() because that
     * is protected and cannot be called from KisView2.
     */
    KisChildDoc * createChildDoc( const QRect& rect, KoDocument* childDoc );

    /**
     * Makes an otherwise empty document ready for import/export
     */
    void prepareForImport();

    /**
     * Set the current image to the specified image and turn undo on.
     */
    void setCurrentImage(KisImageSP image);

    KisUndoAdapter * undoAdapter();

    /**
     * The shape controller matches internal krita image layers with
     * the flake shape hierarchy.
     */
    KoShapeControllerBase * shapeController();

    KoShape * shapeForLayer( KisLayerSP layer );

    /**
     * The layer model provides a light-weight Qt model-view
     * compatible model on the internal Krita image layer hierarchy.
     */
    KisLayerModel * layerModel();

public slots:
    void slotImageUpdated();
    void slotImageUpdated(const QRect& rect);
    void slotDocumentRestored();

signals:
    void sigDocUpdated();
    void sigDocUpdated(QRect rect);
    void sigLoadingFinished();

    /*
     * Emitted every time a command is added to the undo history, or executed
     * due to an undo or redo action.
     */
    void sigCommandExecuted();

public:
    // Overide KoDocument
    virtual KoView* createViewInstance(QWidget *parent);

protected slots:
    // Overide KoDocument
    virtual void openExistingFile(const KUrl& url);
    virtual void openTemplate(const KUrl& url);

private slots:

    void slotUpdate(KisImageSP img, quint32 x, quint32 y, quint32 w, quint32 h);
    void slotIOProgress(qint8 percentage);

private:

    QDomElement saveImage(QDomDocument& doc, KisImageSP img);
    KisImageSP loadImage(const KoXmlElement& elem);
    void loadLayers(const KoXmlElement& element, KisImageSP img, KisGroupLayerSP parent);
    KisLayerSP loadLayer(const KoXmlElement& elem, KisImageSP img);
    KisLayerSP loadPaintLayer(const KoXmlElement& elem, KisImageSP img, const QString & name, qint32 x, qint32 y, qint32 opacity, bool visible, bool locked, const QString & compositeOp);
    KisGroupLayerSP loadGroupLayer(const KoXmlElement& elem, KisImageSP img, const QString & name, qint32 x, qint32 y, qint32 opacity, bool visible, bool locked, const QString &compositeOp);
    KisAdjustmentLayerSP loadAdjustmentLayer(const KoXmlElement& elem, KisImageSP img, const QString & name, qint32 x, qint32 y, qint32 opacity, bool visible, bool locked, const QString & compositeOp);
    KisShapeLayerSP loadShapeLayer(const KoXmlElement& elem, KisImageSP img, const QString & name, qint32 x, qint32 y, qint32 opacity, bool visible, bool locked, const QString &compositeOp);
    bool init();

    void setIOSteps(qint32 nsteps);
    void IOCompletedStep();
    void IODone();

private:

    class KisDocPrivate;
    KisDocPrivate * const m_d;

};

#endif // KIS_DOC_H_

