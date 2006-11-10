/*
 *  kis_tool_select_contiguous - part of Krayon^WKrita
 *
 *  Copyright (c) 1999 Michael Koch <koch@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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

#include <QPainter>
#include <QLayout>
#include <QLabel>
#include <QApplication>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <klocale.h>
#include <knuminput.h>
#include <kcolorbutton.h>

#include <kis_cursor.h>
#include <kis_selection_manager.h>
#include <kis_canvas_subject.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_paint_device.h>
#include <KoPointerEvent.h>
#include <kis_canvas_subject.h>
#include <kis_selection_options.h>
#include <kis_selection.h>
#include <kis_paint_device.h>
#include <kis_iterators_pixel.h>
#include <kis_selection_options.h>
//#include <kis_canvas_observer.h>
#include <kis_fill_painter.h>
#include <kis_undo_adapter.h>
#include <kis_selected_transaction.h>

#include "kis_tool_select_contiguous.h"

KisToolSelectContiguous::KisToolSelectContiguous() : super(i18n("Contiguous Select"))
{
    setObjectName("tool_select_contiguous");
    m_subject = 0;
    m_optWidget = 0;
    m_fuzziness = 20;
    m_sampleMerged = false;
    m_selectAction = SELECTION_ADD;

    setCursor(KisCursor::load("tool_contiguous_selection_cursor.png", 6, 6));
}

KisToolSelectContiguous::~KisToolSelectContiguous()
{
}

void KisToolSelectContiguous::activate()
{
    super::activate();

    if (!m_optWidget)
        return;

    m_optWidget->slotActivated();
}

void KisToolSelectContiguous::buttonPress(KoPointerEvent * e)
{
    if (m_subject) {
        QApplication::setOverrideCursor(KisCursor::waitCursor());

        KisImageSP m_currentImage;
        KisPaintDeviceSP dev;
        QPoint pos;

        if (e->button() != Qt::LeftButton && e->button() != Qt::RightButton)
            return;

        if (!(m_currentImage = m_currentImage))
            return;

        dev = m_currentImage->activeDevice();

        if (!dev || !m_currentImage->activeLayer()->visible())
            return;


        pos = QPoint(e->pos().floorX(), e->pos().floorY());

        KisFillPainter fillpainter(dev);
        fillpainter.setFillThreshold(m_fuzziness);
        fillpainter.setSampleMerged(m_sampleMerged);
        KisSelectionSP selection = fillpainter.createFloodSelection(pos.x(), pos.y());
        KisSelectedTransaction *t = 0;
        if (m_currentImage->undo()) t = new KisSelectedTransaction(i18n("Contiguous Area Selection"), dev);

        if (!dev->hasSelection()) {
            dev->selection()->clear();
            if(m_selectAction==SELECTION_SUBTRACT)
                selection->invert();
        }

        switch (m_selectAction) {
            case SELECTION_SUBTRACT:
                dev->subtractSelection(selection);
                break;
            case SELECTION_ADD:
            default:
                dev->addSelection(selection);
                break;

        }

        dev->setDirty(selection->extent()); // A bit too wide, but that's not that bad
        dev->emitSelectionChanged();

        if (m_currentImage->undo())
            m_currentImage->undoAdapter()->addCommand(t);

        QApplication::restoreOverrideCursor();
    }

}

void KisToolSelectContiguous::setup(KActionCollection *collection)
{
    m_action = collection->action(objectName());

    if (m_action == 0) {
        m_action = new KAction(KIcon("tool_contiguous_selection"),
                               i18n("&Contiguous Area Selection"),
                               collection,
                               objectName());
        Q_CHECK_PTR(m_action);
        connect(m_action, SIGNAL(triggered()), this, SLOT(activate()));
        m_action->setToolTip(i18n("Select a contiguous area"));
        m_action->setActionGroup(actionGroup());
        m_ownAction = true;
    }
}

void KisToolSelectContiguous::slotSetFuzziness(int fuzziness)
{
    m_fuzziness = fuzziness;
}


void KisToolSelectContiguous::slotSetAction(int action)
{
    if (action >= SELECTION_ADD && action <= SELECTION_SUBTRACT)
        m_selectAction =(enumSelectionMode)action;
// XXX: Fix cursors when then are done.
//     switch(m_selectAction) {
//         case SELECTION_ADD:
//             m_subject->setCanvasCursor(KisCursor::pickerPlusCursor());
//             break;
//         case SELECTION_SUBTRACT:
//             m_subject->setCanvasCursor(KisCursor::pickerMinusCursor());
//     };
}


QWidget* KisToolSelectContiguous::createOptionWidget()
{
    m_optWidget = new KisSelectionOptions(parent, m_subject);
    Q_CHECK_PTR(m_optWidget);
    m_optWidget->setWindowTitle(i18n("Contiguous Area Selection"));

    QVBoxLayout * l = dynamic_cast<QVBoxLayout*>(m_optWidget->layout());
    Q_ASSERT(l);
    if (l) {
        l->setSpacing( 6 );

        connect (m_optWidget, SIGNAL(actionChanged(int)), this, SLOT(slotSetAction(int)));

        QHBoxLayout * hbox = new QHBoxLayout();
        Q_CHECK_PTR(hbox);
        l->addLayout(hbox);

        QLabel * lbl = new QLabel(i18n("Fuzziness: "), m_optWidget);
        hbox->addWidget(lbl);

        KIntNumInput * input = new KIntNumInput(m_optWidget);
        Q_CHECK_PTR(input);
        input->setObjectName("fuzziness");

        input->setRange(0, 200, 10, true);
        input->setValue(20);
        hbox->addWidget(input);
        connect(input, SIGNAL(valueChanged(int)), this, SLOT(slotSetFuzziness(int)));

        QCheckBox* samplemerged = new QCheckBox(i18n("Sample merged"), m_optWidget);
        l->addWidget( samplemerged );
        samplemerged->setChecked(m_sampleMerged);
        connect(samplemerged, SIGNAL(stateChanged(int)),
                this, SLOT(slotSetSampleMerged(int)));

        l->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding));
    }

    return m_optWidget;
}

QWidget* KisToolSelectContiguous::optionWidget()
{
        return m_optWidget;
}

void KisToolSelectContiguous::slotSetSampleMerged(int state)
{
    if (state == QCheckBox::NoChange)
        return;
    m_sampleMerged = (state == QCheckBox::On);
}

#include "kis_tool_select_contiguous.moc"
