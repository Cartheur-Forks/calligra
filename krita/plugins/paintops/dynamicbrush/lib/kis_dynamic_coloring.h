/*
 *  Copyright (c) 2006-2007 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _KIS_DYNAMIC_COLORING_H_
#define _KIS_DYNAMIC_COLORING_H_

#include "dynamicbrush_export.h"

#include <KoColor.h>

#include <kis_types.h>

#include "kis_dynamic_transformable.h"

class DYNAMIC_BRUSH_EXPORT KisDynamicColoring : public KisDynamicTransformable {
    public:
        virtual ~KisDynamicColoring();
    public:
        virtual KisDynamicColoring* clone() const = 0;
        virtual void darken(qint32 v) = 0;
        virtual void colorAt(int x, int y, KoColor*) = 0;
};

class KisPlainColoring : public KisDynamicColoring {
    public:
        KisPlainColoring(KoColor color) : m_color(color), m_cacheColor(0) {}
//         KisPlainColoring(const KisPlainColoring& kpc);
        virtual ~KisPlainColoring();
        virtual KisDynamicColoring* clone() const;
        virtual void darken(qint32 v);
        virtual void rotate(double r)
        {}
        virtual void resize(double , double ) {
            // Do nothing as plain color doesn't have size
        }
        virtual void colorAt(int x, int y, KoColor*);
    private:
        KoColor m_color;
        KoColor* m_cacheColor;
};

#endif
