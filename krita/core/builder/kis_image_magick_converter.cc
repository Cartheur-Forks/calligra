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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <magick/api.h>

#include <qfile.h>
#include <qstring.h>

#include <kdeversion.h>
#include <kdebug.h>
#include <kapplication.h>
#include <klocale.h>
#include <kurl.h>
#include <kio/netaccess.h>

#include <qcolor.h>

#include "kis_types.h"
#include "kis_global.h"
#include "kis_doc.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_undo_adapter.h"
#include "kis_image_magick_converter.h"
#include "kis_colorspace_registry.h"
#include "tiles/kis_iterator.h"

#include "../../../config.h"


namespace {

	const PIXELTYPE PIXEL_BLUE = 0;
	const PIXELTYPE PIXEL_GREEN = 1;
	const PIXELTYPE PIXEL_RED = 2;
	const PIXELTYPE PIXEL_ALPHA = 3;

#if 0 // AUTOLAYER
	inline
	void pp2tile(KisPixelDataSP pd, const PixelPacket *pp)
	{
		Q_INT32 i;
		Q_INT32 j;
		QUANTUM *pixel = pd -> data;

		for (j = 0; j < pd -> height; j++) {
			for (i = 0; i < pd -> width; i++) {
				pixel[PIXEL_RED] = Downscale(pp -> red);
				pixel[PIXEL_GREEN] = Downscale(pp -> green);
				pixel[PIXEL_BLUE] = Downscale(pp -> blue);
				pixel[PIXEL_ALPHA] = OPACITY_OPAQUE - Downscale(pp -> opacity);
// 				kdDebug() << (int)pixel[PIXEL_ALPHA] << " " << (int)pixel[PIXEL_RED] << " " << pp -> red << " " <<  pp -> green << " " << pp -> blue << " " << MaxRGB << " " << pp -> opacity << " " << ( pp -> opacity * QUANTUM_MAX ) / MaxRGB << endl;
				pixel += pd -> depth;
				pp++;
			}
		}
	}

	inline
	void tile2pp(PixelPacket *pp, const KisPixelDataSP pd)
	{
		Q_INT32 i;
		Q_INT32 j;
		QUANTUM *pixel = pd -> data;

		for (j = 0; j < pd -> height; j++) {
			for (i = 0; i < pd -> width; i++) {
				pp -> red = Upscale(pixel[PIXEL_RED]);
				pp -> green = Upscale(pixel[PIXEL_GREEN]);
				pp -> blue = Upscale(pixel[PIXEL_BLUE]);
				pp -> opacity = Upscale(OPACITY_OPAQUE - pixel[PIXEL_ALPHA]);
				kdDebug() << pp -> opacity << " " << pixel[PIXEL_ALPHA] << endl;
				pixel += pd -> depth;
				pp++;
			}
		}
	}
#endif //AUTOLAYER
	void InitGlobalMagick()
	{
		static bool init = false;

		if (!init) {
			KApplication *app = KApplication::kApplication();

			InitializeMagick(*app -> argv());
			atexit(DestroyMagick);
			init = true;
		}
	}

	/*
	 * ImageMagick progress monitor callback.  Unfortunately it doesn't support passing in some user
	 * data which complicates things quite a bit.  The plan was to allow the user start multiple
	 * import/scans if he/she so wished.  However, without passing user data it's not possible to tell
	 * on which task we have made progress on.
	 *
	 * Additionally, ImageMagick is thread-safe, not re-entrant... i.e. IM does not relinquish held
	 * locks when calling user defined callbacks, this means that the same thread going back into IM
	 * would deadlock since it would try to acquire locks it already holds.
	 */
#ifdef HAVE_MAGICK6
	MagickBooleanType monitor(const char *text, const ExtendedSignedIntegralType, const ExtendedUnsignedIntegralType, ExceptionInfo *)
	{
		KApplication *app = KApplication::kApplication();

		Q_ASSERT(app);

		if (app -> hasPendingEvents())
			app -> processEvents();

		printf("%s\n", text);
		return MagickTrue;
	}
#else
	unsigned int monitor(const char *text, const ExtendedSignedIntegralType, const ExtendedUnsignedIntegralType, ExceptionInfo *)
	{
		KApplication *app = KApplication::kApplication();

		Q_ASSERT(app);

		if (app -> hasPendingEvents())
			app -> processEvents();

		printf("%s\n", text);
		return true;
	}
#endif

}

KisImageMagickConverter::KisImageMagickConverter(KisDoc *doc, KisUndoAdapter *adapter)
{
	InitGlobalMagick();
	init(doc, adapter);
	SetMonitorHandler(monitor);
	m_stop = false;
}

KisImageMagickConverter::~KisImageMagickConverter()
{
}

KisImageBuilder_Result KisImageMagickConverter::decode(const KURL& uri, bool isBlob)
{
	Image *image;
	Image *images;
	ExceptionInfo ei;
	ImageInfo *ii;

	if (m_stop) {
		m_img = 0;
		return KisImageBuilder_RESULT_INTR;
	}

	GetExceptionInfo(&ei);
	ii = CloneImageInfo(0);

	if (isBlob) {
		// TODO : Test.  Does BlobToImage even work?
		Q_ASSERT(uri.isEmpty());
		images = BlobToImage(ii, &m_data[0], m_data.size(), &ei);
	} else {
		qstrncpy(ii -> filename, QFile::encodeName(uri.path()), MaxTextExtent - 1);

		if (ii -> filename[MaxTextExtent - 1]) {
			emit notifyProgressError(this);
			return KisImageBuilder_RESULT_PATH;
		}

		images = ReadImage(ii, &ei);
	}

	if (ei.severity != UndefinedException)
		CatchException(&ei);

	if (images == 0) {
		DestroyImageInfo(ii);
		DestroyExceptionInfo(&ei);
		emit notifyProgressError(this);
		return KisImageBuilder_RESULT_FAILURE;
	}
// Autolayer removed the following line
// 	m_img = new KisImage(m_adapter, 0, 0, KisColorSpaceRegistry::instance() -> get("RGBA"), m_doc -> nextImageName());

	emit notifyProgressStage(this, i18n("Importing..."), 0);

	m_img = 0;
	
	while ((image = RemoveFirstImageFromList(&images))) {
		ViewInfo *vi = OpenCacheView(image);

		if( ! m_img)
			m_img = new KisImage(m_adapter, image -> columns, image -> rows,
						KisColorSpaceRegistry::instance() -> get("RGBA"), m_doc -> nextImageName());

		if (image -> columns && image -> rows) {
			KisLayerSP layer = new KisLayer(m_img, m_img -> nextLayerName(), OPACITY_OPAQUE);

			m_img->add(layer, 0);

			for (Q_INT32 y = 0; y < image->rows; y ++)
			{
				const PixelPacket *pp = AcquireCacheView(vi, 0, y, image->columns, 1, &ei);

				if(!pp)
				{
					CloseCacheView(vi);
					DestroyImageList(images);
					DestroyImageInfo(ii);
					DestroyExceptionInfo(&ei);
					emit notifyProgressError(this);
					return KisImageBuilder_RESULT_FAILURE;
				}

				KisHLineIterator hiter = layer->createHLineIterator(0, y, image->columns, true);
				while(! hiter.isDone())
				{
					Q_UINT8 *ptr= (Q_UINT8 *)hiter;
					// XXX: not colorstrategy and bitdepth independent
					*(ptr++) = pp->blue;
					*(ptr++) = pp->green;
					*(ptr++) = pp->red;
					*(ptr++) = OPACITY_OPAQUE - pp->opacity;
			
					pp++;	
					hiter++;
				}
				
				emit notifyProgress(this, y * 100 / image->rows);

				if (m_stop) {
					CloseCacheView(vi);
					DestroyImageList(images);
					DestroyImageInfo(ii);
					DestroyExceptionInfo(&ei);
					m_img = 0;
					return KisImageBuilder_RESULT_INTR;
				}
			}
		}
		
		emit notifyProgressDone(this);
		CloseCacheView(vi);
		DestroyImage(image);
	}

	emit notifyProgressDone(this);
	DestroyImageList(images);
	DestroyImageInfo(ii);
	DestroyExceptionInfo(&ei);
	return KisImageBuilder_RESULT_OK;
}

KisImageBuilder_Result KisImageMagickConverter::buildImage(const KURL& uri)
{
	if (uri.isEmpty())
		return KisImageBuilder_RESULT_NO_URI;

	if (!KIO::NetAccess::exists(uri, false, qApp -> mainWidget())) {
		return KisImageBuilder_RESULT_NOT_EXIST;
	}

#if 1
	// We're not set up to handle asynchronous loading at the moment.
	KisImageBuilder_Result result = KisImageBuilder_RESULT_FAILURE;
	QString tmpFile;

	if (KIO::NetAccess::download(uri, tmpFile, qApp -> mainWidget())) {
		result = decode(tmpFile, false);
		KIO::NetAccess::removeTempFile(tmpFile);
	}

	return result;
#else
	if (!uri.isLocalFile()) {
		if (m_job)
			return KisImageBuilder_RESULT_BUSY;

		m_data.resize(0);
		m_job = KIO::get(uri, false, false);
		connect(m_job, SIGNAL(result(KIO::Job*)), SLOT(ioResult(KIO::Job*)));
		connect(m_job, SIGNAL(totalSize(KIO::Job*, KIO::filesize_t)), this, SLOT(ioTotalSize(KIO::Job*, KIO::filesize_t)));
		connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)), this, SLOT(ioData(KIO::Job*, const QByteArray&)));
		return KisImageBuilder_RESULT_PROGRESS;
	}

	return decode(uri, false);
#endif
}


KisImageSP KisImageMagickConverter::image()
{
	return m_img;
}

void KisImageMagickConverter::init(KisDoc *doc, KisUndoAdapter *adapter)
{
	m_doc = doc;
	m_adapter = adapter;
	m_job = 0;
}

KisImageBuilder_Result buildFile(const KURL&, KisImageSP)
{
	return KisImageBuilder_RESULT_UNSUPPORTED;
}

KisImageBuilder_Result KisImageMagickConverter::buildFile(const KURL& uri, KisLayerSP layer)
{
#if 0 //AUTOLAYER
	Image *image;
	ExceptionInfo ei;
	ImageInfo *ii;
	Q_INT32 w;
	Q_INT32 h;
	KisTileMgrSP tm;
	Q_INT32 ntile = 0;
	Q_INT32 totalTiles;

	if (!layer)
		return KisImageBuilder_RESULT_INVALID_ARG;

	if (uri.isEmpty())
		return KisImageBuilder_RESULT_NO_URI;

	if (!uri.isLocalFile())
		return KisImageBuilder_RESULT_NOT_LOCAL;

	GetExceptionInfo(&ei);
	ii = CloneImageInfo(0);
	qstrncpy(ii -> filename, QFile::encodeName(uri.path()), MaxTextExtent - 1);

	if (ii -> filename[MaxTextExtent - 1]) {
		emit notifyProgressError(this);
		return KisImageBuilder_RESULT_PATH;
	}

	if (!layer -> width() || !layer -> height())
		return KisImageBuilder_RESULT_EMPTY;

	image = AllocateImage(ii);
	tm = layer -> tiles();
	image -> columns = layer -> width();
	image -> rows = layer -> height();
#ifdef HAVE_MAGICK6
	if ( layer-> alpha() )
		image -> matte = MagickTrue;
	else
		image -> matte = MagickFalse;
#else       
	image -> matte = layer -> alpha();
#endif
	w = TILE_WIDTH;
	h = TILE_HEIGHT;
	totalTiles = ((image -> columns + TILE_WIDTH - 1) / TILE_WIDTH) * ((image -> rows + TILE_HEIGHT - 1) / TILE_HEIGHT);

	for (Q_INT32 y = 0; y < layer -> height(); y += TILE_HEIGHT) {
		if ((y + h) > layer -> height())
			h = TILE_HEIGHT + layer -> height() - (y + h);

		for (Q_INT32 x = 0; x < layer -> width(); x += TILE_WIDTH) {
			if ((x + w) > layer -> width())
				w = TILE_WIDTH + layer -> width() - (x + w);

			KisPixelDataSP pd = tm -> pixelData(x, y, x + w - 1, y + h - 1, TILEMODE_READ);
			PixelPacket *pp = SetImagePixels(image, x, y, w, h);

			if (!pd || !pp) {
				if (pp)
					SyncImagePixels(image);

				DestroyExceptionInfo(&ei);
				DestroyImage(image);
				emit notifyProgressError(this);
				return KisImageBuilder_RESULT_FAILURE;
			}

			ntile++;
			emit notifyProgressStage(this, i18n("Saving..."), ntile * 100 / totalTiles);
			tile2pp(pp, pd);
			SyncImagePixels(image);
			w = TILE_WIDTH;
		}

		h = TILE_HEIGHT;
	}

	WriteImage(ii, image);
	DestroyExceptionInfo(&ei);
	DestroyImage(image);
	emit notifyProgressDone(this);
	return KisImageBuilder_RESULT_OK;
#endif // AUTOLAYER (below return is a hack)
		return KisImageBuilder_RESULT_EMPTY;
}

void KisImageMagickConverter::ioData(KIO::Job *job, const QByteArray& data)
{
	if (data.isNull() || data.isEmpty()) {
		emit notifyProgressStage(this, i18n("Loading..."), 0);
		return;
	}

	if (m_data.empty()) {
		Image *image;
		ImageInfo *ii;
		ExceptionInfo ei;

		ii = CloneImageInfo(0);
		GetExceptionInfo(&ei);
		image = PingBlob(ii, data.data(), data.size(), &ei);

		if (image == 0 || ei.severity == BlobError) {
			DestroyExceptionInfo(&ei);
			DestroyImageInfo(ii);
			job -> kill();
			emit notifyProgressError(this);
			return;
		}

		DestroyImage(image);
		DestroyExceptionInfo(&ei);
		DestroyImageInfo(ii);
		emit notifyProgressStage(this, i18n("Loading..."), 0);
	}

	Q_ASSERT(data.size() + m_data.size() <= m_size);
	memcpy(&m_data[m_data.size()], data.data(), data.count());
	m_data.resize(m_data.size() + data.count());
	emit notifyProgressStage(this, i18n("Loading..."), m_data.size() * 100 / m_size);

	if (m_stop)
		job -> kill();
}

void KisImageMagickConverter::ioResult(KIO::Job *job)
{
	m_job = 0;

	if (job -> error())
		emit notifyProgressError(this);

	decode(KURL(), true);
}

void KisImageMagickConverter::ioTotalSize(KIO::Job * /*job*/, KIO::filesize_t size)
{
	m_size = size;
	m_data.reserve(size);
	emit notifyProgressStage(this, i18n("Loading..."), 0);
}

void KisImageMagickConverter::cancel()
{
	m_stop = true;
}

/**
 * @name readFilters
 * @return Provide a list of file formats the application can read.
 */
QString KisImageMagickConverter::readFilters()
{
	QString s;
	QString all;
	QString name;
	QString description;
	ExceptionInfo ei;
	const MagickInfo *mi;

	GetExceptionInfo(&ei);
	mi = GetMagickInfo("*", &ei);

	if (!mi)
		return s;

	for (; mi; mi = reinterpret_cast<const MagickInfo*>(mi -> next)) {
		if (mi -> stealth)
			continue;

		if (mi -> decoder) {
			name = mi -> name;
			description = mi -> description;

			if (!description.isEmpty() && !description.contains('/')) {
				all += "*." + name.lower() + " *." + name + " ";
				s += "*." + name.lower() + " *." + name + "|";
				s += i18n(description.utf8());
				s += "\n";
			}
		}
	}

	all += "|" + i18n("All Images");
	all += "\n";
	DestroyExceptionInfo(&ei);
	return all + s;
}

QString KisImageMagickConverter::writeFilters()
{
	QString s;
	QString all;
	QString name;
	QString description;
	ExceptionInfo ei;
	const MagickInfo *mi;

	GetExceptionInfo(&ei);
	mi = GetMagickInfo("*", &ei);

	if (!mi)
		return s;

	for (; mi; mi = reinterpret_cast<const MagickInfo*>(mi -> next)) {
		if (mi -> stealth)
			continue;

		if (mi -> encoder) {
			name = mi -> name;
			description = mi -> description;

			if (!description.isEmpty() && !description.contains('/')) {
				all += "*." + name.lower() + " *." + name + " ";
				s += "*." + name.lower() + " *." + name + "|";
				s += i18n(description.utf8());
				s += "\n";
			}
		}
	}

	all += "|" + i18n("All Images");
	all += "\n";
	DestroyExceptionInfo(&ei);
	return all + s;
}

#include "kis_image_magick_converter.moc"

