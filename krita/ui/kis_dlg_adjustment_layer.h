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
#ifndef KISDLGAdjustMENTLAYER_H
#define KISDLGAdjustMENTLAYER_H

#include <kdialog.h>
#include <QLabel>

class KisFilter;
class QListWidgetItem;
class QLabel;
class KisPreviewWidget;
class KisFiltersListView;
class KisFilterConfiguration;
class QGroupBox;
class KisFilterConfigWidget;
class KLineEdit;

#include "kis_types.h"

/**
 * Create a new adjustment layer.
 */
class KisDlgAdjustmentLayer : public KDialog
{

    Q_OBJECT

public:

    /**
     * Create a new adjustmentlayer dialog
     *
     * @param img the current image
     * @param layerName the name of the adjustment layer
     * @param caption the caption for the dialog -- create or properties
     * @param parent the widget parent of this dialog
     * @param name the QObject name, if any
     */
    KisDlgAdjustmentLayer(KisPaintDeviceSP layer,
                          const QString & layerName,
                          const QString & caption,
                          QWidget *parent = 0,
                          const char *name = 0);

    KisFilterConfiguration * filterConfiguration() const;
    QString layerName() const;

protected slots:

    void slotNameChanged( const QString & );
    void slotConfigChanged();
    void refreshPreview();
    void selectionHasChanged ( QListWidgetItem * item );

private:

    KisPaintDeviceSP m_dev;
    KisFiltersListView * m_filtersList;
    KisPreviewWidget * m_preview;
    QGroupBox * m_configWidgetHolder;
    KisFilterConfigWidget * m_currentConfigWidget;
    KisFilter* m_currentFilter;
    KLineEdit * m_layerName;
    QLabel* m_labelNoConfigWidget;
    bool m_customName;
    bool m_freezeName;
};

#endif
