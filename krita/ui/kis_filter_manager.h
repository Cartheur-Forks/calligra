/*
 *  Copyright (c) 2005 Boudewijn Rempt <boud@valdyas.org>
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

#ifndef _KIS_FILTER_MANAGER_
#define _KIS_FILTER_MANAGER_

#include "q3dict.h"
#include "qobject.h"
#include "q3ptrlist.h"
#include "qsignalmapper.h"
#include "kis_image.h"
#include "kis_selection.h"

#include <krita_export.h>

class KAction;
class KisView2;
class KisDoc;
class KisFilter;
class KActionCollection;

/**
 * Create all the filter actions for the specified view and implement re-apply filter
 */
class KRITAUI_EXPORT KisFilterManager : public QObject {

    Q_OBJECT

public:

    KisFilterManager(KisView2 * parent, KisDoc2 * doc);
    ~KisFilterManager();

    void setup(KActionCollection * ac);
    void updateGUI();


    bool apply();

protected slots:

    void slotApply();
    void slotConfigChanged();
    void slotApplyFilter(int);
    void refreshPreview();
    void slotDelayedRefreshPreview();

private:

    class KisFilterManagerPrivate;
    KisFilterManagerPrivate * m_d;

};

#endif
