/*
 * This file is part of Krita
 *
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
#include <QLayout>
#include <QPixmap>
#include <QPainter>
#include <QLabel>
#include <QComboBox>
#include <qdom.h>
#include <QHBoxLayout>

#include "KoBasicHistogramProducers.h"
#include "KoColorSpace.h"
#include "KoColorTransformation.h"
#include "KoCompositeOp.h"
#include "KoID.h"

#include "kis_bookmarked_configuration_manager.h"
#include "kis_filter_configuration.h"
#include "kis_filter_config_widget.h"
#include "kis_perchannel_filter.h"
#include "kis_paint_device.h"
#include "kis_iterators_pixel.h"
#include "kcurve.h"
#include "kis_histogram.h"
#include "kis_painter.h"

class KisPerChannelFilterConfigurationFactory : public KisFilterConfigurationFactory {
    public:
        KisPerChannelFilterConfigurationFactory() :KisFilterConfigurationFactory("perchannel", 1) {}
        virtual ~KisPerChannelFilterConfigurationFactory() { }
        virtual KisSerializableConfiguration* createDefault()
        {
            return new KisPerChannelFilterConfiguration(0);
        }
        virtual KisSerializableConfiguration* create(const QDomElement& e)
        {
            KisFilterConfiguration* fc = new KisPerChannelFilterConfiguration(0);
            fc->fromXML( e );
            return fc;
        }
};

KisPerChannelFilterConfiguration::KisPerChannelFilterConfiguration(int n)
    : KisFilterConfiguration( "perchannel", 1 )
{
    transfers = new quint16*[n];
    for(int i = 0; i < n; i++) {
        curves.append(KisCurve());

        transfers[i] = new quint16[256];

        for (quint32 j = 0; j < 256; ++j) {
            transfers[i][j] = j * 257;
        }
    }
    nTransfers = n;
    dirty = true;
    oldCs = 0;
    adjustment = 0;
}

KisPerChannelFilterConfiguration::~KisPerChannelFilterConfiguration()
{
    for(int i=0;i<nTransfers;i++)
        delete [] transfers[i];
    delete[] transfers;
    delete adjustment;
}

bool KisPerChannelFilterConfiguration::isCompatible(const KisPaintDeviceSP dev) const
{
    return *dev->colorSpace() == *oldCs;
}

void KisPerChannelFilterConfiguration::fromXML( const QString& s )
{
    QDomDocument doc;
    doc.setContent( s );
    QDomElement e = doc.documentElement();
    QDomNode n = e.firstChild();

    while (!n.isNull()) {

        e = n.toElement();

        if (!e.isNull()) {

            if (e.attribute("name") == "curves") {

                QDomNode curvesNode = e.firstChild();
                int count = 0;
                nTransfers = e.attribute("number").toUShort();

                curves.clear();

                for (int i = 0; i < nTransfers; ++i) {
                    curves.append(KisCurve());
                }

                while (!curvesNode.isNull()) {

                    QDomElement curvesElement = curvesNode.toElement();

                    if (!curvesElement.isNull() && !curvesElement.text().isEmpty()) {
                        QStringList data = curvesElement.text().split( ";" );

                        foreach (QString pair, data) {
                            if (pair.indexOf(",") > -1) {
                                QPointF p;
                                p.rx() = pair.section(",", 0, 0).toDouble();
                                p.ry() = pair.section(",", 1, 1).toDouble();
                                curves[count].append(p);
                            }
                        }
                    }
                    count++;
                    curvesNode = curvesNode.nextSibling();
                }
            }
        }
        n = n.nextSibling();
    }

    for(int ch = 0; ch < nTransfers; ++ch)
    {
        transfers[ch] = new quint16[256];
        for(int i = 0; i < 256; ++i)
        {
            qint32 val;
            val = int(0xFFFF * KCurve::getCurveValue(curves[ch], i /
255.0));
            if(val > 0xFFFF)
                val = 0xFFFF;
            if(val < 0)
                val = 0;

            transfers[ch][i] = val;
        }
    }
    dirty = true;
}

QString KisPerChannelFilterConfiguration::toString()
{
    QDomDocument doc = QDomDocument("filterconfig");
    QDomElement root = doc.createElement( "filterconfig" );
    root.setAttribute( "name", name() );
    root.setAttribute( "version", version() );

    QDomElement c = doc.createElement("curves");
    c.setAttribute("number", nTransfers);
    c.setAttribute("name", "curves");
    for (int i = 0; i < nTransfers; ++i) {
        QDomElement t = doc.createElement("curve");
        KisCurve curve = curves[i];
        QString sCurve;
        QPointF pair;
        foreach (pair, curve) {
            sCurve += QString::number(pair.x());
            sCurve += ',';
            sCurve += QString::number(pair.y());
            sCurve += ';';
        }
        QDomText text = doc.createCDATASection(sCurve);
        t.appendChild(text);
        c.appendChild(t);
    }
    root.appendChild(c);
    doc.appendChild( root );
    return doc.toString();
}

KisPerChannelFilter::KisPerChannelFilter() : KisFilter( id(), CategoryAdjust, i18n("&Color Adjustment curves..."))
{
    setSupportsPainting(true);
    setSupportsPreview(true);
    setSupportsIncrementalPainting(false);
    setColorSpaceIndependence(TO_LAB16);
    setBookmarkManager(new KisBookmarkedConfigurationManager(configEntryGroup(), new KisPerChannelFilterConfigurationFactory()));
}

KisFilterConfigWidget * KisPerChannelFilter::createConfigurationWidget(QWidget *parent, KisPaintDeviceSP dev)
{
    return new KisPerChannelConfigWidget(parent, dev);
}

void KisPerChannelFilter::process(KisFilterConstantProcessingInformation srcInfo,
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
    Q_ASSERT(src != 0);
    Q_ASSERT(dst != 0);
    if (!config) {
        kWarning() <<"No configuration object for per-channel filter";
        return;
    }

    KisPerChannelFilterConfiguration* configBC =
            const_cast<KisPerChannelFilterConfiguration*>(static_cast<const KisPerChannelFilterConfiguration*>(config)); // Somehow, this shouldn't happen
    if (configBC->nTransfers != src->colorSpace()->colorChannelCount()) {
        // We got an illegal number of colorchannels.KisFilter
        return;
    }

    if (configBC->dirty || (src->colorSpace() != configBC->oldCs)) {
        delete configBC->adjustment;
        configBC->adjustment =
                src->colorSpace()->createPerChannelAdjustment(configBC->transfers);
//         kDebug() << configBC->adjustment;
        configBC->oldCs = src->colorSpace();
        configBC->dirty = false;
    }

    KoColorTransformation *adj = configBC->adjustment;


    if (src!=dst) {
        KisPainter gc(dst, dstInfo.selection());
        gc.bitBlt(dstTopLeft.x(), dstTopLeft.y(), COMPOSITE_COPY, src, srcTopLeft.x(), srcTopLeft.y(), size.width(), size.height());
        gc.end();
    }

    KisRectIteratorPixel iter = dst->createRectIterator(dstTopLeft.x(), dstTopLeft.y(), size.width(), size.height(), dstInfo.selection() );
    KoMixColorsOp * mixOp = src->colorSpace()->mixColorsOp();
    qint32 totalCost = (size.width() * size.height()) / 100;
    qint32 pixelsProcessed = 0;

    while( not iter.isDone()  and (progressUpdater and not progressUpdater->interrupted()))
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
                    if (maxpix != 0)
                        ++iter;

                    ++npix;
                }
                pixelsProcessed += npix;
                ++iter;
                break;

            case MAX_SELECTED:
            {
                quint8 *firstPixel = iter.rawData();
                while(iter.selectedness()==MAX_SELECTED && maxpix)
                {
                    --maxpix;
                    ++iter;
                    ++npix;
                }
                // adjust
                adj->transform(firstPixel, firstPixel, npix);
                pixelsProcessed += npix;
                break;
            }

            default:
                // adjust, but since it's partially selected we also only partially adjust
                adj->transform(iter.oldRawData(), iter.rawData(), 1);
                const quint8 *pixels[2] = {iter.oldRawData(), iter.rawData()};
                quint8 weights[2] = {MAX_SELECTED - selectedness, selectedness};
                mixOp->mixColors(pixels, weights, 2, iter.rawData());
                ++iter;
                pixelsProcessed++;
                break;
        }
        if(progressUpdater) progressUpdater->setProgress(pixelsProcessed / totalCost);
    }

}

void KisPerChannelConfigWidget::setActiveChannel(int ch)
{
    int i;
    int height = 256;
    QPixmap pix(256, height);
    pix.fill();
    QPainter p(&pix);
    p.setPen(QPen::QPen(Qt::gray,1, Qt::SolidLine));

    m_histogram->setChannel(ch);

    double highest = (double)m_histogram->calculations().getHighest();
    qint32 bins = m_histogram->producer()->numberOfBins();

    if (m_histogram->getHistogramType() == LINEAR) {
        double factor = (double)height / highest;
        for( i=0; i<bins; ++i ) {
            p.drawLine(i, height, i, height - int(m_histogram->getValue(i) * factor));
        }
    } else {
        double factor = (double)height / (double)log(highest);
        for( i = 0; i < bins; ++i ) {
            p.drawLine(i, height, i, height - int(log((double)m_histogram->getValue(i)) * factor));
        }
    }

    m_curves[m_activeCh] = m_page->kCurve->getCurve();
    m_activeCh = ch;
    m_page->kCurve->setCurve(m_curves[m_activeCh]);

    m_page->kCurve->setPixmap(pix);
}

KisPerChannelConfigWidget::KisPerChannelConfigWidget(QWidget * parent, KisPaintDeviceSP dev, Qt::WFlags f)
    : KisFilterConfigWidget(parent, f)
{
    int i;
    int height;
    m_page = new WdgPerChannel(this);
    QHBoxLayout * l = new QHBoxLayout(this);
    Q_CHECK_PTR(l);

    m_dev = dev;
    m_curves.clear();
    m_activeCh = 0;
    for(unsigned int ch = 0; ch < m_dev->colorSpace()->colorChannelCount(); ch++)
    {
        m_curves.append(KisCurve());
        m_curves[ch].append(QPointF(0, 0));
        m_curves[ch].append(QPointF(1, 1));
    }

    l->addWidget(m_page);
    height = 256;
    connect(m_page->kCurve, SIGNAL(modified()), SIGNAL(sigPleaseUpdatePreview()));

    // Fill in the channel chooser
    QList<KoChannelInfo *> channels = dev->colorSpace()->channels();
    for(unsigned int val=0; val < dev->colorSpace()->colorChannelCount(); val++)
        m_page->cmbChannel->addItem(channels.at(val)->name());
    connect( m_page->cmbChannel, SIGNAL(activated(int)), this, SLOT(setActiveChannel(int)));

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

    QList<KoID> keys =
        KoHistogramProducerFactoryRegistry::instance()->listKeysCompatibleWith(m_dev->colorSpace());
    KoHistogramProducerFactory *hpf;
    hpf = KoHistogramProducerFactoryRegistry::instance()->get(keys.at(0).id());
    m_histogram = new KisHistogram(m_dev, hpf->generate(), LINEAR);

    setActiveChannel(0);
}

KisFilterConfiguration * KisPerChannelConfigWidget::configuration() const
{
    int nCh = m_dev->colorSpace()->colorChannelCount();
    KisPerChannelFilterConfiguration * cfg = new KisPerChannelFilterConfiguration(nCh);

    m_curves[m_activeCh] = m_page->kCurve->getCurve();

    for(int ch = 0; ch < nCh; ch++)
    {
        cfg->curves[ch] = m_curves[ch];

        for(int i = 0; i < 256; i++)
        {
            qint32 val;
            val = int(0xFFFF * m_page->kCurve->getCurveValue(m_curves[ch],  i / 255.0));
            if ( val > 0xFFFF )
                val = 0xFFFF;
            if ( val < 0 )
                val = 0;

            cfg->transfers[ch][i] = val;
        }
    }

    cfg->dirty = true;

    return cfg;
}

void KisPerChannelConfigWidget::setConfiguration(KisFilterConfiguration * config)
{
    KisPerChannelFilterConfiguration * cfg = dynamic_cast<KisPerChannelFilterConfiguration *>(config);
    if (!cfg)
        return;

    for(unsigned int ch = 0; ch < cfg->nTransfers; ch++)
    {
        m_curves[ch] = cfg->curves[ch];
    }
    m_page->kCurve->setCurve(m_curves[m_activeCh]);
    setActiveChannel( 0 );
}

#include "kis_perchannel_filter.moc"

