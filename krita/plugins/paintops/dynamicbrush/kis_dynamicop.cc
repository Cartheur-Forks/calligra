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
#include "kis_dynamicop.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QRect>
#include <QWidget>

#include <kis_debug.h>

#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>

#include <kis_autobrush_resource.h>
#include <kis_bookmarked_configuration_manager.h>
#include <kis_bookmarked_configurations_model.h>
#include <kis_brush.h>
#include <kis_global.h>
#include <KoInputDevice.h>

#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_paintop.h>
#include <kis_selection.h>
#include <kis_types.h>

#include "ui_DynamicBrushOptions.h"

#include "kis_dynamic_brush.h"
#include "kis_dynamic_brush_registry.h"
#include "kis_dynamic_coloring.h"
#include "kis_dynamic_coloring_program.h"
#include "kis_dynamic_scattering.h"
#include "kis_dynamic_shape.h"
#include "kis_dynamic_shape_program.h"
#include "shapes/kis_bristle_shape.h"
#include "shapes/kis_dab_shape.h"

KisPaintOp * KisDynamicOpFactory::createOp(const KisPaintOpSettings *settings, KisPainter * painter, KisImageSP image)
{
    Q_UNUSED(image);
    const KisDynamicOpSettings *dosettings = dynamic_cast<const KisDynamicOpSettings *>(settings);
    Q_ASSERT(dosettings);

    KisPaintOp * op = new KisDynamicOp(dosettings, painter);
    Q_CHECK_PTR(op);
    return op;
}

KisPaintOpSettings *KisDynamicOpFactory::settings(QWidget * parent, const KoInputDevice& inputDevice, KisImageSP image)
{
    Q_UNUSED(image);
    Q_UNUSED(inputDevice);
    return new KisDynamicOpSettings(parent, m_shapeBookmarksManager, m_coloringBookmarksManager);
}

KisDynamicOpSettings::KisDynamicOpSettings(QWidget* parent, KisBookmarkedConfigurationManager* shapeBookmarksManager, KisBookmarkedConfigurationManager* coloringBookmarksManager) :
        QObject(parent),
        KisPaintOpSettings(parent),
        m_optionsWidget(new QWidget(parent)),
        m_uiOptions(new Ui_DynamicBrushOptions()),
        m_shapeBookmarksModel(new KisBookmarkedConfigurationsModel(shapeBookmarksManager)),
        m_coloringBookmarksModel(new KisBookmarkedConfigurationsModel(coloringBookmarksManager))
{
    m_uiOptions->setupUi(m_optionsWidget);
    m_uiOptions->comboBoxShapePrograms->setModel( m_shapeBookmarksModel );
    m_uiOptions->comboBoxColoringPrograms->setModel( m_coloringBookmarksModel );
}

KisDynamicOpSettings::~KisDynamicOpSettings()
{
    delete m_uiOptions;
    delete m_shapeBookmarksModel;
    delete m_coloringBookmarksModel;
}

// TEMP
#include <kis_dynamic_brush.h>
// TEMP

KisDynamicBrush* KisDynamicOpSettings::createBrush(KisPainter *painter) const
{
    KisDynamicBrush* current = new KisDynamicBrush(i18n("example"));
    // Init shape program
    QModelIndex shapeModelIndex = m_shapeBookmarksModel->index(
            m_uiOptions->comboBoxShapePrograms->currentIndex(),0);
    KisDynamicShapeProgram* shapeProgram = static_cast<KisDynamicShapeProgram*>(m_shapeBookmarksModel->configuration( shapeModelIndex ) );
    Q_ASSERT(shapeProgram);
    current->setShapeProgram(shapeProgram);
    // Init coloring program
    QModelIndex coloringModelIndex = m_coloringBookmarksModel->index(
            m_uiOptions->comboBoxColoringPrograms->currentIndex(),0);
    KisDynamicColoringProgram* coloringProgram = static_cast<KisDynamicColoringProgram*>(m_coloringBookmarksModel->configuration( coloringModelIndex ) );
    Q_ASSERT(coloringProgram);
    current->setColoringProgram(coloringProgram);
    // Init shape
    switch(m_uiOptions->comboBoxShapes->currentIndex())
    {
        case 1:
            current->setShape(new KisBristleShape());
            break;
        default:
        case 0:
        {
            current->setShape( new KisDabShape( painter->brush() ) );
        }
    }
    // Init coloring
    KisPlainColoring* coloringsrc = new KisPlainColoring( painter->backgroundColor() , painter->paintColor() );
    current->setColoring( coloringsrc );

    return current;
}

KisDynamicOp::KisDynamicOp(const KisDynamicOpSettings *settings, KisPainter *painter)
    : KisPaintOp(painter), m_settings(settings)
{
    Q_ASSERT(settings);
    m_brush = m_settings->createBrush(painter);
    m_brush->startPainting(painter);
}

KisDynamicOp::~KisDynamicOp()
{
    m_brush->endPainting();
    delete m_brush;
}

void KisDynamicOp::paintAt(const KisPaintInformation& info)
{

    if (not painter()->device()) return;

    KisPaintDeviceSP device = painter()->device();


    quint8 origOpacity = painter()->opacity();
    KoColor origColor = painter()->paintColor();

    KisDynamicScattering scatter = m_brush->shapeProgram()->scattering( info );
    
    double maxDist = scatter.maximumDistance();
    
    for(int i = 0; i < scatter.count(); i ++)
    {
        KisPaintInformation localInfo(info);
        
        localInfo.setPos(localInfo.pos() + QPointF( maxDist * (rand() / (double) RAND_MAX - 0.5), maxDist * (rand() / (double) RAND_MAX - 0.5) ) );
        
        KisDynamicShape* dabsrc = m_brush->shape()->clone();
        KisDynamicColoring* coloringsrc = m_brush->coloring()->clone();
        coloringsrc->selectColor( m_brush->coloringProgram()->mix( localInfo ) );
    
        m_brush->shapeProgram()->apply(dabsrc, localInfo);
        m_brush->coloringProgram()->apply(coloringsrc, localInfo);
        dabsrc->paintAt(localInfo.pos(), localInfo, coloringsrc );
    }




    painter()->setOpacity(origOpacity);
    painter()->setPaintColor(origColor);
}

#include "kis_dynamicop.moc"
