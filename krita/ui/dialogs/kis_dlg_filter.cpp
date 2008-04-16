/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kis_dlg_filter.h"
#include <QHeaderView>
#include <QTreeView>

// From krita/image
#include <filter/kis_filter.h>
#include <filter/kis_filter_config_widget.h>
#include <filter/kis_filter_configuration.h>
#include <kis_filter_mask.h>
#include <kis_layer.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_paint_device.h>

// From krita/ui
#include "kis_bookmarked_configurations_editor.h"
#include "kis_bookmarked_filter_configurations_model.h"
#include "kis_filters_model.h"

#include "ui_wdgfilterdialog.h"

struct KisFilterDialog::Private {
    Private() 
        : currentCentralWidget(0)
        , currentFilterConfigurationWidget(0)
        , currentFilter(0)
        , layer(0)
        , mask(0)
        , currentBookmarkedFilterConfigurationsModel(0)
        , filtersModel(0)
    {
    }

    ~Private()
    {
        delete currentCentralWidget;
        delete widgetLayout;
        delete filtersModel;
    }

    QWidget* currentCentralWidget;
    KisFilterConfigWidget* currentFilterConfigurationWidget;
    KisFilterSP currentFilter;
    KisLayerSP layer;
    KisPaintDeviceSP thumb;
    Ui_FilterDialog uiFilterDialog;
    KisFilterMaskSP mask;
    QGridLayout *widgetLayout;
    KisBookmarkedFilterConfigurationsModel* currentBookmarkedFilterConfigurationsModel;
    KisFiltersModel* filtersModel;
};

KisFilterDialog::KisFilterDialog(QWidget* parent, KisLayerSP layer ) :
    QDialog( parent ),
    d( new Private )
{
    QRect rc = layer->extent();
    setModal( false );
    d->uiFilterDialog.setupUi( this );
    d->widgetLayout = new QGridLayout( d->uiFilterDialog.centralWidgetHolder );
    d->layer = layer;
    d->thumb = layer->paintDevice()->createThumbnailDevice(100, 100);

    d->mask = new KisFilterMask();
    KisPixelSelectionSP psel = d->mask->selection()->getOrCreatePixelSelection();
    psel->select( rc );
    d->mask->selection()->updateProjection();
    
    d->layer->setPreviewMask( d->mask );
    d->filtersModel = new KisFiltersModel(d->thumb);
    d->uiFilterDialog.filtersSelector->setModel(d->filtersModel);
    d->uiFilterDialog.filtersSelector->header()->setVisible(false);
    connect(d->uiFilterDialog.filtersSelector, SIGNAL(entered(const QModelIndex&)), SLOT(setFilterIndex(const QModelIndex &)));
    connect(d->uiFilterDialog.filtersSelector, SIGNAL(clicked(const QModelIndex&)), SLOT(setFilterIndex(const QModelIndex &)));
    connect(d->uiFilterDialog.filtersSelector, SIGNAL(activated(const QModelIndex&)), SLOT(setFilterIndex(const QModelIndex &)));

    connect(d->uiFilterDialog.comboBoxPresets, SIGNAL(activated ( int )),
            SLOT(slotBookmarkedFilterConfigurationSelected(int )) );
    connect(d->uiFilterDialog.pushButtonOk, SIGNAL(pressed()), SLOT(accept()));
    connect(d->uiFilterDialog.pushButtonOk, SIGNAL(pressed()), SLOT(close()));
    connect(d->uiFilterDialog.pushButtonOk, SIGNAL(pressed()), SLOT(apply()));
    connect(d->uiFilterDialog.pushButtonApply, SIGNAL(pressed()), SLOT(apply()));
    connect(d->uiFilterDialog.pushButtonCancel, SIGNAL(pressed()), SLOT(close()));
    connect(d->uiFilterDialog.pushButtonCancel, SIGNAL(pressed()), SLOT(reject()));
    connect(d->uiFilterDialog.pushButtonEditPressets, SIGNAL(pressed()), SLOT(editConfigurations()));
}

KisFilterDialog::~KisFilterDialog()
{
    delete d;
}

void KisFilterDialog::setFilterIndex(const QModelIndex& idx)
{
    KisFilter* filter = const_cast<KisFilter*>(d->filtersModel->indexToFilter( idx ));
    if(filter)
    {
        setFilter( filter  );
    } else {
        bool v = d->uiFilterDialog.filtersSelector->blockSignals( true );
        QModelIndex idx = d->filtersModel->indexForFilter( d->currentFilter->id() );
        d->uiFilterDialog.filtersSelector->setCurrentIndex( idx );
        d->uiFilterDialog.filtersSelector->scrollTo( idx );
        d->uiFilterDialog.filtersSelector->blockSignals( v );
    }
}

void KisFilterDialog::setFilter(KisFilterSP f)
{
    Q_ASSERT(f);
    setWindowTitle(f->name());
    dbgKrita << "setFilter: " << f;
    d->currentFilter = f;
    delete d->currentCentralWidget;
    {
        bool v = d->uiFilterDialog.filtersSelector->blockSignals( true );
        d->uiFilterDialog.filtersSelector->setCurrentIndex( d->filtersModel->indexForFilter( f->id() ) );
        d->uiFilterDialog.filtersSelector->blockSignals( v );
    }
    KisFilterConfigWidget* widget =
        d->currentFilter->createConfigurationWidget( d->uiFilterDialog.centralWidgetHolder, d->layer->paintDevice() );
    if( !widget )
    { // No widget, so display a label instead
        d->currentFilterConfigurationWidget = 0;
        d->currentCentralWidget = new QLabel( i18n("No configuration option."),
                                              d->uiFilterDialog.centralWidgetHolder );
    } else {
        d->currentFilterConfigurationWidget = widget;
        d->currentCentralWidget = widget;
        d->currentFilterConfigurationWidget->setConfiguration(
            d->currentFilter->defaultConfiguration( d->layer->paintDevice() ) );
        connect(d->currentFilterConfigurationWidget, SIGNAL(sigPleaseUpdatePreview()), SLOT(updatePreview()));
    }
    // Change the list of presets
    delete d->currentBookmarkedFilterConfigurationsModel;
    d->currentBookmarkedFilterConfigurationsModel = new KisBookmarkedFilterConfigurationsModel(d->thumb, f );
    d->uiFilterDialog.comboBoxPresets->setModel(  d->currentBookmarkedFilterConfigurationsModel );
    // Add the widget to the layout
    d->widgetLayout->addWidget( d->currentCentralWidget, 0 , 0);
    d->uiFilterDialog.centralWidgetHolder->setMinimumSize( d->currentCentralWidget->minimumSize() );
    updatePreview();
}

void KisFilterDialog::updatePreview()
{
    dbgKrita <<">>>>  KisFilterDialog::updatePreview()";

    if ( !d->currentFilter ) return;

    if ( d->currentFilterConfigurationWidget ) {
        KisFilterConfiguration* config = d->currentFilterConfigurationWidget->configuration();
        d->mask->setFilter( config );
    }
    else {
        d->mask->setFilter( d->currentFilter->defaultConfiguration( d->layer->paintDevice() ) );
    }
     d->mask->setDirty(d->layer->extent());

}

void KisFilterDialog::apply()
{
    if ( !d->currentFilter ) return;
    
    KisFilterConfiguration* config = 0;
    if( d->currentFilterConfigurationWidget )
    {
        config = d->currentFilterConfigurationWidget->configuration();
    }
    emit(sigPleaseApplyFilter(d->layer, config));
}

void KisFilterDialog::close()
{
    d->layer->removePreviewMask();
    d->layer->setDirty(d->layer->extent());
}

void KisFilterDialog::slotBookmarkedFilterConfigurationSelected(int index)
{
    if(d->currentFilterConfigurationWidget)
    {
        QModelIndex modelIndex = d->currentBookmarkedFilterConfigurationsModel->index(index,0);
        KisFilterConfiguration* config  = d->currentBookmarkedFilterConfigurationsModel->configuration( modelIndex );
        d->currentFilterConfigurationWidget->setConfiguration( config );
    }
}

void KisFilterDialog::editConfigurations()
{
    KisSerializableConfiguration* config =
            d->currentFilterConfigurationWidget ? d->currentFilterConfigurationWidget->configuration() : 0;
    KisBookmarkedConfigurationsEditor editor(this, d->currentBookmarkedFilterConfigurationsModel, config);
    editor.exec();
}


#include "kis_dlg_filter.moc"
