/* This file is part of the KDE project
   Copyright (c) 2001 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2002, 2003 Nicolas GOUTTE <goutte@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <unistd.h>
#include <stdio.h>

#include <qbuffer.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qapplication.h>
#include <qdragobject.h>

#include <kdebug.h>
#include <kdeversion.h>
#if ! KDE_IS_VERSION( 3,1,90 )
#include <kdebugclasses.h>
#endif
#include <ktempfile.h>
#include <kprocess.h>

#include "koPictureKey.h"
#include "koPictureBase.h"
#include "koPictureEps.h"


KoPictureEps::KoPictureEps(void) : m_psStreamStart(0), m_psStreamLength(0), m_cacheIsInFastMode(true)
{
    // Forbid QPixmap to cache the X-Window resources (Yes, it is slower!)
    m_cachedPixmap.setOptimization(QPixmap::MemoryOptim);
}

KoPictureEps::~KoPictureEps(void)
{
}

KoPictureBase* KoPictureEps::newCopy(void) const
{
    return new KoPictureEps(*this);
}

KoPictureType::Type KoPictureEps::getType(void) const
{
    return KoPictureType::TypeEps;
}

bool KoPictureEps::isNull(void) const
{
    return m_rawData.isNull();
}

QImage KoPictureEps::scaleWithGhostScript(const QSize& size, const int resolutionx, const int resolutiony )
{
    if (!m_boundingBox.width() || !m_boundingBox.height())
    {
        kdDebug(30003) << "EPS image has a null size! (in KoPictureEps::scaleWithGhostScript)" << endl;
        return QImage();
    }

    // ### TODO: do not call GhostScript up to three times for each re-scaling (one call of GhostScript should be enough to know which device is available: gs --help)
    // png16m is better, but not always available -> fallback to bmp16m, then fallback to ppm (256 colors)
    QImage img;
    int ret = tryScaleWithGhostScript(img, size, resolutionx, resolutiony, "png16m");
    if ( ret == -1 )
    {
        ret = tryScaleWithGhostScript(img, size, resolutionx, resolutiony, "bmp16m");
        if ( ret == -1 )
        {
            ret = tryScaleWithGhostScript(img, size, resolutionx, resolutiony, "ppm");
            if ( ret == -1 )
                kdError(30003) << "Image from GhostScript cannot be loaded (in KoPictureEps::scaleWithGhostScript)" << endl;
        }
    }
    return img;
}

// Helper method for scaleWithGhostScript. Returns 1 on success, 0 on error, -1 if nothing generated
// (in which case another 'output device' can be tried)
int KoPictureEps::tryScaleWithGhostScript(QImage &image, const QSize& size, const int resolutionx, const int resolutiony, const char* device )
// Based on the code of the file kdelibs/kimgio/eps.cpp
{
    kdDebug(30003) << "Sampling with GhostScript, using device " << device << " (in KoPictureEps::tryScaleWithGhostScript)" << endl;

    KTempFile tmpFile;
    tmpFile.setAutoDelete(true);

    if ( tmpFile.status() )
    {
        kdError(30003) << "No KTempFile! (in KoPictureEps::tryScaleWithGhostScript)" << endl;
        return 0; // error
    }

    const int wantedWidth = size.width();
    const int wantedHeight = size.height();
    const double xScale = double(size.width()) / double(m_boundingBox.width());
    const double yScale = double(size.height()) / double(m_boundingBox.height());

    // create GS command line

    QString cmdBuf ( "gs -sOutputFile=" );
    cmdBuf += KProcess::quote(tmpFile.name());
    cmdBuf += " -q -g";
    cmdBuf += QString::number( wantedWidth );
    cmdBuf += "x";
    cmdBuf += QString::number( wantedHeight );

    if ( ( resolutionx > 0) && ( resolutiony > 0) )
    {
#if 0
        // Do not play with resolution for now.
        // It brings more problems at print than solutions
        cmdBuf += " -r";
        cmdBuf += QString::number( resolutionx );
        cmdBuf += "x";
        cmdBuf += QString::number( resolutiony );
#endif
    }

    cmdBuf += " -dSAFER -dPARANOIDSAFER -dNOPAUSE -sDEVICE=";
    cmdBuf += device;
    //cmdBuf += " -c 255 255 255 setrgbcolor fill 0 0 0 setrgbcolor";
    cmdBuf += " -";
    cmdBuf += " -c showpage quit";

    // run ghostview

    FILE* ghostfd = popen (QFile::encodeName(cmdBuf), "w");

    if ( ghostfd == 0 )
    {
        kdError(30003) << "No connection to GhostScript (in KoPictureEps::tryScaleWithGhostScript)" << endl;
        return 0; // error
    }

    // The translation is needed as GhostScript (7.07) cannot handle negative values in the bounding box otherwise.
    fprintf (ghostfd, "\n%d %d translate\n", -qRound(m_boundingBox.left()*xScale), -qRound(m_boundingBox.top()*yScale));
    fprintf (ghostfd, "%g %g scale\n", xScale, yScale);

    // write image to gs

    fwrite( m_rawData.data() + m_psStreamStart, sizeof(char), m_psStreamLength, ghostfd);

    pclose ( ghostfd );

    // load image
    if( !image.load (tmpFile.name()) )
    {
        // It failed - maybe the device isn't supported by gs
        return -1;
    }
    if ( image.size() != size ) // this can happen due to rounding problems
    {
        //kdDebug(30003) << "fixing size to " << size.width() << "x" << size.height()
        //          << " (was " << image.width() << "x" << image.height() << ")" << endl;
        image = image.scale( size ); // hmm, smoothScale instead?
    }
    kdDebug(30003) << "Image parameters: " << image.width() << "x" << image.height() << "x" << image.depth() << endl;
    return 1; // success
}

void KoPictureEps::scaleAndCreatePixmap(const QSize& size, bool fastMode, const int resolutionx, const int resolutiony )
{
    kdDebug(30003) << "KoPictureEps::scaleAndCreatePixmap " << size << " " << (fastMode?QString("fast"):QString("slow"))
        << " resolutionx: " << resolutionx << " resolutiony: " << resolutiony << endl;
    if ((size==m_cachedSize)
        && ((fastMode) || (!m_cacheIsInFastMode)))
    {
        // The cached pixmap has already the right size
        // and:
        // - we are in fast mode (We do not care if the re-size was done slowly previously)
        // - the re-size was already done in slow mode
        kdDebug(30003) << "Already cached!" << endl;
        return;
    }

    // Slow mode can be very slow, especially at high zoom levels -> configurable
    if ( !isSlowResizeModeAllowed() )
    {
        kdDebug(30003) << "User has disallowed slow mode!" << endl;
        fastMode = true;
    }

    // We cannot use fast mode, if nothing was ever cached.
    if ( fastMode && !m_cachedSize.isEmpty())
    {
        kdDebug(30003) << "Fast scaling!" << endl;
        // Slower than caching a QImage, but faster than re-sampling!
        QImage image( m_cachedPixmap.convertToImage() );
        m_cachedPixmap=image.scale( size );
        m_cacheIsInFastMode=true;
        m_cachedSize=size;
    }
    else
    {
        QTime time;
        time.start();

        QApplication::setOverrideCursor( Qt::waitCursor );
        m_cachedPixmap = scaleWithGhostScript( size, resolutionx, resolutiony );
        QApplication::restoreOverrideCursor();
        m_cacheIsInFastMode=false;
        m_cachedSize=size;

        kdDebug(30003) << "Time: " << (time.elapsed()/1000.0) << " s" << endl;
    }
    kdDebug(30003) << "New size: " << size << endl;
}

void KoPictureEps::draw(QPainter& painter, int x, int y, int width, int height, int sx, int sy, int sw, int sh, bool fastMode)
{
    if ( !width || !height )
        return;

    QSize screenSize( width, height );
    //kdDebug() << "KoPictureEps::draw screenSize=" << screenSize.width() << "x" << screenSize.height() << endl;

    QPaintDeviceMetrics metrics (painter.device());
    kdDebug(30003) << "Metrics: X: " << metrics.logicalDpiX() << " x Y: " << metrics.logicalDpiX() << " (in KoPictureEps::draw)" << endl;

    if ( painter.device()->isExtDev() ) // Is it an external device (i.e. printer)
    {
        kdDebug(30003) << "Drawing for a printer (in KoPictureEps::draw)" << endl;
        // For printing, always re-sample the image, as a printer has never the same resolution than a display.
        QImage image( scaleWithGhostScript( screenSize, metrics.logicalDpiX(), metrics.logicalDpiY() ) );
        // sx,sy,sw,sh is meant to be used as a cliprect on the pixmap, but drawImage
        // translates it to the (x,y) point -> we need (x+sx, y+sy).
        painter.drawImage( x + sx, y + sy, image, sx, sy, sw, sh );
    }
    else // No, it is simply a display
    {
        scaleAndCreatePixmap(screenSize, fastMode, metrics.logicalDpiX(), metrics.logicalDpiY() );

        // sx,sy,sw,sh is meant to be used as a cliprect on the pixmap, but drawPixmap
        // translates it to the (x,y) point -> we need (x+sx, y+sy).
        painter.drawPixmap( x + sx, y + sy, m_cachedPixmap, sx, sy, sw, sh );
    }
}

bool KoPictureEps::extractPostScriptStream( void )
// Note: it changes m_rawData, we cannot do anything of the preview.
{
    kdDebug(30003) << "KoPictureEps::extractPostScriptStream" << endl;
    QDataStream data( m_rawData, IO_ReadOnly );
    data.setByteOrder( QDataStream::LittleEndian );
    Q_UINT32 magic, offset, length;
    data >> magic;
    data >> offset;
    data >> length;
    if ( !length )
    {
        kdError(30003) << "Length of PS stream is zero!" << endl;
        return false;
    }
    if ( offset+length>m_rawData.size() )
    {
        kdError(30003) << "Data stream of the EPSF file is longer than file: " << offset << "+" << length << ">" << m_rawData.size() << endl;
        return false;
    }
    QByteArray ps;
    ps.duplicate( m_rawData.data()+offset, length );
    m_rawData=ps;
    return true;
}

bool KoPictureEps::load(const QByteArray& array, const QString& /* extension */ )
{

    kdDebug(30003) << "KoPictureEps::load" << endl;
    // First, read the raw data
    m_rawData=array;

    if (m_rawData.isNull())
    {
        kdError(30003) << "No data was loaded!" << endl;
        return false;
    }

    if ( ( m_rawData[0]==char(0xc5) ) && ( m_rawData[1]==char(0xd0) )
        && ( m_rawData[2]==char(0xd3) ) && ( m_rawData[3]==char(0xc6) ) )
    {
        // We have a so-called "MS-DOS EPS file", we have to extract the PostScript stream
        if (!extractPostScriptStream()) // Changes m_rawData
            return false;
        // ### TODO
        m_psStreamStart = 0;
        m_psStreamLength = m_rawData.size();
    }
    else
    {
        m_psStreamStart = 0;
        m_psStreamLength = m_rawData.size();
    }

    
    QTextStream stream(m_rawData, IO_ReadOnly);
    QString lineBox;
    QString line( stream.readLine() );
    kdDebug(30003) << "Header: " << line << endl;
    if (!line.startsWith("%!"))
    {
        kdError(30003) << "Not a PostScript file!" << endl;
        return false;
    }
    QRect rect;
    for(;;)
    {
        line=stream.readLine();
        kdDebug(30003) << "Checking line: " << line << endl;
        // ### TODO: it seems that the bounding box can be delayed in the trailer (GhostScript 7.07 does not support it either.)
        if (line.startsWith("%%BoundingBox:"))
        {
            lineBox=line;
            break;
        }
        else if (!line.startsWith("%%"))
            break; // Not a EPS comment anymore, so abort as we are not in the EPS header anymore
    }
    if (lineBox.isEmpty())
    {
        kdError(30003) << "KoPictureEps::load: could not find bounding box!" << endl;
        return false;
    }
    QRegExp exp("(\\-?[0-9]+\\.?[0-9]*)\\s(\\-?[0-9]+\\.?[0-9]*)\\s(\\-?[0-9]+\\.?[0-9]*)\\s(\\-?[0-9]+\\.?[0-9]*)");
    // ### FIXME: check return value of search, as the bounding box could be marked as "delayed" (What is the exact text then?)
    exp.search(lineBox);
    kdDebug(30003) << "Reg. Exp. Found: " << exp.capturedTexts() << endl;
    rect.setLeft((int)exp.cap(1).toDouble());
    rect.setTop((int)exp.cap(2).toDouble());
    rect.setRight((int)exp.cap(3).toDouble());
    rect.setBottom((int)exp.cap(4).toDouble());
    m_boundingBox=rect;
    m_originalSize=rect.size();
    kdDebug(30003) << "Rect: " << rect << " Size: "  << m_originalSize << endl;
    return true;
}

bool KoPictureEps::save(QIODevice* io)
{
    // We save the raw data, to avoid damaging the file by many load/save cycles
    Q_ULONG size=io->writeBlock(m_rawData); // WARNING: writeBlock returns Q_LONG but size() Q_ULONG!
    return (size==m_rawData.size());
}

QSize KoPictureEps::getOriginalSize(void) const
{
    return m_originalSize;
}

QPixmap KoPictureEps::generatePixmap(const QSize& size, bool smoothScale)
{
    scaleAndCreatePixmap(size,!smoothScale, 0, 0);
    return m_cachedPixmap;
}

QString KoPictureEps::getMimeType(const QString&) const
{
    return "image/x-eps";
}

QImage KoPictureEps::generateImage(const QSize& size)
{
    // 0, 0 == resolution unknown
    return scaleWithGhostScript(size, 0, 0);
}

void KoPictureEps::clearCache(void)
{
    m_cachedPixmap.resize(0, 0);
    m_cacheIsInFastMode=true;
    m_cachedSize=QSize();
}
