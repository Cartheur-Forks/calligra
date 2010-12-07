/*
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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
#ifndef KIS_EXPERIMENTOP_OPTION_H
#define KIS_EXPERIMENTOP_OPTION_H

#include <kis_paintop_option.h>
#include <krita_export.h>

const QString EXPERIMENT_DISPLACEMENT_ENABLED = "Experiment/displacementEnabled";
const QString EXPERIMENT_DISPLACEMENT_VALUE = "Experiment/displacement";
const QString EXPERIMENT_SMOOTHING = "Experiment/smoothing";
const QString EXPERIMENT_SPEED_ENABLED = "Experiment/speedEnabled";
const QString EXPERIMENT_SPEED_VALUE = "Experiment/speed";


class KisExperimentOpOptionsWidget;

class KisExperimentOpOption : public KisPaintOpOption
{
public:
    KisExperimentOpOption();
    ~KisExperimentOpOption();

    void writeOptionSetting(KisPropertiesConfiguration* setting) const;
    void readOptionSetting(const KisPropertiesConfiguration* setting);

private:
    KisExperimentOpOptionsWidget * m_options;

};

class ExperimentOption{

    public:
        bool isDisplacementEnabled;
        qreal displacement;
        bool isSpeedEnabled;
        qreal speed;
        bool smoothing;
        
        void readOptionSetting(const KisPropertiesConfiguration* setting){
            isDisplacementEnabled = setting->getBool(EXPERIMENT_DISPLACEMENT_ENABLED);
            displacement = setting->getDouble(EXPERIMENT_DISPLACEMENT_VALUE);
            isSpeedEnabled = setting->getBool(EXPERIMENT_SPEED_ENABLED);
            speed = setting->getDouble(EXPERIMENT_SPEED_VALUE);
            smoothing = setting->getBool(EXPERIMENT_SMOOTHING);
        }
};

#endif
