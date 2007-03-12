/*
 *  kis_tool_perspectivegrid.cc - part of Krita
 *
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License.
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
#include <kis_tool_perspectivegrid.h>

#include <qapplication.h>
#include <qpainter.h>
#include <qregion.h>
#include <qwidget.h>
#include <qlayout.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <klocale.h>

#include <KoPointerEvent.h>
#include <kis_cursor.h>
#include <kis_image.h>
#include <KoPointerEvent.h>
#include <kis_perspective_grid_manager.h>
#include <kis_selected_transaction.h>
#include <kis_painter.h>
#include <kis_paintop_registry.h>
#include <kis_vec.h>

#include <kis_canvas.h>

KisToolPerspectiveGrid::KisToolPerspectiveGrid()
    : super(i18n("Perspective Grid")), m_handleSize(13), m_handleHalfSize(6)

{
    setName("tool_perspectivegrid");


    m_dragging = false;
}

KisToolPerspectiveGrid::~KisToolPerspectiveGrid()
{
}

void KisToolPerspectiveGrid::activate()
{
    m_subject->perspectiveGridManager()->startEdition();
    if( ! m_currentImage->perspectiveGrid()->hasSubGrids() )
    {
        m_mode = MODE_CREATION;
        m_points.clear();
    } else {
        m_mode = MODE_EDITING;
        drawGrid();
    }
    super::activate();
}

void KisToolPerspectiveGrid::deactivate()
{
    m_subject->perspectiveGridManager()->stopEdition();
    m_subject->perspectiveGridManager()->setGridVisible( true);
    if( m_mode == MODE_CREATION )
    {
        drawGridCreation();
        m_points.clear();
        m_dragging = false;
    } else {
        drawGrid();
    }
}


void KisToolPerspectiveGrid::update (KisCanvasSubject *subject)
{
    m_subject = subject;
    super::update(m_subject);
}

bool KisToolPerspectiveGrid::mouseNear(const QPoint& mousep, const QPoint point)
{
    return (QRect( (point.x() - m_handleHalfSize), (point.y() - m_handleHalfSize), m_handleSize, m_handleSize).contains(mousep) );
}

void KisToolPerspectiveGrid::buttonPress(KoPointerEvent *event)
{
    KisPerspectiveGrid* pGrid = m_currentImage->perspectiveGrid();
    if(!pGrid->hasSubGrids() && m_mode != MODE_CREATION)
    { // it's possible that the perspectiv grid was cleared
        m_mode = MODE_CREATION;
        m_points.clear();
    }
    if( m_mode == MODE_CREATION && event->button() == Qt::LeftButton)
    {
        m_dragging = true;

        if (m_points.isEmpty())
        {
            m_dragStart = event->pos().toPointF();
            m_dragEnd = event->pos().toPointF();
            m_points.append(m_dragStart);
        } else {
            m_dragStart = m_dragEnd;
            m_dragEnd = event->pos().toPointF();
            drawGridCreation();
        }
    } else if(m_mode == MODE_EDITING && event->button() == Qt::LeftButton){
        // Look for the handle which was pressed
        if (!m_subject)
            return;
        KisCanvasController *controller = m_subject->canvasController();
        Q_ASSERT(controller);
        QPoint mousep = controller->windowToView( event->pos().roundQPoint() );

        for( QList<KisSubPerspectiveGrid*>::const_iterator it = pGrid->begin(); it != pGrid->end(); ++it)
        {
            KisSubPerspectiveGrid* grid = *it;
            if( mouseNear( mousep, controller->windowToView(grid->topLeft()->toPoint() ) ) )
            {
//                 kDebug() << " PRESS TOPLEFT HANDLE " << endl;
                m_mode = MODE_DRAGING_NODE;
                m_selectedNode1 = grid->topLeft();
                break;
            }
            else if( mouseNear( mousep, controller->windowToView(grid->topRight()->toPoint() ) ) )
            {
//                 kDebug() << " PRESS TOPRIGHT HANDLE " << endl;
                m_mode = MODE_DRAGING_NODE;
                m_selectedNode1 = grid->topRight();
                break;
            }
            else if( mouseNear( mousep, controller->windowToView(grid->bottomLeft()->toPoint() ) ) )
            {
//                 kDebug() << " PRESS BOTTOMLEFT HANDLE " << endl;
                m_mode = MODE_DRAGING_NODE;
                m_selectedNode1 = grid->bottomLeft();
                break;
            }
            else if( mouseNear( mousep, controller->windowToView(grid->bottomRight()->toPoint() ) ) )
            {
//                 kDebug() << " PRESS BOTTOMRIGHT HANDLE " << endl;
                m_mode = MODE_DRAGING_NODE;
                m_selectedNode1 = grid->bottomRight();
                break;
            }
            else if( !grid->leftGrid() && mouseNear( mousep, controller->windowToView( QPointF( ((*grid->topLeft() + *grid->bottomLeft() )*0.5) ) ).roundQPoint() ) )
            {
//                 kDebug() << " PRESS LEFT HANDLE " << endl;
                m_mode = MODE_DRAGING_TRANSLATING_TWONODES;
                drawGrid();
                m_selectedNode1 = new KisPerspectiveGridNode( *grid->topLeft() );
                m_selectedNode2 = new KisPerspectiveGridNode( *grid->bottomLeft() );
                KisSubPerspectiveGrid* newsubgrid = new KisSubPerspectiveGrid( m_selectedNode1, grid->topLeft() , grid->bottomLeft(), m_selectedNode2);
                m_dragEnd = event->pos().toPointF();
                newsubgrid->setRightGrid( grid);
                grid->setLeftGrid( newsubgrid);
                pGrid->addNewSubGrid( newsubgrid);
                drawGrid();
                break;
            }
            else if( !grid->rightGrid() && mouseNear( mousep, controller->windowToView( ((*grid->topRight() + *grid->bottomRight() )*0.5) ).roundQPoint() ) )
            {
//                 kDebug() << " PRESS RIGHT HANDLE " << endl;
                m_mode = MODE_DRAGING_TRANSLATING_TWONODES;
                drawGrid();
                m_selectedNode1 = new KisPerspectiveGridNode( *grid->topRight() );
                m_selectedNode2 = new KisPerspectiveGridNode( *grid->bottomRight() );
                KisSubPerspectiveGrid* newsubgrid = new KisSubPerspectiveGrid( grid->topRight(), m_selectedNode1, m_selectedNode2, grid->bottomRight());
                m_dragEnd = event->pos().toPointF();
                newsubgrid->setLeftGrid( grid);
                grid->setRightGrid( newsubgrid);
                pGrid->addNewSubGrid( newsubgrid);
                drawGrid();
                break;
            }
            else if( !grid->topGrid() && mouseNear( mousep, controller->windowToView( ((*grid->topLeft() + *grid->topRight() )*0.5) ).roundQPoint() ) )
            {
//                 kDebug() << " PRESS TOP HANDLE " << endl;
                m_mode = MODE_DRAGING_TRANSLATING_TWONODES;
                drawGrid();
                m_selectedNode1 = new KisPerspectiveGridNode( *grid->topLeft() );
                m_selectedNode2 = new KisPerspectiveGridNode( *grid->topRight() );
                KisSubPerspectiveGrid* newsubgrid = new KisSubPerspectiveGrid( m_selectedNode1, m_selectedNode2,  grid->topRight(), grid->topLeft() );
                m_dragEnd = event->pos().toPointF();
                newsubgrid->setBottomGrid( grid);
                grid->setTopGrid( newsubgrid);
                pGrid->addNewSubGrid( newsubgrid);
                drawGrid();
                break;
            }
            else if( !grid->bottomGrid() && mouseNear( mousep, controller->windowToView( ((*grid->bottomLeft() + *grid->bottomRight() )*0.5) ).roundQPoint() ) )
            {
//                 kDebug() << " PRESS BOTTOM HANDLE " << endl;
                m_mode = MODE_DRAGING_TRANSLATING_TWONODES;
                drawGrid();
                m_selectedNode1 = new KisPerspectiveGridNode( *grid->bottomLeft() );
                m_selectedNode2 = new KisPerspectiveGridNode( *grid->bottomRight() );
                KisSubPerspectiveGrid* newsubgrid = new KisSubPerspectiveGrid( grid->bottomLeft(), grid->bottomRight(), m_selectedNode2, m_selectedNode1);
                m_dragEnd = event->pos().toPointF();
                newsubgrid->setTopGrid( grid);
                grid->setBottomGrid( newsubgrid);
                pGrid->addNewSubGrid( newsubgrid);
                drawGrid();
                break;
            }
        }
    }
}


void KisToolPerspectiveGrid::move(KoPointerEvent *event)
{
    if( m_mode == MODE_CREATION )
    {
        if (m_dragging) {
            // erase old lines on canvas
            drawGridCreation();
            // get current mouse position
            m_dragEnd = event->pos().toPointF();
            // draw new lines on canvas
            drawGridCreation();
        }
    } else {
        if( m_mode == MODE_DRAGING_NODE)
        {
            drawGrid();
            m_selectedNode1->setX( event->pos().x() );
            m_selectedNode1->setY( event->pos().y() );
            drawGrid();
        }
        if( m_mode == MODE_DRAGING_TRANSLATING_TWONODES)
        {
            drawGrid();
            QPointF translate = event->pos().toPointF() - m_dragEnd;
            m_dragEnd = event->pos().toPointF();
            *m_selectedNode1 += translate;;
            *m_selectedNode2 += translate;;
            drawGrid();
        }
    }
}

void KisToolPerspectiveGrid::buttonRelease(KoPointerEvent *event)
{
    if (!m_subject)
        return;

    if( m_mode == MODE_CREATION  )
    {
        if (m_dragging && event->button() == Qt::LeftButton)  {
            m_dragging = false;
            m_points.append (m_dragEnd);
            if( m_points.size() == 4)
            { // wow we have a grid, isn't that cool ?
                drawGridCreation(); // Clean
                m_currentImage->perspectiveGrid()->addNewSubGrid(
                        new KisSubPerspectiveGrid(
                            KisPerspectiveGridNodeSP(new KisPerspectiveGridNode(m_points[0])),
                            KisPerspectiveGridNodeSP(new KisPerspectiveGridNode(m_points[1])),
                            KisPerspectiveGridNodeSP(new KisPerspectiveGridNode(m_points[2])),
                            KisPerspectiveGridNodeSP(new KisPerspectiveGridNode(m_points[3])) ));
                drawGrid();
                m_mode = MODE_EDITING;
            }
        }
    } else {
        m_mode = MODE_EDITING;
        m_selectedNode1 = 0;
        m_selectedNode2 = 0;
    }

/*    if (m_dragging && event->button() == RightButton) {

        }*/
}

void KisToolPerspectiveGrid::paint(QPainter& gc)
{
    if( m_mode == MODE_CREATION )
    {
        drawGridCreation(gc);
    } else {
        drawGrid(gc);
    }
}

void KisToolPerspectiveGrid::paint(QPainter& gc, const QRect&)
{
    if( m_mode == MODE_CREATION )
    {
        drawGridCreation(gc);
    } else {
        drawGrid(gc);
    }
}

void KisToolPerspectiveGrid::drawGridCreation()
{
    if (m_subject) {
        KisCanvasController *controller = m_subject->canvasController();
        KisCanvas *canvas = controller->kiscanvas();
        QPainter gc(canvas->canvasWidget());

        drawGridCreation(gc);
    }
}


void KisToolPerspectiveGrid::drawGridCreation(QPainter& gc)
{
    if (!m_subject)
        return;

    QPen pen(Qt::white);

    gc.setPen(pen);
//         gc.setRasterOp(Qt::XorROP);

    KisCanvasController *controller = m_subject->canvasController();
    QPointF start, end;
    QPoint startPos;
    QPoint endPos;

    if (m_dragging) {
        startPos = controller->windowToView(m_dragStart.toPoint());
        endPos = controller->windowToView(m_dragEnd.toPoint());
        gc.drawLine(startPos, endPos);
    } else {
        for (QPointFVector::iterator it = m_points.begin(); it != m_points.end(); ++it) {

            if (it == m_points.begin())
            {
                start = (*it);
            } else {
                end = (*it);

                startPos = controller->windowToView(start.toPoint());
                endPos = controller->windowToView(end.toPoint());

                gc.drawLine(startPos, endPos);

                start = end;
            }
        }
    }
}

void KisToolPerspectiveGrid::drawSmallRectangle(QPainter& gc, QPoint p)
{
    gc.drawRect( p.x() - m_handleHalfSize - 1, p.y() - m_handleHalfSize - 1, m_handleSize, m_handleSize);
}

void KisToolPerspectiveGrid::drawGrid(QPainter& gc)
{

    if (!m_subject)
        return;

    KisCanvasController *controller = m_subject->canvasController();

    QPen pen(Qt::white);
    QPoint startPos;
    QPoint endPos;

    gc.setPen(pen);
//     gc.setRasterOp(Qt::XorROP);
    KisPerspectiveGrid* pGrid = m_currentImage->perspectiveGrid();

    for( QList<KisSubPerspectiveGrid*>::const_iterator it = pGrid->begin(); it != pGrid->end(); ++it)
    {
        KisSubPerspectiveGrid* grid = *it;
        int index = grid->index();
        bool drawLeft = !(grid->leftGrid() && (index > grid->leftGrid()->index() ) );
        bool drawRight = !(grid->rightGrid() && (index > grid->rightGrid()->index() ) );
        bool drawTop = !(grid->topGrid() && (index > grid->topGrid()->index() ) );
        bool drawBottom = !(grid->bottomGrid() && (index > grid->bottomGrid()->index() ) );
        if(drawTop) {
            startPos = controller->windowToView(grid->topLeft()->toPoint());
            endPos = controller->windowToView(grid->topRight()->toPoint());
            gc.drawLine( startPos, endPos );
            if( !grid->topGrid() )
            {
                drawSmallRectangle(gc, (endPos + startPos) / 2.);
            }
            if(drawLeft) {
                drawSmallRectangle(gc, startPos);
            }
            if(drawRight) {
                drawSmallRectangle(gc, endPos);
            }
        }
        if(drawRight) {
            startPos = controller->windowToView(grid->topRight()->toPoint());
            endPos = controller->windowToView(grid->bottomRight()->toPoint());
            gc.drawLine( startPos, endPos );
            if( !grid->rightGrid() )
            {
                drawSmallRectangle(gc, (endPos + startPos) / 2.);
            }
        }
        if(drawBottom) {
            startPos = controller->windowToView(grid->bottomRight()->toPoint());
            endPos = controller->windowToView(grid->bottomLeft()->toPoint());
            gc.drawLine( startPos, endPos );
            if( !grid->bottomGrid() )
            {
                drawSmallRectangle(gc, (endPos + startPos) / 2.);
            }
            if(drawLeft) {
                drawSmallRectangle(gc, endPos);
            }
            if(drawRight) {
                drawSmallRectangle(gc, startPos);
            }
        }
        if(drawLeft) {
            startPos = controller->windowToView(grid->bottomLeft()->toPoint());
            endPos = controller->windowToView(grid->topLeft()->toPoint());
            gc.drawLine( startPos, endPos );
            if( !grid->leftGrid() )
            {
                drawSmallRectangle(gc, (endPos + startPos) / 2.);
            }
        }
        QPointF tbVpf = grid->topBottomVanishingPoint();
        if( fabs(tbVpf.x()) < 30000000. && fabs(tbVpf.y()) < 30000000.)
        {
            QPoint tbVp = controller->windowToView(tbVpf.toPoint());
            gc.drawLine( tbVp.x() - m_handleHalfSize, tbVp.y() - m_handleHalfSize, tbVp.x() + m_handleHalfSize, tbVp.y() + m_handleHalfSize);
            gc.drawLine( tbVp.x() - m_handleHalfSize, tbVp.y() + m_handleHalfSize, tbVp.x() + m_handleHalfSize, tbVp.y() - m_handleHalfSize);
        }
        QPointF lrVpf = grid->leftRightVanishingPoint();
        if( fabs(lrVpf.x()) < 30000000. && fabs(lrVpf.y()) < 30000000.)
        { // Don't display it, if it is too far, or you get funny results
            QPoint lrVp = controller->windowToView(lrVpf.toPoint());
            gc.drawLine( lrVp.x() - m_handleHalfSize, lrVp.y() - m_handleHalfSize, lrVp.x() + m_handleHalfSize, lrVp.y() + m_handleHalfSize);
            gc.drawLine( lrVp.x() - m_handleHalfSize, lrVp.y() + m_handleHalfSize, lrVp.x() + m_handleHalfSize, lrVp.y() - m_handleHalfSize);
        }
    }
}

void KisToolPerspectiveGrid::drawGrid()
{
    if (m_subject) {
        KisCanvasController *controller = m_subject->canvasController();
        KisCanvas *canvas = controller->kiscanvas();
        QPainter gc(canvas->canvasWidget() );

        drawGrid(gc);
    }

}


void KisToolPerspectiveGrid::setup(KActionCollection *collection)
{
    m_action = collection->action(objectName());

    if (m_action == 0) {
        m_action = new KAction(KIcon("tool_perspectivegrid"),
                               i18n("&Perspective Grid"),
                               collection,
                               objectName());
        Q_CHECK_PTR(m_action);
        connect(m_action, SIGNAL(triggered()),this, SLOT(activate()));
        m_action->setToolTip(i18n("Edit the perspective grid"));
        m_action->setActionGroup(actionGroup());
        m_ownAction = true;
    }
}


// QWidget* KisToolPerspectiveGrid::createOptionWidget()
// {
//     return 0;
// }
//
// QWidget* KisToolPerspectiveGrid::optionWidget()
// {
//         return 0;
// }


#include "kis_tool_perspectivegrid.moc"
