/*
 *  Copyright (c) 2005 Adrian Page <adrian@pagenet.plus.com>
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

#include <qfile.h>

#include <kmessagebox.h>

#include <half.h>
#include <ImfRgbaFile.h>

#include <kgenericfactory.h>
#include <koDocument.h>
#include <koFilterChain.h>

#include "kis_doc.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_annotation.h"
#include "kis_types.h"
#include "kis_colorspace_registry.h"
#include "kis_iterators_pixel.h"
#include "kis_abstract_colorspace.h"
#include "kis_paint_device.h"
#include "kis_rgb_f32_colorspace.h"

#include "kis_openexr_export.h"

using namespace std;
using namespace Imf;
using namespace Imath;

typedef KGenericFactory<KisOpenEXRExport, KoFilter> KisOpenEXRExportFactory;
K_EXPORT_COMPONENT_FACTORY(libkrita_openexr_export, KisOpenEXRExportFactory("kofficefilters"))

KisOpenEXRExport::KisOpenEXRExport(KoFilter *, const char *, const QStringList&) : KoFilter()
{
}

KisOpenEXRExport::~KisOpenEXRExport()
{
}

KoFilter::ConversionStatus KisOpenEXRExport::convert(const QCString& from, const QCString& to)
{
    if (to != "image/x-exr" || from != "application/x-krita") {
        return KoFilter::NotImplemented;
    }

    kdDebug() << "Krita exporting to OpenEXR\n";

    // XXX: Add dialog about flattening layers here

    KisDoc *output = dynamic_cast<KisDoc*>(m_chain -> inputDocument());
    QString filename = m_chain -> outputFile();
    
    if (!output) {
        return KoFilter::CreationError;
    }
    
    if (filename.isEmpty()) {
        return KoFilter::FileNotFound;
    }

    KisImageSP img = new KisImage(*output -> currentImage());
    Q_CHECK_PTR(img);

    // Don't store this information in the document's undo adapter
    bool undo = output -> undoAdapter() -> undo();
    output -> undoAdapter() -> setUndo(false);

    img -> flatten();

    KisLayerSP layer = img -> layer(0);
    Q_ASSERT(layer);
    
    output -> undoAdapter() -> setUndo(undo);

    //KisF32RgbColorSpace * cs = static_cast<KisF32RgbColorSpace *>((KisColorSpaceRegistry::instance() -> get(KisID("RGBAF32", ""))));
    KisF32RgbColorSpace *cs = dynamic_cast<KisF32RgbColorSpace *>(layer -> colorStrategy());

    if (cs == 0) {
        // We could convert automatically, but the conversion wants to be done with
        // selectable profiles and rendering intent.
        KMessageBox::information(0, i18n("The image is using an unsupported color space. "
                                         "Please convert to 32-bit floating point RGB/Alpha "
                                         "before saving in the OpenEXR format."));
        return KoFilter::WrongFormat;
    }

    Box2i displayWindow(V2i(0, 0), V2i(img -> width() - 1, img -> height() - 1));

    QRect dataExtent = layer -> exactBounds();
    int dataWidth = dataExtent.width();
    int dataHeight = dataExtent.height();

    Box2i dataWindow(V2i(dataExtent.left(), dataExtent.top()), V2i(dataExtent.right(), dataExtent.bottom()));

    RgbaOutputFile file(QFile::encodeName(filename), displayWindow, dataWindow, WRITE_RGBA);

    QMemArray<Rgba> pixels(dataWidth);

    for (int y = 0; y < dataHeight; ++y) {

        file.setFrameBuffer(pixels.data() - dataWindow.min.x - (dataWindow.min.y + y) * dataWidth, 1, dataWidth);

        KisHLineIterator it = layer -> createHLineIterator(dataWindow.min.x, dataWindow.min.y + y, dataWidth, false);
        Rgba *rgba = pixels.data();

        while (!it.isDone()) {

            // XXX: Currently we use unmultiplied data so premult it.
            float unmultipliedRed;
            float unmultipliedGreen;
            float unmultipliedBlue;
            float alpha;

            cs -> getPixel(it.rawData(), &unmultipliedRed, &unmultipliedGreen, &unmultipliedBlue, &alpha);
            rgba -> r = unmultipliedRed * alpha;
            rgba -> g = unmultipliedGreen * alpha;
            rgba -> b = unmultipliedBlue * alpha;
            rgba -> a = alpha;
            ++it;
            ++rgba;
        }
        file.writePixels();
    }

    //vKisAnnotationSP_it beginIt = img -> beginAnnotations();
    //vKisAnnotationSP_it endIt = img -> endAnnotations();
    return KoFilter::OK;
}

#include "kis_openexr_export.moc"

