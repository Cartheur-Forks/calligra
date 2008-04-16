/*
 *  Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
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
#ifndef KCURVE_H
#define KCURVE_H

// Qt includes.

#include <QWidget>
#include <QColor>
#include <QPointF>
#include <QPixmap>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QList>

#include <krita_export.h>

class KRITAUI_EXPORT KCurve : public QWidget
{
Q_OBJECT

public:
    KCurve(QWidget *parent = 0, Qt::WFlags f = 0);

    virtual ~KCurve();

    void reset(void);
    void setCurveGuide(const QColor & color);
    void setPixmap(const QPixmap & pix);

signals:

    void modified(void);

protected:

    void keyPressEvent(QKeyEvent *);
    void paintEvent(QPaintEvent *);
    void mousePressEvent (QMouseEvent * e);
    void mouseReleaseEvent ( QMouseEvent * e );
    void mouseMoveEvent ( QMouseEvent * e );
    void leaveEvent ( QEvent * );

public:
    static double getCurveValue(const QList<QPointF> &curve, double x);
    double getCurveValue(double x);

    QList<QPointF> getCurve();
    void setCurve(QList<QPointF> inlist);

private:
    int nearestPointInRange(QPointF pt) const;

    int m_grab_point_index;
    bool m_dragging;
    double m_grabOffsetX;
    double m_grabOffsetY;
    double m_grabOriginalX;
    double m_grabOriginalY;
    QPointF m_draggedawaypoint;
    int m_draggedawaypointindex;

    bool m_readOnlyMode;
    bool m_guideVisible;
    QColor m_colorGuide;
    QList<QPointF> m_points;
    QPixmap m_pix;
};


#endif /* KCURVE_H */
