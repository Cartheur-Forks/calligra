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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qtl.h>
#include <kdebug.h>
#include <kpixmapio.h>
#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_paint_device_visitor.h"
#include "kis_painter.h"
#include "kis_layer.h"
#include "kis_background.h"
#include "kis_channel.h"
#include "kis_doc.h"
#include "kis_mask.h"
#include "kis_nameserver.h"
#include "kis_selection.h"
#include "kistile.h"
#include "kistilemgr.h"
#include "kispixeldata.h"
#include "visitors/kis_flatten.h"

KisImage::KisImage(KisDoc *doc, Q_INT32 width, Q_INT32 height, QUANTUM opacity, const enumImgType& imgType, const QString& name)
{
	init(doc, width, height, opacity, imgType, name);
	setName(name);
}

KisImage::KisImage(const KisImage& rhs) : QObject(), KisRenderInterface(rhs)
{
	if (this != &rhs) {
		m_undoHistory = rhs.m_undoHistory;
		m_uri = rhs.m_uri;
		m_name = QString::null;
		m_author = rhs.m_author;
		m_email = rhs.m_email;
		m_width = rhs.m_width;
		m_height = rhs.m_height;
		m_depth = rhs.m_depth;
		m_ntileCols = rhs.m_ntileCols;
		m_ntileRows = rhs.m_ntileRows;
		m_opacity = rhs.m_opacity;
		m_xres = rhs.m_xres;
		m_yres = rhs.m_yres;
		m_unit = rhs.m_unit;
		m_type = rhs.m_type;
		m_clrMap = rhs.m_clrMap;
		m_dirty = rhs.m_dirty;

		if (rhs.m_shadow)
			m_shadow = new KisTileMgr(*rhs.m_shadow);

		m_bkg = new KisBackground(this, m_width, m_height);
		m_projectionPixmaps.resize(m_ntileCols * m_ntileRows);
		m_projection = new KisLayer(this, m_width, m_height, "projection", OPACITY_OPAQUE);
		m_layers.reserve(rhs.m_layers.size());

		for (vKisLayerSP_cit it = rhs.m_layers.begin(); it != rhs.m_layers.end(); it++) {
			KisLayerSP layer = new KisLayer(**it);

			layer -> setImage(KisImageSP(this));
			m_layers.push_back(layer);	
			m_layerStack.push_back(layer);
			m_activeLayer = layer;
		}

		m_channels.reserve(rhs.m_channels.size());

		for (vKisChannelSP_cit it = rhs.m_channels.begin(); it != rhs.m_channels.end(); it++) {
			KisChannelSP channel = new KisChannel(**it);

			channel -> setImage(KisImageSP(this));
			m_channels.push_back(channel);
			m_activeChannel = channel;
		}

		if (rhs.m_selectionMask)
			m_selectionMask = new KisChannel(*m_selectionMask);
		
		m_visible = rhs.m_visible;
		m_active = rhs.m_active;
		m_alpha = rhs.m_alpha;
		m_maskEnabled = rhs.m_maskEnabled;
		m_maskInverted = rhs.m_maskInverted;
		m_maskClr = rhs.m_maskClr;
		m_nserver = new KisNameServer("Layer %1", rhs.m_nserver -> currentSeed() + 1);
	}
}

KisImage::~KisImage()
{
	delete m_nserver;
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

QString KisImage::nextLayerName() const
{
	if (m_nserver -> currentSeed() == 0) {
		m_nserver -> number();
		return "background";
	}

	return m_nserver -> name();
}

QString KisImage::author() const
{
	return m_author;
}

void KisImage::setAuthor(const QString& author)
{
	m_author = author;
}

QString KisImage::email() const
{
	return m_email;
}

void KisImage::setEmail(const QString& email)
{
	m_email = email;
}

void KisImage::init(KisDoc *, Q_INT32 width, Q_INT32 height, QUANTUM opacity, const enumImgType& imgType, const QString& name)
{
	Q_INT32 n;

	m_nserver = new KisNameServer("Layer %1", 0);
	n = ::imgTypeDepth(imgType);
	m_active.resize(n);
	m_visible.resize(n);
	m_name = name;
	m_width = width;
	m_height = height;
	m_depth = n;
	m_bkg = new KisBackground(this, m_width, m_height);
	m_projection = new KisLayer(this, m_width, m_height, "projection", OPACITY_OPAQUE);
	m_xres = 1.0;
	m_yres = 1.0;
	m_unit = KoUnit::U_PT;
	m_type = imgType;
	m_dirty = false;
	m_maskEnabled = false;
	m_maskInverted = false;
	m_alpha = false;
	m_opacity = opacity;
	m_undoHistory = 0;
	m_ntileCols = (width + TILE_WIDTH - 1) / TILE_WIDTH;
	m_ntileRows = (height + TILE_HEIGHT - 1) / TILE_HEIGHT;
	m_projectionPixmaps.resize(m_ntileCols * m_ntileRows);
}

void KisImage::resize(Q_INT32 w, Q_INT32 h)
{
	if (m_width < w)
		m_width = w;

	if (m_height < h)
		m_height = h;

	m_projection = new KisLayer(this, m_width, m_height, "image projection", OPACITY_OPAQUE);
	m_bkg = new KisBackground(this, m_width, m_height);
	m_projectionPixmaps.clear();
}

void KisImage::resize(const QRect& rc)
{
	resize(rc.width(), rc.height());
}

enumImgType KisImage::imgType() const
{
	switch (m_type) {
	case IMAGE_TYPE_INDEXED:
		return IMAGE_TYPE_INDEXEDA;
	case IMAGE_TYPE_GREY:
		return IMAGE_TYPE_GREYA;
	case IMAGE_TYPE_RGB:
		return IMAGE_TYPE_RGBA;
	case IMAGE_TYPE_CMYK:
		return IMAGE_TYPE_CMYKA;
	case IMAGE_TYPE_LAB:
		return IMAGE_TYPE_LABA;
	case IMAGE_TYPE_YUV:
		return IMAGE_TYPE_YUVA;
	default:
		return m_type;
	}

	return m_type;
}

enumImgType KisImage::nativeImgType() const
{
	return alpha() ? imgTypeWithAlpha() : imgType();
}

enumImgType KisImage::imgTypeWithAlpha() const
{
	switch (m_type) {
	case IMAGE_TYPE_INDEXEDA:
		return IMAGE_TYPE_INDEXED;
	case IMAGE_TYPE_GREYA:
		return IMAGE_TYPE_GREY;
	case IMAGE_TYPE_RGBA:
		return IMAGE_TYPE_RGB;
	case IMAGE_TYPE_CMYKA:
		return IMAGE_TYPE_CMYK;
	case IMAGE_TYPE_LABA:
		return IMAGE_TYPE_LAB;
	case IMAGE_TYPE_YUVA:
		return IMAGE_TYPE_YUV;
	default:
		return m_type;
	}

	return m_type;
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

void KisImage::resolution(double xres, double yres)
{
	m_xres = xres;
	m_yres = yres;
}

void KisImage::resolution(double *xres, double *yres)
{
	if (xres && yres) {
		*xres = m_xres;
		*yres = m_yres;
	}
}

Q_INT32 KisImage::width() const
{
	return m_width;
}

Q_INT32 KisImage::height() const
{
	return m_height;
}

Q_UINT32 KisImage::depth() const
{
	return m_depth;
}

bool KisImage::alpha() const
{
	return m_alpha;
}

bool KisImage::empty() const
{
	return m_layers.size() > 0;
}

bool KisImage::colorMap(KoColorMap& cm)
{
	if (m_clrMap.empty())
		return false;

	cm = m_clrMap;
	return true;
}

KisChannelSP KisImage::mask()
{
	return m_selectionMask;
}

KoColor KisImage::color() const
{
	return KoColor();
}

KoColor KisImage::transformColor() const
{
	return KoColor();
}

KisTileMgrSP KisImage::shadow()
{
	return m_shadow;
}

void KisImage::activeComponent(CHANNELTYPE type, bool value)
{
	PIXELTYPE i = pixelFromChannel(type);

	if (i != PIXEL_UNDEF)
		m_active[i] = value;
}

bool KisImage::activeComponent(CHANNELTYPE type)
{
	PIXELTYPE i = pixelFromChannel(type);

	if (i != PIXEL_UNDEF)
		return m_active[i];

	return false;
}

void KisImage::visibleComponent(CHANNELTYPE type, bool value)
{
	PIXELTYPE i = pixelFromChannel(type);

	if (i != PIXEL_UNDEF && value != m_visible[i]) {
		m_visible[i] = value;
		emit visibilityChanged(KisImageSP(this), type);
	}
}

bool KisImage::visibleComponent(CHANNELTYPE type) const
{
	PIXELTYPE i = pixelFromChannel(type);

	return i != PIXEL_UNDEF && m_visible[i];
}

void KisImage::flush()
{
}

vKisLayerSP KisImage::layers()
{
	return m_layers;
}

const vKisLayerSP& KisImage::layers() const
{
	return m_layers;
}

vKisChannelSP KisImage::channels()
{
	return m_channels;
}

const vKisChannelSP& KisImage::channels() const
{
	return m_channels;
}

KisPaintDeviceSP KisImage::activeDevice()
{
	if (m_selection)
		return m_selection.data();

	if (m_activeChannel)
		return m_activeChannel.data();

	if (m_activeLayer) {
		if (m_activeLayer -> mask())
			return m_activeLayer -> mask().data();

		return m_activeLayer.data();
	}

	return 0;
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

Q_INT32 KisImage::index(KisLayerSP layer)
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
	bool alpha = false;

	if (layer == 0)
		return false;
	
	if (layer -> image() && layer -> image() != KisImageSP(this))
		return false;

	if (qFind(m_layers.begin(), m_layers.end(), layer) != m_layers.end())
		return false;

//	undo_push_layer_add 
	layer -> setImage(KisImageSP(this));

	if (layer -> mask())
		layer -> mask() -> setImage(KisImageSP(this));

	if (position == -1) {
		KisLayerSP active = activeLayer();

		position = active ? index(active) : 0;
	}

	if (position == 0 && m_selection && m_selection != layer)
		position = 1;

	if (m_layers.size() == 1 && m_layers[0] -> alpha())
		alpha = true;

	m_layers.insert(m_layers.begin() + position, layer);
	activate(layer);
	layer -> update();

	if (alpha)
		emit alphaChanged(KisImageSP(this));

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

//	undo_push_layer_remove
	m_layers.erase(it);
	it = qFind(m_layerStack.begin(), m_layerStack.end(), layer);

	if (it != m_layerStack.end())
		m_layerStack.erase(it);

	if (m_selection == layer) {
		m_selection = 0;
		emit selectionChanged(KisImageSP(this));
	}

	if (layer == m_activeLayer) {
		if (m_layers.empty()) {
			m_activeLayer = 0;
			emit activeLayerChanged(KisImageSP(this));
		} else {
			activate(m_layerStack[0]);
		}
	}

	rc = layer -> bounds();
	invalidate(rc.x(), rc.y(), rc.width(), rc.height());

	if (m_layers.size() == 1 && m_layers[0] -> alpha())
		emit alphaChanged(KisImageSP(this));
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

	//undo_push_layer_reposition (gimage, layer);
	qSwap(m_layers[old], m_layers[position]);	
	invalidate();
	return true;
}

Q_INT32 KisImage::nlayers() const
{
	return m_layers.size();
}

KisChannelSP KisImage::activeChannel()
{
	return m_activeChannel;
}

KisChannelSP KisImage::activate(KisChannelSP channel)
{
	if (m_channels.empty() || !channel)
		return 0;

	if (m_selection)
		return 0;

	if (qFind(m_channels.begin(), m_channels.end(), channel) == m_channels.end())
		channel = m_channels[0];

	if (channel != m_activeChannel) {
		m_activeChannel = channel;
		emit activeChannelChanged(KisImageSP(this));
	}

	return channel;
}

KisChannelSP KisImage::activateChannel(Q_INT32 n)
{
	if (n < 0 || static_cast<Q_UINT32>(n) > m_channels.size())
		return 0;

	return activate(m_channels[n]);
}

KisChannelSP KisImage::unsetActiveChannel()
{
	KisChannelSP channel = activeChannel();

	if (channel) {
		m_activeChannel = 0;
		emit activeChannelChanged(KisImageSP(this));

		if (!m_layerStack.empty()) {
			KisLayerSP layer = *m_layerStack.begin();

			activate(layer);
		}
	}

	return channel;
}

Q_INT32 KisImage::index(KisChannelSP channel)
{
	for (Q_UINT32 i = 0; i < m_channels.size(); i++) {
		if (m_channels[i] == channel)
			return i;
	}

	return -1;
}

KisChannelSP KisImage::channel(const QString& name)
{
	for (vKisChannelSP_it it = m_channels.begin(); it != m_channels.end(); it++) {
		if ((*it) -> name() == name)
			return *it;
	}

	return 0;

}

KisChannelSP KisImage::channel(Q_UINT32 npos)
{
	if (npos >= m_channels.size())
		return 0;

	return m_channels[npos];
}

bool KisImage::add(KisChannelSP channel, Q_INT32 position)
{
	if (channel == 0)
		return false;

	if (channel -> image() && channel -> image() != KisImageSP(this))
		return false;

	if (qFind(m_channels.begin(), m_channels.end(), channel) != m_channels.end())
		return false;

	//undo_push_channel_add

	if (position == -1) {
		KisChannelSP active = activeChannel();

		position = active ? index(active) : 0;
	}

	m_channels.insert(m_channels.begin() + position, channel);
	activate(channel);

	if (channel -> visible())
		channel -> update();

	expand(channel.data());
	return true;
}

void KisImage::rm(KisChannelSP channel)
{
	vKisChannelSP_it it;

	if (channel == 0)
		return;

	it = qFind(m_channels.begin(), m_channels.end(), channel);

	if (it == m_channels.end())
		return;

	//undo_push_channel_remove
	m_channels.erase(it);

	if (channel == activeChannel()) {
		if (m_channels.empty())
			unsetActiveChannel();
		else
			activate(m_channels[0]);
	}
}

bool KisImage::raise(KisChannelSP channel)
{
	Q_INT32 position;

	if (channel == 0)
		return false;

	position = index(channel);

	if (position == 0)
		return false;

	return pos(channel, position - 1);
}

bool KisImage::lower(KisChannelSP channel)
{
	Q_INT32 position;
	Q_INT32 size;

	if (channel == 0)
		return false;

	position = index(channel);
	size = m_channels.size();

	if (position == size - 1)
		return false;

	return pos(channel, position + 1);
}

bool KisImage::pos(KisChannelSP channel, Q_INT32 position)
{
	Q_INT32 old;
	Q_INT32 nchannels;

	if (channel == 0)
		return false;

	old = index(channel);

	if (old < 0)
		return false;

	nchannels = m_channels.size();
	position = CLAMP(position, 0, nchannels - 1);

	if (position == old)
		return true;

	//undo_push_channel_reposition (gimage, channel);
	qSwap(m_channels[old], m_channels[position]);
	channel -> update();
	return true;
}

Q_INT32 KisImage::nchannels() const
{
	return m_channels.size();
}

bool KisImage::boundsLayer()
{
	return false;
}

KisLayerSP KisImage::corrolateLayer(Q_INT32 , Q_INT32 )
{
	return 0;
}

void KisImage::enableUndo(KCommandHistory *history)
{
	m_undoHistory = history;
}

PIXELTYPE KisImage::pixelFromChannel(CHANNELTYPE type) const
{
	PIXELTYPE i = PIXEL_UNDEF;

	switch (type) {
		case REDCHANNEL:
			i = PIXEL_RED;
			break;
		case GREENCHANNEL:
			i = PIXEL_GREEN;
			break;
		case BLUECHANNEL:
			i = PIXEL_BLUE;
			break;
		case GRAYCHANNEL:
			i = PIXEL_GRAY;
			break;
		case INDEXEDCHANNEL:
			i = PIXEL_INDEXED;
			break;
		case ALPHACHANNEL:
			switch (imgType()) {
				case IMAGE_TYPE_GREY:
					i = PIXEL_GRAY_ALPHA;
					break;
				case IMAGE_TYPE_INDEXED:
					i = PIXEL_INDEXED_ALPHA;
					break;
				default:
					i = PIXEL_ALPHA;
					break;
			}
			break;
		default:
			break;
	}

	return i;
}

Q_INT32 KisImage::tileNum(Q_INT32 xpix, Q_INT32 ypix) const
{
	Q_INT32 row;
	Q_INT32 col;
	Q_INT32 num;

	if (xpix >= m_width || ypix >= m_height)
		return -1;

	row = ypix / TILE_HEIGHT;
	col = xpix / TILE_WIDTH;
	num = row * m_ntileCols + col;
	return num;
}

void KisImage::invalidate(Q_INT32 tileno)
{
	m_projection -> invalidate(tileno);
}

void KisImage::invalidate(Q_INT32 x, Q_INT32 y, Q_INT32 w, Q_INT32 h)
{
	m_projection -> invalidate(x, y, w, h);
}

void KisImage::invalidate(const QRect& rc)
{
	m_projection -> invalidate(rc);
}

void KisImage::invalidate()
{
	m_projection -> invalidate();
}

QPixmap KisImage::pixmap(Q_INT32 tileNo)
{
	KisTileMgrSP tm = m_projection -> data();
	KisTileSP src;

	Q_ASSERT(tm);
	src = tm -> tile(tileNo, TILEMODE_READ);

	if (src && src -> valid())
		return m_projectionPixmaps[tileNo];

	return m_projectionPixmaps[tileNo] = recreatePixmap(tileNo);
}

QPixmap KisImage::recreatePixmap(Q_INT32 tileNo)
{
	KisTileMgrSP tm = m_projection -> data();
	KisTileSP dst;
	KisPainter gc;
	QPoint pt;

	Q_ASSERT(tm);

	if (tileNo < 0)
		return QPixmap();

	if (static_cast<Q_UINT32>(tileNo) >= m_projectionPixmaps.size()) {
		m_ntileCols = (width() + TILE_WIDTH - 1) / TILE_WIDTH;
		m_ntileRows = (height() + TILE_HEIGHT - 1) / TILE_HEIGHT;
		m_projectionPixmaps.resize(m_ntileCols * m_ntileRows);

		if (static_cast<Q_UINT32>(tileNo) >= m_projectionPixmaps.size())
			return QPixmap();
	}

	dst = tm -> tile(tileNo, TILEMODE_WRITE);

	if (dst -> valid())
		return m_projectionPixmaps[tileNo];

	gc.begin(m_projection.data());
	tm -> tileCoord(dst, pt);
	gc.bitBlt(pt.x(), pt.y(), COMPOSITE_COPY, m_bkg.data(), pt.x(), pt.y(), dst -> width(), dst -> height());

	KisFlatten<flattenAllVisible> visitor(pt.x(), pt.y(), dst -> width(), dst -> height());

	visitor(gc, m_layers);

	if (m_selection)
		visitor(gc, m_selection);

	gc.end();
	renderProjection(m_projectionPixmaps[tileNo], dst);
	// TODO Draw the selection outline
	dst -> valid(true);
	return m_projectionPixmaps[tileNo];
}

void KisImage::setSelection(KisSelectionSP selection)
{
	if (m_selection != selection) {
		unsetSelection();
		m_selection = selection;
		invalidate(m_selection -> bounds());
		emit selectionChanged(this);
	}
}

void KisImage::unsetSelection(bool commit)
{
	if (m_selection) {
		QRect rc = m_selection -> bounds();

		if (commit)
			m_selection -> commit();

		m_selection = 0;
		invalidate(rc);
		emit selectionChanged(KisImageSP(this));
	}
}

KisSelectionSP KisImage::selection() const
{
	return m_selection;
}

void KisImage::expand(KisPaintDeviceSP dev)
{
	if (dev -> width() >= width() || dev -> height() >= height()) {
		resize(dev -> width(), dev -> height());
		invalidate();
	}
}

void KisImage::renderProjection(QPixmap& dst, KisTileSP src)
{
	if (!src)
		return;

	if (dst.width() < src -> width() || dst.height() < src -> height())
		dst.resize(src -> width(), src -> height());

	if (src) {
		QImage image;

		src -> lock();
		image = src -> convertToImage();
		src -> release();

		if (!image.isNull())
			dst = m_pixmapIO.convertToPixmap(image);
	}
}

void KisImage::notify()
{
	invalidate(0, 0, width(), height());
	emit update(KisImageSP(this), QRect(0, 0, width(), height()));
}

void KisImage::notify(Q_INT32 x, Q_INT32 y, Q_INT32 width, Q_INT32 height)
{
	invalidate(x, y, width, height);
	emit update(KisImageSP(this), QRect(x, y, width, height));
}

void KisImage::notify(const QRect& rc)
{
	invalidate(rc);
	emit update(KisImageSP(this), rc);
}

QRect KisImage::bounds() const
{
	return QRect(0, 0, width(), height());
}

#include "kis_image.moc"

