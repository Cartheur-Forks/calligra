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
#ifndef KIS_IMAGE_H_
#define KIS_IMAGE_H_

#include <qbitarray.h>
#include <qobject.h>
#include <qstring.h>
#include <qimage.h>
#include <qvaluevector.h>
#include <qtimer.h>
#include <qmutex.h>

#include <kurl.h>

#include <koColor.h>

#include "kis_global.h"
#include "kis_types.h"
#include "kis_render.h"
#include "kis_guide.h"
#include "kis_scale_visitor.h"

class KoCommandHistory;
class KisNameServer;
class KisUndoAdapter;
class KisPainter;
class DCOPObject;

class KisImage : public QObject, public KisRenderInterface {
	Q_OBJECT

public:
	KisImage(KisUndoAdapter *undoAdapter, Q_INT32 width, Q_INT32 height,
		 KisStrategyColorSpaceSP colorStrategy, const QString& name);
	KisImage(const KisImage& rhs);
	virtual ~KisImage();
	virtual DCOPObject *dcopObject();

public:
	// Implement KisRenderInterface
	virtual Q_INT32 tileNum(Q_INT32 xpix, Q_INT32 ypix) const;
	virtual KisTileMgrSP tiles() const;

	// Composite the specified tile onto the projection layer.
	virtual void renderToProjection(Q_INT32 tileno);

	KisLayerSP projection() const;

public:
	QString name() const;
	void setName(const QString& name);

	QString nextLayerName() const;

	void resize(Q_INT32 w, Q_INT32 h);
	void resize(const QRect& rc);

	void scale(double sx, double sy, KisProgressDisplayInterface *m_progress, enumFilterType ftype = MITCHELL_FILTER);
        void rotate(double angle, KisProgressDisplayInterface *m_progress);
        
	void convertTo(KisStrategyColorSpaceSP colorStrategy);

	void enableUndo(KoCommandHistory *history);
 
	KisStrategyColorSpaceSP colorStrategy() const;

	KURL uri() const;
	void uri(const KURL& uri);

	KoUnit::Unit unit() const;
	void unit(const KoUnit::Unit& u);

	// Resolution of the image == XXX: per inch?
        double xRes();
	double yRes();
	void setResolution(double xres, double yres);

	Q_INT32 width() const;
	Q_INT32 height() const;

	Q_UINT32 depth() const;
	bool alpha() const;
	bool empty() const;
	KisTileMgrSP shadow() const;

	vKisLayerSP layers();
	const vKisLayerSP& layers() const;
	vKisChannelSP channels();
	const vKisChannelSP& channels() const;

	// Get the active painting device (layer, floating selection, mask)
	KisPaintDeviceSP activeDevice();

	KisLayerSP activeLayer();
	const KisLayerSP activeLayer() const;
	KisLayerSP activate(KisLayerSP layer);
	KisLayerSP activateLayer(Q_INT32 n);
	Q_INT32 index(const KisLayerSP &layer);
	KisLayerSP layer(const QString& name);
	KisLayerSP layer(Q_UINT32 npos);
	bool add(KisLayerSP layer, Q_INT32 position);
	void rm(KisLayerSP layer);
	bool raise(KisLayerSP layer);
	bool lower(KisLayerSP layer);
	bool top(KisLayerSP layer);
	bool bottom(KisLayerSP layer);
	bool pos(KisLayerSP layer, Q_INT32 position);
	Q_INT32 nlayers() const;
	Q_INT32 nHiddenLayers() const;
	Q_INT32 nLinkedLayers() const;

	// Merge all visible layers and discard hidden ones.
	void flatten();

	void mergeVisibleLayers();
	void mergeLinkedLayers();

	
	// Channels
	KisChannelSP activeChannel();
	KisChannelSP activate(KisChannelSP channel);
	KisChannelSP activateChannel(Q_INT32 n);
	KisChannelSP unsetActiveChannel();
	Q_INT32 index(KisChannelSP channel);
	KisChannelSP channel(const QString& name);
	KisChannelSP channel(Q_UINT32 npos);
	bool add(KisChannelSP channel, Q_INT32 position);
	void rm(KisChannelSP channel);
	bool raise(KisChannelSP channel);
	bool lower(KisChannelSP channel);
	bool pos(KisChannelSP channel, Q_INT32 position);
	Q_INT32 nchannels() const;

	void setFloatingSelection(KisFloatingSelectionSP floatingSelection);
	void unsetFloatingSelection(bool commit = true);
	KisFloatingSelectionSP floatingSelection() const;

	QRect bounds() const;

	void notify();
	void notify(Q_INT32 x, Q_INT32 y, Q_INT32 width, Q_INT32 height);
	void notify(const QRect& rc);

	void notifyLayersChanged();

	KisUndoAdapter *undoAdapter() const;
	KisGuideMgr *guides() const;

signals:
	void activeLayerChanged(KisImageSP image);
	void activeChannelChanged(KisImageSP image);
	void alphaChanged(KisImageSP image);
	void activeSelectionChanged(KisImageSP image);
	void selectionCreated(KisImageSP image);
	void floatingSelectionChanged(KisImageSP image);
	void update(KisImageSP image, const QRect& rc);
	void layersChanged(KisImageSP image);

private slots:
	void slotUpdateDisplay(); 
	void slotSelectionChanged();
	void slotSelectionCreated();

private:
	KisImage& operator=(const KisImage& rhs);
	void expand(KisPaintDeviceSP dev);
	void init(KisUndoAdapter *adapter, Q_INT32 width, Q_INT32 height,  KisStrategyColorSpaceSP colorStrategy, const QString& name);

	void startUpdateTimer();

private:
	KoCommandHistory *m_undoHistory;
	KURL m_uri;
	QString m_name;

	Q_UINT32 m_depth;

	Q_INT32 m_ntileCols;
	Q_INT32 m_ntileRows;

	double m_xres;
	double m_yres;

	KoUnit::Unit m_unit;

	KisStrategyColorSpaceSP m_colorStrategy;
	KoColorMap m_clrMap;

	bool m_dirty;
	QRect m_dirtyRect;

	KisTileMgrSP m_shadow;
	KisBackgroundSP m_bkg;
	KisLayerSP m_projection;
	vKisLayerSP m_layers;
	vKisChannelSP m_channels;
	vKisLayerSP m_layerStack;
	KisLayerSP m_activeLayer;
	KisChannelSP m_activeChannel;
	KisFloatingSelectionSP m_floatingSelection;
	KisChannelSP m_floatingSelectionMask;
	QBitArray m_visible;
	QBitArray m_active;
	bool m_alpha;
	bool m_maskEnabled;
	bool m_maskInverted;
	KoColor m_maskClr;
	KisNameServer *m_nserver;
	KisUndoAdapter *m_adapter;
	KisGuideMgr m_guides;

	QTimer * m_updateTimer;
	QMutex m_displayMutex;
	DCOPObject *m_dcop;
};

#endif // KIS_IMAGE_H_
