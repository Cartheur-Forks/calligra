/*
 * This file is part of Krita
 *
 * Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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
 
#ifndef _KIS_WDG_COLOR_TO_ALPHA_H_
#define _KIS_WDG_COLOR_TO_ALPHA_H_

#include <kis_filter_config_widget.h>

class KisFilter;
class Ui_WdgColorToAlphaBase;

class KisWdgColorToAlpha : public KisFilterConfigWidget
{
    Q_OBJECT
    public:
        KisWdgColorToAlpha( KisFilter* nfilter, QWidget * parent);
        inline const Ui_WdgColorToAlphaBase* widget() const { return m_widget; }
        virtual void setConfiguration(KisFilterConfiguration*);
        virtual KisFilterConfiguration* configuration() const;
    private:
        Ui_WdgColorToAlphaBase* m_widget;
};

#endif
