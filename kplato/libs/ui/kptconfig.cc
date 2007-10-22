/* This file is part of the KDE project
   Copyright (C) 2004 Dag Andersen <danders@get2net.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation;
   version 2 of the License.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "kptconfig.h"

#include "kptfactory.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kcomponentdata.h>

namespace KPlato
{

Config::Config()
{
    m_readWrite = true;
}

Config::~Config()
{
}

void Config::load() {
    //kDebug();
    KSharedConfigPtr config = Factory::global().config();

/*    if( config->hasGroup("Behavior"))
    {
        config->setGroup("Behavior");
        m_behavior.calculationMode = config->readEntry("CalculationMode",m_behavior.calculationMode);
        m_behavior.allowOverbooking =  config->readEntry("AllowOverbooking",m_behavior.allowOverbooking);
    }*/
    if( config->hasGroup("Task defaults"))
    {
        //TODO: make this default stuff timezone neutral, use LocalZone for now
        KConfigGroup grp(config, "Task defaults");
        m_taskDefaults.setLeader(grp.readEntry("Leader"));
        m_taskDefaults.setDescription(grp.readEntry("Description"));
        m_taskDefaults.setConstraint((Node::ConstraintType)grp.readEntry("ConstraintType",0));
        
        QDateTime dt = grp.readEntry("ConstraintStartTime",QDateTime());
        m_taskDefaults.setConstraintStartTime( DateTime( dt, KDateTime::Spec::LocalZone() ) );
        
        dt = grp.readEntry("ConstraintEndTime",QDateTime());
        m_taskDefaults.setConstraintEndTime( DateTime( dt, KDateTime::Spec::LocalZone() ) );
        
        m_taskDefaults.estimate()->setType((Estimate::Type)grp.readEntry("EstimateType",0));
        m_taskDefaults.estimate()->set(Duration((qint64)(grp.readEntry("ExpectedEstimate",0))*1000)); //FIXME
        m_taskDefaults.estimate()->setPessimisticRatio(grp.readEntry("PessimisticEstimate",0));
        m_taskDefaults.estimate()->setOptimisticRatio(grp.readEntry("OptimisticEstimate",0));
    }
}

void Config::save() {
    //kDebug()<<m_readWrite;
    if (!m_readWrite)
        return;

    KConfigGroup config = Factory::global().config()->group("Task defaults");

    config.writeEntry("Leader", m_taskDefaults.leader());
    config.writeEntry("Description", m_taskDefaults.description());
    config.writeEntry("ConstraintType", (int)m_taskDefaults.constraint());
    config.writeEntry("ConstraintStartTime", m_taskDefaults.constraintStartTime().dateTime());
    config.writeEntry("ConstraintEndTime", m_taskDefaults.constraintEndTime().dateTime());
    config.writeEntry("EstimateType", (int)m_taskDefaults.estimate()->type());
    config.writeEntry("ExpectedEstimate", m_taskDefaults.estimate()->expected().seconds()); //FIXME
    config.writeEntry("PessimisticEstimate", m_taskDefaults.estimate()->pessimisticRatio());
    config.writeEntry("OptimisticEstimate", m_taskDefaults.estimate()->optimisticRatio());
}

}  //KPlato namespace
