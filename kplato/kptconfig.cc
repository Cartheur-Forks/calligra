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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kptconfig.h"

#include "kptfactory.h"

#include <kconfig.h>
#include <kdebug.h>

namespace KPlato
{

KPTConfig::KPTConfig()
{
    m_readWrite = true;
}

KPTConfig::~KPTConfig()
{
}

void KPTConfig::load() {
    //kdDebug()<<k_funcinfo<<endl;
    KConfig *config = KPTFactory::global()->config();
    
    if( config->hasGroup("Behavior"))
    {
        config->setGroup("Behavior");
        m_behavior.dateTimeUsage = config->readNumEntry("UseDateTime",m_behavior.dateTimeUsage);
        m_behavior.calculationMode = config->readNumEntry("CalculationMode",m_behavior.calculationMode);
        m_behavior.allowOverbooking =  config->readBoolEntry("AllowOverbooking",m_behavior.allowOverbooking);
    }
    if( config->hasGroup("Task defaults"))
    {
        config->setGroup("Task defaults");
        m_taskDefaults.setLeader(config->readEntry("Leader"));
        m_taskDefaults.setDescription(config->readEntry("Description"));
        m_taskDefaults.setConstraint((KPTNode::ConstraintType)config->readNumEntry("ConstraintType"));
        m_taskDefaults.setConstraintStartTime(config->readDateTimeEntry("ConstraintStartTime"));
        m_taskDefaults.setConstraintEndTime(config->readDateTimeEntry("ConstraintEndTime"));
        m_taskDefaults.effort()->setType((KPTEffort::Type)config->readNumEntry("EffortType"));
        m_taskDefaults.effort()->set(KPTDuration((Q_INT64)config->readNumEntry("ExpectedEffort")));
        m_taskDefaults.effort()->setPessimisticRatio(config->readNumEntry("PessimisticEffort"));
        m_taskDefaults.effort()->setOptimisticRatio(config->readNumEntry("OptimisticEffort"));
    }
}

void KPTConfig::save() {
    //kdDebug()<<k_funcinfo<<m_readWrite<<endl;
    if (!m_readWrite)
        return;
        
    KConfig *config = KPTFactory::global()->config();
    
    config->setGroup( "Behavior" );
    config->writeEntry("UseDateTime",m_behavior.dateTimeUsage);
    config->writeEntry("CalculationMode",m_behavior.calculationMode);
    config->writeEntry("AllowOverbooking",m_behavior.allowOverbooking);

    config->setGroup("Task defaults");
    config->writeEntry("Leader", m_taskDefaults.leader());
    config->writeEntry("Description", m_taskDefaults.description());
    config->writeEntry("ConstraintType", m_taskDefaults.constraint());
    config->writeEntry("ConstraintStartTime", m_taskDefaults.constraintStartTime());
    config->writeEntry("ConstraintEndTime", m_taskDefaults.constraintEndTime());
    config->writeEntry("EffortType", m_taskDefaults.effort()->type());
    config->writeEntry("ExpectedEffort", m_taskDefaults.effort()->expected().seconds()); //FIXME
    config->writeEntry("PessimisticEffort", m_taskDefaults.effort()->pessimisticRatio());
    config->writeEntry("OptimisticEffort", m_taskDefaults.effort()->optimisticRatio());
}

}  //KPlato namespace
