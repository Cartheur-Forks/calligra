/*
 *  Copyright (c) 2005 Boudewijn Rempt <boud@valdyas.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <sys/types.h>
#include <netinet/in.h>

#include <QString>
#include <QFile>
#include <QImage>
#include <QRadioButton>
#include <q3groupbox.h>
#include <q3buttongroup.h>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QApplication>
#include <QCursor>
#include <QEventLoop>
#include <QProgressDialog>
#include <QTimer>

#include <kglobal.h>
#include <kconfig.h>
#include <knuminput.h>
#include <kgenericfactory.h>
#include <kdialog.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <k3process.h>

#include <KoDocument.h>
#include <KoFilterChain.h>
#include <KoCompositeOp.h>

#include "kis_config.h"
#include "kis_cmb_idlist.h"
#include "kis_types.h"
#include "kis_raw_import.h"
#include "kis_doc2.h"
#include "kis_image.h"
#include "kis_meta_registry.h"
#include "kis_layer.h"
#include "kis_annotation.h"
#include "KoColorProfile.h"
#include "KoColorSpaceRegistry.h"
#include "kis_iterators_pixel.h"
#include "kis_paint_device.h"
#include "kis_paint_layer.h"
#include "kis_undo_adapter.h"

typedef KGenericFactory<KisRawImport> KisRawImportFactory;
K_EXPORT_COMPONENT_FACTORY(libkrita_raw_import, KisRawImportFactory("kofficefilters"))

KisRawImport::KisRawImport(QObject *parent, const QStringList&)
    : KoFilter(parent)
    , m_data(0)
    , m_process(0)
    , m_progress(0)
{
    m_dialog = new KDialog();
    m_dialog->enableButtonApply(false);
    m_page = new WdgRawImport(m_dialog);
    m_dialog -> setMainWidget(m_page);

    connect(m_page->bnPreview, SIGNAL(clicked()), this, SLOT(slotUpdatePreview()));
    connect(m_page->grpColorSpace, SIGNAL(clicked( int )), this, SLOT(slotFillCmbProfiles()));
    connect(m_page->grpChannelDepth, SIGNAL(clicked( int )), this, SLOT(slotFillCmbProfiles()));

    KisConfig cfg;
    QString monitorProfileName = cfg.monitorProfile();
    m_monitorProfile = KoColorSpaceRegistry::instance()->profileByName(monitorProfileName);

    slotFillCmbProfiles();
}

KisRawImport::~KisRawImport()
{
    delete m_dialog;
    delete m_process;
}

KoFilter::ConversionStatus KisRawImport::convert(const QByteArray& from, const QByteArray& to)
{
    if (from != "image/x-dcraw" || to != "application/x-krita") {
        return KoFilter::NotImplemented;
    }

    kDebug(41008) << "Krita importing from Raw\n";

    KisDoc2 * doc = dynamic_cast<KisDoc2*>(m_chain -> outputDocument());
    if (!doc) {
        return KoFilter::CreationError;
    }

    doc -> prepareForImport();

    QString filename = m_chain -> inputFile();

    if (filename.isEmpty()) {
        return KoFilter::FileNotFound;
    }

    slotUpdatePreview();

    // Show dialog
    m_dialog->setCursor(Qt::ArrowCursor);
    QApplication::setOverrideCursor(Qt::ArrowCursor);

    KSharedConfig::Ptr cfg = KGlobal::config();
    cfg->setGroup("rawimport");

    m_page->radioGray->setChecked(cfg->readEntry("gray", false));
    m_page->radioRGB->setChecked(cfg->readEntry("rgb", true));
    m_page->radio8->setChecked(cfg->readEntry("8bit", false));
    m_page->radio16->setChecked(cfg->readEntry("16bit", true));
    m_page->chkFourColorRGB->setChecked( cfg->readEntry("four_color_rgb", false));
    m_page->chkCameraColors->setChecked( cfg->readEntry("camera_colors", false));
    m_page->chkBrightness->setChecked( cfg->readEntry("do_brightness", false));
    m_page->chkBlackpoint->setChecked( cfg->readEntry("do_blackpoint", false));
    m_page->chkRed->setChecked( cfg->readEntry("do_red", false));
    m_page->chkBlue->setChecked( cfg->readEntry("do_blue", false));
    m_page->radioFixed->setChecked( cfg->readEntry("fixed_wb", true));
    m_page->radioAutomatic->setChecked( cfg->readEntry("automatic_wb", false));
    m_page->radioCamera->setChecked( cfg->readEntry("camera_wb", false));
    m_page->chkClip->setChecked( cfg->readEntry("clip", true));
    m_page->chkProfile->setChecked(cfg->readEntry("useprofile", false));
    m_page->dblBrightness->setValue(cfg->readEntry("brightness", 1.0));
    m_page->dblBlackpoint->setValue(cfg->readEntry("blackpoint", 0));
    m_page->dblRed->setValue(cfg->readEntry("red", 1.0));
    m_page->dblBlue->setValue(cfg->readEntry("blue", 1.0));

    if (m_dialog->exec() == QDialog::Accepted) {

        cfg->writeEntry("gray", m_page->radioGray->isChecked());
        cfg->writeEntry("rgb", m_page->radioRGB->isChecked());
        cfg->writeEntry("8bit", m_page->radio8->isChecked());
        cfg->writeEntry("16bit", m_page->radio16->isChecked());
        cfg->writeEntry("four_color_rgb", m_page->chkFourColorRGB -> isChecked());
        cfg->writeEntry("camera_colors", m_page->chkCameraColors -> isChecked());
        cfg->writeEntry("do_brightness", m_page->chkBrightness -> isChecked());
        cfg->writeEntry("do_blackpoint", m_page->chkBlackpoint -> isChecked());
        cfg->writeEntry("do_red", m_page->chkRed -> isChecked());
        cfg->writeEntry("do_blue", m_page->chkBlue -> isChecked());
        cfg->writeEntry("fixed_wb", m_page->radioFixed -> isChecked());
        cfg->writeEntry("automatic_wb", m_page->radioAutomatic -> isChecked());
        cfg->writeEntry("camera_wb", m_page->radioCamera -> isChecked());
        cfg->writeEntry("clip", m_page->chkClip->isChecked());
        cfg->writeEntry("useprofile", m_page->chkProfile->isChecked());
        cfg->writeEntry("brightness", m_page->dblBrightness->value());
        cfg->writeEntry("blackpoint", m_page->dblBlackpoint->value());
        cfg->writeEntry("red", m_page->dblRed->value());
        cfg->writeEntry("blue", m_page->dblBlue->value());

        QApplication::setOverrideCursor(Qt::WaitCursor);
        // Create a busy indicator to show that we didn't die or so
        m_progress = new QProgressDialog();
        m_progress -> setMaximum(0);
        m_progress -> setCancelButton(0);
        QTimer timer;
        connect(&timer, SIGNAL(timeout()), this, SLOT(incrementProgress()));
        timer.start(200);

        doc -> undoAdapter() -> setUndo(false);

        getImageData(createArgumentList(false));

        KisImageSP image = KisImageSP(0);
        KisPaintLayerSP layer = KisPaintLayerSP(0);
        KisPaintDeviceSP device = KisPaintDeviceSP(0);

        QApplication::restoreOverrideCursor();

        delete m_progress;
        m_progress = 0;

        if (m_page->radio8->isChecked()) {
        // 8 bits

            QImage img;
            img.loadFromData(*m_data);

            KoColorSpace * cs = 0;
            if (m_page->radioGray->isChecked()) {
                cs  = KoColorSpaceRegistry::instance()->colorSpace( "GRAYA", profile() );
            }
            else {
               cs  = KoColorSpaceRegistry::instance()->colorSpace( "RGBA", profile() );
            }
            if (cs == 0) { kDebug() << "No CS\n"; return KoFilter::InternalError; }

            image = new KisImage(doc->undoAdapter(), img.width(), img.height(), cs, filename);
            if (image.isNull()) return KoFilter::CreationError;

            layer = dynamic_cast<KisPaintLayer*>( image->newLayer(image -> nextLayerName(), OPACITY_OPAQUE, COMPOSITE_OVER, cs).data());
            if (layer.isNull()) return KoFilter::CreationError;

            device = layer->paintDevice();
            if (device.isNull()) return KoFilter::CreationError;

            device->convertFromQImage(img, "");

        } else {
        // 16 bits

            quint32 startOfImagedata = 0;
            QSize sz = determineSize(startOfImagedata);

            kDebug(41008) << "Total bytes: " << m_data->size()
                    << "\n start of image data: " << startOfImagedata
                    << "\n bytes for pixels left: " << m_data->size() - startOfImagedata
                    << "\n total pixels: " << sz.width() * sz.height()
                    << "\n total pixel bytes: " << sz.width() * sz.height() * 6
                    << "\n total necessary bytes: " << (sz.width() * sz.height() * 6) + startOfImagedata
                    << "\n";


            char * data = m_data->data() + startOfImagedata;

            KoColorSpace * cs = 0;
            if (m_page->radioGray->isChecked()) {
                cs  = KoColorSpaceRegistry::instance()->colorSpace( "GRAYA16", profile() );
            }
            else {
                cs = KoColorSpaceRegistry::instance()->colorSpace( "RGBA16", profile() );
            }
            if (cs == 0) return KoFilter::InternalError;

            image = new KisImage( doc->undoAdapter(), sz.width(), sz.height(), cs, filename);
            if (image.isNull())return KoFilter::CreationError;

            layer = dynamic_cast<KisPaintLayer*> (image->newLayer(image -> nextLayerName(), OPACITY_OPAQUE, COMPOSITE_OVER, cs).data());
            if (layer.isNull()) return KoFilter::CreationError;

            device = layer->paintDevice();
            if (device.isNull()) return KoFilter::CreationError;

            // Copy the colordata to the pixels
            int pos = 0;

            for (int line = 0; line < sz.height(); ++line) {
                KisHLineIterator it = device->createHLineIterator(0, line, sz.width());

                while (!it.isDone()) {
                    if (m_page->radioGray->isChecked()) {
                        quint16 d = (qint16)*(data + pos);
                        d = ntohs(d);
                        memcpy(it.rawData(), &d, 2);
                        pos += 2;
                    }
                    else {
                        // Red
                        quint16 d = (qint16)*(data + pos);
                        d = ntohs(d);
                        memcpy(it.rawData() + 4, &d, 2);

                        // Green
                        pos += 2;
                        d = (qint16)*(data + pos );
                        d = ntohs(d);
                        memcpy(it.rawData() + 2, &d, 2);

                        // Blue
                        pos += 2;
                        d = (qint16)*(data + pos );
                        d = ntohs(d);
                        memcpy(it.rawData(), &d, 2);

                        pos += 2;
                    }
                    cs->setAlpha(it.rawData(), OPACITY_OPAQUE, 1);
                    ++it;
                }
            }
        }
        layer->setDirty();
        kDebug() << "going to set image\n";
        doc -> setCurrentImage(image);
        doc -> undoAdapter() -> setUndo(true);
        doc -> setModified(false);
        kDebug() << "everything ok\n";

        QApplication::restoreOverrideCursor();
        return KoFilter::OK;
    }

    QApplication::restoreOverrideCursor();
    return KoFilter::UserCancelled;
}

void KisRawImport::incrementProgress()
{
    m_progress -> setValue(m_progress -> value() + 10);
}


void KisRawImport::slotUpdatePreview()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    getImageData(createArgumentList(true));

    kDebug(41008) << "Retrieved " << m_data->size() << " bytes of image data\n";

    if (m_data->isNull()) return;

    QImage img;

    if (m_page->radio8->isChecked()) {
        // 8 bits
        img.loadFromData(*m_data);

    } else {
        // 16 bits

        quint32 startOfImagedata = 0;
        QSize sz = determineSize(startOfImagedata);

        kDebug(41008) << "Total bytes: " << m_data->size()
                  << "\n start of image data: " << startOfImagedata
                  << "\n bytes for pixels left: " << m_data->size() - startOfImagedata
                  << "\n total pixels: " << sz.width() * sz.height()
                  << "\n total pixel bytes: " << sz.width() * sz.height() * 6
                  << "\n total necessary bytes: " << (sz.width() * sz.height() * 6) + startOfImagedata
                  << "\n";

        char * data = m_data->data() + startOfImagedata;

        KoColorSpace * cs = 0;
        if (m_page->radioGray->isChecked()) {
            cs  = KoColorSpaceRegistry::instance()->colorSpace( "GRAYA16", profile() );
        }
        else {
            cs = KoColorSpaceRegistry::instance()->colorSpace( "RGBA16", profile() );
        }
        KisPaintDevice * dev = new KisPaintDevice(cs, "preview");
            // Copy the colordata to the pixels
        int pos = 0;

        for (int line = 0; line < sz.height(); ++line) {
            KisHLineIterator it = dev->createHLineIterator(0, line, sz.width());

            while (!it.isDone()) {
                if (m_page->radioGray->isChecked()) {
                    quint16 d = (qint16)*(data + pos);
                    d = ntohs(d);
                    memcpy(it.rawData(), &d, 2);
                    pos += 2;
                }
                else {
                            // Red
                        quint16 d = (qint16)*(data + pos);
                        d = ntohs(d);
                        memcpy(it.rawData() + 4, &d, 2);

                            // Green
                        pos += 2;
                        d = (qint16)*(data + pos );
                        d = ntohs(d);
                        memcpy(it.rawData() + 2, &d, 2);

                            // Blue
                        pos += 2;
                        d = (qint16)*(data + pos );
                        d = ntohs(d);
                        memcpy(it.rawData(), &d, 2);

                        pos += 2;
                }
                cs->setAlpha(it.rawData(), OPACITY_OPAQUE, 1);
                ++it;
            }
        }

        img = dev->convertToQImage(m_monitorProfile);
    }

    m_page->lblPreview->setImage(img);
    QApplication::restoreOverrideCursor();
}


void KisRawImport::getImageData( QStringList arguments )
{
    //    delete m_process;
    delete m_data;

    kDebug(41008) << "getImageData " << arguments.join(" ") << "\n";
    K3Process process (this);
    m_data = new QByteArray(0);

    for (QStringList::iterator it = arguments.begin(); it != arguments.end(); ++it) {
        process << *it;
    }

    process.setUseShell(true);
    connect(&process, SIGNAL(receivedStdout(K3Process *, char *, int)), this, SLOT(slotReceivedStdout(K3Process *, char *, int)));
    connect(&process, SIGNAL(receivedStderr(K3Process *, char *, int)), this, SLOT(slotReceivedStderr(K3Process *, char *, int)));
    connect(&process, SIGNAL(processExited(K3Process *)), this, SLOT(slotProcessDone()));


    kDebug(41008) << "Starting process\n";

    if (!process.start(K3Process::NotifyOnExit, K3Process::AllOutput)) {
        KMessageBox::error( 0, i18n("Cannot convert RAW files because the dcraw executable could not be started."));
    }
    while (process.isRunning()) {
        //kDebug(41008) << "Waiting...\n";
        qApp->processEvents(QEventLoop::ExcludeUserInput);
        //process.wait(2);
    }

    if (process.normalExit()) {
        kDebug(41008) << "Return value of process: " << process.exitStatus() << "\n";
    }
    else {
        kDebug(41008) << "Process did not exit normally. Exit signal: " << process.exitSignal() << "\n";
    }

}


void KisRawImport::slotProcessDone()
{
    kDebug(41008) << "process done!\n";
}

void KisRawImport::slotReceivedStdout(K3Process *, char *buffer, int buflen)
{
    //kDebug(41008) << "stdout received " << buflen << " bytes on stdout.\n";
    //kDebug(41008) << QString::fromAscii(buffer, buflen) << "\n";
    int oldSize = m_data->size();
    m_data->resize(oldSize + buflen);
    memcpy(m_data->data() + oldSize, buffer, buflen);
}

void KisRawImport::slotReceivedStderr(K3Process *, char *buffer, int buflen)
{
    QByteArray b(buflen);
    memcpy(b.data(), buffer, buflen);
    kDebug(41008) << QString(b) << "\n";
    //KMessageBox::error(0, i18n("dcraw says: ") + QString(b));
}


QStringList KisRawImport::createArgumentList(bool forPreview)
{
    QStringList args;

    args.append("dcraw"); // XXX: Create a kritadcraw so we can count on it being available

    //args.append("-v"); // Verbose

    args.append("-c"); // Write to stdout

    if (forPreview) {
        args.append("-h"); // Fast, half size image
    }

    if (m_page->radio8->isChecked()) {
        args.append("-2"); // 8 bits
    }
    else {
        args.append("-4"); // 16 bits
    }

    if (m_page->radioGray->isChecked()) {
        args.append("-d"); // Create grayscale image
    }

    if (m_page->chkCameraColors->isChecked()) {
        args.append("-m"); // Use camera raw colors instead of sRGB
    }

    if (m_page->radioAutomatic->isChecked()) {
        args.append("-a"); // Automatic white balancing
    }

    if (m_page->radioCamera->isChecked()) {
        args.append("-w"); // Use camera white balance, if present
    }

    if (m_page->chkFourColorRGB->isChecked()) {
        args.append("-f"); // Interpolate RGB as four colors
    }

    if (!m_page->chkClip->isChecked()) {
        args.append("-n"); // Do not clip colors
    }

    if (m_page->chkBrightness->isChecked()) {
        args.append("-b " + QString::number(m_page->dblBrightness->value()));
    }

    if (m_page->chkBlackpoint->isChecked()) {
        args.append("-k " + QString::number(m_page->dblBlackpoint->value()));
    }

    if (m_page->chkRed->isChecked()) {
        args.append("-r " + QString::number(m_page->dblRed->value()));
    }

    if (m_page->chkBlue->isChecked()) {
        args.append("-l " + QString::number(m_page->dblBlue->value()));
    }


    KoColorProfile * pf  = profile();
    if (m_page->chkProfile->isChecked()) {
        if (!pf->fileName().isNull()) {
            // Use the user-set profile, if it's not an lcms internal
            // profile. This does not add the profile to the image, we
            // need to do that later.
            args.append("-p \"" + pf->fileName() + "\"");
        }
    }

    // Don't forget the filename
    args.append("\"" + m_chain -> inputFile() + "\"");

    return args;
}

QSize KisRawImport::determineSize(quint32& startOfImageData)
{
    if (m_data->isNull() || m_data->size() < 2048) {
        startOfImageData = 0;
        return QSize(0,0);
    }

    QString magick = QString::fromAscii(m_data->data(), 2);
    if (magick != "P6") {
        kDebug(41008) << " Bad magick! " << magick << "\n";
        startOfImageData = 0;
        return QSize(0,0);
    }

    // Find the third newline that marks the header end in a dcraw generated ppm.
    quint32 i = 0;
    quint32 counter = 0;

    while (true) {
        if (counter == 3) break;
        if (m_data->data()[i] == '\n') {
            counter++;
        }
        ++i;
    }

    QString size = QStringList::split("\n", QString::fromAscii(m_data->data(), i))[1];
    kDebug(41008) << "Header: " << QString::fromAscii(m_data->data(), i) << "\n";
    QStringList sizelist = QStringList::split(" ", size);
    qint32 w = sizelist[0].toInt();
    qint32 h = sizelist[1].toInt();

    startOfImageData = i;
    return QSize(w, h);

}

KoColorProfile * KisRawImport::profile()
{
    if (m_page->chkProfile->isChecked()) {
        return KoColorSpaceRegistry::instance()->profileByName(m_page->cmbProfile->currentText());
    }
    else
        return 0;
}

void KisRawImport::slotFillCmbProfiles()
{
    KoID s = colorSpace();

    KoColorSpaceFactory * csf = KoColorSpaceRegistry::instance() -> value(s.id());
    m_page -> cmbProfile -> clear();
    QList<KoColorProfile *>  profileList = KoColorSpaceRegistry::instance()->profilesFor( csf );

    foreach (KoColorProfile *profile, profileList) {
        m_page->cmbProfile->addItem(profile->name());
    }
}

KoID KisRawImport::colorSpace()
{
    if (m_page->radioRGB->isChecked()) {
        if (m_page->radio16->isChecked()) {
            return KoID( "RGBA16" );
        }
    }
    else {
        if (m_page->radio16->isChecked()) {
            return KoID( "GRAYA16" );
        }
        else {
            return KoID( "GRAYA" );
        }
    }
    return KoID("RGBA");
}

#include "kis_raw_import.moc"
