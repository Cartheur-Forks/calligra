/*
 *  kis_tool_airbrush.cc - part of Krayon
 *
 *  Copyright (c) 1999 Matthias Elter <me@kde.org>
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

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>

#include "kis_brush.h"
#include "kis_doc.h"
#include "kis_view.h"
#include "kis_vec.h"
#include "kis_cursor.h"
#include "kis_util.h"
#include "kis_tool_airbrush.h"
#include "kis_dlg_toolopts.h"
#include "kis_canvas.h"

AirBrushTool::AirBrushTool(KisDoc *doc, KisBrush *brush) : KisTool(doc)
{
    m_dragging = false;
    m_Cursor = KisCursor::airbrushCursor();
    m_dragdist = 0;
    density = 64;
    m_pDoc = doc;

    // initialize airbrush tool settings
    opacity = 255;
    usePattern = false;
    useGradient = false;

    setBrush(brush);

    pos.setX(-1);
    pos.setY(-1);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeoutPaint()));
}

AirBrushTool::~AirBrushTool()
{
}

void AirBrushTool::timeoutPaint()
{
    // kdDebug(0) << "timeoutPaint() caled" << endl;

    if(paint(pos, true))
    {
         m_pDoc->current()->markDirty(QRect(pos
            - m_pBrush->hotSpot(), m_pBrush->size()));
    }
}

void AirBrushTool::setBrush(KisBrush *brush)
{
    m_pBrush = brush;
    brushWidth =  (unsigned int) m_pBrush->width();
    brushHeight = (unsigned int) m_pBrush->height();

    // set the array of points to same size as brush
    brushArray.resize(brushWidth * brushHeight);
    brushArray.fill(0);

    kdDebug() << "setBrush(): "
        << "brushwidth "   << brushWidth
        << " brushHeight " << brushHeight
        << endl;

    // set custom cursor
    m_pView->kisCanvas()->setCursor( KisCursor::airbrushCursor() );
    m_Cursor = KisCursor::airbrushCursor();
}


void AirBrushTool::mousePress(QMouseEvent *e)
{
    KisImage * img = m_pDoc->current();
    if (!img) return;

    if(!img->getCurrentLayer())
        return;

    if(!img->getCurrentLayer()->visible())
        return;

    if (e->button() != QMouseEvent::LeftButton)
        return;

    m_dragging = true;

    pos = e->pos();
    pos = zoomed(pos);
    m_dragStart = pos;
    m_dragdist = 0;

    // clear array
    brushArray.fill(0);
    nPoints = 0;

    kdDebug() << "mousePress(): "
        << "brushwidth " << brushWidth
        << " brushHeight " << brushHeight
        << endl;

    kdDebug() << "mousePress(): "
        << "m_pBrush->hotSpot().x() " << m_pBrush->hotSpot().x()
        << "m_pBrush->hotSpot().y() " << m_pBrush->hotSpot().y()
        << endl;

    // Start the timer - 50 milliseconds or
    // 20 timeouts/second (multishot)
    timer->start(50, FALSE);
}


bool AirBrushTool::paint(QPoint pos, bool timeout)
{
    KisImage * img = m_pDoc->current();
    if (!img)	    return false;

    KisLayer *lay = img->getCurrentLayer();
    if (!lay)       return false;

    if (!m_pBrush)  return false;

    if (!img->colorMode() == cm_RGB && !img->colorMode() == cm_RGBA)
	    return false;

    if(!m_dragging) return false;

    int hotX = m_pBrush->hotSpot().x();
    int hotY = m_pBrush->hotSpot().y();

    int startx = pos.x() - hotX;
    int starty = pos.y() - hotY;

    QRect clipRect(startx, starty, m_pBrush->width(), m_pBrush->height());
    if (!clipRect.intersects(lay->imageExtents()))
        return false;

    clipRect = clipRect.intersect(lay->imageExtents());

    int sx = clipRect.left() - startx;
    int sy = clipRect.top() - starty;
    int ex = clipRect.right() - startx;
    int ey = clipRect.bottom() - starty;

    uchar *sl;
    uchar bv, invbv;
    uchar r, g, b, a;
    int   v;

    int red   = m_pView->fgColor().R();
    int green = m_pView->fgColor().G();
    int blue  = m_pView->fgColor().B();

    bool alpha = (img->colorMode() == cm_RGBA);

    for (int y = sy; y <= ey; y++)
    {
        sl = m_pBrush->scanline(y);

        for (int x = sx; x <= ex; x++)
	    {
            /* get a truly ??? random number and divide it by
            desired density - if x is that number, paint
            a pixel from brush there this turn */
            int nRandom = KApplication::random();

            bool paintPoint = false;

            if((nRandom % density) == ((x - sx) % density))
                paintPoint = true;

            // don't keep painting over points already painted
            // this makes image too dark and grany, eventually -
            // that effect is good with the regular brush tool,
            // but not with an airbrush or with the pen tool
            if(timeout && (brushArray[brushWidth * (y-sy) + (x-sx) ] > 0))
            {
                paintPoint = false;
            }

            if(paintPoint)
            {
	            r = lay->pixel(0, startx + x, starty + y);
	            g = lay->pixel(1, startx + x, starty + y);
	            b = lay->pixel(2, startx + x, starty + y);

	            bv = *(sl + x);
	            if (bv == 0) continue;

                invbv = 255 - bv;

                b = ((blue * bv) + (b * invbv))/255;
                g = ((green * bv) + (g * invbv))/255;
                r = ((red * bv) + (r * invbv))/255;

	            lay->setPixel(0, startx + x, starty + y, r);
	            lay->setPixel(1, startx + x, starty + y, g);
	            lay->setPixel(2, startx + x, starty + y, b);

	            if (alpha)
	            {
	                a = lay->pixel(3, startx + x, starty + y);
	                v = a + bv;
	                if (v < 0 ) v = 0;
		            if (v > 255 ) v = 255;
		            a = (uchar) v;

	                lay->setPixel(3, startx + x, starty + y, a);
	            }
                // add this point to points already painted
                if(timeout)
                {
                    brushArray[brushWidth * (y-sy) + (x-sx)] = 1;
                    nPoints++;
                }
            }
	    }
    }

    return true;
}


void AirBrushTool::mouseMove(QMouseEvent *e)
{
    KisImage * img = m_pDoc->current();
    if (!img) return;

    int spacing = m_pBrush->spacing();
    if (spacing <= 0) spacing = 1;

    if(m_dragging)
    {
        if( !img->getCurrentLayer()->visible() )
        	return;

        pos = e->pos();
        int mouseX = e->x();
        int mouseY = e->y();

        pos = zoomed(pos);
        mouseX = zoomed(mouseX);
        mouseY = zoomed(mouseY);

        KisVector end(mouseX, mouseY);
        KisVector start(m_dragStart.x(), m_dragStart.y());

        KisVector dragVec = end - start;
        float saved_dist = m_dragdist;
        float new_dist = dragVec.length();
        float dist = saved_dist + new_dist;

        if ((int)dist < spacing)
	    {
            // save for next movevent
	        m_dragdist += new_dist;
	        m_dragStart = pos;
	        return;
	    }
        else
	        m_dragdist = 0;

        dragVec.normalize();
        KisVector step = start;

        while (dist >= spacing)
	    {
	        if (saved_dist > 0)
	        {
	            step += dragVec * (spacing-saved_dist);
	            saved_dist -= spacing;
	        }
	        else
	            step += dragVec * spacing;

	        QPoint p(qRound(step.x()), qRound(step.y()));

	        if (paint(p, false))
            {
                img->markDirty(QRect(p - m_pBrush->hotSpot(),
                    m_pBrush->size()));
            }

 	        dist -= spacing;
	    }

        //save for next movevent
        if (dist > 0) m_dragdist = dist;
        m_dragStart = pos;
    }
}


void AirBrushTool::mouseRelease(QMouseEvent *e)
{
    // perhaps erase on right button
    if (e->button() != LeftButton)  return;

    // stop the timer - restart when mouse pressed again
    timer->stop();

    //reset array of points
    brushArray.fill(0);
    nPoints = 0;

    m_dragging = false;
}

void AirBrushTool::optionsDialog()
{
    ToolOptsStruct ts;

    ts.usePattern       = usePattern;
    ts.useGradient      = useGradient;
    ts.opacity          = opacity;

    bool old_usePattern   = usePattern;
    bool old_useGradient  = useGradient;
    unsigned int  old_opacity      = opacity;

    ToolOptionsDialog *pOptsDialog
        = new ToolOptionsDialog(tt_airbrushtool, ts);

    pOptsDialog->exec();

    if(!pOptsDialog->result() == QDialog::Accepted)
        return;
    else {
        opacity       = pOptsDialog->airBrushToolTab()->opacity();
        usePattern    = pOptsDialog->airBrushToolTab()->usePattern();
        useGradient   = pOptsDialog->airBrushToolTab()->useGradient();

        // User change value ?
        if ( old_usePattern != usePattern || old_useGradient != useGradient || old_opacity != opacity ) {
            // set airbrush tool settings
            m_pDoc->setModified( true );
        }
    }
}

void AirBrushTool::setupAction(QObject *collection)
{
	KToggleAction *toggle = new KToggleAction(i18n("&Airbrush tool"), "airbrush", 0, this, 
			SLOT(toolSelect()), collection, "tool_airbrush");

        toggle -> setExclusiveGroup("tools");
}

QDomElement AirBrushTool::saveSettings(QDomDocument& doc) const
{
	QDomElement tool = doc.createElement("tool");

	tool.setAttribute("opacity", opacity);
	tool.setAttribute("useCurrentGradient", static_cast<int>(useGradient));
	tool.setAttribute("useCurrentPattern", static_cast<int>(usePattern));
	return tool;
}

bool AirBrushTool::loadSettings(QDomElement& tool)
{
	bool rc = tool.tagName() == "airbrushTool";

	if (rc) {
		opacity = tool.attribute("opacity").toInt();
		useGradient = static_cast<bool>(tool.attribute("useCurrentGradient").toInt());
		usePattern = static_cast<bool>(tool.attribute("useCurrentPattern").toInt());
	}

	return rc;
}

#include "kis_tool_airbrush.moc"

