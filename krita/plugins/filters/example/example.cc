/*
 * This file is part of the KDE project
 *
 * Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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
#include <stdlib.h>
#include <vector>

#include <QPoint>
#include <QTime>

#include <klocale.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kis_debug.h>
#include <kgenericfactory.h>

#include <KoColorTransformation.h>
#include <KoProgressUpdater.h>

#include <kis_types.h>
#include <kis_selection.h>
#include <kis_image.h>
#include <kis_iterators_pixel.h>
#include <kis_layer.h>
#include <kis_filter_registry.h>
#include <kis_global.h>

// #include <kmessagebox.h>

#include "example.h"

typedef KGenericFactory<KritaExample> KritaExampleFactory;
K_EXPORT_COMPONENT_FACTORY( kritaexample, KritaExampleFactory( "krita" ) )

KritaExample::KritaExample(QObject *parent, const QStringList &)
        : KParts::Plugin(parent)
{
    setComponentData(KritaExampleFactory::componentData());


    if (parent->inherits("KisFilterRegistry")) {
        KisFilterRegistry * manager = dynamic_cast<KisFilterRegistry *>(parent);
        manager->add(KisFilterSP(new KisFilterInvert()));
    }
}

KritaExample::~KritaExample()
{
}

KisFilterInvert::KisFilterInvert() : KisFilter(id(), CategoryAdjust, i18n("&Invert"))
{
    setColorSpaceIndependence(FULLY_INDEPENDENT);
    setSupportsPainting(true);
    setSupportsPreview(true);
    setSupportsIncrementalPainting(false);
}

void KisFilterInvert::process(KisFilterConstProcessingInformation srcInfo,
                 KisFilterProcessingInformation dstInfo,
                 const QSize& size,
                 const KisFilterConfiguration* config,
                 KoUpdater* progressUpdater
        ) const
{
    const KisPaintDeviceSP src = srcInfo.paintDevice();
    KisPaintDeviceSP dst = dstInfo.paintDevice();
    QPoint dstTopLeft = dstInfo.topLeft();
    QPoint srcTopLeft = srcInfo.topLeft();
    Q_UNUSED( config );
    Q_ASSERT(!src.isNull());
    Q_ASSERT(!dst.isNull());

    int cost = (size.width() * size.height()) / 100;
    if( cost == 0 ) cost = 1;
    int count = 0;

    const KoColorSpace * cs = src->colorSpace();

    KoColorTransformation* inverter = cs->createInvertTransformation();

    QTime t;
    t.start();

// Method one: iterate and check every pixel for selectedness. It is
// only slightly slower than the next method and the code is very
// clear. Note that using nextRow() instead of recreating the iterators
// for every row makes a huge difference.

    KisHLineConstIteratorPixel srcIt = src->createHLineConstIterator(srcTopLeft.x(), srcTopLeft.y(), size.width(), srcInfo.selection());
    KisHLineIteratorPixel dstIt = dst->createHLineIterator(dstTopLeft.x(), dstTopLeft.y(), size.width(), dstInfo.selection());

    for ( int row = 0; row < size.height(); ++row ) {
        while ( !srcIt.isDone() ) {
            if ( srcIt.isSelected() ) {
                inverter->transform( srcIt.oldRawData(), dstIt.rawData(), 1);
            }
            ++srcIt;
            ++dstIt;
            if(progressUpdater) progressUpdater->setProgress( (++count) / cost);

        }
        srcIt.nextRow();
        dstIt.nextRow();
    }
    dbgPlugins <<"Per-pixel isSelected():" << t.elapsed() <<" ms";

#if 0
    t.restart();

    bool hasSelection = src->hasSelection();

// Method two: check the number of consecutive pixels the iterators
// points to. Take as large stretches of unselected pixels as possible
// and pass those to the color space transform object in one go. It's
// quite a bit speedier, with the speed improvement more noticeable
// the less happens inside the color transformation.

    srcIt = src->createHLineConstIterator(srcTopLeft.x(), srcTopLeft.y(), size.width());
    dstIt = dst->createHLineIterator(dstTopLeft.x(), dstTopLeft.y(), size.width());

    for (int row = 0; row < size.height(); ++row) {
        while( ! srcIt.isDone() )
        {
            int srcItConseq = srcIt.nConseqHPixels();
            int dstItConseq = srcIt.nConseqHPixels();
            int conseqPixels = qMin( srcItConseq, dstItConseq );

            int pixels = 0;

            if ( hasSelection ) {
                // Get largest horizontal row of selected pixels

                while ( srcIt.isSelected() && pixels < conseqPixels ) {
                    ++pixels;
                }
                inverter->transform( srcIt.oldRawData(), dstIt.rawData(), pixels);

                // We apparently found a non-selected pixels, or the row
                // was done; get the stretch of non-selected pixels
                while ( !srcIt.isSelected() && pixels < conseqPixels ) {
                    ++ pixels;
                }
            }
            else {
                pixels = conseqPixels;
                inverter->transform( srcIt.oldRawData(), dstIt.rawData(), pixels );
            }

            // Update progress
            srcIt += pixels;
            dstIt += pixels;
        }
        srcIt.nextRow();
        dstIt.nextRow();
    }
    dbgPlugins <<"Consecutive pixels:" << t.elapsed() <<" ms";

#endif
    delete inverter;
    //if(progressUpdater) progressUpdater->setProgress( 100 );

    // Two inversions make no inversion? No -- because we're reading
    // from the oldData in both loops without saving the transaction
    // in between, both inversion loops invert the original image.
}
