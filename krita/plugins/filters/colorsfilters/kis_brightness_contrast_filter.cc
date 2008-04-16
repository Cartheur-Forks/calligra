/*
 * This file is part of Krita
 *
 * Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
  * Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
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
 */

#include "kis_brightness_contrast_filter.h"

#include <math.h>

#include <klocale.h>

#include <QLayout>
#include <QPixmap>
#include <QPainter>
#include <QLabel>
#include <qdom.h>
#include <QString>
#include <QStringList>
#include <QPushButton>
#include <QHBoxLayout>

#include "KoBasicHistogramProducers.h"
#include "KoColorSpace.h"
#include "KoColorTransformation.h"
#include "KoCompositeOp.h"

#include "filter/kis_filter_config_widget.h"

#include "kis_bookmarked_configuration_manager.h"
#include "kis_paint_device.h"
#include "kis_iterators_pixel.h"
#include "kis_iterator.h"
#include "widgets/kcurve.h"
#include "kis_histogram.h"
#include "kis_painter.h"

class KisBrightnessContrastFilterConfigurationFactory : public KisFilterConfigurationFactory {
    public:
        KisBrightnessContrastFilterConfigurationFactory() :KisFilterConfigurationFactory("brightnesscontrast", 1) {}
        virtual ~KisBrightnessContrastFilterConfigurationFactory() { }
        virtual KisSerializableConfiguration* createDefault()
        {
            return new KisBrightnessContrastFilterConfiguration();
        }
        virtual KisSerializableConfiguration* create(const QDomElement& e)
        {
            KisFilterConfiguration* fc = new KisBrightnessContrastFilterConfiguration();
            fc->fromXML( e );
            return fc;
        }
};

KisBrightnessContrastFilterConfiguration::KisBrightnessContrastFilterConfiguration()
    : KisFilterConfiguration( "brightnesscontrast", 1 )
{
    for (quint32 i = 0; i < 256; ++i) {
        transfer[i] = i * 257;
    }
    m_adjustment = 0;
    QPointF p;
    p.rx() = 0.0; p.ry()=0.0;
    curve.append(p);
    p.rx() = 1.0; p.ry()=1.0;
    curve.append(p);
}

KisBrightnessContrastFilterConfiguration::~KisBrightnessContrastFilterConfiguration()
{
    delete m_adjustment;
}

void KisBrightnessContrastFilterConfiguration::fromXML( const QString& s )
{
    QDomDocument doc;
    doc.setContent( s );
    QDomElement e = doc.documentElement();
    QDomNode n = e.firstChild();

    while (!n.isNull()) {
        e = n.toElement();
        if (!e.isNull()) {
            if (e.attribute("name") == "transfer") { // ### 1.6 needs .tagName("name") here!
                QStringList data = e.text().split( "," );
                QStringList::Iterator start = data.begin();
                QStringList::Iterator end = data.end();
                int i = 0;
                for ( QStringList::Iterator it = start; it != end && i < 256; ++it ) {
                    QString s = *it;
                    transfer[i] = s.toUShort();
                    i++;
                }
            }
            else if (e.attribute("name") == "curve") { // ### 1.6 needs .tagName("name") here!
                QStringList data = e.text().split( ";" );
                curve.clear();
                foreach (QString pair, data) {
                    if (pair.indexOf(",") > -1) {
                        QPointF p;
                        p.rx() = pair.section(",", 0, 0).toDouble();
                        p.ry() = pair.section(",", 1, 1).toDouble();
                        curve.append(p);
                    }
                }
            }
        }
        n = n.nextSibling();
    }
    // If the adjustment was cached, it now has changed - invalidate it
    delete m_adjustment;
    m_adjustment = 0;
}

QString KisBrightnessContrastFilterConfiguration::toString()
{
    QDomDocument doc = QDomDocument("filterconfig");
    QDomElement root = doc.createElement( "filterconfig" );
    root.setAttribute( "name", name() );
    root.setAttribute( "version", version() );

    doc.appendChild( root );

    QDomElement e = doc.createElement( "transfer" );
    QString sTransfer;
    for ( uint i = 0; i < 256 ; ++i ) {
        sTransfer += QString::number( transfer[i] );
        sTransfer += ',';
    }
    QDomText text = doc.createCDATASection(sTransfer);
    e.appendChild(text);
    root.appendChild(e);

    e = doc.createElement("curve");
    QString sCurve;
    QPointF pair;
    foreach (pair, curve) {
        sCurve += QString::number(pair.x());
        sCurve += ',';
        sCurve += QString::number(pair.y());
        sCurve += ';';
    }
    text = doc.createCDATASection(sCurve);
    e.appendChild(text);
    root.appendChild(e);

    return doc.toString();
}

KisBrightnessContrastFilter::KisBrightnessContrastFilter()
    : KisFilter( id(), CategoryAdjust, i18n("&Brightness/Contrast curve..."))
{
    setSupportsPainting(true);
    setSupportsPreview(true);
    setSupportsIncrementalPainting(false);
    setColorSpaceIndependence(TO_LAB16);
    setBookmarkManager(new KisBookmarkedConfigurationManager(configEntryGroup(), new KisBrightnessContrastFilterConfigurationFactory()));
    
}

KisFilterConfigWidget * KisBrightnessContrastFilter::createConfigurationWidget(QWidget *parent, const KisPaintDeviceSP dev) const
{
    return new KisBrightnessContrastConfigWidget(parent, dev);
}

KisFilterConfiguration* KisBrightnessContrastFilter::factoryConfiguration(const KisPaintDeviceSP)
    const
{
    KisFilterConfiguration* config = new KisBrightnessContrastFilterConfiguration( );
    return config;
}


bool KisBrightnessContrastFilter::workWith(const KoColorSpace* cs) const
{
    return (cs->profile() != 0);
}


void KisBrightnessContrastFilter::process(KisConstProcessingInformation srcInfo,
                 KisProcessingInformation dstInfo,
                 const QSize& size,
                 const KisFilterConfiguration* config,
                 KoUpdater* progressUpdater
        ) const
{
    const KisPaintDeviceSP src = srcInfo.paintDevice();
    KisPaintDeviceSP dst = dstInfo.paintDevice();
    QPoint dstTopLeft = dstInfo.topLeft();
    QPoint srcTopLeft = srcInfo.topLeft();
    Q_ASSERT(src != 0);
    Q_ASSERT(dst != 0);
    if (!config) {
        kWarning() << "No configuration object for brightness/contrast filter\n";
        return;
    }

    KisBrightnessContrastFilterConfiguration* configBC = (KisBrightnessContrastFilterConfiguration*) config;
    Q_ASSERT(config);

    if (src!=dst) {
        KisPainter gc(dst, dstInfo.selection());
        gc.bitBlt(dstTopLeft.x(), dstTopLeft.y(), COMPOSITE_COPY, src, srcTopLeft.x(), srcTopLeft.y(), size.width(), size.height());
        gc.end();
    }

    if (configBC->m_adjustment == 0) {
        configBC->m_adjustment = src->colorSpace()->createBrightnessContrastAdjustment(configBC->transfer);
    }

    KisRectIteratorPixel iter = dst->createRectIterator(srcTopLeft.x(), srcTopLeft.y(), size.width(), size.height(), dstInfo.selection());

    qint32 totalCost = (size.width() * size.height()) / 100;
    if( totalCost == 0 ) totalCost = 1;
    qint32 pixelsProcessed = 0;
    KoMixColorsOp * mixOp = src->colorSpace()->mixColorsOp();
    while( ! iter.isDone()  && !(progressUpdater && progressUpdater->interrupted()))
    {
        quint32 npix=0, maxpix = iter.nConseqPixels();
        quint8 selectedness = iter.selectedness();
        // The idea here is to handle stretches of completely selected and completely unselected pixels.
        // Partially selected pixels are handled one pixel at a time.
        switch(selectedness)
        {
            case MIN_SELECTED:
                while(iter.selectedness()==MIN_SELECTED && maxpix)
                {
                    --maxpix;
                    ++iter;
                    ++npix;
                }
                pixelsProcessed += npix;
                break;

            case MAX_SELECTED:
            {
                quint8 *firstPixel = iter.rawData();
                while(iter.selectedness()==MAX_SELECTED && maxpix)
                {
                    --maxpix;
                    if (maxpix != 0)
                        ++iter;
                    ++npix;
                }
                // adjust
                configBC->m_adjustment->transform(firstPixel, firstPixel, npix);
                pixelsProcessed += npix;
                ++iter;
                break;
            }

            default:
                // adjust, but since it's partially selected we also only partially adjust
                configBC->m_adjustment->transform(iter.oldRawData(), iter.rawData(), 1);
                const quint8 *pixels[2] = {iter.oldRawData(), iter.rawData()};
                qint16 weights[2] = {MAX_SELECTED - selectedness, selectedness};
                mixOp->mixColors(pixels, weights, 2, iter.rawData());
                ++iter;
                pixelsProcessed++;
                break;
        }
        if(progressUpdater) progressUpdater->setProgress(pixelsProcessed / totalCost);
    }

}

KisBrightnessContrastConfigWidget::KisBrightnessContrastConfigWidget(QWidget * parent, KisPaintDeviceSP dev, Qt::WFlags f)
    : KisFilterConfigWidget(parent, f)
{
    int i;
    int height;
    m_page = new WdgBrightnessContrast(this);
    QHBoxLayout * l = new QHBoxLayout(this);
    Q_CHECK_PTR(l);

    //Hide these buttons and labels as they are not implemented in 1.5
    m_page->pb_more_contrast->hide();
    m_page->pb_less_contrast->hide();
    m_page->pb_more_brightness->hide();
    m_page->pb_less_brightness->hide();
    m_page->textLabelBrightness->hide();
    m_page->textLabelContrast->hide();

    l->addWidget(m_page, 0, Qt::AlignTop);
    height = 256;
    connect( m_page->kCurve, SIGNAL(modified()), SIGNAL(sigPleaseUpdatePreview()));

    // Create the horizontal gradient label
    QPixmap hgradientpix(256, 1);
    QPainter hgp(&hgradientpix);
    hgp.setPen(QPen::QPen(QColor(0,0,0),1, Qt::SolidLine));
    for( i=0; i<256; ++i )
    {
        hgp.setPen(QColor(i,i,i));
        hgp.drawPoint(i, 0);
    }
    m_page->hgradient->setPixmap(hgradientpix);

    // Create the vertical gradient label
    QPixmap vgradientpix(1, 256);
    QPainter vgp(&vgradientpix);
    vgp.setPen(QPen::QPen(QColor(0,0,0),1, Qt::SolidLine));
    for( i=0; i<256; ++i )
    {
        vgp.setPen(QColor(i,i,i));
        vgp.drawPoint(0, 255-i);
    }
    m_page->vgradient->setPixmap(vgradientpix);

    KoHistogramProducerSP producer = KoHistogramProducerSP(new KoGenericLabHistogramProducer());
    KisHistogram histogram(dev, producer, LINEAR);
    QPixmap pix(256, height);
    pix.fill();
    QPainter p(&pix);
    p.setPen(QPen::QPen(Qt::gray,1, Qt::SolidLine));

    double highest = (double)histogram.calculations().getHighest();
    qint32 bins = histogram.producer()->numberOfBins();

    if (histogram.getHistogramType() == LINEAR) {
        double factor = (double)height / highest;
        for( i=0; i<bins; ++i ) {
            p.drawLine(i, height, i, height - int(histogram.getValue(i) * factor));
        }
    } else {
        double factor = (double)height / (double)log(highest);
        for( i = 0; i < bins; ++i ) {
            p.drawLine(i, height, i, height - int(log((double)histogram.getValue(i)) * factor));
        }
    }

    m_page->kCurve->setPixmap(pix);

}

KisBrightnessContrastFilterConfiguration * KisBrightnessContrastConfigWidget::configuration() const
{
    KisBrightnessContrastFilterConfiguration * cfg = new KisBrightnessContrastFilterConfiguration();

    for(int i=0; i <256; i++)
    {
        qint32 val;
        val = int(0xFFFF * m_page->kCurve->getCurveValue( i / 255.0));
        if(val >0xFFFF)
            val=0xFFFF;
        if(val <0)
            val = 0;

        cfg->transfer[i] = val;
    }
    cfg->curve = m_page->kCurve->getCurve();
    return cfg;
}

void KisBrightnessContrastConfigWidget::setConfiguration( KisFilterConfiguration * config )
{
    KisBrightnessContrastFilterConfiguration * cfg = dynamic_cast<KisBrightnessContrastFilterConfiguration *>(config);
    Q_ASSERT(cfg);
    m_page->kCurve->setCurve(cfg->curve);
}

#include "kis_brightness_contrast_filter.moc"

