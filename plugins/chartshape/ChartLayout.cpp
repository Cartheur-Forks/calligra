/* This file is part of the KDE project

   Copyright 2017 Dag Andersen <danders@get2net.dk>
   Copyright 2010 Johannes Simon <johannes.simon@gmail.com>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

// Qt
#include <QRectF>
#include <QTransform>

// KoChart
#include "ChartLayout.h"
#include "Legend.h"
#include "ChartDebug.h"
#include "PlotArea.h"
#include "Axis.h"
#include "ScreenConversions.h"

// Calligra
#include <KoShapeContainer.h>

#include <KChartCartesianAxis.h>

using namespace KoChart;

class ChartLayout::LayoutData
{
public:
    int  itemType;
    QRectF rect;
    bool inheritsTransform;
    bool clipped;

    LayoutData(int _itemType = GenericItemType)
        : itemType(_itemType)
        , inheritsTransform(true)
        , clipped(true)
    {}
};

// static
bool ChartLayout::autoPosition(const KoShape *shape)
{
    return shape->additionalStyleAttribute("chart:auto-position") == "true";
}

// static
bool ChartLayout::autoSize(const KoShape *shape)
{
    return shape->additionalStyleAttribute("chart:auto-size") == "true";
}

ChartLayout::ChartLayout()
    : m_doingLayout(false)
    , m_relayoutScheduled(false)
    , m_padding(5., 5., 5., 5.)
    , m_spacing(5., 5.)
    , m_layoutingEnabled(true)
{
}

ChartLayout::~ChartLayout()
{
    foreach(LayoutData *data, m_layoutItems.values())
        delete data;
}

void ChartLayout::add(KoShape *shape)
{
    Q_ASSERT(!m_layoutItems.contains(shape));
    setItemType(shape, GenericItemType);
}

void ChartLayout::remove(KoShape *shape)
{
    m_shapes.remove(m_shapes.key(shape));
    if (m_layoutItems.contains(shape)) {
        // delete LayoutData
        delete m_layoutItems.value(shape);
        m_layoutItems.remove(shape);
        scheduleRelayout();
    }
}

void ChartLayout::setClipped(const KoShape *shape, bool clipping)
{
    Q_ASSERT(m_layoutItems.contains(const_cast<KoShape*>(shape)));
    m_layoutItems.value(const_cast<KoShape*>(shape))->clipped = clipping;
}

bool ChartLayout::isClipped(const KoShape *shape) const
{
    Q_ASSERT(m_layoutItems.contains(const_cast<KoShape*>(shape)));
    return m_layoutItems.value(const_cast<KoShape*>(shape))->clipped;
}

void ChartLayout::setInheritsTransform(const KoShape *shape, bool inherit)
{
    m_layoutItems.value(const_cast<KoShape*>(shape))->inheritsTransform = inherit;
}

bool ChartLayout::inheritsTransform(const KoShape *shape) const
{
    return m_layoutItems.value(const_cast<KoShape*>(shape))->inheritsTransform;
}

int ChartLayout::count() const
{
    return m_layoutItems.size();
}

QList<KoShape*> ChartLayout::shapes() const
{
    return m_layoutItems.keys();
}

void ChartLayout::setContainerSize(const QSizeF &size)
{
    if (size != m_containerSize) {
        m_containerSize = size;
        scheduleRelayout();
    }
}

void ChartLayout::containerChanged(KoShapeContainer *container, KoShape::ChangeType type)
{
    switch(type) {
    case KoShape::SizeChanged:
        setContainerSize(container->size());
        break;
    default:
        break;
    }
}

bool ChartLayout::isChildLocked(const KoShape *shape) const
{
    return shape->isGeometryProtected();
}

void ChartLayout::setItemType(const KoShape *shape, ItemType itemType)
{
    LayoutData *data = m_layoutItems.value(const_cast<KoShape*>(shape));
    if (!data) {
        data = new LayoutData();
        m_layoutItems.insert(const_cast<KoShape*>(shape), data);
    }
    data->itemType = itemType;

    m_shapes.remove(m_shapes.key(const_cast<KoShape*>(shape))); // in case
    m_shapes.insert(itemType, const_cast<KoShape*>(shape));

    debugChartLayout<<m_shapes;
    scheduleRelayout();
}

void ChartLayout::setLayoutingEnabled(bool value)
{
    m_layoutingEnabled = value;
}

void ChartLayout::proposeMove(KoShape *child, QPointF &move)
{
    QRectF current = itemRect(child);
    QRectF newRect = current.adjusted(move.x(), move.y(), move.x(), move.y());
    if (newRect.left() < 0.0) {
        move.setX(-current.left());
    } else if (newRect.right() > m_containerSize.width()) {
        move.setX(m_containerSize.width() - current.right());
    }
    if (newRect.top() < 0.0) {
        move.setY(-current.top());
    } else if (newRect.bottom() > m_containerSize.height()) {
        move.setY(m_containerSize.height() - current.bottom());
    }
}

void ChartLayout::childChanged(KoShape *shape, KoShape::ChangeType type)
{
    Q_UNUSED(shape);

    // Do not relayout again if we're currently in the process of a relayout.
    // Repositioning a layout item or resizing it will result in a cull of this method.
    if (m_doingLayout)
        return;

    // This can be fine-tuned, but right now, simply everything will be re-layouted.
    switch (type) {
    case KoShape::PositionChanged:
    case KoShape::SizeChanged:
        scheduleRelayout();
    // FIXME: There's some cases that would require relayouting but that don't make sense
    // for chart items, e.g. ShearChanged. How should these changes be avoided or handled?
    default:
        break;
    }
}

void ChartLayout::scheduleRelayout()
{
    m_relayoutScheduled = true;
}

KChart::CartesianAxis::Position axisPosition(PlotArea *plotarea, ItemType type)
{
    KChart::CartesianAxis::Position apos = KChart::CartesianAxis::Bottom;
    switch (type) {
        case XAxisTitleType: {
            if (plotarea && plotarea->xAxis()) {
                apos = plotarea->xAxis()->kchartAxisPosition();
            }
            break;
        }
        case YAxisTitleType: {
            if (plotarea && plotarea->yAxis()) {
                apos = plotarea->yAxis()->kchartAxisPosition();
            }
            break;
        }
        case SecondaryXAxisTitleType: {
            if (plotarea && plotarea->secondaryXAxis()) {
                apos = plotarea->secondaryXAxis()->kchartAxisPosition();
            }
            break;
        }
        case SecondaryYAxisTitleType: {
            if (plotarea && plotarea->secondaryYAxis()) {
                apos = plotarea->secondaryYAxis()->kchartAxisPosition();
            }
            break;
        }
    }
    return apos;
}

qreal ChartLayout::xOffset(const QRectF &left, const QRectF &right, bool center) const
{
    qreal x = (left.width() + (left.width() > 0.0 ? m_spacing.x() : 0.0) - right.width() - (right.width() > 0.0 ? m_spacing.x() : 0.0));
    return center ? x / 2.0 : x;
}

qreal ChartLayout::yOffset(const QRectF &top, const QRectF &bottom, bool center) const
{
    qreal y = (top.height() + (top.height() > 0.0 ? m_spacing.y() : 0.0) - bottom.height() - (bottom.height() > 0.0 ? m_spacing.y() : 0.0));
    return center ? y / 2.0 : y;
}

void ChartLayout::calculateLayout()
{
    QRectF area;
    area.setSize(m_containerSize);
    area.adjust(m_padding.left, m_padding.top, -m_padding.right, -m_padding.bottom);

    if (area.size().width() < 0 && area.size().height() < 0) {
        debugChartLayout<<"invalid size:"<<area;
        m_doingLayout = false;
        m_relayoutScheduled = false;
        return;
    }
    debugChartLayout<<"area:"<<area<<"spacing:"<<m_spacing;

    PlotArea *plotarea = dynamic_cast<PlotArea*>(m_shapes.value(PlotAreaType));
    QRectF plotareaRect = area;
    if (plotarea->chartType() == BarChartType && plotarea->isVertical()) debugChartLayout<<"Vertical bar chart";

    QRectF titleRect;
    KoShape *title = m_shapes.value(TitleLabelType);
    if (title && title->isVisible()) {
        titleRect = itemRect(title);
    }
    QRectF subtitleRect;
    KoShape *subtitle = m_shapes.value(SubTitleLabelType);
    if (subtitle && subtitle->isVisible()) {
        subtitleRect = itemRect(subtitle);
    }
    QRectF footerRect;
    KoShape *footer = m_shapes.value(FooterLabelType);
    if (footer && footer->isVisible()) {
        footerRect = itemRect(footer);
    }
    QRectF legendRect;
    Legend *legend = dynamic_cast<Legend*>(m_shapes.value(LegendType));
    if (legend && legend->isVisible()) {
        legendRect = itemRect(legend);
        debugChartLayout<<"legend rect:"<<legendRect;
    }

    // sort axis titles as bottom, left, top, right
    QMap<int, KoShape*> axisTitles;
    QRectF bottomTitleRect; // 1
    QRectF leftTitleRect; // 2
    QRectF topTitleRect; // 3
    QRectF rightTitleRect; // 4

    KoShape *xtitle = m_shapes.value(XAxisTitleType);
    if (xtitle && xtitle->isVisible()) {
        if (plotarea->chartType() == BarChartType && plotarea->isVertical()) {
            debugChartLayout<<"x-axis is vertical: position="<<axisPosition(plotarea, XAxisTitleType);
            if (axisPosition(plotarea, XAxisTitleType) == KChart::CartesianAxis::Top) {
                rightTitleRect = itemRect(xtitle);
                axisTitles.insert(4, xtitle);
            } else {
                leftTitleRect = itemRect(xtitle);
                axisTitles.insert(2, xtitle);
            }
        } else {
            debugChartLayout<<"x-axis is horizontal: position="<<axisPosition(plotarea, XAxisTitleType);
            if (axisPosition(plotarea, XAxisTitleType) == KChart::CartesianAxis::Top) {
                topTitleRect = itemRect(xtitle);
                axisTitles.insert(3, xtitle);
            } else {
                bottomTitleRect = itemRect(xtitle);
                axisTitles.insert(1, xtitle);
            }
        }
    }
    KoShape *ytitle = m_shapes.value(YAxisTitleType);
    if (ytitle && ytitle->isVisible()) {
        if (plotarea->chartType() == BarChartType && plotarea->isVertical()) {
            debugChartLayout<<"y-axis is horizontal: position="<<axisPosition(plotarea, YAxisTitleType);
            if (axisPosition(plotarea, YAxisTitleType) == KChart::CartesianAxis::Left) {
                bottomTitleRect = itemRect(ytitle);
                axisTitles.insert(1, ytitle);
                debugChartLayout<<"ytitle: add to left"<<leftTitleRect;
            } else {
                topTitleRect = itemRect(ytitle);
                axisTitles.insert(3, ytitle);
            }
        } else {
            if (axisPosition(plotarea, YAxisTitleType) == KChart::CartesianAxis::Left) {
                leftTitleRect = itemRect(ytitle);
                axisTitles.insert(2, ytitle);
                debugChartLayout<<"ytitle: add to left"<<leftTitleRect;
            } else {
                rightTitleRect = itemRect(ytitle);
                axisTitles.insert(4, ytitle);
            }
        }
    }
    KoShape *sxtitle = m_shapes.value(SecondaryXAxisTitleType);
    debugChartLayout<<sxtitle<<(sxtitle && sxtitle->isVisible()<<SecondaryXAxisTitleType);
    if (sxtitle && sxtitle->isVisible()) {
        if (plotarea->chartType() == BarChartType && plotarea->isVertical()) {
            debugChartLayout<<"secondary-x-axis is vertical: position="<<axisPosition(plotarea, SecondaryXAxisTitleType);
            if (axisPosition(plotarea, SecondaryXAxisTitleType) == KChart::CartesianAxis::Bottom) {
                leftTitleRect = itemRect(sxtitle);
                axisTitles.insert(2, sxtitle);
            } else {
                rightTitleRect = itemRect(sxtitle);
                axisTitles.insert(4, sxtitle);
            }
        } else {
            if (axisPosition(plotarea, SecondaryXAxisTitleType) == KChart::CartesianAxis::Bottom) {
                bottomTitleRect = itemRect(sxtitle);
                axisTitles.insert(1, sxtitle);
                debugChartLayout<<"sxtitle: add to bottom";
            } else {
                topTitleRect = itemRect(sxtitle);
                axisTitles.insert(3, sxtitle);
            }
        }
    }
    KoShape *sytitle = m_shapes.value(SecondaryYAxisTitleType);
    if (sytitle && sytitle->isVisible()) {
        if (plotarea->chartType() == BarChartType && plotarea->isVertical()) {
            debugChartLayout<<"secondary-y-axis is horizontal: position="<<axisPosition(plotarea, SecondaryYAxisTitleType);
            if (axisPosition(plotarea, SecondaryYAxisTitleType) == KChart::CartesianAxis::Right) {
                topTitleRect = itemRect(sytitle);
                axisTitles.insert(3, sytitle);
                debugChartLayout<<"sytitle: add to top"<<topTitleRect;
            } else {
                bottomTitleRect = itemRect(sytitle);
                axisTitles.insert(1, sytitle);
            }
        } else {
            if (axisPosition(plotarea, SecondaryYAxisTitleType) == KChart::CartesianAxis::Right) {
                rightTitleRect = itemRect(sytitle);
                axisTitles.insert(4, sytitle);
            } else {
                leftTitleRect = itemRect(sytitle);
                axisTitles.insert(2, sytitle);
            }
        }
    }


    QPointF topcenter(area.center().x(), area.top());
    QPointF bottomcenter(area.center().x(), area.bottom());
    QPointF leftcenter(area.left(), area.center().y());
    QPointF rightcenter(area.right(), area.center().y());

    debugChartLayout<<"left:"<<leftcenter<<"right:"<<rightcenter<<"top:"<<topcenter<<"bottom:"<<bottomcenter;

    // title/subtitle/footer is aligned with container
    if (!titleRect.isNull()) {
        if (autoPosition(title)) {
            titleRect.moveLeft(topcenter.x() - titleRect.width() / 2.0);
            titleRect.moveTop(topcenter.y());
        }
        topcenter.setY(titleRect.bottom() + m_spacing.y());
        debugChartLayout<<"title: auto:"<<autoPosition(title)<<titleRect;
    }
    if (!subtitleRect.isNull()) {
        if (autoPosition(subtitle)) {
            subtitleRect.moveLeft(topcenter.x() - subtitleRect.width() / 2.0);
            subtitleRect.moveTop(topcenter.y());
        }
        topcenter.setY(subtitleRect.bottom() + m_spacing.y());
        debugChartLayout<<"subtitle:"<<subtitleRect;
    }
    if (!footerRect.isNull()) {
        if (autoPosition(footer)) {
            footerRect.moveLeft(bottomcenter.x() - footerRect.width() / 2.0);
            footerRect.moveBottom(bottomcenter.y());
        }
        bottomcenter.setY(footerRect.top() - m_spacing.y());
        debugChartLayout<<"footer:"<<footerRect;
    }

    // all items below shall be aligned with the *final* plot area
    plotareaRect.setLeft(leftcenter.x());
    plotareaRect.setTop(topcenter.y());
    plotareaRect.setRight(rightcenter.x());
    plotareaRect.setBottom(bottomcenter.y());

    if (!legendRect.isNull()) {
        // Legend is aligned with plot area so axis titles may resize plot area
        switch (legend->legendPosition()) {
            case StartPosition:
                switch(legend->alignment()) {
                    case Qt::AlignLeft: {
                        // Align at top of the area to the left of plot area
                        qreal offset = yOffset(topTitleRect, QRectF());
                        legendRect.moveTop(plotareaRect.top() + offset);
                        break;
                    }
                    case Qt::AlignCenter: {
                        qreal centerOffset = yOffset(topTitleRect, bottomTitleRect, true);
                        legendRect.moveTop(plotareaRect.center().y() + centerOffset - legendRect.height() / 2.0);
                        break;
                    }
                    case Qt::AlignRight: {
                        // Align at bottom of the area to the left of plot area
                        qreal offset = yOffset(QRectF(), bottomTitleRect);
                        legendRect.moveBottom(plotareaRect.bottom() + offset);
                        break;
                    }
                }
                legendRect.moveLeft(plotareaRect.left());
                plotareaRect.setLeft(legendRect.right() + m_spacing.x());
                break;
            case TopPosition:
                switch(legend->alignment()) {
                    case Qt::AlignLeft: {
                        // Align at left of the area on top of plot area
                        qreal offset = xOffset(leftTitleRect, QRectF());
                        legendRect.moveLeft(plotareaRect.left() + offset);
                        break;
                    }
                    case Qt::AlignCenter: {
                        qreal centerOffset = xOffset(leftTitleRect, rightTitleRect, true);
                        legendRect.moveLeft(plotareaRect.center().x() + centerOffset - legendRect.width() / 2.0);
                        debugChartLayout<<"legend top/center:"<<"to"<<legendRect.left()<<"coffs="<<centerOffset<<"h/2"<<(legendRect.height()/2.0);
                        break;
                    }
                    case Qt::AlignRight: {
                        // Align at right of the area on top of plot area
                        qreal offset = xOffset(QRectF(), rightTitleRect);
                        legendRect.moveRight(plotareaRect.right() + offset);
                        break;
                    }
                }
                legendRect.moveTop(plotareaRect.top());
                plotareaRect.setTop(legendRect.bottom() + m_spacing.y());
                break;
            case EndPosition:
                switch(legend->alignment()) {
                    case Qt::AlignLeft: {
                        // Align at top of the area right of plot area
                        qreal offset = yOffset(topTitleRect, QRectF());
                        legendRect.moveTop(plotareaRect.top() + offset);
                        debugChartLayout<<"legend end/top:"<<"to"<<legendRect.top()<<"offs="<<offset;
                        break;
                    }
                    case Qt::AlignCenter: {
                        qreal centerOffset = yOffset(topTitleRect, bottomTitleRect, true);
                        legendRect.moveTop(plotareaRect.center().y() + centerOffset - legendRect.height() / 2.0);
                        debugChartLayout<<"legend end/center:"<<"to"<<legendRect.top()<<"coffs="<<centerOffset<<"h/2"<<(legendRect.height()/2.0)<<"pa:"<<plotareaRect.center().y();
                        break;
                    }
                    case Qt::AlignRight: {
                        // Align at bottom of the area right of plot area
                        qreal offset = yOffset(QRectF(), bottomTitleRect);
                        legendRect.moveBottom(plotareaRect.bottom() + offset);
                        debugChartLayout<<"legend end/bottom:"<<"to"<<legendRect.bottom()<<"offs="<<offset;
                        break;
                    }
                }
                legendRect.moveRight(plotareaRect.right());
                plotareaRect.setRight(legendRect.left() - m_spacing.x());
                debugChartLayout<<"legend end:"<<legendRect;
                break;
            case BottomPosition:
                switch(legend->alignment()) {
                    case Qt::AlignLeft: {
                        // Align at left of the area below of plot area
                        qreal offset = xOffset(leftTitleRect, QRectF());
                        legendRect.moveLeft(plotareaRect.left() + offset);
                        break;
                    }
                    case Qt::AlignCenter: {
                        qreal centerOffset = xOffset(leftTitleRect, rightTitleRect, true);
                        legendRect.moveLeft(bottomcenter.x() + centerOffset - legendRect.width() / 2.0);
                        break;
                    }
                    case Qt::AlignRight: {
                        // Align at right of the area on below plot area
                        qreal offset = xOffset(QRectF(), rightTitleRect);
                        legendRect.moveRight(plotareaRect.right() + offset);
                        break;
                    }
                }
                legendRect.moveBottom(plotareaRect.bottom());
                plotareaRect.setBottom(legendRect.top() - m_spacing.y());
                break;
            case TopStartPosition: {
                qreal xoffset = xOffset(leftTitleRect, QRectF());
                qreal yoffset = yOffset(topTitleRect, QRectF());
                legendRect.moveTopLeft(area.topLeft());
                if (legendRect.right() + m_spacing.x() - xoffset > plotareaRect.left()) {
                    plotareaRect.setLeft(legendRect.right() + m_spacing.x() - xoffset);
                }
                if (legendRect.bottom() + m_spacing.y() - yoffset > plotareaRect.top()) {
                    plotareaRect.setTop(legendRect.bottom() + m_spacing.y() - yoffset);
                }
                debugChartLayout<<"legend top/start"<<legendRect<<plotareaRect<<"xo:"<<xoffset<<"yo:"<<yoffset;
                break;
            }
            case TopEndPosition: {
                qreal xoffset = xOffset(QRectF(), rightTitleRect);
                qreal yoffset = yOffset(topTitleRect, QRectF());
                legendRect.moveTopRight(area.topRight());
                if (legendRect.left() - m_spacing.x() + xoffset < plotareaRect.right()) {
                    plotareaRect.setRight(legendRect.left() - m_spacing.x() + xoffset);
                }
                if (legendRect.bottom() + m_spacing.y() - yoffset > plotareaRect.top()) {
                    plotareaRect.setTop(legendRect.bottom() + m_spacing.y() - yoffset);
                }
                debugChartLayout<<"legend top/end"<<legendRect<<plotareaRect;
                break;
            }
            case BottomStartPosition: {
                qreal xoffset = xOffset(leftTitleRect, QRectF());
                qreal yoffset = yOffset(QRectF(), bottomTitleRect);
                legendRect.moveBottomLeft(area.bottomLeft());
                if (legendRect.right() + m_spacing.x() - xoffset > plotareaRect.left()) {
                    plotareaRect.setLeft(legendRect.right() + m_spacing.x() - xoffset);
                }
                if (legendRect.top() - m_spacing.y() + yoffset < plotareaRect.bottom()) {
                    plotareaRect.setBottom(legendRect.top() - m_spacing.y() - yoffset);
                }
                debugChartLayout<<"legend bottom/start"<<legendRect<<plotareaRect<<"offs:"<<xoffset<<','<<yoffset;
                break;
            }
            case BottomEndPosition: {
                qreal xoffset = xOffset(QRectF(), rightTitleRect);
                qreal yoffset = yOffset(QRectF(), bottomTitleRect);
                legendRect.moveBottomRight(area.bottomRight());
                if (legendRect.left() - m_spacing.x() + xoffset < plotareaRect.right()) {
                    plotareaRect.setRight(legendRect.left() - m_spacing.x() + xoffset);
                }
                if (legendRect.top() - m_spacing.y() + yoffset < plotareaRect.bottom()) {
                    plotareaRect.setBottom(legendRect.top() - m_spacing.y() - yoffset);
                }
                debugChartLayout<<"legend bottom/end"<<legendRect<<plotareaRect;
                break;
            }
            default:
                if (plotarea) {
                    // try to be backwards compatible with loaded shapes
                    QRectF pr = itemRect(plotarea); // use actual plotarea rect here
                    debugChartLayout<<"legend:"<<legendRect<<"plot:"<<pr<<legendRect.intersects(pr);
                    if (!legendRect.intersects(pr) && legendRect.intersects(plotareaRect)) {
                        if (legendRect.right() < pr.left()) {
                            plotareaRect.setLeft(qMax(plotareaRect.left(), legendRect.right() + m_spacing.x()));
                        } else if (legendRect.left() > pr.right()) {
                            plotareaRect.setRight(qMin(plotareaRect.right(), legendRect.left() - m_spacing.x()));
                        } else if (legendRect.bottom() < pr.top()) {
                            plotareaRect.setTop(qMax(plotareaRect.top(), legendRect.bottom() + m_spacing.y()));
                        } else if (legendRect.top() > pr.bottom()) {
                            plotareaRect.setBottom(qMin(plotareaRect.bottom(), legendRect.top() - m_spacing.y()));
                        }
                        debugChartLayout<<"legend manual:"<<legendRect<<"plot moved to:"<<plotareaRect;
                    }
                }
                break;
        }
        debugChartLayout<<"legend:"<<legendRect<<"pos:"<<legend->legendPosition()<<"align:"<<legend->alignment();
    }

    debugChartLayout<<"axis titles:"<<axisTitles;
    if (!leftTitleRect.isNull()) {
        qreal centerOffset = yOffset(topTitleRect, bottomTitleRect, true);
        leftTitleRect.moveTop(plotareaRect.center().y() + centerOffset - leftTitleRect.height() / 2.0);
        leftTitleRect.moveLeft(plotareaRect.left());
        debugChartLayout<<"left axis:"<<leftTitleRect;
    }
    if (!rightTitleRect.isNull()) {
        qreal centerOffset = yOffset(topTitleRect, bottomTitleRect, true);
        rightTitleRect.moveTop(plotareaRect.center().y() + centerOffset - rightTitleRect.height() / 2.0);
        rightTitleRect.moveRight(plotareaRect.right());
        debugChartLayout<<"right axis:"<<rightTitleRect;
    }
    if (!topTitleRect.isNull()) {
        qreal centerOffset = xOffset(leftTitleRect, rightTitleRect, true);
        topTitleRect.moveLeft(plotareaRect.center().x() + centerOffset - topTitleRect.width() / 2.0);
        topTitleRect.moveTop(plotareaRect.top());
        debugChartLayout<<"top axis:"<<topTitleRect;
    }
    if (!bottomTitleRect.isNull()) {
        qreal centerOffset = xOffset(leftTitleRect, rightTitleRect, true);
        bottomTitleRect.moveLeft(plotareaRect.center().x() + centerOffset - bottomTitleRect.width() / 2.0);
        bottomTitleRect.moveBottom(plotareaRect.bottom());
        debugChartLayout<<"bottom axis:"<<bottomTitleRect;
    }
    // now resize plot area
    if (!leftTitleRect.isNull()) {
        plotareaRect.setLeft(leftTitleRect.right() + m_spacing.x());
    }
    if (!rightTitleRect.isNull()) {
        plotareaRect.setRight(rightTitleRect.left() - m_spacing.x());
    }
    if (!topTitleRect.isNull()) {
        plotareaRect.setTop(topTitleRect.bottom() + m_spacing.y());
    }
    if (!bottomTitleRect.isNull()) {
        plotareaRect.setBottom(bottomTitleRect.top() - m_spacing.y());
    }
    if (title && title->isVisible()) {
        m_layoutItems[title]->rect = titleRect;
        debugChartLayout<<"title"<<dbg(title)<<titleRect;
    }
    if (subtitle && subtitle->isVisible()) {
        m_layoutItems[subtitle]->rect = subtitleRect;
        debugChartLayout<<"subtitle"<<dbg(subtitle)<<subtitleRect;
    }
    if (footer && footer->isVisible()) {
        m_layoutItems[footer]->rect = footerRect;
        debugChartLayout<<"footer"<<dbg(footer)<<footerRect;
    }
    if (legend && legend->isVisible()) {
        m_layoutItems[legend]->rect = legendRect;
        debugChartLayout<<"legend"<<dbg(legend)<<legendRect<<legendRect.center();
    }

    if (axisTitles.contains(1)) {
        m_layoutItems[axisTitles[1]]->rect = bottomTitleRect;
        debugChartLayout<<"bottom title"<<dbg(axisTitles[1])<<bottomTitleRect<<bottomTitleRect.center();
    }
    if (axisTitles.contains(2)) {
        m_layoutItems[axisTitles[2]]->rect = leftTitleRect;
        debugChartLayout<<"left title"<<dbg(axisTitles[2])<<leftTitleRect<<leftTitleRect.center();
    }
    if (axisTitles.contains(3)) {
        m_layoutItems[axisTitles[3]]->rect = topTitleRect;
        debugChartLayout<<"top title"<<dbg(axisTitles[3])<<topTitleRect<<topTitleRect.center();
    }
    if (axisTitles.contains(4)) {
        m_layoutItems[axisTitles[4]]->rect = rightTitleRect;
        debugChartLayout<<"right title"<<dbg(axisTitles[4])<<rightTitleRect<<rightTitleRect.center();
    }

    if (plotarea && plotarea->isVisible()) {
        m_layoutItems[plotarea]->rect = plotareaRect;
        debugChartLayout<<"plot area"<<dbg(plotarea)<<plotareaRect<<plotareaRect.center();
    }
}

void ChartLayout::layout()
{
    Q_ASSERT(!m_doingLayout);

    if (!m_layoutingEnabled || !m_relayoutScheduled) {
        return;
    }
    m_doingLayout = true;

    calculateLayout();
    QMap<KoShape*, LayoutData*>::const_iterator it;
    for (it = m_layoutItems.constBegin(); it != m_layoutItems.constEnd(); ++it) {
        if (it.key()->isVisible()) {
            setItemPosition(it.key(), it.value()->rect.topLeft());
            debugChartLayout<<dbg(it.key())<<it.value()->rect.topLeft()<<itemPosition(it.key());
            if (it.value()->itemType == PlotAreaType) {
                setItemSize(it.key(), it.value()->rect.size());
                debugChartLayout<<dbg(it.key())<<it.value()->rect.size()<<itemSize(it.key());
            }
        }
    }
    m_doingLayout = false;
    m_relayoutScheduled = false;
}


void ChartLayout::setPadding(const KoInsets &padding)
{
    m_padding = padding;
    scheduleRelayout();
}

KoInsets ChartLayout::padding() const
{
    return m_padding;
}

void ChartLayout::setSpacing(qreal hSpacing, qreal vSpacing)
{
    m_spacing = QPointF(hSpacing, vSpacing);
}

QPointF ChartLayout::spacing() const
{
    return m_spacing;
}

/*static*/ QPointF ChartLayout::itemPosition(const KoShape *shape)
{
    const QRectF boundingRect = QRectF(QPointF(0, 0), shape->size());
    return shape->transformation().mapRect(boundingRect).topLeft();
}

/*static*/ QSizeF ChartLayout::itemSize(const KoShape *shape)
{
    const QRectF boundingRect = QRectF(QPointF(0, 0), shape->size());
    return shape->transformation().mapRect(boundingRect).size();
}

/*static*/ void ChartLayout::setItemSize(KoShape *shape, const QSizeF &size)
{
    shape->setSize(size);
}

/*static*/ QRectF ChartLayout::itemRect(const KoShape *shape)
{
    const QRectF boundingRect = QRectF(itemPosition(shape), itemSize(shape));
    return boundingRect;
}

/*static*/ void ChartLayout::setItemPosition(KoShape *shape, const QPointF& pos)
{
    const QPointF offset =  shape->position() - itemPosition(shape);
    shape->setPosition(pos + offset);
}

QRectF ChartLayout::diagramArea(const KoShape *shape)
{
    return diagramArea(shape, itemRect(shape));
}

// FIXME: Get the actual plot area ex axis labels from KChart
QRectF ChartLayout::diagramArea(const KoShape *shape, const QRectF &rect)
{
    const PlotArea* plotArea = dynamic_cast<const PlotArea*>(shape);
    if (!plotArea) {
        return rect;
    }
    qreal bottom = 0.0;
    qreal left = 0.0;
    qreal top = 0.0;
    qreal right = 0.0;
    // HACK: KChart has some spacing between axis and label
    qreal xspace = ScreenConversions::pxToPtX(6.0) * 2.0;
    qreal yspace = ScreenConversions::pxToPtY(6.0) * 2.0;
    if (plotArea->xAxis() && plotArea->xAxis()->showLabels()) {
        bottom = plotArea->xAxis()->fontSize();
        bottom += yspace;
    }
    if (plotArea->yAxis() && plotArea->yAxis()->showLabels()) {
        left = plotArea->yAxis()->fontSize();
        left += xspace;
    }
    if (plotArea->secondaryXAxis() && plotArea->secondaryXAxis()->showLabels()) {
        top = plotArea->secondaryXAxis()->fontSize();
        top += yspace;
    }
    if (plotArea->secondaryYAxis() && plotArea->secondaryYAxis()->showLabels()) {
        right = plotArea->secondaryYAxis()->fontSize();
        right += xspace;
    }
    return rect.adjusted(left, top, -right, -bottom);
}

QString ChartLayout::dbg(const KoShape *shape) const
{
    QString s;
    LayoutData *data = m_layoutItems[const_cast<KoShape*>(shape)];
    switch(data->itemType) {
        case GenericItemType: s = "KoShape[Generic:"+ shape->shapeId() + "]"; break;
        case TitleLabelType: s = "KoShape[ChartTitle]"; break;
        case SubTitleLabelType: s = "KoShape[ChartSubTitle]"; break;
        case FooterLabelType: s = "KoShape[ChartFooter]"; break;
        case PlotAreaType: s = "KoShape[PlotArea]"; break;
        case LegendType:
            s = "KoShape[Legend";
            switch(static_cast<const Legend*>(shape)->alignment()) {
                case Qt::AlignLeft: s += ":Start"; break;
                case Qt::AlignCenter: s += ":Center"; break;
                case Qt::AlignRight: s += ":End"; break;
                default: s += ":Float"; break;
            }
            s += ']';
            break;
        case XAxisTitleType: s = "KoShape[XAxisTitle]"; break;
        case YAxisTitleType: s = "KoShape[YAxisTitle]"; break;
        case SecondaryXAxisTitleType: s = "KoShape[SXAxisTitle]"; break;
        case SecondaryYAxisTitleType: s = "KoShape[SYAxisTitle]"; break;
        default: s = "KoShape[Unknown]"; break;
    }
    return s;
}

QString ChartLayout::dbg(const QList<KoShape*> &shapes) const
{
    QString s = "(";
    for (int i = 0; i < shapes.count(); ++i) {
        if (i > 0) s += ',';
        s += dbg(shapes.at(i));
    }
    s += ')';
    return s;
}
