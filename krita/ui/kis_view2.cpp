/*
 *  This file is part of KimageShop^WKrayon^WKrita
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *                1999 Michael Koch    <koch@kde.org>
 *                1999 Carsten Pfeiffer <pfeiffer@kde.org>
 *                2002 Patrick Julien <freak@codepimps.org>
 *                2003-2007 Boudewijn Rempt <boud@valdyas.org>
 *                2004 Clarence Dang <dang@kde.org>
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
#include <kprinter.h>
#include "kis_view2.h"

#include <QGridLayout>
#include <QRect>
#include <QWidget>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QApplication>

#include <k3urldrag.h>
#include <kaction.h>
#include <klocale.h>
#include <kmenu.h>
#include <kparts/componentfactory.h>
#include <kparts/event.h>
#include <kparts/plugin.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <kstandardaction.h>
#include <kstandardaction.h>
#include <ktogglefullscreenaction.h>
#include <kurl.h>
#include <kxmlguifactory.h>
#include <kxmlguifactory.h>
#include <kmessagebox.h>

#include <KoMainWindow.h>
#include <KoCanvasController.h>
#include <KoShapeManager.h>
#include <KoShape.h>
#include <KoSelection.h>
#include <KoToolBoxFactory.h>
#include <KoZoomHandler.h>
#include <KoViewConverter.h>
#include <KoView.h>
#include <KoToolDockerFactory.h>
#include <KoColorDocker.h>
#include "KoColorSpaceRegistry.h"

#include <kactioncollection.h>

#include <kis_image.h>
#include <kis_undo_adapter.h>
#include <kis_layer.h>

#include "kis_config.h"
#include "kis_statusbar.h"
#include "kis_canvas2.h"
#include "kis_doc2.h"
#include "kis_factory2.h"
#include "kis_filter_manager.h"
#include "kis_resource_provider.h"
#include "kis_resource_provider.h"
#include "kis_selection_manager.h"
#include "kis_image_manager.h"
#include "kis_controlframe.h"
#include "kis_birdeye_box.h"
#include "kis_layerbox.h"
#include "kis_layer_manager.h"
#include "kis_zoom_manager.h"
#include "kis_grid_manager.h"
#include "kis_perspective_grid_manager.h"
#include "kis_mask_manager.h"
#include "kis_dlg_preferences.h"
#include "kis_group_layer.h"
#include "kis_custom_palette.h"
#include "ui_wdgpalettechooser.h"
#include "kis_resourceserver.h"
#include "kis_palette_docker.h"
#include "kis_layer_model.h"

class KisView2::KisView2Private {

public:

    KisView2Private()
        : canvas( 0 )
        , doc( 0 )
        , viewConverter( 0 )
        , canvasController( 0 )
        , resourceProvider( 0 )
        , filterManager( 0 )
        , statusBar( 0 )
        , selectionManager( 0 )
        , controlFrame( 0 )
        , birdEyeBox( 0 )
        , layerBox( 0 )
        , layerManager( 0 )
        , zoomManager( 0 )
        , imageManager( 0 )
//        , maskManager( 0 )
        , gridManager( 0 )
        , perspectiveGridManager( 0 )
        {
        }

    ~KisView2Private()
        {
            KoToolManager::instance()->removeCanvasController( canvasController );
            delete canvas;
            delete filterManager;
            delete selectionManager;
            delete layerManager;
            delete zoomManager;
            delete imageManager;
//            delete maskManager;
            delete gridManager;
            delete perspectiveGridManager;
        }

public:

    KisCanvas2 *canvas;
    KisDoc2 *doc;
    KoZoomHandler * viewConverter;
    KoCanvasController * canvasController;
    KisResourceProvider * resourceProvider;
    KisFilterManager * filterManager;
    KisStatusBar * statusBar;
    QAction * fullScreen;
    KisSelectionManager *selectionManager;
    KisControlFrame * controlFrame;
    KisBirdEyeBox * birdEyeBox;
    KisLayerBox * layerBox;
    KisLayerManager * layerManager;
    KisZoomManager * zoomManager;
    KisImageManager * imageManager;
//    KisMaskManager * maskManager;
    KisGridManager * gridManager;
    KisPerspectiveGridManager * perspectiveGridManager;
};


KisView2::KisView2(KisDoc2 * doc, QWidget * parent)
    : KoView(doc, parent)
{

    setComponentData(KisFactory2::componentData(), false);

    if (!doc->isReadWrite())
        setXMLFile("krita_readonly.rc");
    else
        setXMLFile("krita.rc");

    if( mainWindow() )
        actionCollection()->addAction(KStandardAction::KeyBindings, "keybindings", mainWindow()->guiFactory(), SLOT( configureShortcuts() ));

    m_d = new KisView2Private();

    m_d->doc = doc;
    m_d->viewConverter = new KoZoomHandler();
    m_d->canvasController = new KoCanvasController( this );
    m_d->canvas = new KisCanvas2( m_d->viewConverter, this, doc->shapeController() );
    m_d->canvasController->setCanvas( m_d->canvas );
    m_d->resourceProvider = new KisResourceProvider( this );

    createActions();
    createManagers();
    createGUI();

    loadPlugins();

    // Wait for the async image to have loaded
    if ( m_d->doc->isLoading() ) {
        connect( m_d->doc, SIGNAL( sigLoadingFinished() ), this, SLOT( slotLoadingFinished() ) );
    }
    else {
        slotLoadingFinished();
    }

    setAcceptDrops(true);
}


KisView2::~KisView2()
{
    delete m_d->viewConverter;
    delete m_d;
}


void KisView2::dragEnterEvent(QDragEnterEvent *event)
{
    kDebug() << "KisView2::dragEnterEvent" << endl;
    // Only accept drag if we're not busy, particularly as we may
    // be showing a progress bar and calling qApp->processEvents().
    if (K3URLDrag::canDecode(event) && QApplication::overrideCursor() == 0) {
        event->accept();
    } else {
        event->ignore();
    }
}

void KisView2::dropEvent(QDropEvent *event)
{
    KUrl::List urls;

    if (K3URLDrag::decode(event, urls))
    {
        if (urls.count() > 0) {

            KMenu popup(this);
            popup.setObjectName("drop_popup");

            QAction *insertAsNewLayer = new QAction(i18n("Insert as New Layer"), &popup);
            QAction *insertAsNewLayers = new QAction(i18n("Insert as New Layers"), &popup);

            QAction *openInNewDocument = new QAction(i18n("Open in New Document"), &popup);
            QAction *openInNewDocuments = new QAction(i18n("Open in New Documents"), &popup);

            QAction *cancel = new QAction(i18n("Cancel"), &popup);

            if (urls.count() == 1) {
                if (!image().isNull()) {
                    popup.addAction(insertAsNewLayer);
                }
                popup.addAction(openInNewDocument);
            }
            else {
                if (!image().isNull()) {
                    popup.addAction(insertAsNewLayers);
                }
                popup.addAction(openInNewDocuments);
            }

            popup.addSeparator();
            popup.addAction(cancel);

            QAction *action = popup.exec(QCursor::pos());

            if (action != 0 && action != cancel) {
                for (KUrl::List::ConstIterator it = urls.begin (); it != urls.end (); ++it) {
                    KUrl url = *it;

                    if (action == insertAsNewLayer || action == insertAsNewLayers) {
                        m_d->imageManager->importImage(url);
                    } else {
                        Q_ASSERT(action == openInNewDocument || action == openInNewDocuments);

                        if (shell() != 0) {
                            shell()->openDocument(url);
                        }
                    }
                }
            }
        }
    }
}


void KisView2::slotChildActivated(bool a) {

    KoView::slotChildActivated(a);
}


void KisView2::canvasAddChild(KoViewChild *child) {
    KoView::canvasAddChild(child);
    connect(this, SIGNAL(viewTransformationsChanged()), child, SLOT(reposition()));
}



KisImageSP KisView2::image()
{
    return m_d->doc->image();
}

KisResourceProvider * KisView2::resourceProvider()
{
    return m_d->resourceProvider;
}

KisCanvas2 * KisView2::canvasBase() const
{
    return m_d->canvas;
}

QWidget* KisView2::canvas() const
{
    return m_d->canvas->canvasWidget();
}

KisStatusBar * KisView2::statusBar() const
{
    return m_d->statusBar;
}

KisSelectionManager * KisView2::selectionManager()
{
    return m_d->selectionManager;
}

KoCanvasController * KisView2::canvasController()
{
    return m_d->canvasController;
}

KisLayerManager * KisView2::layerManager()
{
    return m_d->layerManager;
}

KisLayerSP KisView2::activeLayer()
{
    if ( m_d->layerManager )
        return m_d->layerManager->activeLayer();
    else
        return 0;
}

KisPaintDeviceSP KisView2::activeDevice()
{
    if ( m_d->layerManager && m_d->layerManager->activeLayer() )
        return m_d->layerManager->activeLayer()->paintDevice();
    else
        return 0;
}

KisZoomManager * KisView2::zoomManager()
{
    return m_d->zoomManager;
}

KisFilterManager * KisView2::filterManager()
{
    return m_d->filterManager;
}

KisImageManager * KisView2::imageManager()
{
    return m_d->imageManager;
}

KisUndoAdapter * KisView2::undoAdapter()
{
    return m_d->doc->undoAdapter();
}


void KisView2::slotLoadingFinished()
{
    disconnect(m_d->doc, SIGNAL(sigLoadingFinished()), this, SLOT(slotLoadingFinished()));

    KisImageSP img = image();

    m_d->canvas->setImageSize( img->width(), img->height() );

    if(m_d->statusBar) {
        m_d->statusBar->imageSizeChanged(img->width(), img->height());
    }
    m_d->resourceProvider->slotSetImageSize( img->width(), img->height() );

    m_d->layerManager->layersUpdated();
    updateGUI();

    KoToolDockerFactory toolDockerFactory;
    KoToolDocker * d =  dynamic_cast<KoToolDocker*>( createDockWidget( &toolDockerFactory ) );
    if(d)
        connect(m_d->canvasController, SIGNAL(toolOptionWidgetChanged(QWidget*)), d, SLOT(newOptionWidget(QWidget*)));
    else
        kWarning(41007) << "Could not create tool docker: " << d << endl;

    connectCurrentImage();
    img->blockSignals( false );
    img->unlock();


//     kDebug(41007) << "image finished loading, active layer: " << img->activeLayer() << ", root layer: " << img->rootLayer() << endl;

}


void KisView2::createGUI()
{
    KoToolManager::instance()->addController(m_d->canvasController);

    KoToolBoxFactory toolBoxFactory( m_d->canvasController, "Krita" );
    createDockWidget( &toolBoxFactory );

    KoColorDockerFactory colorDockerFactory;
    KoColorDocker * docker = qobject_cast<KoColorDocker*>( createDockWidget( &colorDockerFactory ) );
    Q_UNUSED( docker );

    KisPaletteDockerFactory paletteDockerFactory(this);
    KisPaletteDocker* paletteDocker = qobject_cast<KisPaletteDocker*>( createDockWidget( &paletteDockerFactory ) );
    Q_UNUSED( paletteDocker );

    KisBirdEyeBoxFactory birdeyeFactory(this);
    m_d->birdEyeBox = qobject_cast<KisBirdEyeBox*>( createDockWidget( &birdeyeFactory ) );

    KisLayerBoxFactory layerboxFactory;
    m_d->layerBox = qobject_cast<KisLayerBox*>( createDockWidget( &layerboxFactory ) );

    m_d->statusBar = KoView::statusBar() ? new KisStatusBar( KoView::statusBar(), this ) : 0;
    connect(m_d->canvasController, SIGNAL( documentMousePositionChanged(const QPointF & )),
            m_d->statusBar, SLOT( documentMousePositionChanged( const QPointF & ) ) );

    m_d->controlFrame = new KisControlFrame( mainWindow(), this );

    show();

    connect(m_d->layerBox, SIGNAL(sigRequestLayer(KisGroupLayerSP, KisLayerSP)),
            m_d->layerManager, SLOT(addLayer(KisGroupLayerSP, KisLayerSP)));

    connect(m_d->layerBox, SIGNAL(sigRequestGroupLayer(KisGroupLayerSP, KisLayerSP)),
            m_d->layerManager, SLOT(addGroupLayer(KisGroupLayerSP, KisLayerSP)));

    connect(m_d->layerBox, SIGNAL(sigRequestAdjustmentLayer(KisGroupLayerSP, KisLayerSP)),
            m_d->layerManager, SLOT(addAdjustmentLayer(KisGroupLayerSP, KisLayerSP)));

    connect(m_d->layerBox, SIGNAL(sigRequestLayerProperties(KisLayerSP)),
            m_d->layerManager, SLOT(showLayerProperties(KisLayerSP)));

    connect(m_d->layerBox, SIGNAL(sigOpacityChanged(double, bool)),
            m_d->layerManager, SLOT(layerOpacity(double, bool)));

    connect(m_d->layerBox, SIGNAL(sigItemComposite(const KoCompositeOp*)),
            m_d->layerManager, SLOT(layerCompositeOp(const KoCompositeOp*)));
}


void KisView2::createActions()
{
    actionCollection()->addAction(KStandardAction::FullScreen, "full_screen", this, SLOT( slotUpdateFullScreen( bool )));
    actionCollection()->addAction(KStandardAction::Preferences,  "preferences", this, SLOT(slotPreferences()));

    KAction* action = new KAction(i18n("Edit Palette..."), this);
    actionCollection()->addAction("edit_palette", action );
    connect(action, SIGNAL(triggered()), this, SLOT(slotEditPalette()));
}


void KisView2::createManagers()
{
    // Create the managers for filters, selections, layers etc.
    // XXX: When the currentlayer changes, call updateGUI on all
    // managers

    m_d->filterManager = new KisFilterManager(this, m_d->doc);
    m_d->filterManager->setup(actionCollection());

    m_d->selectionManager = new KisSelectionManager( this, m_d->doc );
    m_d->selectionManager->setup( actionCollection() );

    m_d->layerManager = new KisLayerManager( this, m_d->doc );
    m_d->layerManager->setup( actionCollection() );

    // the following cast is not really safe, but better here than in the zoomManager
    // best place would be outside kisview too
    m_d->zoomManager = new KisZoomManager( this, m_d->viewConverter, m_d->canvasController);
    m_d->zoomManager->setup( actionCollection() );

    m_d->imageManager = new KisImageManager( this );
    m_d->imageManager->setup( actionCollection() );

//     m_d->maskManager = new KisMaskManager( this );
//     m_d->maskManager->setup( actionCollection() );

    m_d->gridManager = new KisGridManager( this );
    m_d->gridManager->setup( actionCollection() );

    m_d->perspectiveGridManager = new KisPerspectiveGridManager( this );
    m_d->perspectiveGridManager->setup( actionCollection() );

}

void KisView2::updateGUI()
{

    m_d->layerManager->updateGUI();
    m_d->selectionManager->updateGUI();
    m_d->filterManager->updateGUI();
    m_d->zoomManager->updateGUI();
    m_d->imageManager->updateGUI();
//     m_d->maskManager->updateGUI();
    m_d->gridManager->updateGUI();
    m_d->perspectiveGridManager->updateGUI();

}


void KisView2::connectCurrentImage()
{
    KisImageSP img = image();
    if (img) {
//         kDebug(41007) << "Going to connect current image\n";

        connect(img.data(), SIGNAL(sigActiveSelectionChanged(KisImageSP)), m_d->selectionManager, SLOT(imgSelectionChanged(KisImageSP)));
        //connect(img.data(), SIGNAL(sigActiveSelectionChanged(KisImageSP)), this, SLOT(updateCanvas()));
        if( m_d->statusBar ) {
            connect(img.data(), SIGNAL(sigColorSpaceChanged(KoColorSpace *)), m_d->statusBar, SLOT(updateStatusBarProfileLabel()));
            connect(img.data(), SIGNAL(sigProfileChanged(KoColorProfile * )), m_d->statusBar, SLOT(updateStatusBarProfileLabel()));
            connect(img.data(), SIGNAL(sigSizeChanged(qint32, qint32)), m_d->statusBar, SLOT(imageSizeChanged(qint32, qint32)));

        }
        connect(img.data(), SIGNAL( sigSizeChanged( qint32, qint32 ) ), m_d->resourceProvider, SLOT( slotSetImageSize( qint32, qint32 ) ) );
        connect(img.data(), SIGNAL(sigLayersChanged(KisGroupLayerSP)), m_d->layerManager, SLOT(layersUpdated()));
//         connect(img.data(), SIGNAL(sigMaskInfoChanged()), m_d->maskManager, SLOT(maskUpdated()));
        connect(img.data(), SIGNAL(sigLayerAdded(KisLayerSP)), m_d->layerManager, SLOT(layersUpdated()));
        connect(img.data(), SIGNAL(sigLayerRemoved(KisLayerSP, KisGroupLayerSP, KisLayerSP)), m_d->layerManager, SLOT(layersUpdated()));
        connect(img.data(), SIGNAL(sigLayerMoved(KisLayerSP, KisGroupLayerSP, KisLayerSP)), m_d->layerManager, SLOT(layersUpdated()));
        connect(img.data(), SIGNAL(sigLayerPropertiesChanged(KisLayerSP)), m_d->layerManager, SLOT(layersUpdated()));

//         m_d->maskManager->maskUpdated();

        // Temporary forwarding of signals until these deprecated
        // signals are gone from KisImage
        connect( img.data(), SIGNAL( sigColorSpaceChanged( KoColorSpace * ) ),
                 m_d->layerManager, SIGNAL( currentColorSpaceChanged( KoColorSpace * ) ) );
        connect( img.data(), SIGNAL( sigLayerActivated( KisLayerSP ) ),
                 m_d->layerManager, SIGNAL( sigLayerActivated( KisLayerSP  ) ) );

        connect(m_d->layerManager, SIGNAL(sigLayerActivated(KisLayerSP)), m_d->layerManager, SLOT(layersUpdated()));
        connect(m_d->layerManager, SIGNAL(sigLayerActivated(KisLayerSP)), m_d->canvas, SLOT(updateCanvas()));

    }
    m_d->canvas->connectCurrentImage();

    if( m_d->layerBox ) {
        m_d->layerBox->setImage( img, m_d->doc->layerModel() );
        connect( m_d->doc->layerModel(), SIGNAL( layerActivated( KisLayerSP ) ), m_d->layerManager, SLOT( activateLayer( KisLayerSP ) ) );
    }

    if( m_d->birdEyeBox )
        m_d->birdEyeBox->setImage( img );
}

void KisView2::disconnectCurrentImage()
{
    KisImageSP img = image();

    if (img) {

        img->disconnect(this);
        img->disconnect( m_d->layerManager );
        img->disconnect( m_d->selectionManager );
        if( m_d->statusBar )
            img->disconnect( m_d->statusBar );

        if( m_d->layerBox )
            m_d->layerBox->setImage(KisImageSP(0), 0);
        if( m_d->birdEyeBox )
            m_d->birdEyeBox->setImage(KisImageSP(0));
        m_d->canvas->disconnectCurrentImage();
    }
}

void KisView2::setupPrinter(KPrinter &p)
{
    p.setMinMax(1, 1);
}

void KisView2::print(KPrinter& printer)
{
    QPainter gc(&printer);

    KisImageSP img = image();
    if (!img) return;

    gc.setClipping(false);

    KisConfig cfg;
    QString printerProfileName = cfg.printerProfile();
    KoColorProfile *printerProfile = KoColorSpaceRegistry::instance()->profileByName(printerProfileName);

    double exposure = m_d->canvas->resourceProvider()->resource(KisResourceProvider::HdrExposure ).toDouble();

    double scaleX = printer.resolution() / (72.0 * img->xRes());
    double scaleY = printer.resolution() / (72.0 * img->yRes());

    QRect r = img->bounds();

    gc.scale(scaleX, scaleY);
    img->renderToPainter(0, 0, r.x(), r.y(), r.width(), r.height(), gc, printerProfile, exposure);
}

void KisView2::slotUpdateFullScreen(bool toggle)
{
    if (KoView::shell()) {

        Qt::WindowStates newState = KoView::shell()->windowState();

        if (toggle) {
            newState |= Qt::WindowFullScreen;
        } else {
            newState &= ~Qt::WindowFullScreen;
        }

        KoView::shell()->setWindowState(newState);
    }
}

void KisView2::slotPreferences()
{
    if (PreferencesDialog::editPreferences()) {
        m_d->canvas->resetCanvas();
    }
}

void KisView2::slotEditPalette()
{
    KisResourceServerBase* srv = KisResourceServerRegistry::instance()->value("PaletteServer");
    if (!srv) {
        return;
    }
    QList<KoResource*> resources = srv->resources();
    QList<KoColorSet*> palettes;

    foreach (KoResource *resource, resources) {
        KoColorSet* palette = dynamic_cast<KoColorSet*>(resource);
        palettes.append(palette);
    }

    KDialog* base = new KDialog(this );
    base->setCaption(  i18n("Edit Palette") );
    base->setButtons( KDialog::Ok);
    base->setDefaultButton( KDialog::Ok);
    KisCustomPalette* cp = new KisCustomPalette(palettes, base, "edit palette", i18n("Edit Palette"), this);
    base->setMainWidget(cp);
    base->show();
}

void KisView2::loadPlugins()
{
    // Load all plugins
    KService::List offers = KServiceTypeTrader::self()->query(QString::fromLatin1("Krita/ViewPlugin"),
                                                              QString::fromLatin1("(Type == 'Service') and "
                                                                                  "([X-Krita-Version] == 3)"));
    KService::List::ConstIterator iter;
    for(iter = offers.begin(); iter != offers.end(); ++iter)
    {
        KService::Ptr service = *iter;
        int errCode = 0;
        KParts::Plugin* plugin =
            KService::createInstance<KParts::Plugin> ( service, this, QStringList(), &errCode);
        if ( plugin ) {
//             kDebug(41006) << "found plugin " << service->property("Name").toString() << "\n";
            insertChildClient(plugin);
        }
        else {
//             kDebug(41006) << "found plugin " << service->property("Name").toString() << ", " << errCode << "\n";
            if( errCode == KLibLoader::ErrNoLibrary)
            {
                kWarning() << " Error loading plugin was : ErrNoLibrary " << KLibLoader::self()->lastErrorMessage() << endl;
            }
        }
    }
}

KisDoc2 * KisView2::document() const
{
    return m_d->doc;
}

#include "kis_view2.moc"
