/*
 *  kis_tool.cc - part of KImageShop
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qevent.h>
#include <qpainter.h>
#include <qpointarray.h>

#include <kdebug.h>
#include <kmessagebox.h>

#include "kis_canvas.h"
#include "kis_cursor.h"
#include "kis_doc.h"
#include "kis_view.h"
#include "kis_tool.h"

KisTool::KisTool(KisDoc *doc, const char * /*name*/) //: QObject(doc, name)
{
	m_doc = doc;
	m_view = doc -> currentView();
	Q_ASSERT(m_view);
	m_cursor = KisCursor::arrowCursor();
}

KisTool::~KisTool() 
{
	QObject::disconnect(this);
}

void KisTool::optionsDialog()
{
	KMessageBox::sorry(0, "Options for current tool... coming soon", "", false);  
}

void KisTool::setChecked(bool /*check*/)
{
}

void KisTool::setCursor(const QCursor& c)
{
	m_cursor = c;
}

void KisTool::setCursor()
{
	setCursor(arrowCursor);
}

QCursor KisTool::cursor() const
{
	return m_cursor;
}

// translate integer for zoom factor
int KisTool::zoomed(int n) const
{
	return static_cast<int>(n / m_view -> zoomFactor());
}

// translate integer for zoom factor
int KisTool::zoomedX(int n) const
{
	// current zoom factor for this view
	float zF = m_view -> zoomFactor();

	return static_cast<int>(n / zF);
}

// translate integer for zoom factor
int KisTool::zoomedY(int n) const
{
	// current zoom factor for this view
	float zF = m_view -> zoomFactor();

	return static_cast<int>(n / zF);    
}

// translate point for zoom factor
QPoint KisTool::zoomed(const QPoint & pt) const
{
	// current zoom factor for this view
	float zF = m_view -> zoomFactor();
   
	// translate startpoint for zoom factor
	// this is almost always a from a mouse event
     
	int startx = static_cast<int>(pt.x() / zF);
	int starty = static_cast<int>(pt.y() / zF);
    
	// just dealing with canvas, no scroll accounting        
	return QPoint(startx, starty);
}

void KisTool::mousePress(QMouseEvent*)
{
}

void KisTool::mouseMove(QMouseEvent*)
{
}

void KisTool::mouseRelease(QMouseEvent*)
{
}

// get QRect for draw freehand in layer.
QRect KisTool::getDrawRect(const QPointArray& points) const
{
	int maxX = 0, maxY = 0;
	int minX = 0, minY = 0;
	int tmpX = 0, tmpY = 0;
	bool first = true;
	QPointArray::ConstIterator it;

	for (it = points.begin(); it != points.end(); it++) {
		const QPoint& pt = *it;

		tmpX = pt.x();
		tmpY = pt.y();

		if (first) {
			maxX = tmpX;
			maxY = tmpY;
			minX = tmpX;
			minY = tmpY;
			first = false;
		}

		if (maxX < tmpX)
			maxX = tmpX;

		if (maxY < tmpY)
			maxY = tmpY;

		if (minX > tmpX)
			minX = tmpX;

		if (minY > tmpY)
			minY = minY;
	}

	QPoint topLeft = QPoint(minX, minY);
	QPoint bottomRight = QPoint(maxX, maxY);
	return QRect(zoomed(topLeft), zoomed(bottomRight));
}

// get QPointArray for draw freehand and polyline in layer.
QPointArray KisTool::zoomPointArray(const QPointArray& points) const
{
	QPointArray rc(points.size());
	int count = 0;
    
	for (QPointArray::ConstIterator it = points.begin(); it != points.end(); it++) {
		rc.setPoint(count, zoomed(*it));
		count++;
	}

	return rc;
}

// clip select area image
void KisTool::setClipImage()
{
    // set select area clip
    if ( !m_doc->setClipImage() ) {
        kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): m_doc->setClipImage() failed" << endl;
        return;
    }

    // get select area clip
    if ( m_doc->getClipImage() ) {
        kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): m_doc->getClipImage() success!!" << endl;
        m_clipImage = *m_doc->getClipImage();

        if ( m_clipImage.isNull() ) {
            kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): clip image is null!" << endl;
            return;
        }
        // if dealing with 1 or 8 bit image, convert to 16 bit
        if ( m_clipImage.depth() < 16 ) {
            QImage smoothImage = m_clipImage.smoothScale( m_clipImage.width(), m_clipImage.height() );
            m_clipImage = smoothImage;

            if ( m_clipImage.isNull() ) {
                kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): smooth scale clip image is null!" << endl;
                return;
            }
        }

        m_clipPixmap.convertFromImage( m_clipImage, QPixmap::AutoColor );
        if ( m_clipPixmap.isNull() ) {
            kdDebug() << "FreehandSelectTool::setClipImage(): can't convert from image!" << endl;
            return;
        }

        if ( !m_clipImage.hasAlphaBuffer() )
            kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): clip image has no alpha buffer!" << endl;
    }

    kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): Success set up clip image!!" << endl;
}

// drag clip image
void KisTool::dragSelectImage(const QPoint& dragPoint, const QPoint& hotSpot) const
{
    KisImage *img = m_doc->currentImg();
    if ( !img )
        return;

    KisLayer *lay = img->getCurrentLayer();
    if ( !lay )
        return;

    float zF = m_view->zoomFactor();
    int pX = dragPoint.x();
    int pY = dragPoint.y();
    pX = (int)( pX / zF );
    pY = (int)( pY / zF );
    QPoint point = QPoint( pX, pY );

    QPainter p;
    p.begin( m_view->kisCanvas() );
    p.scale( zF, zF );

    QRect imageRect( point.x() - hotSpot.x(), point.y() - hotSpot.y(), 
                     m_clipPixmap.width(), m_clipPixmap.height() );
    imageRect = imageRect.intersect( img->imageExtents() );

    if ( imageRect.top() > img->height() || imageRect.left() > img->width()
         || imageRect.bottom() < 0 || imageRect.right() < 0 ) {
        p.end();
        return;
    }

    if ( !imageRect.intersects( img->imageExtents() ) ) {
        p.end();
        return;
    }

    imageRect = imageRect.intersect( img->imageExtents() );

    int startX = 0;
    int startY = 0;

    if ( m_clipPixmap.width() > imageRect.right() )
        startX = m_clipPixmap.width() - imageRect.right();
    if ( m_clipPixmap.height() > imageRect.bottom() )
        startY = m_clipPixmap.height() - imageRect.bottom();

    // paranioa
    if( startX < 0 ) 
        startX = 0;
    if( startY < 0 )
        startY = 0;
    if( startX > m_clipPixmap.width() )
        startX = m_clipPixmap.width();
    if( startY > m_clipPixmap.height() )
        startY = m_clipPixmap.height();

    int xt = m_view->xPaintOffset() - m_view->xScrollOffset();
    int yt = m_view->yPaintOffset() - m_view->yScrollOffset();

    p.translate( xt, yt );

    p.drawPixmap( imageRect.left(), imageRect.top(),
                  m_clipPixmap,
                  startX, startY,
                  imageRect.width(), imageRect.height() );

    p.end();
}

// pasete clip image
bool KisTool::pasteClipImage(const QPoint& pos)
{
	KisImage *img = m_doc -> currentImg();

	if (!img)
		return false;

	KisLayer *lay = img -> getCurrentLayer();

	if (!lay)
		return false;

	QImage *qimg = &m_clipImage;

	int startx = pos.x();
	int starty = pos.y();

	QRect clipRect(startx, starty, qimg -> width(), qimg -> height());

	if (!clipRect.intersects(img -> getCurrentLayer() -> imageExtents()))
		return false;

	clipRect = clipRect.intersect(img -> getCurrentLayer() -> imageExtents());

	int sx = clipRect.left() - startx;
	int sy = clipRect.top() - starty;
	int ex = clipRect.right() - startx;
	int ey = clipRect.bottom() - starty;

	uchar r, g, b, a;
	int   v = 255;
	int   bv = 0;
	QRgb rgb;

	int red     = m_view->fgColor().R();
	int green   = m_view->fgColor().G();
	int blue    = m_view->fgColor().B();

	bool grayscale = false;
	bool colorBlending = false;
	bool layerAlpha = ( img->colorMode() == cm_RGBA );
	bool imageAlpha = qimg->hasAlphaBuffer();

	for (int y = sy; y <= ey; ++y ) {
		for (int x = sx; x <= ex; ++x) {
			// destination binary values by channel
			rgb = lay -> pixel(startx + x, starty + y);
			r = qRed(rgb);
			g = qGreen(rgb);
			b = qBlue(rgb);
			a = qAlpha(rgb);

			// pixel value in scanline at x offset to right
			uint *p = (uint *)qimg -> scanLine(y) + x;

			// if the alpha value of the pixel in the selection
			// image is 0, don't paint the pixel.  It's transparent.
			if (imageAlpha && qAlpha(*p) == 0)
				continue;

			if (layerAlpha) {
				if (grayscale) {
					v = a + bv;
				}
				else {
					v = qAlpha(*p);
					v += a;
				}
				
				if (v < 0) 
					v = 0;

				if (v > 255)
					v = 255;

				a = (uchar) v;
			}

			if (colorBlending) {
				// make mud!
				r = (qRed(*p) + r + red ) / 3;
				g = (qGreen(*p) + g + green) / 3;
				b = (qBlue(*p) + b + blue) / 3;
				lay -> setPixel(startx + x, starty + y, qRgba(r, g, b, a));
			}
			else
				lay -> setPixel(startx + x, starty + y, *p);
		}
	}

	return true;
}

bool KisTool::shouldRepaint() const
{
	return false;
}

void KisTool::setBrush(KisBrush *brush)
{
	m_brush = brush;
}

void KisTool::clearOld()
{
}

void KisTool::toolSelect()
{
	if (m_view)
		m_view -> activateTool(this);
}

void KisTool::setPattern(KisPattern *pattern)
{
	m_pattern = pattern;
}

bool KisTool::willModify() const
{
	return true;
}

// set select cursor
void KisTool::setSelectCursor()
{
	m_cursor = KisCursor::selectCursor();

	if (m_view)
		m_view -> kisCanvas() -> setCursor(KisCursor::selectCursor());
}

// set move cursor
void KisTool::setMoveCursor()
{
	m_cursor = KisCursor::moveCursor();

	if (m_view)
		m_view -> kisCanvas() -> setCursor(KisCursor::moveCursor());
}

QDomElement KisTool::saveSettings(QDomDocument& /*doc*/) const
{
	return QDomElement();
}

bool KisTool::loadSettings(QDomElement& /*elem*/)
{
	return false;
}

void KisTool::paintEvent(QPaintEvent *)
{
}

void KisTool::enterEvent(QEvent *)
{
}

void KisTool::leaveEvent(QEvent *)
{
}

bool KisTool::setClip()
{
	return false;
}

QString KisTool::settingsName() const
{
	return "tool";
}

#include "kis_tool.moc"

