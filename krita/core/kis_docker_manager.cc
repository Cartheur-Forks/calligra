/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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

#include <qobject.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qdockwindow.h>
#include <qwidget.h>
#include <qtoolbox.h>

#include <kdebug.h>
#include <kaction.h>
#include <klocale.h>
#include <kstdaction.h>

#include <kotooldockmanager.h>
#include <kotooldockbase.h>
#include <koMainWindow.h>

#include "kis_docker_manager.h"
#include "kis_resource_mediator.h"
#include "kis_resourceserver.h"
#include "kis_dockframedocker.h"
#include "kis_basedocker.h"
#include "kis_id.h"
#include "kis_view.h"
#include "kis_config.h"
#include "kis_global.h"
#include "kis_tool.h"

// Widgets
#include "kis_autobrush.h"
#include "kis_text_brush.h"
#include "kis_autogradient.h"
#include "kis_hsv_widget.h"
#include "kis_rgb_widget.h"
#include "kis_gray_widget.h"
#include "kis_palette_widget.h"
#include "kis_paint_box.h"
#include "kis_filter_box.h"
#include "kis_birdeye_box.h"
#include "kis_controlframe.h"
#include "kis_layerbox.h"
#include "kis_channelview.h"
#include "kis_icon_item.h"

KisDockerManager::KisDockerManager(KisView * view, KActionCollection * ac)
	: m_view(view),
	  m_ac(ac)
{
	m_tabs = new KisGenericRegistry<QWidget*>();
	m_dockWindows = new KisGenericRegistry<KisDockFrameDocker*>();
	m_toolBoxes = new KisGenericRegistry<QToolBox*>();
	m_sliders = new KisGenericRegistry<KoTabbedToolDock*>();

	// XXX: Old way, moved from KisView
	KisConfig cfg;
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_slidersSetup = false;
	}

	m_brushMediator = 0;

	m_layerBox = 0;
	
	m_layerchannelslider = 0;
	m_shapesslider = 0;
	m_fillsslider = 0;
	m_toolcontrolslider = 0;
	m_colorslider = 0;

	m_layerchanneldocker = 0;
	m_shapesdocker = 0;
	m_fillsdocker = 0;
	m_toolcontroldocker = 0;
	m_colordocker = 0;

	m_hsvwidget = 0;
	m_rgbwidget = 0;
	m_graywidget = 0;
	m_palettewidget = 0;

	setupDockers();
}

KisDockerManager::~KisDockerManager()
{
	KisConfig cfg;
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		delete m_toolDockManager;
	}

}

void KisDockerManager::addDockerTab(QWidget * tab, const QString & docker, enumDockerStyle docktype)
{
	m_tabs -> add(KisID(tab -> name(), tab -> caption()), tab);
	
	if (docktype == DOCKER_DOCKER) {
	}
	else if (docktype == DOCKER_SLIDER) {
	}
	else if (docktype == DOCKER_TOOLBOX) {
	}
	
}

void KisDockerManager::unsetToolOptionWidget(KisTool * oldTool)
{
	KisConfig cfg;
	if (oldTool -> optionWidget()) {
		if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
			if (m_toolcontrolslider) m_toolcontrolslider -> unplug(oldTool -> optionWidget());
		}
		else {
			if (m_toolcontroldocker) m_toolcontroldocker -> unplug(oldTool -> optionWidget());
		}
	}
}

void KisDockerManager::setToolOptionWidget(KisTool * oldTool, KisTool * newTool)
{
	KisConfig cfg;

	if (oldTool)
	{
		if (oldTool -> optionWidget())
			if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
				if (m_toolcontrolslider) m_toolcontrolslider-> unplug(oldTool -> optionWidget());
			}
			else {
				if (m_toolcontroldocker) m_toolcontroldocker -> unplug(oldTool -> optionWidget());
			}
	}

	if (newTool) {
		if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
			if(newTool -> createOptionWidget(m_toolcontrolslider)){
				if (m_toolcontrolslider) {
					m_toolcontrolslider-> plug(newTool -> optionWidget());
					m_toolcontrolslider-> showPage(newTool -> optionWidget());
				}
			}
		}
		else {
			if(newTool -> createOptionWidget(m_toolcontroldocker)){
				if (m_toolcontroldocker) {
					m_toolcontroldocker -> plug(newTool -> optionWidget());
					m_toolcontroldocker -> showPage(newTool -> optionWidget());
				}
			}
		}

	}
}
// Old stuff
void KisDockerManager::setupDockers()
{
	KisResourceServer *rserver = new KisResourceServer;
	
	connect(m_view, SIGNAL(brushChanged(KisBrush *)), this, SLOT(slotBrushChanged(KisBrush* )));
	connect(m_view, SIGNAL(gradientChanged(KisGradient *)), this, SLOT(slotGradientChanged(KisGradient* )));
	connect(m_view, SIGNAL(patternChanged(KisPattern *)), this, SLOT(slotPatternChanged(KisPattern* )));
	
	KisConfig cfg;
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {

		m_toolDockManager = new KoToolDockManager(m_view);

		// Tools
		m_toolcontrolslider = m_toolDockManager -> createTabbedToolDock("info");
		m_toolcontrolslider -> setCaption(i18n("Navigator/Info/Options"));
		KToggleAction * show = new KToggleAction(i18n( "&Navigator/Info/Tool Options" ), 0, 0,
							 m_ac, "view_control_docker" );
		connect(show, SIGNAL(toggled(bool)), m_toolcontrolslider, SLOT(makeVisible(bool)));
		connect(m_toolcontrolslider, SIGNAL(visibleChange(bool)), SLOT(viewControlSlider(bool)));

		// Layers
		m_layerchannelslider = m_toolDockManager -> createTabbedToolDock("layers/channels");
		m_layerchannelslider -> setCaption(i18n("Layers/Channels" ));
		show = new KToggleAction(i18n( "&Layers/Channels" ), 0, 0, m_ac, "view_layer_docker" );
		connect(show, SIGNAL(toggled(bool)), m_layerchannelslider, SLOT(makeVisible(bool)));
		connect(m_layerchannelslider, SIGNAL(visibleChange(bool)), SLOT(viewLayerChannelSlider(bool)));

		// Shapes
		m_shapesslider = m_toolDockManager -> createTabbedToolDock("shapes");
		m_shapesslider -> setCaption(i18n("Brush Shapes"));
		show = new KToggleAction(i18n( "&Shapes" ), 0, 0, m_ac, "view_shapes_docker" );
		connect(show, SIGNAL(toggled(bool)), m_shapesslider, SLOT(makeVisible(bool)));
		connect(m_shapesslider, SIGNAL(visibleChange(bool)), SLOT(viewShapesSlider(bool)));

		// Fills
		m_fillsslider = m_toolDockManager -> createTabbedToolDock("fills");
		m_fillsslider -> setCaption(i18n("Fills"));

		show = new KToggleAction(i18n( "&Fills" ), 0, 0, m_ac, "view_fills_docker" );
		connect(show, SIGNAL(toggled(bool)), m_fillsslider, SLOT(makeVisible(bool)));
		connect(m_fillsslider, SIGNAL(visibleChange(bool)), SLOT(viewFillsSlider(bool)));

		// Colors
		m_colorslider = m_toolDockManager -> createTabbedToolDock("color");
		m_colorslider -> setCaption(i18n("Color Manager"));
		show = new KToggleAction(i18n( "&Colors" ), 0, 0, m_ac, "view_color_docker" );
		connect(show, SIGNAL(toggled(bool)), m_colorslider, SLOT(makeVisible(bool)));
		connect(m_colorslider, SIGNAL(visibleChange(bool)), SLOT(viewColorSlider(bool)));

	}

	if ( cfg.dockerStyle() == DOCKER_DOCKER ) {

		m_shapesdocker = new KisDockFrameDocker(m_view);
		m_shapesdocker -> setCaption(i18n("Brush Shapes"));

		m_fillsdocker = new KisDockFrameDocker(m_view);
		m_fillsdocker -> setCaption(i18n("Fills"));

		(void)new KAction(i18n( "&Shapes" ), 0, this, SLOT( viewShapesDocker() ), m_ac, "view_shapes_docker" );
		(void)new KAction(i18n( "&Fills" ), 0, this, SLOT( viewFillsDocker() ), m_ac, "view_fills_docker" );

	}
	if ( cfg.dockerStyle() == DOCKER_DOCKER || cfg.dockerStyle() == DOCKER_TOOLBOX ) {

		m_toolcontroldocker = new KisDockFrameDocker(m_view);
		m_toolcontroldocker -> setCaption(i18n("Navigator/Info/Options"));

		m_colordocker = new KisDockFrameDocker(m_view);
		m_colordocker -> setCaption(i18n("Color Manager"));

		m_layerchanneldocker = new KisDockFrameDocker(m_view);
		m_layerchanneldocker -> setCaption(i18n("Layers/Channels"));


		(void)new KAction(i18n( "&Layers/Channels" ), 0, this, SLOT( viewLayerChannelDocker() ), m_ac, "view_layer_docker" );
		(void)new KAction(i18n( "&Color Manager" ), 0, this, SLOT( viewColorDocker() ), m_ac, "view_color_docker" );
		(void)new KAction(i18n( "&Tool Properties" ), 0, this, SLOT( viewControlDocker() ), m_ac, "view_control_docker" );

	}

	// No matter what the docker style, we always have a paint box
	m_paintboxdocker = new KisPaintBox( m_view );
	m_paintboxdocker -> setCaption( i18n( "Tools" ) );
	(void)new KAction(i18n( "&Tools" ), 0, this, SLOT( viewPaintBoxDocker() ), m_ac, "view_paintop_docker" );


	// ---------------------------------------------------------------------
	// Control box

	m_controlWidget = new ControlFrame(m_toolcontroldocker);
	m_controlWidget -> setCaption(i18n("General"));

	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_toolcontrolslider -> plug( m_controlWidget );
	}
	else {
		m_toolcontroldocker -> plug(m_controlWidget);
	}


	m_controlWidget -> slotSetBGColor(m_view -> bgColor());
	m_controlWidget -> slotSetFGColor(m_view -> fgColor());
	
	connect(m_controlWidget, SIGNAL(fgColorChanged(const QColor&)), m_view, SLOT(slotSetFGColor(const QColor&)));
	connect(m_controlWidget, SIGNAL(bgColorChanged(const QColor&)), m_view, SLOT(slotSetBGColor(const QColor&)));
	
	connect(m_view, SIGNAL(fgColorChanged(const QColor&)), m_controlWidget, SLOT(slotSetFGColor(const QColor&)));
	connect(m_view, SIGNAL(bgColorChanged(const QColor&)), m_controlWidget, SLOT(slotSetBGColor(const QColor&)));

	// ---------------------------------------------------------------------
	// Bird's eye box
	m_birdEyeBox = new KisBirdEyeBox(m_toolcontroldocker);
	m_birdEyeBox -> setCaption(i18n("Overview"));

	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_toolcontrolslider -> plug ( m_birdEyeBox );
	}
	else {
		m_toolcontroldocker -> plug(m_birdEyeBox);
	}


	// ---------------------------------------------------------------------
	// Layers

	m_layerBox = new KisLayerBox(i18n("Layer"), KisLayerBox::SHOWALL, m_layerchanneldocker);
	m_layerBox -> setCaption(i18n("Layers"));

	connect(m_layerBox, SIGNAL(itemToggleVisible()), m_view, SLOT(layerToggleVisible()));
	connect(m_layerBox, SIGNAL(itemSelected(int)), m_view, SLOT(layerSelected(int)));
	connect(m_layerBox, SIGNAL(itemToggleLinked()), m_view, SLOT(layerToggleLinked()));
	connect(m_layerBox, SIGNAL(itemProperties()), m_view, SLOT(layerProperties()));
	connect(m_layerBox, SIGNAL(itemAdd()), m_view, SLOT(layerAdd()));
	connect(m_layerBox, SIGNAL(itemRemove()), m_view, SLOT(layerRemove()));
	connect(m_layerBox, SIGNAL(itemAddMask(int)), m_view, SLOT(layerAddMask(int)));
	connect(m_layerBox, SIGNAL(itemRmMask(int)), m_view, SLOT(layerRmMask(int)));
	connect(m_layerBox, SIGNAL(itemRaise()), m_view, SLOT(layerRaise()));
	connect(m_layerBox, SIGNAL(itemLower()), m_view, SLOT(layerLower()));
	connect(m_layerBox, SIGNAL(itemFront()), m_view, SLOT(layerFront()));
	connect(m_layerBox, SIGNAL(itemBack()), m_view, SLOT(layerBack()));
	connect(m_layerBox, SIGNAL(itemLevel(int)), m_view, SLOT(layerLevel(int)));
	connect(m_view, SIGNAL(currentLayerChanged(int)), m_layerBox, SLOT(slotSetCurrentItem(int)));
	
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_layerchannelslider -> plug( m_layerBox );
	}
	else {
		m_layerchanneldocker -> plug(m_layerBox);
	}

#if 0	// This doesn't do anything useful yet.
	// Channels
	m_channelView = new KisChannelView(m_view -> document(), m_view);
	m_channelView -> setCaption(i18n("Channels"));
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_layerchannelslider -> plug( m_channelView );
	}
	else {
		m_layerchanneldocker -> plug(m_channelView);
	}
#endif

	// ---------------------------------------------------------------------
	// Shapes

	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_brushMediator = new KisResourceMediator(MEDIATE_BRUSHES, rserver, i18n("Brushes"),
						  m_shapesslider, "brush_chooser", m_view);
		m_shapesslider -> plug( m_brushMediator -> chooserWidget());

	}
	else if ( cfg.dockerStyle() == DOCKER_TOOLBOX ) {
		m_brushMediator = new KisResourceMediator(MEDIATE_BRUSHES, rserver, i18n("Brushes"),
						  m_paintboxdocker, "brush_chooser", m_view);
		m_paintboxdocker -> plug( m_brushMediator -> chooserWidget());
	}
	else {
		m_brushMediator = new KisResourceMediator(MEDIATE_BRUSHES, rserver, i18n("Brushes"),
						  m_shapesdocker, "brush_chooser", m_view);
		m_shapesdocker -> plug(m_brushMediator -> chooserWidget());
	}

	m_view -> brushActivated(dynamic_cast<KisBrush*>(m_brushMediator -> currentResource()));
	connect(m_brushMediator, SIGNAL(activatedResource(KisResource*)), m_view, SLOT(brushActivated(KisResource*)));


	// AutoBrush
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_autobrush = new KisAutobrush(m_shapesslider, "autobrush", i18n("Autobrush"));
		m_shapesslider -> plug( m_autobrush );
	}
	else if ( cfg.dockerStyle() == DOCKER_TOOLBOX ) {
		m_autobrush = new KisAutobrush(m_paintboxdocker, "autobrush", i18n("Autobrush"));
		m_paintboxdocker -> plug( m_autobrush );
	}
	else {
		m_autobrush = new KisAutobrush(m_shapesdocker, "autobrush", i18n("Autobrush"));
		m_shapesdocker -> plug(m_autobrush);
	}
	connect(m_autobrush, SIGNAL(activatedResource(KisResource*)), m_view, SLOT(brushActivated(KisResource*)));

	// TextBrush
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_textBrush = new KisTextBrush(m_shapesslider, "textbrush", i18n("Text Brush"));
		m_shapesslider -> plug(m_textBrush);
	}
	else if ( cfg.dockerStyle() == DOCKER_TOOLBOX ) {
		m_textBrush = new KisTextBrush(m_paintboxdocker, "textbrush", i18n("Text Brush"));
		m_paintboxdocker -> plug(m_textBrush);
	}
	else {
		m_textBrush = new KisTextBrush(m_shapesdocker, "textbrush", i18n("Text Brush"));
		m_shapesdocker -> plug(m_textBrush);
	}
	connect(m_textBrush, SIGNAL(activatedResource(KisResource*)), m_view, SLOT(brushActivated(KisResource*)));
// 
	// ---------------------------------------------------------------------
	// Fills


	// Setup patterns
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_patternMediator = new KisResourceMediator(MEDIATE_PATTERNS, rserver, i18n("Patterns"),
							    m_fillsslider, "pattern chooser", m_view);
		m_fillsslider -> plug(m_patternMediator -> chooserWidget());
	}
	else if ( cfg.dockerStyle() == DOCKER_TOOLBOX ) {
		m_patternMediator = new KisResourceMediator(MEDIATE_PATTERNS, rserver, i18n("Patterns"),
						    m_paintboxdocker, "pattern chooser", m_view);
		m_paintboxdocker -> plug(m_patternMediator -> chooserWidget());

	}
	else {
		m_patternMediator = new KisResourceMediator(MEDIATE_PATTERNS, rserver, i18n("Patterns"),
						    m_fillsdocker, "pattern chooser", m_view);
		m_fillsdocker -> plug(m_patternMediator -> chooserWidget());
	}
	m_view -> patternActivated(dynamic_cast<KisPattern*>(m_patternMediator -> currentResource()));
	
	connect(m_patternMediator, SIGNAL(activatedResource(KisResource*)), m_view, SLOT(patternActivated(KisResource*)));

	// Setup gradients
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_gradientMediator = new KisResourceMediator(MEDIATE_GRADIENTS, rserver, i18n("Gradients"),
							     m_fillsslider, "gradient chooser", m_view);
		m_fillsslider -> plug(m_gradientMediator -> chooserWidget());

	}
	else if ( cfg.dockerStyle() == DOCKER_TOOLBOX ) {
		m_gradientMediator = new KisResourceMediator(MEDIATE_GRADIENTS, rserver, i18n("Gradients"),
							     m_paintboxdocker, "gradient chooser", m_view);
		m_paintboxdocker -> plug(m_gradientMediator -> chooserWidget());
	}
	else {
		m_gradientMediator = new KisResourceMediator(MEDIATE_GRADIENTS, rserver, i18n("Gradients"),
							     m_fillsdocker, "gradient chooser", m_view);
		m_fillsdocker -> plug(m_gradientMediator -> chooserWidget());
	}
	m_view -> gradientActivated(dynamic_cast<KisGradient*>(m_gradientMediator -> currentResource()));
	connect(m_gradientMediator, SIGNAL(activatedResource(KisResource*)), m_view, SLOT(gradientActivated(KisResource*)));

	// Autogradient
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_autogradient = new KisAutogradient(m_fillsslider, "autogradient", i18n("Autogradient"));
		m_fillsslider -> plug(m_autogradient);
	}
	else if ( cfg.dockerStyle() == DOCKER_TOOLBOX ) {
		m_autogradient = new KisAutogradient(m_paintboxdocker, "autogradient", i18n("Autogradient"));
		m_paintboxdocker -> plug(m_autogradient);
	}
	else {
		m_autogradient = new KisAutogradient(m_fillsdocker, "autogradient", i18n("Autogradient"));
		m_fillsdocker -> plug(m_autogradient);
	}
	connect(m_autogradient, SIGNAL(activatedResource(KisResource*)), m_view, SLOT(gradientActivated(KisResource*)));


	// ---------------------------------------------------------------------
	// Colors


	m_hsvwidget = new KisHSVWidget(m_view, "hsv");
	m_hsvwidget -> setCaption(i18n("HSV"));
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_colorslider -> plug( m_hsvwidget );
	}
	else {
		m_colordocker -> plug(m_hsvwidget);
	}
	m_view -> attach(m_hsvwidget);

	m_rgbwidget = new KisRGBWidget(m_view, "rgb");
	m_rgbwidget -> setCaption(i18n("RGB"));
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_colorslider -> plug(m_rgbwidget);
	}
	else {
		m_colordocker -> plug(m_rgbwidget);
	}
	m_view -> attach(m_rgbwidget);

	m_graywidget = new KisGrayWidget(m_view, "gray");
	m_graywidget -> setCaption(i18n("Gray"));
	if ( cfg.dockerStyle() == DOCKER_SLIDER ) {
		m_colorslider -> plug(m_graywidget );
	}
	else {
		m_colordocker -> plug(m_graywidget);
	}
	m_view -> attach(m_graywidget);

	m_palettewidget = new KisPaletteWidget(m_view);
	m_palettewidget -> setCaption(i18n("Palettes"));

	connect(rserver, SIGNAL(loadedPalette(KisResource*)), m_palettewidget, SLOT(slotAddPalette(KisResource*)));
	rserver->palettes();
	connect(m_palettewidget, SIGNAL(colorSelected(const QColor &)), m_view, SLOT(slotSetFGColor(const QColor &)));

	if (cfg.dockerStyle() == DOCKER_SLIDER) {
		m_colorslider -> plug(m_palettewidget);
	}
	else {
		m_colordocker -> plug(m_palettewidget);
	}


#if 0
	// Filters XXX: Disable for the release.
	m_filterBox = new KisFilterBox(m_view, m_view, "filter box");
	m_paintboxdocker -> plug( m_filterBox, "Painting filters" );
	m_filterBox -> init();
#endif

	if ( cfg.dockerStyle() == DOCKER_DOCKER || cfg.dockerStyle() == DOCKER_TOOLBOX ) {
		// TODO Here should be a better check
		if ( m_view -> mainWindow() -> isDockEnabled( DockBottom)) {
			viewControlDocker();
			viewLayerChannelDocker();
			viewPaintBoxDocker();
			viewColorDocker();

			m_view -> mainWindow() -> setDockEnabled( DockBottom, false);
			m_view -> mainWindow() -> setDockEnabled( DockTop, false);

		}
	}

	if ( cfg.dockerStyle() == DOCKER_DOCKER ) {

		// TODO Here should be a better check
		if ( m_view -> mainWindow() -> isDockEnabled( DockBottom))
		{
			viewShapesDocker();
			viewFillsDocker();

		}
	}

}

void KisDockerManager::viewColorSlider(bool v)
{
	((KToggleAction*)m_ac->action("view_color_docker")) -> setChecked(v);
}

void KisDockerManager::viewControlSlider(bool v)
{
	((KToggleAction*)m_ac->action("view_control_docker")) -> setChecked(v);
}

void KisDockerManager::viewLayerChannelSlider(bool v)
{
	((KToggleAction*)m_ac->action("view_layer_docker")) -> setChecked(v);}

void KisDockerManager::viewFillsSlider(bool v)
{
	((KToggleAction*)m_ac->action("view_fills_docker")) -> setChecked(v);}


void KisDockerManager::viewShapesSlider(bool v)
{
	((KToggleAction*)m_ac->action("view_shapes_docker")) -> setChecked(v);
}

void KisDockerManager::viewColorDocker()
{
	if( m_colordocker->isVisible() == false )
	{
		m_view -> mainWindow()->addDockWindow( m_colordocker, DockRight );
		m_colordocker->show();
	}
}

void KisDockerManager::viewControlDocker()
{
	if( m_toolcontroldocker->isVisible() == false )
	{
		m_view -> mainWindow()->addDockWindow( m_toolcontroldocker, DockRight );
		m_toolcontroldocker->show();
	}
}

void KisDockerManager::viewLayerChannelDocker()
{
	if( m_layerchanneldocker->isVisible() == false )
	{
		m_view -> mainWindow()->addDockWindow( m_layerchanneldocker, DockRight );
		m_layerchanneldocker->show();
	}
}

void KisDockerManager::viewFillsDocker()
{
	if( m_fillsdocker->isVisible() == false )
	{
		m_view -> mainWindow()->addDockWindow( m_fillsdocker, DockRight );
		m_shapesdocker->show();
	}
}


void KisDockerManager::viewShapesDocker()
{
	if( m_shapesdocker->isVisible() == false )
	{
		m_view -> mainWindow()->addDockWindow( m_shapesdocker, DockRight );
		m_shapesdocker->show();
	}
}

void KisDockerManager::viewPaintBoxDocker()
{
	if ( m_paintboxdocker->isVisible() == false )
	{
		m_view -> mainWindow()->addDockWindow( m_paintboxdocker, DockRight );
		m_paintboxdocker->show();
	}
}


void KisDockerManager::slotBrushChanged(KisBrush * brush)
{
	KisIconItem *item;

	if((item = m_brushMediator -> itemFor(brush)))
	{
		if (m_controlWidget) m_controlWidget -> slotSetBrush(item);
	} else {
		if (m_controlWidget) m_controlWidget -> slotSetBrush( new KisIconItem(brush) );
	}

}

void KisDockerManager::slotGradientChanged(KisGradient * gradient)
{
	KisIconItem *item;
	if (gradient && (item = m_gradientMediator -> itemFor(gradient)))
		if (m_controlWidget) m_controlWidget -> slotSetGradient(item);

}

void KisDockerManager::slotPatternChanged(KisPattern * pattern)
{
	KisIconItem *item;
	if (pattern && (item = m_patternMediator -> itemFor(pattern)))
		if (m_controlWidget) m_controlWidget -> slotSetPattern(item);


}

void KisDockerManager::resetLayerBox(KisImageSP img, KisLayerSP layer)
{
	m_layerBox -> setUpdatesEnabled(false);
	m_layerBox -> clear();

	if (img) {
		vKisLayerSP l = img -> layers();

		for (vKisLayerSP_it it = l.begin(); it != l.end(); it++)
			m_layerBox -> insertItem((*it) -> name(), (*it) -> visible(), (*it) -> linked());

		m_layerBox -> slotSetCurrentItem(img -> index(layer));
	}

	m_layerBox -> setUpdatesEnabled(true);
}

#include "kis_docker_manager.moc"
