/*
 * This file is part of the KDE project
 *
 * Copyright (c) 2005 Boudewijn <boud@valdyas.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * This implementation completely and utterly based on the gimp's bumpmap.c,
 * copyright:
 * Copyright (C) 1997 Federico Mena Quintero <federico@nuclecu.unam.mx>
 * Copyright (C) 1997-2000 Jens Lautenbacher <jtl@gimp.org>
 * Copyright (C) 2000 Sven Neumann <sven@gimp.org>
 *
 */

#include <stdlib.h>

#include <QButtonGroup>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLayout>
#include <QLineEdit>
#include <QPoint>
#include <QPushButton>
#include <QString>

#include <kcomponentdata.h>
#include <kgenericfactory.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kstandarddirs.h>

#include <KoColorTransformation.h>
#include <KoIntegerMaths.h>
#include <KoProgressUpdater.h>

#include <kis_debug.h>
#include <kis_doc2.h>
#include <kis_filter_registry.h>
#include <kis_global.h>
#include <kis_image.h>
#include <kis_iterators_pixel.h>
#include <kis_layer.h>
#include <kis_selection.h>
#include <kis_types.h>
#include <kis_filter_configuration.h>
#include <kis_selection.h>
#include <kis_paint_device.h>
#include <kis_filter_processing_information.h>


#include "bumpmap.h"

#define MOD(x, y)                                               \
    ((x) < 0 ? ((y) - 1 - ((y) - 1 - (x)) % (y)) : (x) % (y))

typedef KGenericFactory<KritaBumpmap> KritaBumpmapFactory;
K_EXPORT_COMPONENT_FACTORY( kritabumpmap, KritaBumpmapFactory( "krita" ) )

    KritaBumpmap::KritaBumpmap(QObject *parent, const QStringList &)
        : KParts::Plugin(parent)
{
    setComponentData(KritaBumpmapFactory::componentData());


    if (parent->inherits("KisFilterRegistry")) {
        KisFilterRegistry * manager = dynamic_cast<KisFilterRegistry *>(parent);
        manager->add(KisFilterSP(new KisFilterBumpmap()));
    }
}

KritaBumpmap::~KritaBumpmap()
{
}

KisFilterBumpmap::KisFilterBumpmap()
    : KisFilter(KoID("bumpmap", i18n("Bumpmap")), KisFilter::CategoryMap, i18n("&Bumpmap..."))
{
    setColorSpaceIndependence( TO_LAB16 );
    setSupportsPainting( true );
    setSupportsPreview( true );
    setSupportsIncrementalPainting( true );


}

namespace {
    void convertRow(const KisPaintDevice * orig, quint8 * row, qint32 x, qint32 y, qint32 w,  quint8 * lut, qint32 waterlevel)
    {
        const KoColorSpace * csOrig = orig->colorSpace();

        KisHLineConstIteratorPixel origIt = orig->createHLineConstIterator(x, y, w);
        for (int i = 0; i < w; ++i) {
            row[0] = csOrig->intensity8(origIt.rawData());
            row[0] = lut[waterlevel + ((row[0] -  waterlevel) * csOrig->alpha(origIt.rawData())) / 255];

            ++row;
            ++origIt;
        }
    }

}

KisFilterConfiguration* KisFilterBumpmap::factoryConfiguration(const KisPaintDeviceSP) const
{
    KisFilterConfiguration* config = new KisFilterConfiguration(id(), 0);
    config->setProperty("bumpmap", "");
    config->setProperty("azimuth", 135.0);
    config->setProperty("elevation", 45.0);
    config->setProperty("depth", 3);
    config->setProperty("xofs", 0);
    config->setProperty("yofs", 0);
    config->setProperty("waterlevel",0);
    config->setProperty("ambient", 0);
    config->setProperty("compensate", true);
    config->setProperty("invert", false);
    config->setProperty("tiled", true);
    config->setProperty("type",krita::LINEAR);

    return config;
}


void KisFilterBumpmap::process(KisFilterConstProcessingInformation srcInfo,
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
    
    if (!src) return;
    if (!dst) return;
    if (!config) return;
    if (!size.isValid()) return;
    if (size.isNull()) return;
    if (size.isEmpty()) return;

    qint32 lx, ly;       /* X and Y components of light vector */
    qint32 nz2, nzlz;    /* nz^2, nz*lz */
    qint32 background;   /* Shade for vertical normals */
    double  compensation; /* Background compensation */
    quint8 lut[256];     /* Look-up table for modes */

    double azimuth = config->getDouble("azimuth", 135.0);
    double elevation = config->getDouble("elevation", 45.0);
    qint32 lz, nz;
    qint32 i;
    double n;

    // ------------------ Prepare parameters

    /* Convert to radians */
    azimuth   = M_PI * azimuth / 180.0;
    elevation = M_PI * elevation / 180.0;

    /* Calculate the light vector */
    lx = (qint32)(cos(azimuth) * cos(elevation) * 255.0);
    ly = (qint32)(sin(azimuth) * cos(elevation) * 255.0);

    lz = (qint32)(sin(elevation) * 255.0);

    /* Calculate constant Z component of surface normal */
    nz = (qint32)((6 * 255) / config->getDouble("depth", 3));
    nz2  = nz * nz;
    nzlz = nz * lz;

    /* Optimize for vertical normals */
    background = lz;

    /* Calculate darkness compensation factor */
    compensation = sin(elevation);

    /* Create look-up table for map type */
    for (i = 0; i < 256; i++)
    {
        switch ((enumBumpmapType)config->getInt("type",krita::LINEAR  ))
        {
        case SPHERICAL:
            n = i / 255.0 - 1.0;
            lut[i] = (int) (255.0 * sqrt(1.0 - n * n) + 0.5);
            break;

        case SINUSOIDAL:
            n = i / 255.0;
            lut[i] = (int) (255.0 *
                            (sin((-M_PI / 2.0) + M_PI * n) + 1.0) /
                            2.0 + 0.5);
            break;

        case LINEAR:
        default:
            lut[i] = i;
        }

        if (config->getBool("invert", false))
            lut[i] = 255 - lut[i];
    }


    // Crate a grayscale layer from the bumpmap layer.
    QRect bmRect;
    const KisPaintDevice * bumpmap;

    QString bumpmapname = config->getString("bumpmap", "");

#if 0
    //XXX: Get a better way to get at the layer stack instead of through the paint device
    if (!bumpmapname.isNull() && src->image()) {
        KisLayerSP l = src->image()->findLayer(bumpmapname);
        KisPaintDeviceSP bumplayer = KisPaintDeviceSP(0);

        KisPaintLayer * pl = dynamic_cast<KisPaintLayer*>(l.data());
        if (pl) {
            bumplayer = pl->paintDevice();
        }
        else {
            KisGroupLayer * gl = dynamic_cast<KisGroupLayer*>(l.data());
            if (gl) {
                bumplayer = gl->projection();
            }
            else {
                KisAdjustmentLayer * al = dynamic_cast<KisAdjustmentLayer*>(l.data());
                if (al) {
                    bumplayer = al->cachedPaintDevice();
                }
            }
        }


        if (bumplayer) {
            bmRect = bumplayer->exactBounds();
            bumpmap = bumplayer;
        }
        else {
            bmRect = QRect(srcTopLeft, size) ;
            bumpmap = src;
        }
    }
    else {
        bmRect = QRect(srcTopLeft, size);
        bumpmap = src;
    }
#endif

    return;
    
    qint32 sel_h = size.height();
    qint32 sel_w = size.width();

    qint32 bm_h = qMin(1, bmRect.height());
    qint32 bm_w = qMin(1, bmRect.width());
    qint32 bm_x = bmRect.x();

    int cost = size.height();
    int count = 0;

    // ------------------- Map the bumps
    qint32 yofs1, yofs2, yofs3;

    // ------------------- Initialize offsets
    if (config->getBool("tiled", true) ) {
        yofs2 = MOD (config->getInt("yofs", 0) + dstTopLeft.y(), bm_h);
        yofs1 = MOD (yofs2 - 1, bm_h);
        yofs3 = MOD (yofs2 + 1,  bm_h);
    }
    else {
        yofs2 = CLAMP (config->getInt("yofs", 0) + dstTopLeft.y(), 0, bm_h - 1);
        yofs1 = yofs2;
        yofs3 = CLAMP (yofs2 + 1, 0, bm_h - 1);

    }

    // ---------------------- Load initial three bumpmap scanlines

    const KoColorSpace * srcCs = src->colorSpace();
    QList<KoChannelInfo *> channels = srcCs->channels();

    // One byte per pixel, converted from the bumpmap layer.
    quint8 * bm_row1 = new quint8[bm_w];
    quint8 * bm_row2 = new quint8[bm_w];
    quint8 * bm_row3 = new quint8[bm_w];
    quint8 * tmp_row;

    convertRow(bumpmap, bm_row1, bm_x, yofs1, bm_w, lut, config->getInt("waterlevel",0));
    convertRow(bumpmap, bm_row2, bm_x, yofs2, bm_w, lut, config->getInt("waterlevel",0));
    convertRow(bumpmap, bm_row3, bm_x, yofs3, bm_w, lut, config->getInt("waterlevel",0));

    bool row_in_bumpmap;


    qint32 xofs1, xofs2, xofs3, shade = 0, ndotl, nx, ny;

    KisHLineIteratorPixel dstIt = dst->createHLineIterator(dstTopLeft.x(), dstTopLeft.y(), sel_w, srcInfo.selection());
    KisHLineConstIteratorPixel srcIt = src->createHLineConstIterator(srcTopLeft.x(), srcTopLeft.y(), sel_w, dstInfo.selection());
    KoColorTransformation* darkenTransfo = srcCs->createDarkenAdjustment(shade, config->getBool("compensate", true), compensation);

    for (int y = 0; y < sel_h; y++) {

        row_in_bumpmap = (y >= - config->getInt("yofs",0) && y < - config->getInt("yofs",0) + bm_h);

        // Bumpmap


        qint32 tmp = config->getInt("xofs",0) + dstTopLeft.x();
        xofs2 = MOD (tmp, bm_w);

        qint32 x = 0;
        //while (x < sel_w || cancelRequested()) {
        while (!srcIt.isDone() && !( progressUpdater && progressUpdater->interrupted()) ) {
            if (srcIt.isSelected()) {
                // Calculate surface normal from bumpmap
                if (config->getBool("tiled", true) || row_in_bumpmap &&
                    x >= - tmp&& x < - tmp + bm_w) {

                    if (config->getBool("tiled", true)) {
                        xofs1 = MOD (xofs2 - 1, bm_w);
                        xofs3 = MOD (xofs2 + 1, bm_w);
                    }
                    else {
                        xofs1 = CLAMP (xofs2 - 1, 0, bm_w - 1);
                        xofs3 = CLAMP (xofs2 + 1, 0, bm_w - 1);
                    }

                    nx = (bm_row1[xofs1] + bm_row2[xofs1] + bm_row3[xofs1] -
                          bm_row1[xofs3] - bm_row2[xofs3] - bm_row3[xofs3]);
                    ny = (bm_row3[xofs1] + bm_row3[xofs2] + bm_row3[xofs3] -
                          bm_row1[xofs1] - bm_row1[xofs2] - bm_row1[xofs3]);


                }
                else {
                    nx = 0;
                    ny = 0;
                }

                // Shade

                if ((nx == 0) && (ny == 0)) {
                    shade = background;
                }
                else {
                    ndotl = (nx * lx) + (ny * ly) + nzlz;

                    if (ndotl < 0) {
                        shade = (qint32)(compensation * config->getInt("ambient", 0));
                    }
                    else {
                        shade = (qint32)(ndotl / sqrt(nx * nx + ny * ny + nz2));
                        shade = (qint32)(shade + qMax(0, (int)((255 * compensation - shade)) * config->getInt("ambient",0) / 255));
                    }
                }

                // Paint
                darkenTransfo->transform(srcIt.rawData(), dstIt.rawData(), 1);
            }
            if (++xofs2 == bm_w)
                xofs2 = 0;
            ++srcIt;
            ++dstIt;
            ++x;
        }
        srcIt.nextRow();
        dstIt.nextRow();

        // Next line
        if (config->getBool("tiled", true) || row_in_bumpmap) {
            tmp_row = bm_row1;
            bm_row1 = bm_row2;
            bm_row2 = bm_row3;
            bm_row3 = tmp_row;

            if (++yofs2 == bm_h) {
                yofs2 = 0;
            }
            if (config->getBool("tiled", true)) {
                yofs3 = MOD(yofs2 + 1, bm_h);
            }
            else {
                yofs3 = CLAMP(yofs2 + 1, 0, bm_h - 1);
            }

            convertRow(bumpmap, bm_row3, bm_x, yofs3, bm_w, lut, config->getInt("waterlevel", 0));
        }
        if(progressUpdater) progressUpdater->setProgress( (++count) / cost);
    }
    delete darkenTransfo;

    delete [] bm_row1;
    delete [] bm_row2;
    delete [] bm_row3;
}

KisFilterConfigWidget * KisFilterBumpmap::createConfigurationWidget(QWidget* parent, const KisPaintDeviceSP dev) const
{
    KisBumpmapConfigWidget * w = new KisBumpmapConfigWidget(dev, parent);

    return w;
}


KisBumpmapConfigWidget::KisBumpmapConfigWidget(const KisPaintDeviceSP dev, QWidget * parent, Qt::WFlags f)
    : KisFilterConfigWidget(parent, f),
      m_device(dev)
{
    Q_ASSERT(m_device);

    m_page = new BumpmapWidget(this);
    
    QHBoxLayout * l = new QHBoxLayout(this);
    Q_CHECK_PTR(l);

    l->addWidget(m_page);
    m_page->txtSourceLayer->setText( "" );
    connect( m_page->bnRefresh, SIGNAL(clicked()), SIGNAL(sigPleaseUpdatePreview()));
}

void KisBumpmapConfigWidget::setConfiguration(KisFilterConfiguration * cfg)
{
    if (!cfg) return;

    m_page->txtSourceLayer->setText( cfg->getString("bumpmap", "") );
    m_page->dblAzimuth->setValue(cfg->getDouble("azimuth", 135.0) );
    m_page->dblElevation->setValue(cfg->getDouble("elevation", 45.0));
    m_page->dblDepth->setValue(cfg->getInt("depth", 3));
    m_page->intXOffset->setValue(cfg->getInt("xofs",0));
    m_page->intYOffset->setValue(cfg->getInt("yofs",0));
    m_page->intWaterLevel->setValue(cfg->getInt("waterlevel",0));
    m_page->intAmbient->setValue(cfg->getInt("ambient", 0));
    m_page->chkCompensate->setChecked(cfg->getBool("compensate", true));
    m_page->chkInvert->setChecked(cfg->getBool("invert", false));
    m_page->chkTiled->setChecked(cfg->getBool("tiled", true));

    switch (cfg->getInt("type", krita::LINEAR) ) {
    default:
    case LINEAR:
        m_page->radioLinear->setChecked(true);
        break;
    case SPHERICAL:
        m_page->radioSpherical->setChecked(true);
        break;
    case SINUSOIDAL:
        m_page->radioSinusoidal->setChecked(true);
        break;
    }
}

KisFilterConfiguration* KisBumpmapConfigWidget::configuration() const
{
    KisFilterConfiguration * cfg = new KisFilterConfiguration("bumpmap", 1);
    cfg->setProperty("bumpmap", m_page->txtSourceLayer->text());
    cfg->setProperty("azimuth",m_page->dblAzimuth->value());
    cfg->setProperty("elevation",m_page->dblElevation->value());
    cfg->setProperty("depth",m_page->dblDepth->value());
    cfg->setProperty("xofs",m_page->intXOffset->value());
    cfg->setProperty("yofs",m_page->intYOffset->value());
    cfg->setProperty("waterlevel",m_page->intWaterLevel->value());
    cfg->setProperty("ambient",m_page->intAmbient->value());
    cfg->setProperty("compensate",m_page->chkCompensate->isChecked());
    cfg->setProperty("invert",m_page->chkInvert->isChecked());
    cfg->setProperty("tiled",m_page->chkTiled->isChecked());

    if (m_page->radioLinear->isChecked()) {
        cfg->setProperty("type", LINEAR);
    } else if (m_page->radioSpherical->isChecked()) {
        cfg->setProperty("type", SPHERICAL);
    } else {
        Q_ASSERT(m_page->radioSinusoidal->isChecked());
        cfg->setProperty("type", SINUSOIDAL);
    }

    return cfg;
}

#include "bumpmap.moc"
