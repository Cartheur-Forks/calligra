/*
 * This file is part of Krita
 *
 * Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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
#ifndef KISDLGFILTERSPREVIEW_H
#define KISDLGFILTERSPREVIEW_H

#include <kdialogbase.h>

class KisView;
class KisFilter;
class QIconViewItem;
class QHBoxLayout;
class KisPreviewWidget;

namespace Krita {
namespace Plugins {
namespace FiltersPreview {
	class KisFiltersListView;

/**
@author Cyrille Berger
*/
class KisDlgFiltersPreview : public KDialogBase
{
	Q_OBJECT
	public:
		KisDlgFiltersPreview(KisView* view, QWidget* parent,const char *name = "");

                ~KisDlgFiltersPreview();
        public:
          inline KisFilter* currentFilter() { return m_currentFilter; };
	private slots:
		void refreshPreview();
		void selectionHasChanged ( QIconViewItem * item );
	private:
		KisPreviewWidget* m_previewWidget;
		KisView* m_view;
		KisFiltersListView* m_kflw;
		QWidget* m_currentConfigWidget;
		KisFilter* m_currentFilter;
		QHBoxLayout* m_hlayout;
};

};
};
};

#endif
