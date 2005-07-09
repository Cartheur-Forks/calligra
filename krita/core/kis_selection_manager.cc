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
#include <qcolor.h>

#include <kdebug.h>
#include <kaction.h>
#include <klocale.h>
#include <kstdaction.h>

#include <koDocument.h>
#include <koMainWindow.h>
#include <koQueryTrader.h>

#include "kis_clipboard.h"
#include "kis_types.h"
#include "kis_view.h"
#include "kis_doc.h"
#include "kis_image.h"
#include "kis_selection.h"
#include "kis_selection_manager.h"
#include "kis_painter.h"
#include "kis_iterators_pixel.h"
#include <kis_iteratorpixeltrait.h>
#include "kis_layer.h"
#include "kis_paint_device.h"
#include "kis_colorspace_registry.h"
#include "kis_dlg_apply_profile.h"
#include "kis_config.h"
#include "kis_global.h"
#include "kis_transaction.h"
#include "kis_undo_adapter.h"
#include "kis_selected_transaction.h"
#include "kis_convolution_painter.h"
#include "kis_integer_maths.h"

KisSelectionManager::KisSelectionManager(KisView * parent, KisDoc * doc)
	: m_parent(parent),
	  m_doc(doc),
	  m_copy(0),
	  m_cut(0),
	  m_paste(0),
	  m_pasteNew(0),
	  m_cutToNewLayer(0),
	  m_selectAll(0),
	  m_deselect(0),
	  m_clear(0),
	  m_reselect(0),
	  m_invert(0),
	  m_toNewLayer(0),
	  m_feather(0),
	  m_border(0),
	  m_expand(0),
	  m_smooth(0),
	  m_contract(0),
	  m_grow(0),
	  m_similar(0),
	  m_transform(0),
	  m_load(0),
	  m_save(0)
{
	m_pluginActions.setAutoDelete(true);
	m_clipboard = KisClipboard::instance();
}

KisSelectionManager::~KisSelectionManager()
{
	m_pluginActions.clear();
}


void KisSelectionManager::setup(KActionCollection * collection)
{
	// XXX: setup shortcuts!

        m_cut =
		KStdAction::cut(this,
				SLOT(cut()),
				collection,
				"cut");

        m_copy =
		KStdAction::copy(this,
				 SLOT(copy()),
				 collection,
				 "copy");

        m_paste =
		KStdAction::paste(this,
				  SLOT(paste()),
				  collection,
				  "paste");

	m_pasteNew =
		new KAction(i18n("Paste into &New Image"),
				0, 0,
				this, SLOT(pasteNew()),
				collection,
				"paste_new");


        m_selectAll =
		KStdAction::selectAll(this,
				      SLOT(selectAll()),
				      collection,
				      "select_all");

        m_deselect =
		KStdAction::deselect(this,
				     SLOT(deselect()),
				     collection,
				     "deselect");


        m_clear =
		KStdAction::clear(this,
				  SLOT(clear()),
				  collection,
				  "clear");
	
	m_reselect =
		new KAction(i18n("&Reselect"),
			    0, 0,
			    this, SLOT(reselect()),
			    collection, "reselect");
	
	m_invert =
		new KAction(i18n("&Invert"),
			    0, 0,
			    this, SLOT(invert()),
			    collection, "invert");


        m_toNewLayer =
		new KAction(i18n("Copy Selection to New Layer"),
			    "ctrl+shift+j",
			    this, SLOT(copySelectionToNewLayer()),
			    collection, "copy_selection_to_new_layer");


	m_cutToNewLayer =
		new KAction(i18n("Cut Selection to New Layer"),
			"ctrl+j",
			this, SLOT(cutToNewLayer()),
			collection, "cut_selection_to_new_layer");


	m_feather =
		new KAction(i18n("Feather..."),
			    0, 0,
			    this, SLOT(feather()),
			    collection, "feather");

#if 0 // Not implemented yet
	m_border =
		new KAction(i18n("Border..."),
			    0, 0,
			    this, SLOT(border()),
			    collection, "border");

	m_expand =
		new KAction(i18n("Expand..."),
			    0, 0,
			    this, SLOT(expand()),
			    collection, "expand");

	m_smooth =
		new KAction(i18n("Smooth..."),
			    0, 0,
			    this, SLOT(smooth()),
			    collection, "smooth");


	m_contract =
		new KAction(i18n("Contract..."),
			    0, 0,
			    this, SLOT(contract()),
			    collection, "contract");

	m_grow =
		new KAction(i18n("Grow"),
			    0, 0,
			    this, SLOT(grow()),
			    collection, "grow");

	m_similar =
		new KAction(i18n("Similar"),
			    0, 0,
			    this, SLOT(similar()),
			    collection, "similar");


	m_transform
		= new KAction(i18n("Transform..."),
			      0, 0,
			      this, SLOT(transform()),
			      collection, "transform_selection");


	m_load
		= new KAction(i18n("Load..."),
			      0, 0,
			      this, SLOT(load()),
			      collection, "load_selection");


	m_save
		= new KAction(i18n("Save As..."),
			      0, 0,
			      this, SLOT(save()),
			      collection, "save_selection");
#endif

        QClipboard *cb = QApplication::clipboard();
        connect(cb, SIGNAL(dataChanged()), SLOT(clipboardDataChanged()));
}

void KisSelectionManager::clipboardDataChanged()
{
	updateGUI();
}


void KisSelectionManager::addSelectionAction(KAction * action)
{
	m_pluginActions.append(action);
}


void KisSelectionManager::updateGUI()
{
	Q_ASSERT(m_parent);
	Q_ASSERT(m_clipboard);

	if (m_parent == 0) {
		// "Eek, no parent!
		return;
	}

	if (m_clipboard == 0) {
		// Eek, no clipboard!
		return;
	}

        KisImageSP img = m_parent -> currentImg();
	bool enable = false;
	if (img) {
		enable = img && img -> activeLayer() && img -> activeLayer() -> hasSelection();
	}
	m_copy -> setEnabled(enable);
	m_cut -> setEnabled(enable);
	m_paste -> setEnabled(img != 0 && m_clipboard -> hasClip());
	m_pasteNew -> setEnabled(img != 0 && m_clipboard -> hasClip());
	m_cutToNewLayer->setEnabled(enable);
	m_selectAll -> setEnabled(img != 0);
	m_deselect -> setEnabled(enable);
	m_clear -> setEnabled(enable);
	m_reselect -> setEnabled( ! enable);
	m_invert -> setEnabled(enable);
	m_toNewLayer -> setEnabled(enable);
	m_feather -> setEnabled(enable);
#if 0 // Not implemented yet
	m_border -> setEnabled(enable);
	m_expand -> setEnabled(enable);
	m_smooth -> setEnabled(enable);
	m_contract -> setEnabled(enable);
	m_grow -> setEnabled(enable);
	m_similar -> setEnabled(enable);
	m_transform -> setEnabled(enable);
	m_load -> setEnabled(enable);
	m_save -> setEnabled(enable);
#endif
	m_parent -> updateStatusBarSelectionLabel();

}

void KisSelectionManager::imgSelectionChanged(KisImageSP img)
{
	kdDebug(DBG_AREA_CORE) << "KisSelectionManager::imgSelectionChanged\n";
        if (img == m_parent -> currentImg()) {
                updateGUI();
		m_parent -> updateCanvas();
	}

}

void KisSelectionManager::cut()
{
        copy();
        clear();
}

void KisSelectionManager::copy()
{
        KisImageSP img = m_parent -> currentImg();
        if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	if (!layer -> hasSelection()) return;

	KisSelectionSP selection = layer -> selection();
	
	QRect r = selection -> selectedExactRect();
	
	kdDebug(DBG_AREA_CORE) << "Selection rect: "
		  << r.x() << ", "
		  << r.y() << ", "
		  << r.width() << ", "
		  << r.height() << "\n";

	KisPaintDeviceSP clip = new KisPaintDevice(img -> activeDevice() -> colorStrategy(),
						   "Copy from " + img -> activeDevice() -> name() );
	Q_CHECK_PTR(clip);

	clip -> setCompositeOp(COMPOSITE_OVER);
	clip -> setProfile(layer -> profile());

	// TODO if the source is linked... copy from all linked layers?!?

	// Copy image data
	KisPainter gc;
	gc.begin(clip);
	gc.bitBlt(0, 0, COMPOSITE_COPY, layer.data(), r.x(), r.y(), r.width(), r.height());
	gc.end();

	// Apply selection mask.

	for (Q_INT32 y = 0; y < r.height(); y++) {
		KisHLineIterator layerIt = clip -> createHLineIterator(0, y, r.width(), true);
		KisHLineIterator selectionIt = selection -> createHLineIterator(r.x(), r.y() + y, r.width(), false);

		while (!layerIt.isDone()) {
			KisPixel p = clip -> toPixel(layerIt.rawData());
			KisPixel s = selection -> toPixel(selectionIt.rawData());
			Q_UINT16 p_alpha, s_alpha;
			p_alpha = p.alpha();
			s_alpha = s.alpha();

			p.alpha() = UINT8_MULT(p_alpha, s_alpha);

			++layerIt;
			++selectionIt;
		}
	}

	kdDebug(DBG_AREA_CORE) << "Selection copied: "
			       << r.x() << ", "
			       << r.y() << ", "
			       << r.width() << ", "
			       << r.height() << "\n";


 	m_clipboard -> setClip(clip);
 	imgSelectionChanged(m_parent -> currentImg());
}


KisLayerSP KisSelectionManager::paste()
{
        KisImageSP img = m_parent -> currentImg();
        if (!img) return 0;

	KisPaintDeviceSP clip = m_clipboard -> clip();

	if (clip) {
		KisLayerSP layer = new KisLayer(img, img -> nextLayerName() + "(pasted)", OPACITY_OPAQUE);
		Q_CHECK_PTR(layer);

		QRect r = clip -> extent();
		KisPainter gc;
		gc.begin(layer.data());
		gc.bitBlt(0, 0, COMPOSITE_COPY, clip.data(), r.x(), r.y(), r.width(), r.height());
		gc.end();

		KisConfig cfg;
		if (cfg.askProfileOnPaste() && clip -> profile() == 0 && img -> profile() != 0) {
			KisDlgApplyProfile * dlg = new KisDlgApplyProfile(m_parent);
			Q_CHECK_PTR(dlg);

			if (dlg -> exec() == QDialog::Accepted) {
				KisProfileSP profile = dlg -> profile();
				if (profile != img -> profile()) {
					layer -> setProfile(profile);
					layer -> convertTo(img -> colorStrategy(), img -> profile(), dlg -> renderIntent());
				}
			}
		}


		img->layerAdd(layer, img -> index(layer));
		layer -> move(0,0);
		img -> notify();

		return layer;
	}
	return 0;
}

void KisSelectionManager::pasteNew()
{
	kdDebug() << "Paste new!\n";
	
        KisPaintDeviceSP clip = m_clipboard -> clip();
	if (!clip) return;

	QRect r = clip->exactBounds();
	if (r.width() < 1 && r.height() < 1) {
		// Don't paste empty clips
		return;
	}

	const QCString mimetype = KoDocument::readNativeFormatMimeType();
	KoDocumentEntry entry = KoDocumentEntry::queryByMimeType( mimetype );
	KisDoc * doc = (KisDoc*) entry.createDoc();

	KisImageSP img = new KisImage(doc, r.width(), r.height(), clip->colorStrategy(), "Pasted");
	KisLayerSP layer = new KisLayer(img, clip->name(), OPACITY_OPAQUE, clip->colorStrategy());
	KisPainter p(layer);
	p.bitBlt(0, 0, COMPOSITE_COPY, clip.data(), OPACITY_OPAQUE, r.x(), r.y(), r.width(), r.height());
	p.end();
	img->add(layer,0);
	
	doc->setCurrentImage( img );
	KoMainWindow *win = new KoMainWindow( doc->instance() );
	win->show();
	win->setRootDocument( doc );

}



void KisSelectionManager::selectAll()
{
        KisImageSP img = m_parent -> currentImg();
	if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	KisSelectedTransaction * t = new KisSelectedTransaction(i18n("Select &All"), layer.data());
	Q_CHECK_PTR(t);

	layer -> selection() -> clear();
	layer -> selection() -> invert();
	
	if (img -> undoAdapter())
		img -> undoAdapter() -> addCommand(t);
	layer -> emitSelectionChanged();
}


void KisSelectionManager::deselect()
{
        KisImageSP img = m_parent -> currentImg();
	if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	KisSelectedTransaction * t = new KisSelectedTransaction(i18n("&Deselect"), layer.data());
	Q_CHECK_PTR(t);
	
	// The following also emits selectionChanged
	layer -> deselect();
	
	if (img -> undoAdapter())
		img -> undoAdapter() -> addCommand(t);
}


void KisSelectionManager::clear()
{
        KisImageSP img = m_parent -> currentImg();
        if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	if (!layer -> hasSelection()) return;

	KisSelectionSP selection = layer -> selection();

	KisTransaction * t = 0;
	if (img -> undoAdapter()) {
		t = new KisTransaction("Cut", layer.data());
		Q_CHECK_PTR(t);
	}

	layer -> clearSelection();
	
	if (img -> undoAdapter()) img -> undoAdapter() -> addCommand(t);
	layer -> deselect();
}



void KisSelectionManager::reselect()
{
	KisImageSP img = m_parent -> currentImg();
	if (!img) return;

	KisLayerSP layer = img ->activeLayer();
	if (!layer) return;

	KisSelectedTransaction * t = new KisSelectedTransaction(i18n("&Reselect"), layer.data());
	Q_CHECK_PTR(t);
	
	// The following also emits selectionChanged
	layer -> selection(); // also sets hasSelection=true
	
	if (img -> undoAdapter())
		img -> undoAdapter() -> addCommand(t);
}


void KisSelectionManager::invert()
{
        KisImageSP img = m_parent -> currentImg();
	if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	if (layer -> hasSelection()) {
		KisSelectionSP s = layer -> selection();
	
		KisTransaction * t = 0;
		if (img -> undoAdapter())
		{
			t = new KisTransaction(i18n("&Invert"), s.data());
			Q_CHECK_PTR(t);
		}

		s -> invert();
	
		if (img -> undoAdapter())
			img -> undoAdapter() -> addCommand(t);
	}
	
	layer -> emitSelectionChanged();
}

void KisSelectionManager::copySelectionToNewLayer()
{
        KisImageSP img = m_parent -> currentImg();
	if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	copy();
	paste();
}

void KisSelectionManager::cutToNewLayer()
{

        KisImageSP img = m_parent -> currentImg();
	if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	cut();
	paste();
}


// XXX Krita post 1.4: Make feather radius configurable
void KisSelectionManager::feather()
{
	KisImageSP img = m_parent -> currentImg();
	if (!img) return;
	KisPaintDeviceSP dev = img -> activeDevice();
	if (!dev) return;
	
	if (!dev -> hasSelection()) {
		// activate it, but don't do anything with it
		dev -> selection();
		return;
	}

	KisSelectionSP selection = dev -> selection();

	KisSelectedTransaction * t = new KisSelectedTransaction(i18n("Feather..."), dev);
	Q_CHECK_PTR(t);


	// XXX: we should let gaussian blur & others influence alpha channels as well
	// (on demand of the caller)
	
	KisConvolutionPainter painter(selection.data());
	
	KisKernel k;
	k.width = 3;
	k.height = 3;
	k.factor = 16;
	k.offset = 0;
	k.data.push_back( 1 );
	k.data.push_back( 2 );
	k.data.push_back( 1 );
	k.data.push_back( 2 );
	k.data.push_back( 4 );
	k.data.push_back( 2 );
	k.data.push_back( 1 );
	k.data.push_back( 2 );
	k.data.push_back( 1 );
	
	QRect rect = selection -> extent();
	// Make sure we've got enough space around the edges.
	rect = QRect(rect.x() - 3, rect.y() - 3, rect.width() + 3, rect.height() + 3);
	
	painter.applyMatrix(&k, selection.data(), rect.x(), rect.y(), rect.width(), rect.height(), BORDER_AVOID, CONVOLVE_ALPHA);
	painter.end();

	if (img -> undoAdapter())
		img -> undoAdapter() -> addCommand(t);

	dev -> emitSelectionChanged();
}

// XXX: Maybe move these esoteric functions to plugins?
void KisSelectionManager::border() {}
void KisSelectionManager::expand() {}
void KisSelectionManager::smooth() {}
void KisSelectionManager::contract() {}
void KisSelectionManager::grow() {}
void KisSelectionManager::similar() {}
void KisSelectionManager::transform() {}
void KisSelectionManager::load() {}
void KisSelectionManager::save() {}


#include "kis_selection_manager.moc"
