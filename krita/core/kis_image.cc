/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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
#include <stdlib.h>
#include <math.h>

#include <config.h>
#include LCMS_HEADER

#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qtl.h>

#include <kcommand.h>
#include <kocommandhistory.h>
#include <kdebug.h>
#include <klocale.h>

#include "KIsImageIface.h"

#include "kis_guide.h"
#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_paint_device_visitor.h"
#include "kis_painter.h"
#include "kis_fill_painter.h"
#include "kis_layer.h"
#include "kis_background.h"
#include "kis_doc.h"
#include "kis_nameserver.h"
#include "kistile.h"
#include "kistilemgr.h"
#include "kispixeldata.h"
#include "visitors/kis_flatten.h"
#include "visitors/kis_merge.h"
#include "kis_scale_visitor.h"
#include "kis_profile.h"

#define DEBUG_IMAGES 0

#if DEBUG_IMAGES
static int numImages = 0;
#endif

namespace {

	// Whether to repaint the display every
	// DISPLAY_UPDATE_FREQUENCY milliseconds
	const bool DISPLAY_TIMER = false;
	const int DISPLAY_UPDATE_FREQUENCY = 50; // in milliseconds

	class KisResizeImageCmd : public KNamedCommand {
		typedef KNamedCommand super;

	public:
		KisResizeImageCmd(KisUndoAdapter *adapter,
				  KisImageSP img,
				  Q_INT32 width,
				  Q_INT32 height,
				  Q_INT32 oldWidth,
				  Q_INT32 oldHeight) : super(i18n("Resize Image"))
			{
				m_adapter = adapter;
				m_img = img;
				m_before = QSize(oldWidth, oldHeight);
				m_after = QSize(width, height);
			}

		virtual ~KisResizeImageCmd()
			{
			}

	public:
		virtual void execute()
			{
				m_adapter -> setUndo(false);
				m_img -> resize(m_after.width(), m_after.height());
				m_adapter -> setUndo(true);
				m_img -> notify(0, 0, QMAX(m_before.width(), m_after.width()), QMAX(m_before.height(), m_after.height()));
			}

		virtual void unexecute()
			{
				m_adapter -> setUndo(false);
				m_img -> resize(m_before.width(), m_before.height());
				m_adapter -> setUndo(true);
				m_img -> notify(0, 0, QMAX(m_before.width(), m_after.width()), QMAX(m_before.height(), m_after.height()));
			}

	private:
		KisUndoAdapter *m_adapter;
		KisImageSP m_img;
		QSize m_before;
		QSize m_after;
	};


	class KisChangeLayersCmd : public KNamedCommand {
		typedef KNamedCommand super;

	public:
		KisChangeLayersCmd(KisUndoAdapter *adapter, KisImageSP img, vKisLayerSP& beforeLayers, vKisLayerSP& afterLayers, const QString& name) : super(name)
			{
				m_adapter = adapter;
				m_img = img;
				m_beforeLayers = beforeLayers;
				m_afterLayers = afterLayers;
			}

		virtual ~KisChangeLayersCmd()
			{
			}

	public:
		virtual void execute()
			{
				m_adapter -> setUndo(false);

				for (vKisLayerSP::const_iterator it = m_beforeLayers.begin(); it != m_beforeLayers.end(); it++) {
					m_img -> rm(*it);
				}

				for (vKisLayerSP::const_iterator it = m_afterLayers.begin(); it != m_afterLayers.end(); it++) {
					m_img -> add(*it, -1);
				}

				m_adapter -> setUndo(true);
				m_img -> notify();
				m_img -> notifyLayersChanged();
			}

		virtual void unexecute()
			{
				m_adapter -> setUndo(false);

				for (vKisLayerSP::const_iterator it = m_afterLayers.begin(); it != m_afterLayers.end(); it++) {
					m_img -> rm(*it);
				}

				for (vKisLayerSP::const_iterator it = m_beforeLayers.begin(); it != m_beforeLayers.end(); it++) {
					m_img -> add(*it, -1);
				}

				m_adapter -> setUndo(true);
				m_img -> notify();
				m_img -> notifyLayersChanged();
			}

	private:
		KisUndoAdapter *m_adapter;
		KisImageSP m_img;
		vKisLayerSP m_beforeLayers;
		vKisLayerSP m_afterLayers;
	};

}

KisImage::KisImage(KisUndoAdapter *undoAdapter, Q_INT32 width, Q_INT32 height,  KisStrategyColorSpaceSP colorStrategy, const QString& name)
{
#if DEBUG_IMAGES
	numImages++;
	kdDebug() << "IMAGE " << name << " CREATED total now = " << numImages << endl;
#endif
	init(undoAdapter, width, height, colorStrategy, name);
	setName(name);
	startUpdateTimer();
        m_dcop = 0L;
	m_profile = 0;
}

KisImage::KisImage(const KisImage& rhs) : QObject(), KisRenderInterface(rhs)
{
#if DEBUG_IMAGES
	numImages++;
	kdDebug() << "IMAGE " << rhs.m_name << " copy CREATED total now = " << numImages << endl;
#endif
	m_dcop = 0L;
	if (this != &rhs) {
		m_undoHistory = rhs.m_undoHistory;
		m_uri = rhs.m_uri;
		m_name = QString::null;
		m_depth = rhs.m_depth;
		m_ntileCols = rhs.m_ntileCols;
		m_ntileRows = rhs.m_ntileRows;
		m_xres = rhs.m_xres;
		m_yres = rhs.m_yres;
		m_unit = rhs.m_unit;
		m_colorStrategy = rhs.m_colorStrategy;
		m_clrMap = rhs.m_clrMap;
		m_dirty = rhs.m_dirty;
		m_adapter = rhs.m_adapter;
		m_profile = rhs.m_profile;
		if (rhs.m_shadow)
			m_shadow = new KisTileMgr(*rhs.m_shadow);

		m_bkg = new KisBackground(this, rhs.width(), rhs.height());
		m_projection = new KisLayer(this, rhs.width(), rhs.height(), "projection", OPACITY_OPAQUE);
		m_layers.reserve(rhs.m_layers.size());

		for (vKisLayerSP_cit it = rhs.m_layers.begin(); it != rhs.m_layers.end(); it++) {
			KisLayerSP layer = new KisLayer(**it);

			layer -> setImage(this);
			m_layers.push_back(layer);
			m_layerStack.push_back(layer);
			m_activeLayer = layer;
		}


		m_visible = rhs.m_visible;
		m_active = rhs.m_active;
		m_nserver = new KisNameServer(i18n("Layer %1"), rhs.m_nserver -> currentSeed() + 1);
		m_guides = rhs.m_guides;
		startUpdateTimer();
	}

}


DCOPObject *KisImage::dcopObject()
{
	if (!m_dcop)
		m_dcop = new KIsImageIface(this);
	return m_dcop;
}

KisImage::~KisImage()
{
#if DEBUG_IMAGES
	numImages--;
	kdDebug() << "IMAGE " << name() << " DESTROYED total now = " << numImages << endl;
#endif
	delete m_nserver;
        delete m_dcop;
	// Not necessary to destroy m_updateTimer
}

QString KisImage::name() const
{
	return m_name;
}

void KisImage::setName(const QString& name)
{
	if (!name.isEmpty())
		m_name = name;
}

QString KisImage::description() const
{
	return m_description;
}

void KisImage::setDescription(const QString& description)
{
	if (!description.isEmpty())
		m_description = description;
}


QString KisImage::nextLayerName() const
{
	if (m_nserver -> currentSeed() == 0) {
		m_nserver -> number();
		return i18n("background");
	}

	return m_nserver -> name();
}

void KisImage::init(KisUndoAdapter *adapter, Q_INT32 width, Q_INT32 height,  KisStrategyColorSpaceSP colorStrategy, const QString& name)
{
	Q_INT32 n;

	m_adapter = adapter;
	m_nserver = new KisNameServer(i18n("Layer %1"), 1);
	n = colorStrategy->depth();
	m_active.resize(n);
	m_visible.resize(n);
	m_name = name;
	m_depth = n;
	m_colorStrategy = colorStrategy;
	m_bkg = new KisBackground(this, width, height);
	m_projection = new KisLayer(this, width, height, "projection", OPACITY_OPAQUE);
	m_xres = 1.0;
	m_yres = 1.0;
	m_unit = KoUnit::U_PT;
	m_dirty = false;
	m_undoHistory = 0;
	m_ntileCols = (width + TILE_WIDTH - 1) / TILE_WIDTH;
	m_ntileRows = (height + TILE_HEIGHT - 1) / TILE_HEIGHT;
}

void KisImage::resize(Q_INT32 w, Q_INT32 h)
{
// 	kdDebug() << "KisImage::resize. From: (" 
// 		  << width()
// 		  << ", " 
// 		  << height()
// 		  << ") to (" 
// 		  << w 
// 		  << ", " 
// 		  << h 
// 		  << ")\n";

	if (w != width() || h != height()) {
		if (m_adapter && m_adapter -> undo()) {
			m_adapter -> beginMacro("Resize image");
			m_adapter -> addCommand(new KisResizeImageCmd(m_adapter, this, w, h, width(), height()));
		}

		m_ntileCols = (w + TILE_WIDTH - 1) / TILE_WIDTH;
		m_ntileRows = (h + TILE_HEIGHT - 1) / TILE_HEIGHT;

		m_projection = new KisLayer(this, w, h, "projection", OPACITY_OPAQUE);
 		m_bkg = new KisBackground(this, w, h);

		if (m_adapter && m_adapter -> undo()) {
			m_adapter -> endMacro();
		}

		emit sizeChanged(KisImageSP(this), w, h);
	}
}

void KisImage::resize(const QRect& rc)
{
	resize(rc.width(), rc.height());
}

void KisImage::scale(double sx, double sy, KisProgressDisplayInterface *m_progress, enumFilterType ftype) 
{
// 	kdDebug() << "KisImage::scale. SX: " 
// 		  << sx
// 		  << ", SY:" 
// 		  << sy 
// 		  << "\n";

	if (m_layers.empty()) return; // Nothing to scale

	// New image size. XXX: Pass along to discourage rounding errors?
	Q_INT32 w, h;	
	w = (Q_INT32)(( width() * sx) + 0.5);
	h = (Q_INT32)(( height() * sy) + 0.5); 
	
// 	kdDebug() << "Scaling from (" << m_projection -> width() 
// 		  << ", " << m_projection -> height() 
// 		  << "to: (" << w << ", " << h << ")\n";

	if (w != width() || h != height()) {

		undoAdapter() -> beginMacro("Scale image");

		vKisLayerSP_it it;
		for ( it = m_layers.begin(); it != m_layers.end(); ++it ) {
			KisLayerSP layer = (*it);
			layer -> scale(sx, sy, m_progress, ftype);
		}

		m_adapter -> addCommand(new KisResizeImageCmd(m_adapter, this, w, h, width(), height()));
		
		m_ntileCols = (w + TILE_WIDTH - 1) / TILE_WIDTH;
		m_ntileRows = (h + TILE_HEIGHT - 1) / TILE_HEIGHT;

		m_projection = new KisLayer(this, w, h, "projection", OPACITY_OPAQUE);
		m_bkg = new KisBackground(this, w, h);

		undoAdapter()->endMacro();

		emit sizeChanged(KisImageSP(this), w, h);
	}
}

void KisImage::rotate(double angle, KisProgressDisplayInterface *m_progress) 
{
	
        const double pi=3.1415926535897932385;

	if (m_layers.empty()) return; // Nothing to scale
        Q_INT32 w, h;
        w = (Q_INT32)( width()*QABS(cos(angle*pi/180)) + height()*QABS(sin(angle*pi/180)) + 1 );
        h = (Q_INT32)( height()*QABS(cos(angle*pi/180)) + width()*QABS(sin(angle*pi/180)) + 1 );
	if (w != width() || h != height()) {

		undoAdapter() -> beginMacro("Rotate image");

		vKisLayerSP_it it;
		for ( it = m_layers.begin(); it != m_layers.end(); ++it ) {
			KisLayerSP layer = (*it);
			layer -> rotate(angle, m_progress);
		}

		m_adapter -> addCommand(new KisResizeImageCmd(m_adapter, this, w, h, width(), height()));
		
		m_ntileCols = (w + TILE_WIDTH - 1) / TILE_WIDTH;
		m_ntileRows = (h + TILE_HEIGHT - 1) / TILE_HEIGHT;

		m_projection = new KisLayer(this, w, h, "projection", OPACITY_OPAQUE);
		m_bkg = new KisBackground(this, w, h);

		undoAdapter()->endMacro();

		emit sizeChanged(KisImageSP(this), w, h);
	}
}

void KisImage::shear(double angleX, double angleY, KisProgressDisplayInterface *m_progress) 
{
	const double pi=3.1415926535897932385;
        
        if (m_layers.empty()) return; // Nothing to scale

        //new image size
	Q_INT32 w=width();
        Q_INT32 h=height();	
        
        
        if(angleX != 0 || angleY != 0){
                double deltaY=height()*QABS(tan(angleX*pi/180)*tan(angleY*pi/180));
                w = (Q_INT32) ( width() + QABS(height()*tan(angleX*pi/180)) );
                //ugly fix for the problem of having two extra pixels if only a shear along one
                //axis is done. This has to be fixed in the cropping code in KisRotateVisitor!
                if (angleX == 0 || angleY == 0)
                        h = (Q_INT32) ( height() + QABS(w*tan(angleY*pi/180)) );
                else if (angleX > 0 && angleY > 0)
                        h = (Q_INT32) ( height() + QABS(w*tan(angleY*pi/180))- 2 * deltaY + 2 );
                else if (angleX < 0 && angleY < 0)
                        h = (Q_INT32) ( height() + QABS(w*tan(angleY*pi/180))- 2 * deltaY + 2 );
                else 
                        h = (Q_INT32) ( height() + QABS(w*tan(angleY*pi/180)) );
        }
        
	if (w != width() || h != height()) {

		undoAdapter() -> beginMacro("Shear image");

		vKisLayerSP_it it;
		for ( it = m_layers.begin(); it != m_layers.end(); ++it ) {
			KisLayerSP layer = (*it);
			layer -> shear(angleX, angleY, m_progress);
		}

		m_adapter -> addCommand(new KisResizeImageCmd(m_adapter, this, w, h, width(), height()));
		
		m_ntileCols = (w + TILE_WIDTH - 1) / TILE_WIDTH;
		m_ntileRows = (h + TILE_HEIGHT - 1) / TILE_HEIGHT;

		m_projection = new KisLayer(this, w, h, "projection", OPACITY_OPAQUE);
		m_bkg = new KisBackground(this, w, h);

		undoAdapter()->endMacro();

		emit sizeChanged(KisImageSP(this), w, h);
	}
}

void KisImage::convertTo( KisStrategyColorSpaceSP colorStrategy)
{
	if (m_layers.empty()) return; // Nothing to convert
	vKisLayerSP_it it;
	for ( it = m_layers.begin(); it != m_layers.end(); ++it ) {
		(*it) -> convertTo(colorStrategy);
	}
	m_projection -> convertTo(colorStrategy);
}

void KisImage::convertTo(KisStrategyColorSpaceSP colorStrategy, KisProfileSP /*profile*/)
{
	// XXX: Implement
	convertTo(colorStrategy);
}

KisProfileSP KisImage::profile() const
{
	return m_profile;
}

void KisImage::setProfile(const KisProfileSP& profile) 
{
	if (profile && profile -> valid()) {
		kdDebug() << "KisImage::setProfile: " << profile -> productName() << "\n";
		m_profile = profile;
	}
	else {
		kdDebug() << "KisImage::setProfile: empty profile\n";
		m_profile = 0;
	}
}

KURL KisImage::uri() const
{
	return m_uri;
}

void KisImage::uri(const KURL& uri)
{
	if (uri.isValid())
		m_uri = uri;
}

KoUnit::Unit KisImage::unit() const
{
	return m_unit;
}

void KisImage::unit(const KoUnit::Unit& u)
{
	m_unit = u;
}

double KisImage::xRes()
{
	return m_xres;
}

double KisImage::yRes()
{
	return m_yres;
}

void KisImage::setResolution(double xres, double yres)
{
	m_xres = xres;
	m_yres = yres;
}

Q_INT32 KisImage::width() const
{
	return m_projection -> width();
}

Q_INT32 KisImage::height() const
{
	return m_projection -> height();
}

Q_UINT32 KisImage::depth() const
{
	return m_depth;
}

bool KisImage::alpha() const
{
	return m_colorStrategy -> alpha();
}

bool KisImage::empty() const
{
	return m_layers.size() > 0;
}

KisTileMgrSP KisImage::shadow() const
{
	return m_shadow;
}

vKisLayerSP KisImage::layers()
{
	return m_layers;
}

const vKisLayerSP& KisImage::layers() const
{
	return m_layers;
}

KisPaintDeviceSP KisImage::activeDevice()
{
	if (m_activeLayer) {
		return m_activeLayer.data();
	}

	return 0;
}

const KisLayerSP KisImage::activeLayer() const
{
	return m_activeLayer;
}

KisLayerSP KisImage::activeLayer()
{
	return m_activeLayer;
}

KisLayerSP KisImage::activate(KisLayerSP layer)
{
	vKisLayerSP_it it;

	if (m_layers.empty() || !layer)
		return 0;

	it = qFind(m_layers.begin(), m_layers.end(), layer);

	if (it == m_layers.end()) {
		layer = m_layers[0];
		it = m_layers.begin();
	}

	if (layer) {
		it = qFind(m_layerStack.begin(), m_layerStack.end(), layer);

		if (it != m_layerStack.end())
			m_layerStack.erase(it);

		m_layerStack.insert(m_layerStack.begin() + 0, layer);

		connect(layer, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));
		connect(layer, SIGNAL(selectionCreated()), this, SLOT(slotSelectionCreated()));
	}

	if (layer != m_activeLayer) {
		m_activeLayer = layer;
		emit activeLayerChanged(KisImageSP(this));
	}


	return layer;
}

KisLayerSP KisImage::activateLayer(Q_INT32 n)
{
	if (n < 0 || static_cast<Q_UINT32>(n) > m_layers.size())
		return 0;

	return activate(m_layers[n]);
}

Q_INT32 KisImage::index(const KisLayerSP &layer)
{
	for (Q_UINT32 i = 0; i < m_layers.size(); i++) {
		if (m_layers[i] == layer)
			return i;
	}

	return -1;
}

KisLayerSP KisImage::layer(const QString& name)
{
	for (vKisLayerSP_it it = m_layers.begin(); it != m_layers.end(); it++) {
		if ((*it) -> name() == name)
			return *it;
	}

	return 0;
}

KisLayerSP KisImage::layer(Q_UINT32 npos)
{
	if (npos >= m_layers.size())
		return 0;

	return m_layers[npos];
}

bool KisImage::add(KisLayerSP layer, Q_INT32 position)
{
	if (layer == 0)
		return false;

	if (layer -> image() && layer -> image() != KisImageSP(this))
		return false;

	if (qFind(m_layers.begin(), m_layers.end(), layer) != m_layers.end())
		return false;

	layer -> setImage(KisImageSP(this));

	if (position == -1) {
		// Add to bottom of layer stack
		position = m_layers.size();
	}

	m_layers.insert(m_layers.begin() + position, layer);
	activate(layer);

	m_layerStack.push_back(layer);
	expand(layer.data());
	return true;
}

void KisImage::rm(KisLayerSP layer)
{
	QRect rc;
	vKisLayerSP_it it;

	if (layer == 0)
		return;

	it = qFind(m_layers.begin(), m_layers.end(), layer);

	if (it == m_layers.end())
		return;

	m_layers.erase(it);
	it = qFind(m_layerStack.begin(), m_layerStack.end(), layer);

	if (it != m_layerStack.end())
		m_layerStack.erase(it);

	layer -> setImage(0);

	if (layer == m_activeLayer) {
		if (m_layers.empty()) {
			m_activeLayer = 0;
			emit activeLayerChanged(KisImageSP(this));
		} else {
			activate(m_layerStack[0]);
		}
	}

	rc = layer -> bounds();


}

bool KisImage::raise(KisLayerSP layer)
{
	Q_INT32 position;

	if (layer == 0)
		return false;

	position = index(layer);

	if (position <= 0)
		return false;

	return pos(layer, position - 1);
}

bool KisImage::lower(KisLayerSP layer)
{
	Q_INT32 position;
	Q_INT32 size;

	if (layer == 0)
		return false;

	position = index(layer);
	size = m_layers.size();

	if (position >= size)
		return false;

	return pos(layer, position + 1);
}

bool KisImage::top(KisLayerSP layer)
{
	Q_INT32 position;

	if (layer == 0)
		return false;

	position = index(layer);

	if (position == 0)
		return false;

	return pos(layer, 0);
}

bool KisImage::bottom(KisLayerSP layer)
{
	Q_INT32 position;
	Q_INT32 size;

	if (layer == 0)
		return false;

	position = index(layer);
	size = m_layers.size();

	if (position >= size - 1)
		return false;

	return pos(layer, size - 1);
}

bool KisImage::pos(KisLayerSP layer, Q_INT32 position)
{
	Q_INT32 old;
	Q_INT32 nlayers;

	if (layer == 0)
		return false;

	old = index(layer);

	if (old < 0)
		return false;

	nlayers = m_layers.size();

	if (position < 0)
		position = 0;

	if (position >= nlayers)
		position = nlayers - 1;

	if (old == position)
		return true;

	if (position < old) {
		m_layers.erase(m_layers.begin() + old);
		m_layers.insert(m_layers.begin() + position, layer);
	}
	else {
		m_layers.insert(m_layers.begin() + position + 1, layer);
		m_layers.erase(m_layers.begin() + old);
	}

	return true;
}

Q_INT32 KisImage::nlayers() const
{
	return m_layers.size();
}

Q_INT32 KisImage::nHiddenLayers() const
{
	Q_INT32 n = 0;

	for (vKisLayerSP_cit it = m_layers.begin(); it != m_layers.end(); it++) {
		const KisLayerSP& layer = *it;

		if (!layer -> visible()) {
			n++;
		}
	}

	return n;
}

Q_INT32 KisImage::nLinkedLayers() const
{
	Q_INT32 n = 0;

	for (vKisLayerSP_cit it = m_layers.begin(); it != m_layers.end(); it++) {
		const KisLayerSP& layer = *it;

		if (layer -> linked()) {
			n++;
		}
	}

	return n;
}

void KisImage::flatten()
{
	vKisLayerSP beforeLayers = m_layers;

	KisLayerSP dst = new KisLayer(this, width(), height(), nextLayerName(), OPACITY_OPAQUE);
	KisFillPainter painter(dst.data());
	painter.fillRect(0, 0, dst -> width(), dst -> height(), KoColor(0, 0, 0), OPACITY_TRANSPARENT);

	vKisLayerSP mergeLayers = layers();
	KisMerge<isVisible, All> visitor(this);
	visitor(painter, mergeLayers);
	add(dst, -1);

	notify();
	notifyLayersChanged();

	if (m_adapter && m_adapter -> undo()) {
		m_adapter -> addCommand(new KisChangeLayersCmd(m_adapter, this, beforeLayers, m_layers, i18n("Flatten Image")));
	}
}

void KisImage::mergeVisibleLayers()
{
	vKisLayerSP beforeLayers = m_layers;

	KisLayerSP dst = new KisLayer(this, width(), height(), nextLayerName(), OPACITY_OPAQUE);
	KisFillPainter painter(dst.data());
	painter.fillRect(0, 0, dst -> width(), dst -> height(), KoColor(0, 0, 0), OPACITY_TRANSPARENT);

	vKisLayerSP mergeLayers = layers();
	KisMerge<isVisible, isVisible> visitor(this);
	visitor(painter, mergeLayers);

	int insertIndex = -1;

	if (visitor.insertMergedAboveLayer() != 0) {
		insertIndex = index(visitor.insertMergedAboveLayer());
	}

	add(dst, insertIndex);

	notify();
	notifyLayersChanged();

	if (m_adapter && m_adapter -> undo()) {
		m_adapter -> addCommand(new KisChangeLayersCmd(m_adapter, this, beforeLayers, m_layers, i18n("Merge Visible Layers")));
	}
}

void KisImage::mergeLinkedLayers()
{
	vKisLayerSP beforeLayers = m_layers;

	KisLayerSP dst = new KisLayer(this, width(), height(), nextLayerName(), OPACITY_OPAQUE);
	KisFillPainter painter(dst.data());
	painter.fillRect(0, 0, dst -> width(), dst -> height(), KoColor(0, 0, 0), OPACITY_TRANSPARENT);

	vKisLayerSP mergeLayers = layers();
	KisMerge<isLinked, isLinked> visitor(this);
	visitor(painter, mergeLayers);

	int insertIndex = -1;

	if (visitor.insertMergedAboveLayer() != 0) {
		insertIndex = index(visitor.insertMergedAboveLayer());
	}

	add(dst, insertIndex);

	notify();
	notifyLayersChanged();

	if (m_adapter && m_adapter -> undo()) {
		m_adapter -> addCommand(new KisChangeLayersCmd(m_adapter, this, beforeLayers, m_layers, i18n("Merge Linked Layers")));
	}
}

void KisImage::mergeLayer(KisLayerSP l) 
{
	if (bottom(l)) return;

	vKisLayerSP beforeLayers = m_layers;

	KisLayerSP dst = new KisLayer(this, width(), height(), nextLayerName(), OPACITY_OPAQUE);
	KisFillPainter painter(dst.data());
	painter.fillRect(0, 0, dst -> width(), dst -> height(), KoColor(0, 0, 0), OPACITY_TRANSPARENT);

	vKisLayerSP mergeLayers;

	mergeLayers.push_back(l);
	mergeLayers.push_back(layer(index(l) + 1));


	KisMerge<isLinked, isLinked> visitor(this);
	visitor(painter, mergeLayers);

	int insertIndex = -1;

	if (visitor.insertMergedAboveLayer() != 0) {
		insertIndex = index(visitor.insertMergedAboveLayer());
	}

	add(dst, insertIndex);

	notify();
	notifyLayersChanged();

	if (m_adapter && m_adapter -> undo()) {
		m_adapter -> addCommand(new KisChangeLayersCmd(m_adapter, this, beforeLayers, m_layers, i18n("Merge Linked Layers")));
	}

}


void KisImage::enableUndo(KoCommandHistory *history)
{
	m_undoHistory = history;
}

Q_INT32 KisImage::tileNum(Q_INT32 xpix, Q_INT32 ypix) const
{
	Q_INT32 row;
	Q_INT32 col;
	Q_INT32 num;

	if (xpix >= width() || ypix >= height())
		return -1;

	row = ypix / TILE_HEIGHT;
	col = xpix / TILE_WIDTH;
	num = row * m_ntileCols + col;
	return num;
}


// Composite the specified tile onto the projection layer.
void KisImage::renderToProjection(Q_INT32 tileno)
{
	KisTileMgrSP tm = m_projection -> tiles();
	KisTileSP dst;
	KisPainter gc;
	QPoint pt;

	Q_ASSERT(tm);

	if (tileno < 0)
		return;

	if (tileno >= (m_ntileCols * m_ntileRows)) {
		m_ntileCols = (width() + TILE_WIDTH - 1) / TILE_WIDTH;
		m_ntileRows = (height() + TILE_HEIGHT - 1) / TILE_HEIGHT;

		if (tileno >= (m_ntileCols * m_ntileRows))
			return;
	}

	dst = tm -> tile(tileno, TILEMODE_WRITE);
	
	if (!dst)
		return;

	gc.begin(m_projection.data());

	tm -> tileCoord(dst, pt);
	gc.bitBlt(pt.x(), pt.y(), COMPOSITE_COPY, m_bkg.data(), pt.x(), pt.y(), dst -> width(), dst -> height());

	if (!m_layers.empty()) {
		KisFlatten<flattenAllVisible> visitor(pt.x(), pt.y(), dst -> width(), dst -> height());

		visitor(gc, m_layers);

		if (m_activeLayer -> hasSelection()) {
			KisSelectionSP s = m_activeLayer -> selection();
			visitor(gc, s);
		}
	}

	gc.end();
}


void KisImage::expand(KisPaintDeviceSP dev)
{
	Q_INT32 w;
	Q_INT32 h;

	if (dev -> width() > width() || dev -> height() > height()) {
		w = QMAX(dev -> width(), width());
		h = QMAX(dev -> height(), height());
		resize(w, h);

	}
}

void KisImage::notify()
{
	notify(0, 0, width(), height());
}

void KisImage::notify(Q_INT32 x, Q_INT32 y, Q_INT32 width, Q_INT32 height)
{
	notify(QRect(x, y, width, height));
}

void KisImage::notify(const QRect& rc)
{
	if (DISPLAY_TIMER) {
		m_displayMutex.lock();
		if (m_dirtyRect.isValid()) {
			m_dirtyRect |= rc;
		} else {
			m_dirtyRect = rc;
		}
		m_displayMutex.unlock();
	} else {
		if (rc.isValid()) {

			emit update(KisImageSP(this), rc);
		}
	}


}

void KisImage::notifyLayersChanged()
{
	emit layersChanged(KisImageSP(this));
}

QRect KisImage::bounds() const
{
	return QRect(0, 0, width(), height());
}

KisUndoAdapter* KisImage::undoAdapter() const
{
	return m_adapter;
}

KisGuideMgr *KisImage::guides() const
{
	return const_cast<KisGuideMgr*>(&m_guides);
}

KisTileMgrSP KisImage::tiles() const
{
	return m_projection -> tiles();
}

void KisImage::slotUpdateDisplay()
{
	if (m_dirtyRect.isValid()) {
		m_displayMutex.lock();
		QRect rect = m_dirtyRect;
		m_dirtyRect = QRect();
		m_displayMutex.unlock();

		emit update(KisImageSP(this), rect);
	}
}

void KisImage::slotSelectionChanged() 
{
// 	kdDebug() << "KisImage::slotSelectionChanged\n";
	emit activeSelectionChanged(KisImageSP(this));
}

void KisImage::slotSelectionCreated()
{
// 	kdDebug() << "KisImage::slotSelectionCreated\n";
	emit selectionCreated(KisImageSP(this));
}


void KisImage::startUpdateTimer()
{
	if (DISPLAY_TIMER) {
		m_updateTimer = new QTimer(this);
		connect( m_updateTimer, SIGNAL(timeout()), this, SLOT(slotUpdateDisplay()) );
		m_updateTimer -> start( DISPLAY_UPDATE_FREQUENCY );
	}
}

KisStrategyColorSpaceSP KisImage::colorStrategy() const
{
	return m_colorStrategy;
}

KisLayerSP KisImage::projection() const
{
	return m_projection;
}

#include "kis_image.moc"

