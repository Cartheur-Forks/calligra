/*
 *  kis_view.cc - part of Krayon
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *                1999 Michael Koch    <koch@kde.org>
 *                1999 Carsten Pfeiffer <pfeiffer@kde.org>
 *
 *  Copyright (c) 2000 John Califf <jcaliff@compuzone.net>
 *  Copyright (c) 2002 Patrick Julien <freak@ideasandassociates.com>
 *
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// qt includes
#include <qevent.h>
#include <qpainter.h>
#include <qbutton.h>
#include <qscrollbar.h>
#include <qstringlist.h>
#include <qclipboard.h>

// kde includes
#include <kmessagebox.h>
#include <kruler.h>
#include <kaction.h>
#include <klocale.h>
#include <khelpmenu.h>
#include <kaboutdata.h>
#include <kstdaction.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kimageeffect.h>
#include <kapplication.h>
#include <kmimetype.h>

// Grmbl, X headers.....
#ifdef GrayScale
#define FooXGrayScale GrayScale
#undef GrayScale
#endif
#include <kprinter.h>
#ifdef FooXGrayScale
#define GrayScale 1
#undef FooXGrayScale
#endif

// core classes
#include "kis_view.h"
#include "kis_doc.h"
#include "kis_util.h"
#include "kis_canvas.h"
#include "kis_framebuffer.h"
#include "kis_painter.h"
#include "kis_sidebar.h"
#include "kis_tabbar.h"
#include "kis_krayon.h"
#include "kis_brush.h"
#include "kis_pattern.h"
#include "kis_tool.h"
#include "kis_factory.h"
#include "kis_gradient.h"
#include "kis_pluginserver.h"
#include "kis_selection.h"
#include "kfloatingdialog.h"
#include "KRayonViewIface.h"

// sidebar
#include "kis_brushchooser.h"
#include "kis_patternchooser.h"
#include "kis_krayonchooser.h"
#include "kis_layerview.h"
#include "kis_channelview.h"

// dialogs
#include "kis_dlg_gradient.h"
#include "kis_dlg_preferences.h"
#include "kis_dlg_new.h"
#include "kis_dlg_new_layer.h"

// tools
#include "kis_tool_factory.h"
#include "kis_tool_paste.h"	

// debug
#include <kdebug.h>
#include "kis_timer.h"

/*
    KisView - constructor.  What is a KoView?  A widget, but it
    also seems to be a hybrid or composite of several koffice objects
    designed to contain the document and display it. Every koffice app
    does it differently but krayon's is the exemplary view which sets
    the standard.   A document can have more than one view.

    In my opinion it is a flaw to have a view limited to a single document.
    A view should be able to contain multiple documents, switching them
    in and out as needed. This is somewhat overcome in  Krayon by allowing
    multiple images per document, but each view always shows the same
    image and even the same layer within each image, although
    with varying magnificaitons, etc. This is bad. Especially with split
    views one should be able to have a different image in each pane, and
    cut and paste, make comparisons, etc., between them.
*/
KisView::KisView(KisDoc* doc, QWidget* parent, const char* name)
    : super(doc, parent, name)
{
	m_doc = doc;
	m_zoomFactor = 1.0;
	setInstance(KisFactory::global());
	setXMLFile("krita.rc");
	m_pTool = 0;
	m_dcop = 0;
	dcopObject(); // build it

	QObject::connect(m_doc, SIGNAL(docUpdated()), this, SLOT(slotDocUpdated()));
	QObject::connect(m_doc, SIGNAL(docUpdated(const QRect&)), this, SLOT(slotDocUpdated(const QRect&)));
	QObject::connect(this, SIGNAL(embeddImage(const QString&)), this, SLOT(slotEmbeddImage(const QString&)));

	m_fg = KisColor::black();
	m_bg = KisColor::white();

	m_xPaintOffset = 0;
	m_yPaintOffset = 0;

	m_pTool     = 0L;
	m_pBrush    = 0L;
	m_pPattern  = 0L;
	m_pGradient = 0L;

	setupPainter();
	setupCanvas();
	setupScrollBars();
	setupRulers();
	setupTabBar();
	setupActions();
	setupSideBar();
	slotSetBrush(m_pBrush);
	slotSetPattern(m_pPattern);
}

/*
    KisView destructor.  Delete objects created apart from the
    widget and its children.
*/
KisView::~KisView()
{
	delete m_dcop;
}

DCOPObject* KisView::dcopObject()
{
    if (!m_dcop)
	m_dcop = new KRayonViewIface(this);

    return m_dcop;
}


/*
    Set up painter object for use of QPainter methods to draw
    into KisLayer memory.  Note:  The painter object should be
    attached to the image, not view.  It needs to be moved.
    The reason for this is that the painter paints on the image,
    and has nothing to do with the canvas or the view. If there
    are multiple views then the same painter paints images which
    are shown in each view.

    Even better, the current image should be set by the view and not
    by the document.  Each view could show a different current
    image.  Curently each view shows the same image, which is very
    limiting, although it's more compliant with koffice standards.
    The best solution is to have a painter for each image, which would
    take more memory, but detaches it from either document or view.
*/
void KisView::setupPainter()
{
	m_pPainter = new KisPainter(m_doc, this);
}


/*
    Canvas for document (image) area - it's just a plain QWidget used to
    pass signals (messages) on to the tools and to create an area on
    which to show the content of the document (the image).  Note that
    while the depth of the image is not limited, the depth of the
    pixmap displayed on the widget is limited to the hardware display
    depth, which is often 16 bit even though the KisImage is 32 bit and
    can eventually be extended to 64 bit.
*/
void KisView::setupCanvas()
{
    m_pCanvas = new KisCanvas(this, "kis_canvas");

    QObject::connect(m_pCanvas, SIGNAL(mousePressed( QMouseEvent*)),
        this, SLOT(canvasGotMousePressEvent (QMouseEvent*)) );

    QObject::connect(m_pCanvas, SIGNAL(mouseMoved( QMouseEvent*)),
        this, SLOT(canvasGotMouseMoveEvent (QMouseEvent*)) );

    QObject::connect(m_pCanvas, SIGNAL(mouseReleased (QMouseEvent*)),
        this, SLOT(canvasGotMouseReleaseEvent (QMouseEvent*)) );

    QObject::connect(m_pCanvas, SIGNAL(gotPaintEvent (QPaintEvent*)),
		this, SLOT(canvasGotPaintEvent (QPaintEvent*)) );

    QObject::connect(m_pCanvas, SIGNAL(gotEnterEvent (QEvent*)),
		this, SLOT(canvasGotEnterEvent (QEvent*)) );

    QObject::connect(m_pCanvas, SIGNAL(gotLeaveEvent (QEvent*)),
		this, SLOT(canvasGotLeaveEvent (QEvent*)) );

    QObject::connect(m_pCanvas, SIGNAL(mouseWheelEvent( QWheelEvent*)),
                      this, SLOT(canvasGotMouseWheelEvent(QWheelEvent*)) );
}


/*
    SideBar - has tabs for brushes, layers, channels, etc.
    Nonstandard, but who sets the standards?  Unrelieved
    uniformity is the mark of small minds.  Note:  Some of these
    sidebar widgets are unfinished and just have placeholders
    so far.  Note that the sidebar itself, and each widget in it,
    can be detached or floated if the user wants them to be
    separate windows.
*/
void KisView::setupSideBar()
{
    m_pSideBar = new KisSideBar(this, "kis_sidebar");

    // krayon chooser
    m_pKrayonChooser = new KisKrayonChooser(this);
    m_pKrayon = m_pKrayonChooser->currentKrayon();
    QObject::connect(m_pKrayonChooser,
        SIGNAL(selected(KisKrayon *)),
        this, SLOT(slotSetKrayon(KisKrayon*)));

    m_pKrayonChooser->setCaption(i18n("Krayons"));
    m_pSideBar->plug(m_pKrayonChooser);

    // brush chooser
    m_pBrushChooser = new KisBrushChooser(m_pSideBar->dockFrame());
    m_pBrush = m_pBrushChooser->currentBrush();
    QObject::connect(m_pBrushChooser,
        SIGNAL(selected(KisBrush *)),
        this, SLOT(slotSetBrush(KisBrush*)));

    m_pBrushChooser->setCaption(i18n("Brushes"));
    m_pSideBar->plug(m_pBrushChooser);

    // pattern chooser
    m_pPatternChooser = new KisPatternChooser(this);
    m_pPattern = m_pPatternChooser->currentPattern();
    QObject::connect(m_pPatternChooser,
        SIGNAL(selected(KisPattern *)),
        this, SLOT(slotSetPattern(KisPattern*)));

    m_pPatternChooser->setCaption(i18n("Patterns"));
    m_pSideBar->plug(m_pPatternChooser);

   // gradient chooser
    m_pGradientChooser = new QWidget(this);
    m_pGradient = new KisGradient;

    /*
    m_pGradient = m_pGradientChooser->currentGradient();
    QObject::connect(m_pGradientChooser,
        SIGNAL(selected(KisGradient *)),
        this, SLOT(slotSetGradient(KisGradient*)));
    */
    m_pGradientChooser->setCaption(i18n("Gradients"));
    m_pSideBar->plug(m_pGradientChooser);

   // image file chooser
    m_pImageChooser = new QWidget(this);
    /*
    m_pImage = m_pImageFileChooser->currentImageFile();
    QObject::connect(m_pImageFileChooser,
        SIGNAL(selected(KisImageFile *)),
        this, SLOT(slotSetImageFile(KisImageFile*)));
    */
    m_pImageChooser->setCaption(i18n("Images"));
    m_pSideBar->plug(m_pImageChooser);

   // palette chooser
    m_pPaletteChooser = new QWidget(this);
    /*
    m_pPalette = m_pPaletteChooser->currentPattern();
    QObject::connect(m_pPaletteChooser,
        SIGNAL(selected(KisPalette *)),
        this, SLOT(slotSetPalette(KisPalette *)));
    */
    m_pPaletteChooser->setCaption(i18n("Palettes"));
    m_pSideBar->plug(m_pPaletteChooser);

    // layer view
    m_pLayerView = new KisLayerView(m_doc, this);
    m_pLayerView->setCaption(i18n("Layers"));
    m_pSideBar->plug(m_pLayerView);

    // channel view
    m_pChannelView = new KisChannelView(m_doc, this);
    m_pChannelView->setCaption(i18n("Channels"));
    m_pSideBar->plug(m_pChannelView);

    // activate brushes tab
    m_pSideBar->slotActivateTab(i18n("Brushes"));

    // init sidebar
    m_pSideBar->slotSetBrush(*m_pBrush);
    m_pSideBar->slotSetFGColor(m_fg);
    m_pSideBar->slotSetBGColor(m_bg);

    connect(m_pSideBar, SIGNAL(fgColorChanged(const KisColor&)),
        this, SLOT(slotSetFGColor(const KisColor&)));
    connect(m_pSideBar, SIGNAL(bgColorChanged(const KisColor&)),
        this, SLOT(slotSetBGColor(const KisColor&)));

    connect(this, SIGNAL(fgColorChanged(const KisColor&)),
        m_pSideBar, SLOT(slotSetFGColor(const KisColor&)));
    connect(this, SIGNAL(bgColorChanged(const KisColor&)),
        m_pSideBar, SLOT(slotSetBGColor(const KisColor&)));

    m_side_bar->setChecked(true);
}

/*
    setupScrollBars - setting them up is easy.  Now why don't the
    darned scroll bars always show up when they should?
*/
void KisView::setupScrollBars()
{
    m_pVert = new QScrollBar(QScrollBar::Vertical, this);
    m_pHorz = new QScrollBar(QScrollBar::Horizontal, this);

    m_pVert->setGeometry(width()-16, 20, 16, height()-36);
    m_pHorz->setGeometry(20, height()-16, width()-36, 16);
    m_pHorz->setValue(0);
    m_pVert->setValue(0);

    QObject::connect(m_pVert,
        SIGNAL(valueChanged(int)), this, SLOT(scrollV(int)));
    QObject::connect(m_pHorz,
        SIGNAL(valueChanged(int)), this, SLOT(scrollH(int)));
}

/*
    Where are the numbers on the ruler?  What about a grid aligned
    to the rulers. We also need to change the tick marks as zoom
    levels change. Coming....
*/
void KisView::setupRulers()
{
    m_pHRuler = new KRuler(Qt::Horizontal, this);
    m_pVRuler = new KRuler(Qt::Vertical, this);

    m_pHRuler->setGeometry(20, 0, width()-20, 20);
    m_pVRuler->setGeometry(0, 20, 20, height()-20);

    m_pVRuler->setRulerMetricStyle(KRuler::Pixel);
    m_pHRuler->setRulerMetricStyle(KRuler::Pixel);

    m_pVRuler->setFrameStyle(QFrame::Panel | QFrame::Raised);
    m_pHRuler->setFrameStyle(QFrame::Panel | QFrame::Raised);

    m_pHRuler->setLineWidth(1);
    m_pVRuler->setLineWidth(1);

    //m_pHRuler->setShowEndLabel(true);
    //m_pVRuler->setShowEndLabel(true);
}


/*
    setupTabBar - for the image(s) - one tab per image.
    This Nonstandard(tm) tabbar violates koffice style guidelines,
    but many koffice apps now use these tabbars!  Who sets these standards
    that nobody follows because they are impractical, anyway?
*/
void KisView::setupTabBar()
{
    // tabbar
    m_pTabBar = new KisTabBar(this, m_doc);
    m_pTabBar->slotImageListUpdated();

    QObject::connect(m_pTabBar, SIGNAL(tabSelected(const QString&)),
		    m_doc, SLOT(setCurrentImage(const QString&)) );

    QObject::connect(m_doc, SIGNAL(imageListUpdated()),
		    m_pTabBar, SLOT(slotImageListUpdated()) );

    // tabbar control buttons
    m_pTabFirst = new QPushButton(this);
    m_pTabFirst->setPixmap(QPixmap(BarIcon( "tab_first")) );
    QObject::connect(m_pTabFirst, SIGNAL(clicked()),
        m_pTabBar, SLOT(slotScrollFirst()));

    m_pTabLeft = new QPushButton(this);
    m_pTabLeft->setPixmap(QPixmap(BarIcon( "tab_left")) );
    QObject::connect(m_pTabLeft, SIGNAL(clicked()),
        m_pTabBar, SLOT(slotScrollLeft()));

    m_pTabRight = new QPushButton(this);
    m_pTabRight->setPixmap(QPixmap(BarIcon( "tab_right")) );
    QObject::connect(m_pTabRight, SIGNAL(clicked()),
        m_pTabBar, SLOT(slotScrollRight()));

    m_pTabLast = new QPushButton(this);
    m_pTabLast->setPixmap(QPixmap(BarIcon( "tab_last")) );
    QObject::connect(m_pTabLast, SIGNAL(clicked()),
        m_pTabBar, SLOT(slotScrollLast()));
}

/*
    Actions - these seem to be menu actions, toolbar actions
    and keyboard actions.  Any action can take on any of these forms,
    at least.  However, using Kde's brain-damaged xmlGui because it
    is the "right(tm)" thing to do, slots cannot take any paramaters
    and each handler must have its own method, greatly increasing
    code size and precluding consolidation or related actions, not to
    mention slower startup required by interpretation of rc files.
    For every action there is an equal and opposite reaction.
*/

void KisView::setupActions()
{
	// history actions
	(void)KStdAction::undo(this, SLOT(undo()), actionCollection(), "edit_undo");
	(void)KStdAction::redo(this, SLOT(redo()), actionCollection(), "edit_redo");

	// navigation actions
	(void)new KAction(i18n("Refresh Canvas"), "reload", 0, this, SLOT(slotDocUpdated()), actionCollection(), "refresh_canvas");
	(void)new KAction(i18n("Panic Button"), "stop", 0, this, SLOT(slotHalt()), actionCollection(), "panic_button");

	// selection actions
	(void)KStdAction::cut(this, SLOT(cut()), actionCollection(), "cut");
	(void)KStdAction::copy(this, SLOT(copy()), actionCollection(), "copy");
	(void)KStdAction::paste(this, SLOT(paste()), actionCollection(), "paste_special");
	(void)new KAction(i18n("Remove selection"), "remove", 0, this, SLOT(removeSelection()), actionCollection(), "remove");
	(void)new KAction(i18n("Copy selection to new layer"), "crop", 0,  this, SLOT(crop()), actionCollection(), "crop");
	(void)KStdAction::selectAll(this, SLOT(selectAll()), actionCollection(), "select_all");
	(void)new KAction(i18n("Select None"), 0, this, SLOT(unSelectAll()), actionCollection(), "select_none");

	// import/export actions
	(void)new KAction(i18n("Import Image"), "wizard", 0, this, SLOT(import_image()), actionCollection(), "import_image");
	(void)new KAction(i18n("Export Image"), "wizard", 0, this, SLOT(export_image()), actionCollection(), "export_image");

	// view actions
	(void)new KAction(i18n("Zoom &in"), "viewmag+", 0, this, SLOT(zoom_in()), actionCollection(), "zoom_in");
	(void)new KAction(i18n("Zoom &out"), "viewmag-", 0, this, SLOT(zoom_out()), actionCollection(), "zoom_out"); 

	// tool settings actions
	(void)new KAction(i18n("&Gradient Dialog"), "blend", 0, this, SLOT(dialog_gradient()), actionCollection(), "dialog_gradient");

	// tool actions
	(void)new KAction(i18n("&Current Tool Properties..."), "configure", 0, this, SLOT(tool_properties()), actionCollection(), "current_tool_properties");

	// layer actions
	(void)new KAction(i18n("&Add layer..."), 0, this, SLOT(insert_layer()), actionCollection(), "insert_layer");
	(void)new KAction(i18n("&Remove layer..."), 0, this, SLOT(remove_layer()), actionCollection(), "remove_layer"); 
	(void)new KAction(i18n("&Link/Unlink layer..."), 0, this, SLOT(link_layer()), actionCollection(), "link_layer");
	(void)new KAction(i18n("&Hide/Show layer..."), 0, this, SLOT(hide_layer()), actionCollection(), "hide_layer");
	(void)new KAction(i18n("&Next layer..."), "forward", 0, this, SLOT(next_layer()), actionCollection(), "next_layer");
	(void)new KAction(i18n("&Previous layer..."), "back", 0, this, SLOT(previous_layer()), actionCollection(), "previous_layer");
	(void)new KAction(i18n("Layer Properties..."), 0, this, SLOT(layer_properties()), actionCollection(), "layer_properties");
	(void)new KAction(i18n("I&nsert image as layer..."), 0, this, SLOT(insert_image_as_layer()), actionCollection(), "insert_image_as_layer");
	(void)new KAction(i18n("Save layer as image..."), 0, this, SLOT(save_layer_as_image()), actionCollection(), "save_layer_as_image");

	// layer transformations - should be generic, for selection too
	(void)new KAction(i18n("Scale layer smoothly"), 0, this, SLOT(layer_scale_smooth()), actionCollection(), "layer_scale_smooth");
	(void)new KAction(i18n("Scale layer - keep palette"), 0, this, SLOT(layer_scale_rough()), actionCollection(), "layer_scale_rough"); 
	(void)new KAction(i18n("Rotate &180"), 0, this, SLOT(layer_rotate180()), actionCollection(), "layer_rotate180");
	(void)new KAction(i18n("Rotate &270"), 0, this, SLOT(layer_rotateleft90()), actionCollection(), "layer_rotateleft90");
	(void)new KAction(i18n("Rotate &90"), 0, this, SLOT(layer_rotateright90()), actionCollection(), "layer_rotateright90");
	(void)new KAction(i18n("Rotate &Custom"), 0, this, SLOT(layer_rotate_custom()), actionCollection(), "layer_rotate_custom");
	(void)new KAction(i18n("Mirror &X"), 0, this, SLOT(layer_mirrorX()), actionCollection(), "layer_mirrorX");
	(void)new KAction(i18n("Mirror &Y"), 0, this, SLOT(layer_mirrorY()), actionCollection(), "layer_mirrorY");

	// image actions
	(void)new KAction(i18n("Add new image"), 0, this, SLOT(add_new_image_tab()), actionCollection(), "add_new_image_tab");
	(void)new KAction(i18n("Remove current image"), 0, this, SLOT(remove_current_image_tab()), actionCollection(), "remove_current_image_tab");
	(void)new KAction(i18n("Merge &all layers"), 0, this, SLOT(merge_all_layers()), actionCollection(), "merge_all_layers");
	(void)new KAction(i18n("Merge &visible layers"), 0, this, SLOT(merge_visible_layers()), actionCollection(), "merge_visible_layers");
	(void)new KAction(i18n("Merge &linked layers"), 0, this, SLOT(merge_linked_layers()), actionCollection(), "merge_linked_layers");

	// setting actions
	(void)new KToggleAction(i18n("Toggle Paint Offset"), "remove_view", 0, this, SLOT(slotSetPaintOffset()), actionCollection(), "toggle_paint_offset");
	m_side_bar = new KToggleAction(i18n("Show/Hide Sidebar"), "ok", 0, this, SLOT(showSidebar()), actionCollection(), "show_sidebar");
	m_float_side_bar = new KToggleAction(i18n("Dock/Undock Sidebar"), "attach", 0, this, SLOT(floatSidebar()), actionCollection(), "float_sidebar");
	m_lsidebar = new KToggleAction(i18n("Left/Right Sidebar"), "view_right", 0, this, SLOT(leftSidebar()), actionCollection(), "left_sidebar");
	(void)KStdAction::saveOptions(this, SLOT(saveOptions()), actionCollection(), "save_options"); 
	(void)new KAction(i18n("Krayon Preferences"), "edit", 0, this, SLOT(preferences()), actionCollection(), "preferences");

	// krayon box toolbar actions - these will be used only
	// to dock and undock wideget in the krayon box
	m_dialog_colors = new KToggleAction(i18n("&Colors"), "color_dialog", 0, this, SLOT(dialog_colors()), actionCollection(), "colors_dialog");
	m_dialog_krayons = new KToggleAction(i18n("&Krayons"), "krayon_box", 0, this, SLOT(dialog_krayons()), actionCollection(), "krayons_dialog");
	m_dialog_brushes = new KToggleAction(i18n("Brushes"), "brush_dialog", 0, this, SLOT(dialog_brushes()), actionCollection(), "brushes_dialog");
	m_dialog_patterns = new KToggleAction(i18n("Patterns"), "pattern_dialog", 0, this, SLOT(dialog_patterns()), actionCollection(), "patterns_dialog");
	m_dialog_layers = new KToggleAction(i18n("Layers"), "layer_dialog", 0, this, SLOT(dialog_layers()), actionCollection(), "layers_dialog");
	m_dialog_channels = new KToggleAction(i18n("Channels"), "channel_dialog", 0, this, SLOT(dialog_channels()), actionCollection(), "channels_dialog");

	m_dialog_brushes -> setChecked (true);
	m_dialog_patterns -> setChecked (true);
	m_dialog_layers -> setChecked (true);
	m_dialog_channels -> setChecked (true);

	// help actions - these are standard kde actions
	m_helpMenu = new KHelpMenu(this);

	(void)KStdAction::helpContents(m_helpMenu, SLOT(appHelpActivated()), actionCollection(), "help_contents");
	(void)KStdAction::whatsThis(m_helpMenu, SLOT(contextHelpActivated()), actionCollection(), "help_whatsthis");
	(void)KStdAction::reportBug(m_helpMenu, SLOT(reportBug()), actionCollection(), "help_bugreport");
	(void)KStdAction::aboutApp(m_helpMenu, SLOT(aboutApplication()), actionCollection(), "help_about");
}

/*
    slotHalt - try to restore reasonable defaults for a user
    who may have pushed krayon beyond its limits or the
    limits of his/her hardware and system memory!  This can happen
    when someone sets a ridiculously high zoom factor which
    requires a supercomputer for all the floating point
    calculatons.  Krayon is not idiot proof!
*/
void KisView::slotHalt()
{
    KMessageBox::error(NULL,
        "STOP! In the name of Love ...", "System Error", FALSE);

    zoom(0, 0, 1.0);
    slotUpdateImage();
    slotRefreshPainter();
}

/*
    slotGimp - a copout for the weak of mind and faint
    of heart.  Wiblur sucks, remember that!
*/
void KisView::slotGimp()
{
    KMessageBox::error(NULL,
        "Have you lost your mind?", "User Error", FALSE);
    // save current image, export to xcf, open in gimp - coming!
}

/*
    slotTabSelected - these refer to the tabs for images. Currently
    this is the only way to change the current image.  There should
    be other ways, also.
*/
void KisView::slotTabSelected(const QString& name)
{
    m_doc->setCurrentImage(name);
    slotRefreshPainter();
}


/*
    refreshPainter - refresh and resize the painter device
    whenever the image or layer is changed
*/
void KisView::slotRefreshPainter()
{
	KisImage *img = m_doc -> current();

	if(img) {
		KisLayer *lay = img -> getCurrentLayer();

		if(lay) {
			QRect extents(lay -> imageExtents());

			m_pPainter -> resize(extents.left() + extents.width(), extents.top() + extents.height());
		}

		m_pPainter -> clearAll();
	}
}

/*
    resizeEvent - only via a resize event are things shown
    in the view.  To start, nothing is shown.  The first
    resize comes when the objects to be shown are created.
    This methold handles much which is not obvious, reducing
    the need for so many methods to explicitly resize things.
*/
void KisView::resizeEvent(QResizeEvent*)
{
    // sidebar width right or left side
    int rsideW = 0;
    int lsideW = 0;

    // ruler thickness
    int ruler = 20;

    // tab bar dimensions
    int tbarOffset = 64;
    int tbarBtnH = 16;
    int tbarBtnW = 16;

    // show or hide sidebar
    if(!m_pSideBar)
    {
        rsideW = 0;
        lsideW = 0;
    }
    else
    {
        if(m_side_bar->isChecked() && !m_float_side_bar->isChecked())
        {
            if(m_lsidebar->isChecked())
            {
                rsideW = 0;
                lsideW = m_pSideBar->width();
            }
            else
            {
                rsideW = m_pSideBar->width();
                lsideW = 0;
            }
        }
        else
        {
           rsideW = 0;
           lsideW = 0;
        }
    }

    // sidebar geometry - only set if visible and NOT free floating
    if (m_pSideBar && !m_float_side_bar->isChecked()
        && m_side_bar->isChecked())
    {
        if(m_lsidebar->isChecked())
            m_pSideBar->setGeometry(0, 0, lsideW, height());
        else
            m_pSideBar->setGeometry(width() - rsideW, 0, rsideW, height());

        m_pSideBar->show();
    }

    // ruler geometry
    m_pHRuler->setGeometry(ruler + lsideW, 0,
        width() - ruler - rsideW - lsideW, ruler);
    m_pVRuler->setGeometry(0 + lsideW, ruler,
        ruler, height() - (ruler + tbarBtnH));

    // tabbar control buttons
    m_pTabFirst->setGeometry(0 + lsideW, height() - tbarBtnH,
        tbarBtnW, tbarBtnH);
    m_pTabFirst->show();

    m_pTabLeft->setGeometry(tbarBtnW + lsideW, height() - tbarBtnH,
        tbarBtnW, tbarBtnH);
    m_pTabLeft->show();

    m_pTabRight->setGeometry(2 * tbarBtnW + lsideW, height() - tbarBtnH,
        tbarBtnW, tbarBtnH);
    m_pTabRight->show();

    m_pTabLast->setGeometry(3 * tbarBtnW + lsideW, height() - tbarBtnH,
        tbarBtnW, tbarBtnH);
    m_pTabLast->show();

    // KisView height/width - ruler height/width
    int drawH = height() - ruler - tbarBtnH;
    int drawW = width() - ruler - lsideW - rsideW;

    // doc width and height are exactly same as the
    // current image's width and height
    int docW = docWidth();
    int docH = docHeight();

    // adjust for zoom scaling - the higher the scaling,
    // the larger the image in direct proportion
    docW = (int)((zoomFactor()) * docW);
    docH = (int)((zoomFactor()) * docH);

    // resize the pixmap for drawing zoomed doc content.
    // this must be done *before* canvas is shown

     // we need no scrollbars
    if (docH <= drawH && docW <= drawW)
    {
        m_pVert->hide();
        m_pHorz->hide();
        m_pVert->setValue(0);
        m_pHorz->setValue(0);

        m_pCanvas->setGeometry(ruler + lsideW, ruler, drawW, drawH);
        m_pCanvas->show();

        m_pTabBar->setGeometry(tbarOffset + lsideW, height() - tbarBtnH,
           width() - rsideW - lsideW - tbarOffset, tbarBtnH);
        m_pTabBar->show();
    }
    // we need a horizontal scrollbar only
    else if (docH <= drawH)
    {
        m_pVert->hide();
        m_pVert->setValue(0);

        //m_pHorz->setRange(0, docW - drawW);
        m_pHorz->setRange(0, (int)((docW - drawW)/zoomFactor()));
        m_pHorz->setGeometry(
            tbarOffset + lsideW + (width() - rsideW -lsideW - tbarOffset)/2,
            height() - tbarBtnH,
            (width() - rsideW -lsideW - tbarOffset)/2,
            tbarBtnH);
        m_pHorz->show();

        m_pCanvas->setGeometry(ruler + lsideW, ruler, drawW, drawH);
        m_pCanvas->show();

        m_pTabBar->setGeometry(tbarOffset + lsideW, height() - tbarBtnH,
           (width() - rsideW - lsideW - tbarOffset)/2, tbarBtnH);
        m_pTabBar->show();
    }
    // we need a vertical scrollbar only
    else if(docW <= drawW)
    {
        m_pHorz->hide();
        m_pHorz->setValue(0);

        //m_pVert->setRange(0, docH - drawH);
        m_pVert->setRange(0, (int)((docH - drawH)/zoomFactor()));
        m_pVert->setGeometry(width() - tbarBtnW - rsideW, ruler,
            tbarBtnW, height() - (ruler + tbarBtnH));
        m_pVert->show();

        m_pCanvas->setGeometry(ruler + lsideW, ruler,
            drawW - tbarBtnW, drawH);
        m_pCanvas->show();

        m_pTabBar->setGeometry(tbarOffset + lsideW, height() - tbarBtnH,
           width() - rsideW -lsideW - tbarOffset, tbarBtnH);
        m_pTabBar->show();
    }
    // we need both scrollbars
    else
    {
        //m_pVert->setRange(0, docH - drawH);
        m_pVert->setRange(0, (int)((docH - drawH)/zoomFactor()));
        m_pVert->setGeometry(width() - tbarBtnW - rsideW, ruler,
            tbarBtnW, height() - (ruler + tbarBtnH));
        m_pVert->show();

        //m_pHorz->setRange(0, docW - drawW);
        m_pHorz->setRange(0, (int)((docW - drawW)/zoomFactor()));
        m_pHorz->setGeometry(
            tbarOffset + lsideW + (width() - rsideW -lsideW - tbarOffset)/2,
            height() - tbarBtnH,
            (width() - rsideW -lsideW - tbarOffset)/2,
            tbarBtnH);
        m_pHorz->show();

        m_pCanvas->setGeometry(ruler + lsideW, ruler,
            drawW - tbarBtnW, drawH);
        m_pCanvas->show();

        m_pTabBar->setGeometry(tbarOffset + lsideW, height() - tbarBtnH,
                (width() - rsideW -lsideW - tbarOffset)/2, tbarBtnH);
        m_pTabBar->show();
    }

    // ruler geometry - need to adjust for zoom factor -jwc-

    // ruler ranges
    m_pVRuler->setRange(0, docH + (int)(100 * zoomFactor()));
    m_pHRuler->setRange(0, docW + (int)(100 * zoomFactor()));

    // ruler offset
    if(m_pVert->isVisible())
        m_pVRuler->setOffset(m_pVert->value());
    else
        m_pVRuler->setOffset(-yPaintOffset());

    if(m_pHorz->isVisible())
        m_pHRuler->setOffset(m_pHorz->value());
    else
        m_pHRuler->setOffset(-xPaintOffset());

    // don't show tiny rulers - min. zoom of 1/8
    m_pHRuler->show();
    m_pVRuler->show();

    // kdDebug() << "Canvas width: "   << m_pCanvas->width()
    // << " Canvas Height: " << m_pCanvas->height() << endl;
}

/*
    updateReadWrite - for the functionally illiterate
*/
void KisView::updateReadWrite(bool /*readwrite*/)
{
}


/*
    scrollH - This sends a paint event to canvas
    when you scroll horizontally, handled in canvasGotPaintEvent().
    The ruler offset is adjusted to the scrollbar value.  Its scale
    needs to adjusted for zoom factor here.
*/
void KisView::scrollH(int)
{

    m_pHRuler->setOffset(m_pHorz->value());
    m_pCanvas->repaint();
}

/*
    scrollH - This sends a paint event to canvas
    when you scroll vertically, handled in canvasGotPaintEvent().
    The ruler offset is adjusted to the scrollbar value.
    Its scale needs to adjusted for zoom factor here.
*/
void KisView::scrollV(int)
{
    m_pVRuler->setOffset(m_pVert->value());
    m_pCanvas->repaint();
}

/*
    slotUpdateImage - a cheap hack to mark the entire image
    dirty to force a repaint AND to send a fake resize event
    to force the view to show the scrollbars
*/
void KisView::slotUpdateImage()
{
    KisImage* img = m_doc->current();
    if(img)
    {
        QRect updateRect(0, 0, img->width(), img->height());
        img->markDirty(updateRect);
    }
}

/*
    slotDocUpdated - response to a signal from the document
    that there is a new or different current image for the
    document - setCurrentImage() in kis_doc.cc
*/
void KisView::slotDocUpdated()
{
	//kdDebug() << "slotDocUpdated()" << endl;

	/*
	 * clear the entire canvas area - especially needed when
	 * the new current image is smaller than the former to remove
	 * artifacts of the former image from the canvas.  This is very
	 * fast, even if the image is quite large, because only the
	 * canvas pixmap, which is no bigger than the visible viewport,
	 * is cleared.
	 */

	QPainter p;
	QRect ur(0, 0, m_pCanvas -> width(), m_pCanvas -> height());

	p.begin(m_pCanvas);
	p.eraseRect(ur);
	p.end();

	/*
	 * repaint contents of view. There is already a new or
	 * different current Image.  It is not changed, but the contents
	 * of the image are displayed on the fresh canvas
	 */

	m_pCanvas -> repaint();

	/*
	 * reset and resize the KisPainter pixmap (paint device).
	 * The pixmap could be too small for the new image, causing
	 * a crash,  or too large, causing inefficent use of memory
	 */

	slotRefreshPainter();
}

/*
    slotDocUpdated - response to a signal from
    the document that content has changed and that we
    need to update the canvas -  a definite update area
    is given, so only update that rectangle's contents.
*/
void KisView::slotDocUpdated(const QRect& rect)
{
	KisImage *img;
	QPainter p;
	int xt;
	int yt;

	if (!(img = m_doc -> current()))
		return;

	QRect ur = rect;


	ur = ur.intersect(img->imageExtents());
	ur.setBottom(ur.bottom() + 1);
	ur.setRight(ur.right() + 1);
	xt = xPaintOffset() + ur.x() - m_pHorz -> value();
	yt = yPaintOffset() + ur.y() - m_pVert -> value();

	p.begin(m_pCanvas);
	p.setClipRect(ur);
	Q_ASSERT(p.hasClipping());
	p.scale(zoomFactor(), zoomFactor());
	p.translate(xt, yt);

	// This is the place where image is drawn on the canvas
	// p is the paint device, ur is the update rectangle,
	// bool transparency, ptr to the particular view to update

	koDocument() -> paintEverything(p, ur, false, this);
	p.end();
}

void KisView::clearCanvas(const QRect& rc)
{
	QPainter p;

	kdDebug(0) << "No curent image" << endl;
	p.begin(m_pCanvas);
	p.eraseRect(rc);
	p.end();
}

void KisView::paintView(const QRect& rc)
{
	KisImage* img = m_doc -> current();
	QRect ur = rc;
	QPainter p;

	if (!img) {
		clearCanvas(ur);
		return;
	}
	
	p.begin(m_pCanvas);
	
	// erase strip along left side
	p.eraseRect(0, 0, xPaintOffset(), height());

	// erase strip along top
	p.eraseRect(xPaintOffset(), 0, width(), yPaintOffset());

	// erase area to the below - account for zoomed width of doc
	p.eraseRect(xPaintOffset(), yPaintOffset() + static_cast<int>(docHeight() * zoomFactor()), width(), height());

	// erase area to right - account for zoomed height of doc
	p.eraseRect(xPaintOffset() + static_cast<int>(docWidth() * zoomFactor()), yPaintOffset(), width(), height());

	// scale the paint device only after clearing border areas
	p.scale(zoomFactor(), zoomFactor());

	ur.moveBy(- xPaintOffset() + m_pHorz -> value(), - yPaintOffset() + m_pVert -> value());
	ur = ur.intersect(img -> imageExtents());
	ur.setBottom(ur.bottom() + 1);
	ur.setRight(ur.right() + 1);

	if (ur.top() > img -> height() || ur.left() > img -> width())
		return;

	int xt = xPaintOffset() + ur.x() - m_pHorz -> value();
	int yt = yPaintOffset() + ur.y() - m_pVert -> value();

	p.translate(xt, yt);
	m_doc -> paintContent(p, ur);
	p.end();

	if (m_pTool && !m_pTool -> willModify()) {
		QPaintEvent ev(ur, false);

		m_pTool -> paintEvent(&ev);
	}
}

/*
    updateCanvas - update canvas regardless of paint event
    for transferring offscreen updates that do not generate
    paint events for the canvas
*/
void KisView::updateCanvas(const QRect& rc)
{
	QRect ur = rc;

	// reduce size of update rectangle by inverse of zoom factor
	// only do this at higher/lower zooms.
	if(zoomFactor() > 1.0 || zoomFactor() < 1.0) {
		int urW = ur.width();
		int urH = ur.height();

		urW = (int)((float)(urW) / zoomFactor());
		urH = (int)((float)(urH) / zoomFactor());
		ur.setWidth(urW);
		ur.setHeight(urH);
	}

	paintView(ur);
}

/*
    canvasGotPaintEvent - handles repaint of canvas (image) area
*/
void KisView::canvasGotPaintEvent(QPaintEvent *e)
{
	QRect ur = e -> rect();

	// reduce size of update rectangle by inverse of zoom factor
	// also reduce offset into image by same factor (1/zoomFactor())
	if (zoomFactor() > 1.0 || zoomFactor() < 1.0) {
		int urL = ur.left();
		int urT = ur.top();
		int urW = ur.width();
		int urH = ur.height();

		urL = (int)((float)(urL) / zoomFactor());
		urT = (int)((float)(urT) / zoomFactor());
		urW = (int)((float)(urW) / zoomFactor());
		urH = (int)((float)(urH) / zoomFactor());
		ur.setLeft(urL);
		ur.setTop(urT);
		ur.setWidth(urW);
		ur.setHeight(urH);
	}

	paintView(ur);
}


/*
    canvasGotMousePressEvent - just passes the signal on
    to the appropriate tool
*/
void KisView::canvasGotMousePressEvent(QMouseEvent *e)
{
	if (m_pTool) {
		int x = e -> pos().x() - xPaintOffset() + (int)(zoomFactor() * m_pHorz -> value());
		int y = e -> pos().y() - yPaintOffset() + (int)(zoomFactor() * m_pVert -> value());
		QMouseEvent ev(QEvent::MouseButtonPress, QPoint(x, y), e -> globalPos(), e -> button(), e -> state());

		m_pTool -> mousePress(&ev);

		if (e -> button() == Qt::LeftButton && m_pTool && m_pTool -> willModify())
			m_doc -> setModified(true);
	}
}


/*
    canvasGotMouseMoveEvent - just passes the signal on
    to the appropriate tool - also sets ruler pointers
*/
void KisView::canvasGotMouseMoveEvent (QMouseEvent *e)
{
	if (m_pTool) {
		int x = e -> pos().x() - xPaintOffset() + (int)(zoomFactor() * m_pHorz -> value());
		int y = e -> pos().y() - yPaintOffset() + (int)(zoomFactor() * m_pVert -> value());
		QMouseEvent ev(QEvent::MouseMove, QPoint(x, y), e->globalPos(), e->button(), e->state());

		// set ruler pointers
		if (zoomFactor() >= 1.0 / 4.0) {
			m_pHRuler -> setValue(e -> pos().x() - xPaintOffset());
			m_pVRuler -> setValue(e -> pos().y() - yPaintOffset());
		}

		m_pTool -> mouseMove(&ev);
	}
}

/*
    canvasGotMouseReleaseEvent - just passes the signal on
    to the appropriate tool
*/
void KisView::canvasGotMouseReleaseEvent (QMouseEvent *e)
{
	if (m_pTool) {
		int x = e -> pos().x() - xPaintOffset() + (int)(zoomFactor() * m_pHorz -> value());
		int y = e -> pos().y() - yPaintOffset() + (int)(zoomFactor() * m_pVert -> value());
		QMouseEvent ev(QEvent::MouseButtonRelease, QPoint(x, y), e -> globalPos(), e -> button(), e -> state());

		m_pTool -> mouseRelease(&ev);
	}
}

/*
    canvasGotEnterEvent - just passes the signal on
    to the appropriate tool
*/
void KisView::canvasGotEnterEvent (QEvent *e)
{
	if (m_pTool) {
		QEvent ev(*e);
		m_pTool -> enterEvent(&ev);
	}
}

/*
    canvasGotLeaveEvent - just passes the signal on
    to the appropriate tool
*/
void KisView::canvasGotLeaveEvent (QEvent *e)
{
	if (m_pTool) {
		// clear artifacts from tools which paint on canvas
		// this does not affect the image or layer
		if (m_pTool -> shouldRepaint())
			m_pCanvas -> repaint();

		QEvent ev(*e);

		m_pTool -> leaveEvent(&ev);
	}
}

void KisView::canvasGotMouseWheelEvent(QWheelEvent *e)
{
	QApplication::sendEvent(m_pVert, e);
}

/*
    activateTool - make the selected tool the active tool and
    establish connections via the base kis_tool class
*/
void KisView::activateTool(KisTool* t)
{
	if (!t)
		return;

	// remove the selection outline, if any
	// prevent old tool from receiving events from canvas
	if (m_pTool) {
		m_pTool -> clearOld();
		m_pTool -> setChecked(false);
	}

	m_pTool = t;
	m_pTool -> setChecked(true);
	m_pTool -> setBrush(m_pBrush);
	m_pTool -> setPattern(m_pPattern);

	if (m_pCanvas)
		m_pCanvas -> setCursor(m_pTool -> cursor());
}

/*
    tool_properties invokes the optionsDialog() method for the
    current active tool.  There should be an options dialog for
    each tool, but these can also be consolidated into a master
    options dialog by reparenting the widgets for each tool to a
    tabbed properties dialog, each tool getting a tab  - later
*/
void KisView::tool_properties()
{
	Q_ASSERT(m_pTool);
	m_pTool -> optionsDialog();
}

/*---------------------------
    history action slots
----------------------------*/

void KisView::undo()
{
    kdDebug() << "UNDO called" << endl;
}

void KisView::redo()
{
    kdDebug() << "REDO called" << endl;
}

/*---------------------------------
    edit selection action slots
----------------------------------*/

/*
    copy - copy selection contents to global kapp->clipboard()
*/
void KisView::copy()
{
	// set local clip
	if (!m_doc -> setClipImage())
		kdDebug() << "m_doc->setClipImage() failed" << endl;

	// copy local clip to global clipboard
	if (m_doc -> getClipImage()) {
		kdDebug() << "got m_doc->getClipImage()" << endl;
		QImage cImage = *m_doc -> getClipImage();
		kapp -> clipboard() -> setImage(cImage);
        
		if (kapp -> clipboard() -> image().isNull())
			kdDebug() << "clip image is null" << endl;
		else
			kdDebug() << "clip image is NOT null" << endl;
	}
}

/*
    cut - move selection contents to global kapp->clipboard()
*/
void KisView::cut()
{
	copy();
	removeSelection();
}

/*
    same as cut but don't move selection contents to clipboard
*/
void KisView::removeSelection()
{
    // remove selection in place
    if(!m_doc->getSelection()->erase())
        kdDebug() << "m_doc->m_Selection.erase() failed" << endl;

    // clear old selection outline
    m_pTool->clearOld();

    /* refresh canvas */
    slotUpdateImage();
}


/*
    paste - from the global kapp->clipboard(). The image
    in the clipboard (if any) is copied to the past tool clip
    image so it can be used like a brush or stamp tool to paint
    with, or it can just be moved into place and pasted in.
*/
void KisView::paste()
{
	// get local clip from global clipboard
	if (m_doc -> getClipImage()) {
		m_paste -> setClip();
		activateTool(m_paste);
		slotUpdateImage();
	}
	else
		KMessageBox::sorry(0, i18n("Nothing to paste!"), "", false);
}

/*
    create a new layer from the selection, same size as the
    selection (in the case of a non-rectangular selection, find
    and use the bounding rectangle for selected pixels)
*/
void KisView::crop()
{
    if(!m_doc->hasSelection())
    {
        KMessageBox::sorry(NULL, i18n("No selection to crop!"), "", FALSE);
        return;
    }
    // copy contents of the current selection to a QImage
    if(!m_doc->setClipImage())
    {
        kdDebug() << "m_doc->setClipImage() failed" << endl;
        return;
    }
    // contents of current selection - make sure it's good
    if(!m_doc->getClipImage())
    {
        kdDebug() << "m_doc->getClipImage() failed" << endl;
        return;
    }

    QImage cImage = *m_doc->getClipImage();

    // add new layer same size as selection rectangle,
    // then paste cropped image into it at 0,0 offset
    // keep old image - user can remove it later if he wants
    // by removing its layer or may want to keep the original.

    KisImage* img = m_doc->current();
    if(!img) return;

    QRect layerRect(0, 0, cImage.width(), cImage.height());
    QString name = i18n("layer %1").arg(img->layerList().count());

    img->addLayer(layerRect, white, true, name);
    uint indx = img->layerList().count() - 1;
    img->setCurrentLayer(indx);
    img->setFrontLayer(indx);

    m_pLayerView->layerTable()->updateTable();
    m_pLayerView->layerTable()->updateAllCells();

    // copy the image into the layer - this should now
    // be handled by the framebuffer object, not the doc

    if(!m_doc->QtImageToLayer(&cImage, this))
    {
        kdDebug(0) << "crop: can't load image into layer." << endl;
    }
    else
    {
        slotUpdateImage();
        slotRefreshPainter();
    }

    /* remove the current clip image which now belongs to the
    previous layer - selection also is removed. To crop again,
    you must first make another selection in the current layer.*/

    m_doc->removeClipImage();
    m_doc->clearSelection();

}

/*
    selectAll - use the bounding rectangle of the layer itself
*/
void KisView::selectAll()
{
    KisImage *img = m_doc->current();
    if(!img) return;

    QRect imageR = img->getCurrentLayer()->imageExtents();
    m_doc->setSelection(imageR);
}

/*
    unSelectAll - clear the selection, if any
*/
void KisView::unSelectAll()
{
    m_doc->clearSelection();
}


/*--------------------------
       Zooming
---------------------------*/

void KisView::zoom(int _x, int _y, float zf)
{
    /* Avoid divide by zero errors by disallowing a zoom
    factor of zero, which is impossible anyway, as it would
    make the image infinitely small in size. */
    if (zf == 0) zf = 1;

    /* Set a reasonable lower limit for a zoom factor of 1/8.
    At this level a 1600x1600 image would be 200 x 200 in size.
    Extremely low zooms, like extremely high ones, can be very
    expensive in terms of processor cycles and degrade performace,
    although not nearly as much as extrmely high zooms.*/
    if (zf < 0.15) zf = 1.0/8.0;

    /* Set a reasonable upper limit for a zoom factor of 16. At this
    level each pixel in the layer is shown as a 16x16 rectangle.
    Zoom levels higher than this serve no useful purpose and are
    *VERY* expensive.  It's possible to accidentally set a very high
    zoom by continuing to click on the image with the zoom tool
    without this limit. */
    else if(zf > 16.0) zf = 16.0;

    setZoomFactor(zf);

    // clear everything
    QPainter p;
    p.begin(m_pCanvas);
    p.eraseRect(0, 0, width(), height());
    p.end();

    // adjust scaling of rulers to zoom factor
    if(zf > 3.0)
    {
        // 8 / 16 pixels per mark at 8.0 / 16.0 zoom factors
        m_pHRuler->setPixelPerMark((int)(zf * 1.0));
        m_pVRuler->setPixelPerMark((int)(zf * 1.0));
    }
    else
    {
        // to pixels per mark at zoom factor of 1.0
        m_pHRuler->setPixelPerMark((int)(zf * 10.0));
        m_pVRuler->setPixelPerMark((int)(zf * 10.0));
    }

    // Kruler - lacks sane builtin limits at tiny sizes
    // this causes hangups - avoid tiny rulers

    if(zf > 3.0)
    {
        m_pHRuler->setValuePerLittleMark(1);
        m_pVRuler->setValuePerLittleMark(1);
    }
    else
    {
        m_pHRuler->setValuePerLittleMark(10);
        m_pVRuler->setValuePerLittleMark(10);
    }

    // zoom factor of 1/4
    if(zf < 0.30)
    {
        m_pHRuler->setShowLittleMarks(false);
        m_pVRuler->setShowLittleMarks(false);
    }
    // zoom factor of 1/2 or greater
    else
    {
        m_pHRuler->setShowLittleMarks(true);
        m_pVRuler->setShowLittleMarks(true);
    }

    // zoom factor of 1/8 - lowest possible
    if(zf < 0.20)
    {
        m_pHRuler->setShowMediumMarks(false);
        m_pVRuler->setShowMediumMarks(false);
    }
    // zoom factor of 1/4 or greater
    else
    {
        m_pHRuler->setShowMediumMarks(true);
        m_pVRuler->setShowMediumMarks(true);
    }

    /* scroll to the point clicked on and update the canvas.
    Currently scrollTo() doesn't do anything but the zoomed view
    does have the same offset as the prior view so it
    approximately works */

    int x = static_cast<int> (_x * zf - docWidth() / 2);
    int y = static_cast<int> (_y * zf - docHeight() / 2);

    if (x < 0) x = 0;
    if (y < 0) y = 0;

    scrollTo(QPoint(x, y));

    m_pCanvas->update();

    /* at low zoom levels mark everything dirty and redraw the
    entire image to insure that the previous image is erased
    completely from border areas.  Otherwise screen artificats
    can be seen.*/

    if(zf < 1.0) slotUpdateImage();

}


void KisView::zoom_in(int x, int y)
{
    float zf = zoomFactor();
    zf *= 2;
    zoom(x, y, zf);
}


void KisView::zoom_out(int x, int y)
{
    float zf = zoomFactor();
    zf /= 2;
    zoom(x, y, zf);
}


void KisView::zoom_in()
{
    zoom_in(0, 0);
}


void KisView::zoom_out()
{
    zoom_out(0, 0);
}


/*
    dialog_gradient - invokes a GradientDialog which is
    now an options dialog.  Gradients can be used by many tools
    and are not a tool in themselves.
*/
void KisView::dialog_gradient()
{
    GradientDialog *pGradientDialog = new GradientDialog(m_pGradient);
    pGradientDialog->exec();

    if(pGradientDialog->result() == QDialog::Accepted)
    {
        /* set m_pGradient here and update gradientwidget in sidebar
        to show sample of gradient selected. Also update effect for
        the framebuffer's gradient, which is used in painting */

        int type = pGradientDialog->gradientTab()->gradientType();
        m_pGradient->setEffect(static_cast<KImageEffect::GradientType>(type));

        KisFrameBuffer *fb = m_doc->frameBuffer();
        fb->setGradientEffect(static_cast<KImageEffect::GradientType>(type));

        kdDebug() << "gradient type is " << type << endl;
    }
}


void KisView::dialog_colors()
{

}


void KisView::dialog_krayons()
{

}


void KisView::dialog_brushes()
{
    KFloatingDialog *f = static_cast<KFloatingDialog *>(m_pBrushChooser);

    if(m_dialog_brushes->isChecked())
        f->setDocked(true);
    else
        f->setDocked(false);
}


void KisView::dialog_patterns()
{
    if(m_dialog_patterns->isChecked())
        m_pSideBar->plug(m_pPatternChooser);
    else
        m_pSideBar->unplug(m_pPatternChooser);
}


void KisView::dialog_layers()
{
    if(m_dialog_layers->isChecked())
        m_pSideBar->plug(m_pLayerView);
    else
        m_pSideBar->unplug(m_pLayerView);

}

void KisView::dialog_channels()
{
    if(m_dialog_channels->isChecked())
        m_pSideBar->plug(m_pChannelView);
    else
        m_pSideBar->unplug(m_pChannelView);
}


/*-------------------------------
    layer action slots
--------------------------------*/

/*
    Properties dialog for the current layer.
    Only for changing name and opacity so far.
*/
void KisView::layer_properties()
{
    KisImage * img = m_doc->current();
    if (!img)  return;

    KisLayer *lay = img->getCurrentLayer();
    if (!lay)  return;

    if(LayerPropertyDialog::editProperties(*(lay)))
    {
        QRect updateRect = lay->imageExtents();
        m_pLayerView->layerTable()->updateAllCells();
        img->markDirty(updateRect);
    }
}

/*
    insert new layer into the current image - using "new layer" dialog
    for layer size (should also have fields for name and opacity).
    This new layer will also be made uppermost so it is visble
*/
void KisView::insert_layer()
{
    KisImage* img = m_doc->current();
    if(!img) return;

    NewLayerDialog *pNewLayerDialog = new NewLayerDialog();
    pNewLayerDialog->exec();

    if(!pNewLayerDialog->result() == QDialog::Accepted)
        return;

    QRect layerRect(0, 0,
        pNewLayerDialog->width(), pNewLayerDialog->height());

    QString name = i18n("layer %1").arg(img->layerList().count());

    // new layers are currently appended - perhaps they should
    // be prepended so more recent ones are on top in the
    // list and the background layer is at the bottom

    img->addLayer(layerRect, white, true, name);

    uint indx = img->layerList().count() - 1;
    img->setCurrentLayer(indx);
    img->setFrontLayer(indx);
    m_pLayerView->layerTable()->selectLayer(indx);

    // update layerview table so change show up there
    m_pLayerView->layerTable()->updateTable();
    m_pLayerView->layerTable()->updateAllCells();

    slotUpdateImage();
    slotRefreshPainter();

    m_doc->setModified(true);
}

/*
    remove current layer - to remove other layers, a user must
    access the layers tableview in a dialog or sidebar widget
*/
void KisView::remove_layer()
{
    m_pLayerView->layerTable()->slotRemoveLayer();
    slotUpdateImage();
    slotRefreshPainter();

    m_doc->setModified(true);
}

/*
    hide/show the current layer - to hide other layers, a user must
    access the layers tableview in a dialog or sidebar widget
*/
void KisView::hide_layer()
{
    KisImage * img = m_doc->current();
    if (!img) return;

    uint indx = img->getCurrentLayerIndex();
    m_pLayerView->layerTable()->slotInverseVisibility(indx);

    m_pLayerView->layerTable()->updateTable();
    m_pLayerView->layerTable()->updateAllCells();

    m_doc->setModified(true);
}

/*
    link/unlink the current layer - to link other layers, a user must
    access the layers tableview in a dialog or sidebar widget
*/
void KisView::link_layer()
{
    KisImage * img = m_doc->current();
    if (!img) return;

    uint indx = img->getCurrentLayerIndex();
    m_pLayerView->layerTable()->slotInverseLinking(indx);

    m_pLayerView->layerTable()->updateTable();
    m_pLayerView->layerTable()->updateAllCells();

    m_doc->setModified(true);
}

/*
    make the next layer in the layers list the active one and
    bring it to the front of the view
*/
void KisView::next_layer()
{
    KisImage * img = m_doc->current();
    if (!img) return;

    uint indx = img->getCurrentLayerIndex();
    if(indx < img->layerList().count() - 1)
    {
        // make the next layer the current one, select it,
        // and make sure it's visible
        ++indx;
        img->setCurrentLayer(indx);
        m_pLayerView->layerTable()->selectLayer(indx);
        img->layerList().at(indx)->setVisible(true);

        // hide all layers on top of this one so this
        // one is clearly visible and can be painted on!

        while(++indx <= img->layerList().count() - 1)
        {
            img->layerList().at(indx)->setVisible(false);
        }

        img->markDirty(img->getCurrentLayer()->layerExtents());
        m_pLayerView->layerTable()->updateTable();
        m_pLayerView->layerTable()->updateAllCells();
        slotRefreshPainter();

        m_doc->setModified(true);
    }
}


/*
    make the previous layer in the layers list the active one and
    bring it to the front of the view
*/
void KisView::previous_layer()
{
    KisImage * img = m_doc->current();
    if (!img)  return;

    uint indx = img->getCurrentLayerIndex();
    if(indx > 0)
    {
        // make the previous layer the current one, select it,
        // and make sure it's visible
        --indx;
        img->setCurrentLayer(indx);
        m_pLayerView->layerTable()->selectLayer(indx);
        img->layerList().at(indx)->setVisible(true);

        // hide all layers beyond this one so this
        // one is clearly visible and can be painted on!
        while(++indx <= img->layerList().count() - 1)
        {
            img->layerList().at(indx)->setVisible(false);
        }

        img->markDirty(img->getCurrentLayer()->layerExtents());

        m_pLayerView->layerTable()->updateTable();
        m_pLayerView->layerTable()->updateAllCells();
        slotRefreshPainter();

        m_doc->setModified(true);
   }
}


void KisView::import_image()
{
	if (!insert_layer_image(true))
		m_doc -> setModified(true);
}

void KisView::export_image()
{
    save_layer_image(true);
}

void KisView::insert_image_as_layer()
{
    insert_layer_image(false);

    m_doc->setModified(true);
}

void KisView::save_layer_as_image()
{
    save_layer_image(false);
}

void KisView::slotEmbeddImage(const QString &filename)
{
    insert_layer_image(true, filename);
}

/*
    insert_layer_image - Insert a standard image like png or jpg
    into the current layer.  This is the same as "import" in other
    koffice apps, but since everything is organized by layers,
    one must create a new layer and/or a new image to contain it -
    not necessarily a new doc.

    Note: After opening the file and getting a url which is not empty,
    the remainder of this code belongs in kis_doc.cc, not in the view,
    because it can also be used for importing an image file during
    doc init.  Eventually it needs to go into koffice/filters.
*/
int KisView::insert_layer_image(bool newImage, const QString &filename)
{
	KURL url(filename);
	QImage fileImage;

	if(filename.isEmpty())
		url = KFileDialog::getOpenURL(QString::null, KisUtil::readFilters(), 0, i18n("Image file for layer"));

	if (url.isEmpty())
		return -1;

	if (!fileImage.load(url.path())) {
		KMimeType::Ptr mt = KMimeType::findByURL(url, 0, true);

		kdDebug() << "Can't create QImage from file" << endl;
		KMessageBox::error(this, i18n("Could not import file of type\n%1").arg(mt -> name()), i18n("Missing import filter"));
		return -1;
	}

	if (fileImage.depth() == 1) {
		kdDebug() << "No 1 bit images. " << "Where's your 2 bits worth?" << endl;
		return -1;
	}

	/* convert indexed images, all gifs and some pngs of 8 bits
	   or less, to 16 bit by creating a QPixmap from the file and
	   blitting it into a 16 bit RGBA pixmap - you can blit from a
	   lesser depth to a greater but not the other way around. This
	   is the only way, and since the really huge images are 16
	   bits or greater in depth (jpg and tiff), it's not too slow
	   for most gifs, indexed pings, etc., which are usually much
	   smaller. One bit images are taboo because of bigendian
	   problems and are rejected */

	if (fileImage.depth() < 16) {
		QPixmap filePixmap(url.path());
		QPixmap buffer(filePixmap.width(), filePixmap.height());

		if (!filePixmap.isNull() && !buffer.isNull())
			bitBlt(&buffer, 0, 0, &filePixmap, 0, 0, filePixmap.width(), filePixmap.height());

		fileImage = buffer;

		if (fileImage.depth() < 16) {
			KMessageBox::error(this, i18n("Image cannot be converted to 16 bit."), i18n("Error loading file"));
			kdDebug() << "newImage can't be converted to 16 bit" << endl;
			return -1;
		}
	}

	// establish a rectangle the same size as the QImage loaded
	// from file. This will be used to set the size of the new
	// KisLayer for the picture and/or a new KisImage
	if (newImage)
		appendToDocImgList(fileImage, url);
	else
		addHasNewLayer(fileImage, url);

	// copy the image into the layer regardless of whether
	// a new image or just a new layer was created for it above.
	if (!m_doc -> QtImageToLayer(&fileImage, this)) {
		kdDebug(0) << "inset_layer_image: " << "Can't load image into layer." << endl;

		// remove empty image
		if(newImage)
			remove_current_image_tab();
	}
	else {
		slotUpdateImage();
		slotRefreshPainter();
	}

	return 0;
}


/*
    save_layer_image - export the current image after merging
    layers or just export the current layer -  like the above
    method, the body of this, after a valid url is obtained,
    belongs in the doc, not the view and eventually needs to be
    moved from the doc to koffice/filters.
*/
void KisView::save_layer_image(bool mergeLayers)
{
    KURL url = KFileDialog::getSaveURL(QString::null,
        KisUtil::readFilters(), 0, i18n("Image file for layer"));

    if(!url.isEmpty())
    {
        if(mergeLayers)
        {
            /* merge should not always remove layers -
            merged into another but should have an option
            for keeping old layers and merging into a new
            one created for that purpose with a Yes/No dialog
            to confirm, at least. */
            merge_all_layers();
        }

        //  save as standard image file (jpg, png, xpm, ppm,
        //  bmp, tiff, but NO gif due to patent restrictions)
        if(!m_doc->saveAsQtImage(url.path(), mergeLayers))
            kdDebug(0) << "Can't save doc as image" << endl;
    }
}


void KisView::layer_scale_smooth()
{
    layerScale(true);
}


void KisView::layer_scale_rough()
{
    layerScale(false);
}


void KisView::layerScale(bool smooth)
{
    KisImage * img = m_doc->current();
    if (!img)  return;

    KisLayer *lay = img->getCurrentLayer();
    if (!lay)  return;

    KisFrameBuffer *fb = m_doc->frameBuffer();
    if (!fb)  return;

    NewLayerDialog *pNewLayerDialog = new NewLayerDialog();
    pNewLayerDialog->exec();
    if(!pNewLayerDialog->result() == QDialog::Accepted)
        return;

    QRect srcR(lay->imageExtents());

    // only get the part of the layer which is inside the
    // image boundaries - layer can be bigger or can overlap
    srcR = srcR.intersect(img->imageExtents());

    bool ok;

    if(smooth)
        ok = fb->scaleSmooth(srcR,
            pNewLayerDialog->width(), pNewLayerDialog->height());
    else
        ok = fb->scaleRough(srcR,
            pNewLayerDialog->width(), pNewLayerDialog->height());

    if(!ok)
    {
        kdDebug() << "layer_scale() failed" << endl;
    }
    else
    {
        // bring new scaled layer to front
        uint indx = img->layerList().count() - 1;
        img->setCurrentLayer(indx);
        img->markDirty(img->getCurrentLayer()->layerExtents());
        m_pLayerView->layerTable()->selectLayer(indx);

        m_pLayerView->layerTable()->updateTable();
        m_pLayerView->layerTable()->updateAllCells();
        slotRefreshPainter();

        m_doc->setModified(true);
    }
}


void KisView::layer_rotate180()
{

}

void KisView::layer_rotateleft90()
{

}

void KisView::layer_rotateright90()
{

}

void KisView::layer_rotate_custom()
{

}

void KisView::layer_mirrorX()
{

}


void KisView::layer_mirrorY()
{

}

/*--------------------------
    image action slots
--------------------------*/

void KisView::add_new_image_tab()
{
    if(m_doc->slotNewImage())
    {
        slotUpdateImage();
        slotRefreshPainter();

        m_doc->setModified(true);
    }
}


void KisView::remove_current_image_tab()
{
    if (m_doc->current())
    {
        m_doc->removeImage(m_doc->current());
        slotUpdateImage();
        slotRefreshPainter();

        m_doc->setModified(true);
    }
}


void KisView::merge_all_layers()
{
    if (m_doc->current())
    {
        m_doc->current()->mergeAllLayers();
        slotUpdateImage();
        slotRefreshPainter();

        m_doc->setModified(true);
    }
}


void KisView::merge_visible_layers()
{
    if (m_doc->current())
    {
        m_doc->current()->mergeVisibleLayers();
        slotUpdateImage();
        slotRefreshPainter();

        m_doc->setModified(true);
    }
}


void KisView::merge_linked_layers()
{
    if (m_doc->current())
    {
        m_doc->current()->mergeLinkedLayers();
        slotUpdateImage();
        slotRefreshPainter();

        m_doc->setModified(true);
    }
}


/*------------------------------------
  Preferences and configuration slots
---------------------------------------*/
void KisView::showMenubar()
{
}


void KisView::showToolbar()
{
}


void KisView::showStatusbar()
{
}


void KisView::showSidebar()
{
    if (m_side_bar->isChecked())
    {
        m_pSideBar->show();
    }
    else
    {
        m_pSideBar->hide();
    }

    // force resize to show scrollbars, etc
    resizeEvent(0L);
}


void KisView::floatSidebar()
{
    KFloatingDialog *f = (KFloatingDialog *)m_pSideBar;
    f->setDocked(!m_float_side_bar->isChecked());

    // force resize to show scrollbars, etc
    resizeEvent(0L);
}

/*
    leftSidebar -this does nothing except force a resize to show scrollbars
    Repositioning of sidebar is handled by resizeEvent() entirley
*/
void KisView::leftSidebar()
{
    resizeEvent(0L);
}

/*
    saveOptions - here we need to write entries to a congig
    file.
*/
void KisView::saveOptions()
{
}


/*
    preferences - the main Krayon preferences dialog - modeled
    after the konqueror prefs dialog - quite nice compound dialog
*/
void KisView::preferences()
{
    PreferencesDialog::editPreferences();
}


/*
    docWidth - simply returns the width of the document which is
    exactly the same as the width of the current image
*/
int KisView::docWidth()
{
    if (m_doc->current()) return m_doc->current()->width();
    else return 0;
}


/*
    docHeight - simply returns the height of the document which is
    exactly the same as the height of the current image
*/
int KisView::docHeight()
{
    if (m_doc->current()) return m_doc->current()->height();
    else return 0;
}


void KisView::slotSetPaintOffset()
{
    // dialog to set x and y paint offsets needed
    if(xPaintOffset() == 0)
    {
        m_xPaintOffset = 20;
        m_yPaintOffset = 20;
    }
    else
    {
        m_xPaintOffset = 0;
        m_yPaintOffset = 0;
    }
}


int KisView::xPaintOffset()
{
	return m_xPaintOffset;
}


int KisView::yPaintOffset()
{
	return m_yPaintOffset;
}


void KisView::scrollTo(QPoint pt)
{
    kdDebug() << "scroll to " << pt.x() << "," << pt.y() << endl;

    // this needs to update the scrollbar values and
    // let resizeEvent() handle the repositioning
    // with showScollBars()
}


float KisView::zoomFactor() const
{
	return m_zoomFactor;
}


void KisView::setZoomFactor(float zf)
{
	m_zoomFactor = zf;
}

void KisView::slotSetBrush(KisBrush* b)
{
	Q_ASSERT(b);
	m_pBrush = b;

	if (m_pTool) {
		m_pTool -> setBrush(b);
		m_pTool -> setCursor();
	}
}

void KisView::slotSetKrayon(KisKrayon* k)
{
	m_pKrayon = k;
	m_pSideBar -> slotSetKrayon(*k);
}

void KisView::slotSetPattern(KisPattern* p)
{
	// set current pattern for this view
	m_pPattern = p;

	// set pattern for other things that use patterns
	Q_ASSERT(m_pSideBar);
	Q_ASSERT(m_doc);
	m_pSideBar -> slotSetPattern(*p);
	m_doc -> frameBuffer() -> setPattern(p);
}


/*
    The new foreground color should show up in the color selector
    via signal sent to colorselector
*/
void KisView::slotSetFGColor(const KisColor& c)
{
	m_fg = c;
	emit fgColorChanged(c);
}

/*
    The new background color should show up in the color selector
    via signal sent to colorselector
*/
void KisView::slotSetBGColor(const KisColor& c)
{
	m_bg = c;
	emit bgColorChanged(c);
}

void KisView::setupPrinter(KPrinter &printer)
{
    printer.setPageSelection(KPrinter::ApplicationSide);

    int count = 0;
    QStringList imageList = m_doc->images();
    for (QStringList::Iterator it = imageList.begin(); it != imageList.end(); ++it) {
        if (*it == m_doc->current()->name())
            break;
        ++count;
    }

    printer.setCurrentPage(1 + count);
    printer.setMinMax(1, m_doc->images().count());
    printer.setPageSize(KPrinter::A4);
    printer.setOrientation(KPrinter::Portrait);
}

void KisView::print(KPrinter &printer)
{
    printer.setFullPage(true);
    QPainter paint;
    paint.begin(&printer);
    paint.setClipping(false);
    QValueList<int> imageList;
    int from = printer.fromPage();
    int to = printer.toPage();
    if(!from && !to)
    {
        from = printer.minPage();
        to = printer.maxPage();
    }
    for (int i = from; i <= to; i++)
        imageList.append(i);
    QString tmp_currentImageName = m_doc->currentImage();
    QValueList<int>::Iterator it = imageList.begin();
    for (; it != imageList.end(); ++it)
    {
        int imageNumber = *it - 1;
        if (it != imageList.begin())
            printer.newPage();

        m_doc->setImage(*m_doc->images().at(imageNumber));
        m_doc->paintContent(paint, m_doc->getImageRect());
    }
    paint.end ();
    m_doc->setImage(tmp_currentImageName);
}

void KisView::appendToDocImgList(QImage& loadedImg, KURL& u)
{
	QRect layerRect(0, 0, loadedImg.width(), loadedImg.height());
	QString layerName(u.fileName());
	KisImage *newimg = m_doc -> newImage(layerName, layerRect.width(), layerRect.height());

	// add background for layer - should this always be white?
	bgMode bg = bm_White;

	if (bg == bm_White)
		newimg -> addLayer(QRect(0, 0, newimg -> width(), newimg -> height()), KisColor::white(), false, i18n("background"));
	else if (bg == bm_Transparent)
		newimg -> addLayer(QRect(0, 0, newimg -> width(), newimg -> height()), KisColor::white(), true, i18n("background"));
	else if (bg == bm_ForegroundColor)
		newimg -> addLayer(QRect(0, 0, newimg -> width(), newimg -> height()), KisColor::white(), false, i18n("background"));
	else if (bg == bm_BackgroundColor)
		newimg -> addLayer(QRect(0, 0, newimg -> width(), newimg -> height()), KisColor::white(), false, i18n("background"));

	newimg -> markDirty(QRect(0, 0, newimg -> width(), newimg -> height()));
	m_doc -> setCurrentImage(newimg);
}

void KisView::addHasNewLayer(QImage& loadedImg, KURL& u)
{
	KisImage *img = m_doc -> current();
	QRect layerRect(0, 0, loadedImg.width(), loadedImg.height());
	QString layerName(u.fileName());
	uint indx;

	img -> addLayer(layerRect, white, false, layerName);
	indx = img -> layerList().count() - 1;
	img -> setCurrentLayer(indx);
	img -> setFrontLayer(indx);
	m_pLayerView -> layerTable() -> selectLayer(indx);
	m_pLayerView -> layerTable() -> updateTable();
	m_pLayerView -> layerTable() -> updateAllCells();
}

void KisView::setupTools()
{
	if (m_tools.size())
		return;

	m_tools = m_doc -> getTools();

	if (!m_tools.size()) {
		m_tools = ::toolFactory(m_pCanvas, m_pBrush, m_pPattern, m_doc);
		m_paste = new PasteTool(m_doc, m_pCanvas);
		m_tools.push_back(m_paste);
	}

	for (ktvector_size_type i = 0; i < m_tools.size(); i++) {
		KisTool *p = m_tools[i];

		Q_ASSERT(p);
		p -> setupAction(actionCollection());
	}

	if (m_doc -> viewCount() < 1)
		m_doc -> setTools(m_tools);

	m_tools[0] -> toolSelect();
	activateTool(m_tools[0]);
}

#include "kis_view.moc"

