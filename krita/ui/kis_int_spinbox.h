/*
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2006 Casper Boemann <cbr@boemann.dk>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA..
 */
#ifndef KIS_INT_SPINBOX_H_
#define KIS_INT_SPINBOX_H_

#include <QHBoxLayout>
#include <QWidget>

#include <krita_export.h>

/**
 * @short An input widget for integer numbers, consisting of a spinbox and
 * a dropdown slider.
 *
 * KisIntSpinbox combines a QSpinBox and a dropdown QSlider
 * to make an easy to use control for setting some integer
 * parameter.
 *
 *
 */
class KRITAUI_EXPORT KisIntSpinbox : public QWidget
{

    Q_OBJECT
    Q_PROPERTY( int value READ value WRITE setValue )
    Q_PROPERTY( int minValue READ minValue WRITE setMinValue )
    Q_PROPERTY( int maxValue READ maxValue WRITE setMaxValue )

public:

    /**
     * Constructs an input control for integer values
     * with base 10 and initial value 0.
     *
     * @param parent parent QWidget
     * @param name   internal name for this widget
     */
    KisIntSpinbox(QWidget *parent=0, const char *name=0);
    /**
     * Constructor
     * It constructs a QSpinBox that allows the input of integer numbers
     * in the range of -INT_MAX to +INT_MAX.
     * To enforce the value being in a range, use setRange().
     *
     * @param label the tabel (may contain &, and my be empty)
     * @param value  initial value for the control
     * @param parent parent QWidget
     * @param name   internal name for this widget
     */
    KisIntSpinbox(const QString & label, int value, QWidget* parent=0, const char *name=0);

    /**
     * Destructor
     *
     *
     */
    virtual ~KisIntSpinbox();

   /**
     * @return the current value.
     */
    int value() const;

    /**
     * @param min  minimum value
     * @param max  maximum value
     * @param step step size for the QSlider
     */
    void setRange(int min, int max, int step=1);
    /**
     * Sets the minimum value.
     */
    void setMinValue(int min);
    /**
     * @return the minimum value.
     */
    int minValue() const;
    /**
     * Sets the maximum value.
     */
    void setMaxValue(int max);
    /**
     * @return the maximum value.
     */
    int maxValue() const;

    /**
     * Sets the spacing of tickmarks for the slider.
     *
     * @param minor Minor tickmark separation.
     * @param major Major tickmark separation.
     */
    void setSteps(int minor, int major);

    void setLabel(const QString & label);

public slots:
    /**
     * Sets the value of the control.
     */
    void setValue(int);


    void spinboxValueChanged(int val);
    void sliderValueChanged(int val);

    void slotTimeout();

signals:

    /**
     * Emitted every time the value changes (by calling setValue() or
     * by user interaction).
     * @param value the new opacity
     */
    void valueChanged(int value);

    /**
     * Emitted every time the value changes (by calling setValue() or
     * by user interaction).
     * @param value the new opacity
     * @param withSlider whether the value was set by dragging the slider
     */
    void valueChanged(int value, bool withSlider);

    /**
     * Emitted after the slider has been hidden, if the value was changed while it was shown.
     * @param previous the value before the slider was shown
     * @param value the value after the slider was hidden
     */
    void finishedChanging(int previous, int value);

private slots:
    void slotAboutToShow();
    void slotAboutToHide();

private:
    void init(int val);

private:

    class KisIntSpinboxPrivate;
    KisIntSpinboxPrivate *d;
};

#endif
