/*
 * This file is part of Krita
 *
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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

#ifndef KIS_PERSPECTIVE_GRID_H
#define KIS_PERSPECTIVE_GRID_H

#include <QList>
#include <QPointF>

#include <kis_perspective_math.h>
#include <kis_shared.h>
#include <kis_shared_ptr.h>
#include <krita_export.h>

class KisPerspectiveGridNode : public QPointF, public KisShared {
    public:
        inline KisPerspectiveGridNode(double x, double y) : QPointF(x,y)  { }
        inline KisPerspectiveGridNode(QPointF p) : QPointF(p)  { }
};
typedef KisSharedPtr<KisPerspectiveGridNode> KisPerspectiveGridNodeSP;

class KisSubPerspectiveGrid {
    public:
        KisSubPerspectiveGrid(KisPerspectiveGridNodeSP topLeft, KisPerspectiveGridNodeSP topRight, KisPerspectiveGridNodeSP bottomRight, KisPerspectiveGridNodeSP bottomLeft);
        
        inline QPointF topBottomVanishingPoint() { return computeVanishingPoint( topLeft(), topRight(), bottomLeft(), bottomRight() ); }
        inline QPointF leftRightVanishingPoint() { return computeVanishingPoint( topLeft(), bottomLeft(), topRight(), bottomRight() ); }
        
        inline KisSubPerspectiveGrid* leftGrid() { return m_leftGrid; }
        inline void setLeftGrid(KisSubPerspectiveGrid* g) { Q_ASSERT(m_leftGrid==0); m_leftGrid = g; }
        inline KisSubPerspectiveGrid* rightGrid() { return m_rightGrid; }
        inline void setRightGrid(KisSubPerspectiveGrid* g) { Q_ASSERT(m_rightGrid==0); m_rightGrid = g; }
        inline KisSubPerspectiveGrid* topGrid() { return m_topGrid; }
        inline void setTopGrid(KisSubPerspectiveGrid* g) { Q_ASSERT(m_topGrid==0); m_topGrid = g; }
        inline KisSubPerspectiveGrid* bottomGrid() { return m_bottomGrid; }
        inline void setBottomGrid(KisSubPerspectiveGrid* g) { Q_ASSERT(m_bottomGrid==0); m_bottomGrid = g; }
        inline const KisPerspectiveGridNodeSP topLeft() const { return m_topLeft; }
        inline KisPerspectiveGridNodeSP topLeft() { return m_topLeft; }
        inline const KisPerspectiveGridNodeSP topRight() const { return m_topRight; }
        inline KisPerspectiveGridNodeSP topRight() { return m_topRight; }
        inline const KisPerspectiveGridNodeSP bottomLeft() const { return m_bottomLeft; }
        inline KisPerspectiveGridNodeSP bottomLeft() { return m_bottomLeft; }
        inline const KisPerspectiveGridNodeSP bottomRight() const { return m_bottomRight; }
        inline KisPerspectiveGridNodeSP bottomRight() { return m_bottomRight; }
        inline int subdivisions() const { return m_subdivisions; }
        /**
         * Return the index of the subgrid, the value is automatically set when the KisSubPerspectiveGrid, it is useful for
         * drawing the perspective grid, to avoid drawing twice the same border, or points
         */
        inline int index() const { return m_index; }

        /**
         * @return true if the point p is contain by the grid
         */
        bool contains(const QPointF p) const;
    private:
        inline QPointF computeVanishingPoint(KisPerspectiveGridNodeSP p11, KisPerspectiveGridNodeSP p12, KisPerspectiveGridNodeSP p21, KisPerspectiveGridNodeSP p22)
        {
            KisPerspectiveMath::LineEquation d1 = KisPerspectiveMath::computeLineEquation( p11.data(), p12.data() );
            KisPerspectiveMath::LineEquation d2 = KisPerspectiveMath::computeLineEquation( p21.data(), p22.data() );
            return KisPerspectiveMath::computeIntersection(d1,d2);
        }
    private:
        KisPerspectiveGridNodeSP m_topLeft, m_topRight, m_bottomLeft, m_bottomRight;
        KisSubPerspectiveGrid *m_leftGrid, *m_rightGrid, *m_topGrid, *m_bottomGrid;
        int m_subdivisions;
        int m_index;
        static int s_lastIndex;
};

class KRITAIMAGE_EXPORT KisPerspectiveGrid {
    public:
        KisPerspectiveGrid();
        ~KisPerspectiveGrid();
        /**
         * @return false if the grid wasn't added, note that subgrids must be attached to an other grid, except if it's the first grid
         */
        bool addNewSubGrid( KisSubPerspectiveGrid* ng );
        inline QList<KisSubPerspectiveGrid*>::const_iterator begin() const { return m_subGrids.begin(); }
        inline QList<KisSubPerspectiveGrid*>::const_iterator end() const { return m_subGrids.end(); }
        inline bool hasSubGrids() const { return !m_subGrids.isEmpty(); }
        void clearSubGrids();
        inline int countSubGrids() const { return m_subGrids.size(); }
        /**
         * @return the first grid hit by the point p
         */
        KisSubPerspectiveGrid* gridAt(QPointF p);
    private:
        QList<KisSubPerspectiveGrid*> m_subGrids;
};

#endif
