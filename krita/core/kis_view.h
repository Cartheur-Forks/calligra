/*
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *                1999 Michael Koch    <koch@kde.org>
 *                1999 Carsten Pfeiffer <pfeiffer@kde.org>
 *                2002 Patrick Julien <freak@codepimps.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef KIS_VIEW_H_
#define KIS_VIEW_H_

#include <qdatetime.h>
#include <qpixmap.h>

#include <list>
#include <map>

#include <qcolor.h>
#include <koView.h>
#include <kdebug.h>

#include "kis_canvas_controller.h"
#include "kis_canvas_subject.h"
#include "kis_global.h"
#include "kis_tool_controller.h"
#include "kis_tool_registry.h"
#include "kis_types.h"
#include "kis_scale_visitor.h"
#include "kis_profile.h"

class QButton;
class QLabel;
class QPaintEvent;
class QScrollBar;
class QWidget;

class DCOPObject;
class KAction;
class KPrinter;
class KToggleAction;

class KoIconItem;
class KoTabBar;
class KoToolDockManager;
class KoTabbedToolDock;

class KisCanvasObserver;
class KisRuler;
class KisBrush;
class KisBuilderMonitor;
class KisCanvas;
class KisChannelView;
class KisLabelProgress;
class KisDoc;
class KisGradient;
class KisPattern;
class KisResource;
class KisResourceMediator;
class KisAutobrush;
class KisTextBrush;
class KisAutogradient;
class ControlFrame;
class KisUndoAdapter;
class KisRect;
class KisPoint;
class KisLayerBox;
class KisButtonPressEvent;
class KisButtonReleaseEvent;
class KisMoveEvent;
class KisHSVWidget;
class KisRGBWidget;
class KisGrayWidget;
class KisFilterRegistry;
class KisSelectionManager;

class KisView
	: public KoView,
	  private KisCanvasSubject,
	  private KisCanvasControllerInterface,
	  private KisToolControllerInterface
{

	Q_OBJECT

	typedef KoView super;
	typedef std::list<KisCanvasObserver*> vKisCanvasObserver;
	typedef vKisCanvasObserver::iterator vKisCanvasObserver_it;
	typedef vKisCanvasObserver::const_iterator vKisCanvasObserver_cit;

	typedef std::map<enumInputDevice, KisTool *> InputDeviceToolMap;
	typedef std::map<enumInputDevice, vKisTool> InputDeviceToolSetMap;


public:
	KisView(KisDoc *doc, KisUndoAdapter *adapter, QWidget *parent = 0, const char *name = 0);
	virtual ~KisView();


public: // KoView implementation
	virtual bool eventFilter(QObject *o, QEvent *e);
 	virtual int canvasXOffset() const;
 	virtual int canvasYOffset() const;

	virtual DCOPObject* dcopObject();

	virtual void print(KPrinter &printer);
	virtual void setupPrinter(KPrinter &printer);

	virtual void updateReadWrite(bool readwrite);
	virtual void guiActivateEvent(KParts::GUIActivateEvent *event);

	Q_INT32 docWidth() const;
	Q_INT32 docHeight() const;
	Q_INT32 importImage(bool createLayer, bool modal = false, const KURL& url = KURL());

	KisFilterRegistrySP filterRegistry() const;
	KisToolRegistry * toolRegistry() const { return m_toolRegistry; }

	KoToolDockManager * toolDockManager() { return m_toolDockManager; }

	void updateStatusBarSelectionLabel();

	/**
	 * Reset the monitor profile to the new settings.
	 */
	void resetMonitorProfile();

public: // Plugin access API. XXX: This needs redesign.

	virtual KisImageSP currentImg() const;

	virtual void updateCanvas();
	virtual void updateCanvas(Q_INT32 x, Q_INT32 y, Q_INT32 w, Q_INT32 h);
	virtual void updateCanvas(const QRect& rc);
	virtual void updateCanvas(const KisRect& rc);

	void layersUpdated();

	KisDoc * getDocument() { return m_doc; }

	KisSelectionManager * selectionManager() { return m_selectionManager; }

signals:
	void bgColorChanged(const QColor& c);
	void fgColorChanged(const QColor& c);
	void cursorPosition(Q_INT32 xpos, Q_INT32 ypos);
	void cursorEnter();
	void cursorLeave();


public slots:
	void dialog_gradient();
	void slotSetFGColor(const QColor& c);
	void slotSetBGColor(const QColor& c);

	void next_layer();
	void previous_layer();

	// image action slots
	// XXX: Rename to make all names consistent with slotDoX() pattern
	void slotImportImage();
	void export_image();
	void slotImageProperties();
	void imgResizeToActiveLayer();
	void add_new_image_tab();
	void remove_current_image_tab();
	void resizeCurrentImage(Q_INT32 w, Q_INT32 h);
	void scaleCurrentImage(double sx, double sy, enumFilterType ftype = MITCHELL_FILTER);
        void rotateCurrentImage(double angle);
        void shearCurrentImage(double angleX, double angleY);

	// Layer action slots
	void rotateLayer180();
	void rotateLayerLeft90();
	void rotateLayerRight90();
	void rotateLayerCustom();
	void mirrorLayerX();
	void mirrorLayerY();
	void resizeLayer(Q_INT32 w, Q_INT32 h);
	void scaleLayer(double sx, double sy, enumFilterType ftype = MITCHELL_FILTER);
        void rotateLayer(double angle);
        void shearLayer(double angleX, double angleY);
        void rainDropsFilter(Q_UINT32 dropSize, Q_UINT32 number, Q_UINT32 fishEyes);
        void oilPaintFilter(Q_UINT32 brushSize, Q_UINT32 smooth);
        
	// settings action slots
	void preferences();

protected:

	virtual void resizeEvent(QResizeEvent*);

private:
	// Implement KisCanvasSubject
	virtual void attach(KisCanvasObserver *observer);
	virtual void detach(KisCanvasObserver *observer);
	virtual void notify();
	virtual QString currentImgName() const;
	virtual QColor bgColor() const;
	virtual void setBGColor(const QColor& c);
	virtual QColor fgColor() const;
	virtual void setFGColor(const QColor& c);
	virtual KisBrush *currentBrush() const;
	virtual KisPattern *currentPattern() const;
	virtual KisGradient *currentGradient() const;
	virtual double zoomFactor() const;
	virtual KisUndoAdapter *undoAdapter() const;
	virtual KisCanvasControllerInterface *canvasController() const;
	virtual KisToolControllerInterface *toolController() const;
	virtual KoDocument *document() const;
	virtual KisProgressDisplayInterface *progressDisplay() const;
	virtual KisFilterSP filterGet(const QString& name);
	virtual QStringList filterList();

private:
	// Implement KisCanvasControllerInterface
	virtual QWidget *canvas() const;
	virtual Q_INT32 horzValue() const;
	virtual Q_INT32 vertValue() const;
	virtual void scrollTo(Q_INT32 x, Q_INT32 y);
	virtual void zoomIn();
	virtual void zoomIn(Q_INT32 x, Q_INT32 y);
	virtual void zoomOut();
	virtual void zoomOut(Q_INT32 x, Q_INT32 y);
	virtual void zoomTo(Q_INT32 x, Q_INT32 y, Q_INT32 w, Q_INT32 h);
	virtual void zoomTo(const QRect& r);
	virtual void zoomTo(const KisRect& r);
	virtual QPoint viewToWindow(const QPoint& pt);
	virtual KisPoint viewToWindow(const KisPoint& pt);
	virtual QRect viewToWindow(const QRect& rc);
	virtual KisRect viewToWindow(const KisRect& rc);
	virtual void viewToWindow(Q_INT32 *x, Q_INT32 *y);
	virtual QPoint windowToView(const QPoint& pt);
	virtual KisPoint windowToView(const KisPoint& pt);
	virtual QRect windowToView(const QRect& rc);
	virtual KisRect windowToView(const KisRect& rc);
	virtual void windowToView(Q_INT32 *x, Q_INT32 *y);

private:
	// Implement KisToolControllerInterface
	virtual void setCurrentTool(KisTool *tool);
	virtual KisTool *currentTool() const;

private:


	void clearCanvas(const QRect& rc);
	void connectCurrentImg() const;
	void disconnectCurrentImg() const;
	void eraseGuides();
	void paintGuides();
	void updateGuides();
	void imgUpdateGUI();

	void layerUpdateGUI(bool enable);
	void paintView(const KisRect& rc);

	/** 
	 * Get the profile that this view uses to display itself on 
	 * he monitor.
	 */
	KisProfileSP monitorProfile();

	bool selectColor(QColor& result);
	void selectImage(KisImageSP img);

	void setupActions();
	void setupCanvas();
	void setupRulers();
	void setupScrollBars();
	void setupDockers();
	void setupTabBar();
	void setupStatusBar();
	void setupTools();

        void updateStatusBarZoomLabel();
	void updateStatusBarProfileLabel();

	void zoomUpdateGUI(Q_INT32 x, Q_INT32 y, double zf);

	void setInputDevice(enumInputDevice inputDevice);
	enumInputDevice currentInputDevice() const;

	KisTool *findTool(QString toolName, enumInputDevice inputDevice = INPUT_DEVICE_UNKNOWN) const;

private slots:

	void duplicateCurrentImg();
	void popupTabBarMenu( const QPoint& );
	void moveImage( unsigned, unsigned );
	void slotRename();

	void canvasGotMoveEvent(KisMoveEvent *e);
	void canvasGotButtonPressEvent(KisButtonPressEvent *e);
	void canvasGotButtonReleaseEvent(KisButtonReleaseEvent *e);
	void canvasGotPaintEvent(QPaintEvent *e);
	void canvasGotEnterEvent(QEvent *e);
	void canvasGotLeaveEvent(QEvent *e);
	void canvasGotMouseWheelEvent(QWheelEvent *e);
	void canvasRefresh();
	void canvasGotKeyPressEvent(QKeyEvent*);
	void canvasGotKeyReleaseEvent(QKeyEvent*);
	void canvasGotDragEnterEvent(QDragEnterEvent*);
	void canvasGotDropEvent(QDropEvent*);

	void docImageListUpdate();
	void layerToggleVisible();
	void layerSelected(int n);
	void layerToggleLinked();
	void layerProperties();

	void layerResizeToImage();
	void layerToImage();
	void layerAdd();
	void layerRemove();
	void layerDuplicate();
	void layerAddMask(int n);
	void layerRmMask(int n);
	void layerRaise();
	void layerLower();
	void layerFront();
	void layerBack();
	void layerLevel(int n);

	void layersUpdated(KisImageSP img);

	QPoint mapToScreen(const QPoint& pt);
	void flattenImage();
	void mergeVisibleLayers();
	void mergeLayer();
	void mergeLinkedLayers();
	void nBuilders(Q_INT32 size);
	void saveLayerAsImage();
	void currentImageUpdated(KisImageSP img);
	void selectFGColor();
	void selectBGColor();
	void reverseFGAndBGColors();
	void reset();
	void selectImage(const QString&);
	void brushActivated(KisResource *brush);
	void patternActivated(KisResource *pattern);
	void gradientActivated(KisResource *gradient);
	void scrollH(int value);
	void scrollV(int value);
	void slotEmbedImage(const QString& filename);
	void slotInsertImageAsLayer();
	void imgUpdated(KisImageSP img, const QRect& rc);
	void imgUpdated(KisImageSP img);
	void profileChanged(KisProfileSP profile);
	void slotZoomIn();
	void slotZoomOut();
	void slotImageSizeChanged(KisImageSP img, Q_INT32 w, Q_INT32 h);
	void viewColorDocker(bool v = true);
	void viewControlDocker(bool v = true);
	void viewLayerChannelDocker(bool v = true);
	void viewShapesDocker(bool v = true);
	void viewFillsDocker(bool v = true);
	void slotUpdateFullScreen(bool toggle);

	void updateTabBar();
	void showRuler();


private:
	KisDoc *m_doc;
	KisCanvas *m_canvas;

	KisSelectionManager * m_selectionManager;

        // Fringe benefits
	KoTabBar *m_tabBar;
	QButton *m_tabFirst;
	QButton *m_tabLeft;
	QButton *m_tabRight;
	QButton *m_tabLast;

	KisRuler *m_hRuler;
	KisRuler *m_vRuler;

        // Actions
	KAction *m_imgDup;
	KAction *m_imgExport;
	KAction *m_imgImport;
	KAction *m_imgFlatten;
	KAction *m_imgMergeLinked;
	KAction *m_imgMergeVisible;
	KAction *m_imgMergeLayer;
	KAction *m_imgRename;
	KAction *m_imgResizeToLayer;
	KAction *m_imgRm;
	KAction *m_imgScan;
	KAction *m_layerAdd;
	KAction *m_layerBottom;
	KAction *m_layerDup;
	KAction *m_layerHide;
	KAction *m_layerLink;
	KAction *m_layerLower;
	KAction *m_layerProperties;
	KAction *m_layerRaise;
	KAction *m_layerResizeToImage;
	KAction *m_layerRm;
	KAction *m_layerSaveAs;
	KAction *m_layerToImage;
	KAction *m_layerTop;
//	KAction *m_layerTransform;
	KAction *m_zoomIn;
	KAction *m_zoomOut;
	KAction *m_fullScreen;
	KAction *m_imgProperties;
	KToggleAction *m_RulerAction;

	DCOPObject *m_dcop;

        // Widgets
	QScrollBar *m_hScroll; // XXX: the sizing of the scrollthumbs
	QScrollBar *m_vScroll; // is not right yet.
	int m_scrollX;
	int m_scrollY;

	KisHSVWidget *m_hsvwidget;
	KisRGBWidget *m_rgbwidget;
	KisGrayWidget *m_graywidget;

        // Dockers
	KoTabbedToolDock *m_layerchanneldocker;
	KoTabbedToolDock *m_shapesdocker;
	KoTabbedToolDock *m_fillsdocker;
	KoTabbedToolDock *m_toolcontroldocker;
	KoTabbedToolDock *m_colordocker;

	// Dialogs
	QWidget *m_paletteChooser;
	QWidget *m_gradientChooser;
	QWidget *m_imageChooser;

	ControlFrame *m_controlWidget;
	KisChannelView *m_channelView;
	QWidget *m_pathView;
	KisBuilderMonitor *m_imgBuilderMgr;
	KisLabelProgress *m_progress;

	KisResourceMediator *m_brushMediator;
	KisResourceMediator *m_patternMediator;
	KisResourceMediator *m_gradientMediator;
	KisAutobrush *m_autobrush;
	KisTextBrush *m_textBrush;
	KisAutogradient* m_autogradient;

	// Current colours, brushes, patterns etc.
	Q_INT32 m_xoff;
	Q_INT32 m_yoff;
	QColor m_fg;
	QColor m_bg;
	KisBrush *m_brush;
	KisPattern *m_pattern;
	KisGradient *m_gradient;
	KisLayerBox *m_layerBox;
	KisGuideSP m_currentGuide;
	QPoint m_lastGuidePoint;
	KisUndoAdapter *m_adapter;
	vKisCanvasObserver m_observers;

	QLabel *m_statusBarZoomLabel;
	QLabel *m_statusBarSelectionLabel;
	QLabel *m_statusBarProfileLabel;

	enumInputDevice m_inputDevice;
	InputDeviceToolMap m_inputDeviceToolMap;
	InputDeviceToolSetMap m_inputDeviceToolSetMap;

	QTime m_tabletEventTimer;
	QTabletEvent::TabletDevice m_lastTabletEventDevice;
	KisFilterRegistrySP m_filterRegistry;
	QPixmap m_canvasPixmap;

	KisToolRegistry * m_toolRegistry;
	KoToolDockManager * m_toolDockManager;

	bool m_dockersSetup;

	// Monitorprofile for this view
	KisProfileSP m_monitorProfile;

private:
	mutable KisImageSP m_current;
};

#endif // KIS_VIEW_H_

