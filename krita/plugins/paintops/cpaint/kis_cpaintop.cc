/*
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
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

#include <QColor>
#include <QComboBox>
#include <QDomElement>
#include <QPainter>
#include <QRect>

#include <kis_debug.h>

#include <KoInputDevice.h>

#include <kis_brush.h>
#include <kis_global.h>
#include <kis_paint_device.h>

#include <kis_painter.h>
#include <kis_types.h>
#include <kis_paintop.h>
#include <kis_iterator.h>
#include <kis_iterators_pixel.h>
#include <kis_selection.h>

#include <kis_int_spinbox.h>

#include "kis_cpaintop.h"
#include "ui_wdgcpaintoptions.h"
#include "brush.h"
#include "qtoolbutton.h"
#include "stroke.h"
#include "sample.h"
#include "bristle.h"


KisCPaintOpFactory::KisCPaintOpFactory()
{
    m_brushes.resize( 6 );
    for ( int i=0; i < 6; i++ )
        m_brushes[i] = new Brush ( i+1 );
}

KisCPaintOpFactory::~KisCPaintOpFactory()
{
/*
    for (uint i = 0; i < m_brushes.count(); i++) {
        delete m_brushes[i];
        m_brushes[i] = 0;
    }
*/
}

KisPaintOp * KisCPaintOpFactory::createOp(const KisPaintOpSettings *settings,
                                          KisPainter * painter,
                                          KisImageSP image)
{
    Q_UNUSED( image );
    dbgKrita << settings;
    
    const KisCPaintOpSettings * cpaintOpSettings =
        dynamic_cast<const KisCPaintOpSettings*>( settings );

    Q_ASSERT( cpaintOpSettings );
    if ( !cpaintOpSettings ) return 0;

    int curBrush = cpaintOpSettings->brush();
    if ( curBrush > 5 ) {
        curBrush = 5;
    }
    else if ( curBrush < 0 ) {
        curBrush = 0;
    }

    KisPaintOp * op = new KisCPaintOp( m_brushes[curBrush], cpaintOpSettings, painter );

    Q_CHECK_PTR( op );
    return op;
}

KisPaintOpSettings *KisCPaintOpFactory::settings(QWidget * parent, const KoInputDevice& inputDevice, KisImageSP image)
{
    Q_UNUSED( inputDevice );
    Q_UNUSED( image );
    return new KisCPaintOpSettings( parent,  m_brushes);
}

//=================

KisCPaintOpSettings::KisCPaintOpSettings( QWidget * parent,  Q3ValueVector<Brush*> brushes)
    : KisPaintOpSettings( )
{
    m_brushes = brushes;
    m_optionsWidget = new QWidget( parent );
    m_options = new Ui::WdgCPaintOptions( );
    m_options->setupUi( m_optionsWidget );
    m_options->intInk->setRange( 0, 255 );
    m_options->intWater->setRange( 0, 255 );


    connect( m_options->bnInk, SIGNAL( clicked() ), this, SLOT( resetCurrentBrush() ) );
}


int KisCPaintOpSettings::brush() const
{
    return m_options->cmbBrush->currentIndex();
}

int KisCPaintOpSettings::ink() const
{
    return m_options->intInk->value();
}

int KisCPaintOpSettings::water() const
{
    return m_options->intWater->value();
}


void KisCPaintOpSettings::resetCurrentBrush()
{
    Brush * b = m_brushes[m_options->cmbBrush->currentIndex()];
    b->addInk();
}

void KisCPaintOpSettings::fromXML(const QDomElement& elt)
{
    m_options->cmbBrush->setCurrentIndex( elt.attribute("brush", "0" ).toInt() );
    m_options->intInk->setValue( elt.attribute("ink", "0" ).toInt() );
    m_options->intWater->setValue( elt.attribute("water", "0" ).toInt() );
}

void KisCPaintOpSettings::toXML(QDomDocument& /*doc*/, QDomElement& elt) const
{
    elt.setAttribute("brush", QString::number(brush()));
    elt.setAttribute("ink", QString::number(ink()));
    elt.setAttribute("water", QString::number(water()));
}

//=================

KisCPaintOp::KisCPaintOp(Brush * brush, const KisCPaintOpSettings * settings, KisPainter * painter)
    : KisPaintOp(painter)
{
    m_currentBrush = brush;
    m_ink = settings->ink();
    m_water = settings->water();
    newStrokeFlag = true;
    m_color = this->painter()->paintColor();
    m_stroke = 0;
    KisPaintDeviceSP dev = painter->device();
}

KisCPaintOp::~KisCPaintOp()
{
    delete m_stroke;
}



void KisCPaintOp::paintAt(const KisPaintInformation& info)
{
    if (!painter()->device()) return;

    KisPaintDeviceSP device = painter()->device();

    sampleCount++;
    Sample * newSample = new Sample;
    newSample->setPressure ( (int)( info.pressure() * 500 ) );
    newSample->setX ( (int)info.pos().x() );
    newSample->setY ( (int)info.pos().y() );
    newSample->setTiltX ( (int)info.xTilt() );
    newSample->setTiltY ( (int)info.yTilt() );


    if ( newStrokeFlag ) {
        m_lastPoint = info.pos();
        m_stroke = new Stroke( m_currentBrush);
        m_stroke->setColor( m_color );
        m_stroke->storeSample( newSample );
        m_stroke->storeOldPath( info.pos().x(), info.pos().y() );
        newStrokeFlag = false;
    }
    else {
        if ( m_stroke )
            m_stroke->storeSample( newSample );
        else {
            delete newSample;
            sampleCount--;
            return;
        }
    }

    if ( m_stroke ) {
        //int brushSize = m_currentBrush->size();
        KisPaintDeviceSP dab = new KisPaintDevice(device->colorSpace());
        m_stroke->draw( dab );
        painter()->bitBlt( QPoint( 0, 0 ), dab, m_tempImage.rect() );
    }
    m_lastPoint = info.pos();
}

#include "kis_cpaintop.moc"
