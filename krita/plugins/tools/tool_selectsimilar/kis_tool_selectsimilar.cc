/*
 *  Copyright (c) 1999 Matthias Elter <me@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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

#include <QPoint>
#include <QLayout>
#include <QCheckBox>
#include <QLabel>
#include <QComboBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <klocale.h>
#include <knuminput.h>

#include <kis_cursor.h>
#include <kis_selection_manager.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_paint_device.h>
#include <KoPointerEvent.h>
#include <kis_selection_options.h>
#include <kis_selection.h>
#include <kis_paint_device.h>
#include <kis_iterators_pixel.h>
#include <kis_selected_transaction.h>
#include <kis_canvas2.h>

#include "kis_tool_selectsimilar.h"

void selectByColor(KisPaintDeviceSP dev, KisSelectionSP selection, const quint8 * c, int fuzziness, selectionAction mode)
{
    // XXX: Multithread this!
    qint32 x, y, w, h;

    dev->exactBounds(x, y, w, h);

    const KoColorSpace * cs = dev->colorSpace();

    KisHLineConstIterator hiter = dev->createHLineConstIterator(x, y, w);
    KisHLineIterator selIter = selection->createHLineIterator(x, y, w);

    for (int row = y; row < y + h; ++row) {
        while (!hiter.isDone()) {
            //if (dev->colorSpace()->hasAlpha())
            //    opacity = dev->colorSpace()->alpha(hiter.rawData());

            quint8 match = cs->difference(c, hiter.rawData());

            if (mode == SELECTION_ADD || mode == SELECTION_REPLACE) {
                if (match <= fuzziness) {
                    *(selIter.rawData()) = MAX_SELECTED;
                }
            }
            else if (mode == SELECTION_SUBTRACT) {
                if (match <= fuzziness) {
                    *(selIter.rawData()) = MIN_SELECTED;
                }
            }
            ++hiter;
            ++selIter;
        }
        hiter.nextRow();
        selIter.nextRow();
    }

}



KisToolSelectSimilar::KisToolSelectSimilar(KoCanvasBase * canvas)
    : KisTool(canvas, KisCursor::load("tool_similar_selection_plus_cursor.png", 6, 6))
{
    m_addCursor = KisCursor::load("tool_similar_selection_plus_cursor.png", 1, 21);
    m_subtractCursor = KisCursor::load("tool_similar_selection_minus_cursor.png", 1, 21);
    m_optWidget = 0;
    m_selectionOptionsWidget = 0;
    m_fuzziness = 20;
    m_currentSelectAction = m_defaultSelectAction = SELECTION_REPLACE;
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), SLOT(slotTimer()) );
}

KisToolSelectSimilar::~KisToolSelectSimilar()
{
}

void KisToolSelectSimilar::activate()
{
    super::activate();
//    m_timer->start(50);
//    setPickerCursor(m_currentSelectAction);

    if (m_selectionOptionsWidget) {
        m_selectionOptionsWidget->slotActivated();
    }
}

void KisToolSelectSimilar::deactivate()
{
    m_timer->stop();
}

void KisToolSelectSimilar::mousePressEvent(KoPointerEvent *e)
{
useCursor(m_subtractCursor);
    if (m_canvas) {
//         QApplication::setOverrideCursor(KisCursor::waitCursor());
        quint8 opacity = OPACITY_OPAQUE;

        if (e->button() != Qt::LeftButton && e->button() != Qt::RightButton)
            return;

        if (not (currentImage() ))
            return;

        KisPaintDeviceSP dev = currentLayer()->paintDevice();

        if (!dev || !currentLayer()->visible())
            return;

        QPointF pos = convertToPixelCoord(e);
        KisSelectionSP selection = currentLayer()->selection();
        bool hasSelection = selection;
#if 0 // XXX_SELECTIOn
        KisSelectedTransaction * t = new KisSelectedTransaction(i18n("Similar Selection"),dev);
#endif
        if (!hasSelection || m_defaultSelectAction == SELECTION_REPLACE)
        {
            selection->clear();
            if(m_defaultSelectAction == SELECTION_SUBTRACT)
                selection->invert();
        }

        KoColor c = dev->colorAt(pos.x(), pos.y());
        opacity = dev->colorSpace()->alpha(c.data());

        // XXX we should make this configurable: "allow to select transparent"
        // if (opacity > OPACITY_TRANSPARENT)
        selectByColor(dev, selection, c.data(), m_fuzziness, m_currentSelectAction);
#if 0
        dev->setDirty();
        dev->emitSelectionChanged();

        m_canvas->addCommand(t);
#endif
//         QApplication::restoreOverrideCursor();
    }
}

#if 0
void KisToolSelectSimilar::slotTimer()
{
    int state = QApplication::keyboardModifiers() & (Qt::ShiftModifier|Qt::ControlModifier|Qt::AltModifier);
    selectionAction action;

    if (state == Qt::ShiftModifier)
        action = SELECTION_ADD;
    else if (state == Qt::ControlModifier)
        action = SELECTION_SUBTRACT;
    else
        action = m_defaultSelectAction;

    if (action != m_currentSelectAction) {
        m_currentSelectAction = action;
        setPickerCursor(action);
    }
}

void KisToolSelectSimilar::setPickerCursor(selectionAction action)
{
//     switch (action) {
//         case SELECTION_ADD:
//             useCursor(m_addCursor);
//             break;
//         case SELECTION_SUBTRACT:
//             useCursor(m_subtractCursor);
//     }
}
#endif

void KisToolSelectSimilar::slotSetFuzziness(int fuzziness)
{
    m_fuzziness = fuzziness;
}

void KisToolSelectSimilar::slotSetAction(int action)
{
    m_defaultSelectAction = (selectionAction)action;
}

QWidget* KisToolSelectSimilar::createOptionWidget()
{
    m_optWidget = new QWidget();
    Q_CHECK_PTR(m_optWidget);

    m_optWidget->setWindowTitle(i18n("Similar Selection"));

    QVBoxLayout * l = new QVBoxLayout(m_optWidget);
    Q_CHECK_PTR(l);
    l->setMargin(0);
    l->setSpacing(6);

    KisCanvas2* canvas = dynamic_cast<KisCanvas2*>(m_canvas);
    Q_ASSERT(canvas);
    m_selectionOptionsWidget = new KisSelectionOptions(canvas);
    Q_CHECK_PTR(m_selectionOptionsWidget);
    m_selectionOptionsWidget->disableAntiAliasSelectionOption();
    m_selectionOptionsWidget->disableSelectionModeOption();

    l->addWidget(m_selectionOptionsWidget);
    connect (m_selectionOptionsWidget, SIGNAL(actionChanged(int)), this, SLOT(slotSetAction(int)));

    QHBoxLayout * hbox = new QHBoxLayout();
    Q_CHECK_PTR(hbox);
    l->addLayout(hbox);

    QLabel * lbl = new QLabel(i18n("Fuzziness: "), m_optWidget);
    Q_CHECK_PTR(lbl);

    hbox->addWidget(lbl);

    KIntNumInput * input = new KIntNumInput(m_optWidget);
    Q_CHECK_PTR(input);
    input->setObjectName("fuzziness");

    input->setRange(0, 200, 10, true);
    input->setValue(20);
    hbox->addWidget(input);
    connect(input, SIGNAL(valueChanged(int)), this, SLOT(slotSetFuzziness(int)));

    l->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding));

    return m_optWidget;
}

QWidget* KisToolSelectSimilar::optionWidget()
{
    return m_optWidget;
}

#include "kis_tool_selectsimilar.moc"
