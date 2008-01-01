/*
 *  Copyright (c) 2000 Clara Chan
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
#include <math.h>

#include <QPainter>
#include <QRect>

#include <kis_debug.h>

#include <KoColor.h>

#include <kis_vec.h>
#include <kis_random_accessor.h>
#include <kis_paint_device.h>

#include "brush.h"
#include "bristle.h"
#include "stroke.h"
#include "sample.h"


// Constructor
Stroke::Stroke (Brush * brush )
{
    m_brush = brush;
    m_numBristles = brush->numberOfBristles();
    m_oldPathx.resize(m_numBristles);
    m_oldPathy.resize(m_numBristles);
    m_valid.resize(m_numBristles);
}

Stroke::~Stroke()
{
    foreach (Sample* sample, m_samples) delete sample;
}


void Stroke::storeOldPath ( double x1, double y1 )
{
    int i;
    for ( i=0; i < m_numBristles; i++ ) {
        m_oldPathx[i].append( x1 );
        m_oldPathy[i].append( y1 );
        m_valid[i].append( true );
    }
}

void Stroke::drawLine( KisPaintDeviceSP dev, double x1, double y1, double x2, double y2, double width, const KoColor & color )
{
    Q_UNUSED(width);
    
    if (!dev) return;

    QPointF pos1 = QPointF( x1, y1 );
    QPointF pos2 = QPointF( x2, y2 );

    KisVector2D end(pos2);
    KisVector2D start(pos1);

    KisVector2D dragVec = end - start;
    KisVector2D movement = dragVec;

    double xScale = 1;
    double yScale = 1;

    double dist = dragVec.length();

    if (dist < 1) {
        return;
    }

    dragVec.normalize();

    KisVector2D step(0, 0);

    //dbgKrita << "drawLine " << x1 << ", " << y1 << " : " << x2 << ", " << y2 << ". Width: " << width << ", dist: " << dist << endl;

    while (dist >= 1) {
        step += dragVec;
        QPoint p = QPointF(start.x() + (step.x() / xScale), start.y() + (step.y() / yScale)).toPoint();
        
        dev->setPixel(p.x(), p.y(), color);
        dist -= 1;
    }
}

void Stroke::drawWuLine(KisPaintDeviceSP dev, double x1, double y1, double x2, double y2, double width, const KoColor & color )
{
    Q_UNUSED(width);
    Q_UNUSED(color);
    // From Wikipedia...
    
    if (!dev) return;
    
    if (x2 < x1) {
        double tmp = x2;
        x2 = x1;
        x1 = tmp;
        tmp = y2;
        y2 = y1;
        y1 = tmp;
    }

    double dx = x2 - x1;
    double dy = y2 - y1;
    double gradient = dy / dx;

    double xend = round(x1);
    double yend = y1 + gradient * (xend - x1);
    double tmp;
    double xgap = 1 - modf(x1 + 0.5, &tmp);
    
    double xpxl1 = xend;  // this will be used in the main loop
    
    double ypxl1 = static_cast<int>(yend); // Remove the fractional part the fast way

    dev->setPixel( (qint32)xpxl1, (qint32)ypxl1, Qt::black, (qint32) ( (1 - modf(yend, &tmp)) * xgap) );
    dev->setPixel( (qint32)xpxl1, (qint32)ypxl1 + 1, Qt::black, (qint32) ( modf(yend, &tmp) * xgap) );

    double intery = yend + gradient; // first y-intersection for the main loop

    // handle second endpoint
    xend = round(x2);
    yend = y2 + gradient * (xend - x2);
    xgap = modf(x2 + 0.5, &tmp);

    double xpxl2 = xend;  // this will be used in the main loop
    double ypxl2 = static_cast<int>(yend);
    
    dev->setPixel( (qint32)xpxl2, (qint32)ypxl2, Qt::black, (qint32) ( (1 - modf(yend, &tmp)) * xgap) );
    dev->setPixel( (qint32)xpxl2, (qint32)ypxl2 + 1, Qt::black, (qint32) ( modf(yend, &tmp) * xgap) );

    // main loop
    for (int x =  (qint32)(xpxl1 + 1); x < xpxl2; ++x) {
        dev->setPixel(x, static_cast<int>(intery), Qt::black, (qint32)(1 - modf(intery, &tmp)));
        dev->setPixel(x, static_cast<int>(intery) + 1, Qt::black, (qint32)modf(intery, &tmp));
        intery = intery + gradient;
    }
    
}

// draw the stroke by drawing the old paths, then the new segment
void Stroke::draw (KisPaintDeviceSP dev)
{
    
    if ( m_samples.size() <= 1 ) return; // No sample to initialize from

    int i = m_samples.size() - 1;
    double pressure = (double)m_samples[i]->pressure();
    int x = m_samples[i]->x();
    int y = m_samples[i]->y();
    double tiltx = m_samples[i]->tiltX();
    double tilty = m_samples[i]->tiltY();

    // using pressure info to reposition bristles
    m_brush->repositionBristles( pressure );

    // draw the new segment
    for ( i = 0; i < m_numBristles; i++ ) {
        if ( testThreshold ( i, pressure, tiltx, tilty ) ) {
            if ( m_valid[i] [ m_oldPathx[i].size() -1 ] ) {
                drawLine( dev,
                          m_oldPathx[i][m_oldPathx[i].size()-1],
                          m_oldPathy[i][m_oldPathy[i].size()-1],
                          m_brush->m_bristles[i].getX() + x,
                          m_brush->m_bristles[i].getY() + y,
                          m_brush->m_bristles[i].getThickness(),
                          m_color);
                          
                m_brush->m_bristles[i].depleteInk ( 1 ); //remove one unit of ink from bristle
            }
            m_valid[i].append( true );
        }
        else {
            m_valid[i].append( false );
        }
        // store new positions in oldPaths
        m_oldPathx[i].append( m_brush->m_bristles[i].getX()+x );
        m_oldPathy[i].append( m_brush->m_bristles[i].getY()+y );
    }

}


void Stroke::setColor ( const KoColor & color )
{
    m_color = color;
}

void Stroke::storeSample ( Sample * sample )
{
    m_samples.append( sample );
}

bool Stroke::testThreshold ( int i, double pressure, double tx, double ty )
{
    Q_UNUSED(tx);
    Q_UNUSED(ty);
    
    if ( ( m_brush->m_bristles[i].getPreThres() < pressure )
         && m_brush->m_bristles[i].getInkAmount() > 0 )
        return 1;
    else
        return 0;
}







