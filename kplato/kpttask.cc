/* This file is part of the KDE project
   Copyright (C) 2001 Thomas zander <zander@kde.org>
   Copyright (C) 2004 Dag Andersen <danders@get2net.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kpttask.h"
#include "kptproject.h"
#include "kpttaskdialog.h"
#include "kptduration.h"
#include "kptrelation.h"
#include "kptdatetime.h"

#include <qdom.h>
#include <qbrush.h>
#include <kdebug.h>
#include <koRect.h> //DEBUGRECT

namespace KPlato
{

KPTTask::KPTTask(KPTNode *parent) : KPTNode(parent), m_resource() {
    m_resource.setAutoDelete(true);
    KPTDuration d(1, 0, 0);
    m_effort = new KPTEffort(d) ;
    m_requests = 0;

    if (m_parent)
        m_leader = m_parent->leader();
        
    m_parentProxyRelations.setAutoDelete(true);
    m_childProxyRelations.setAutoDelete(true);
}


KPTTask::~KPTTask() {
    delete m_effort;
}

int KPTTask::type() const {
	if ( numChildren() > 0) {
	  return KPTNode::Type_Summarytask;
	}
	else if ( 0 == effort()->expected().seconds() ) {
		return KPTNode::Type_Milestone;
	}
	else {
		return KPTNode::Type_Task;
	}
}



KPTDuration *KPTTask::getExpectedDuration() {
    //kdDebug()<<k_funcinfo<<endl;
    // Duration should already be calculated
    return new KPTDuration(m_duration);
}

KPTDuration *KPTTask::getRandomDuration() {
    return 0L;
}

KPTDuration *KPTTask::getFloat() {
    return new KPTDuration;
}

KPTResourceGroupRequest *KPTTask::resourceGroupRequest(KPTResourceGroup *group) const {
    if (m_requests)
        return m_requests->find(group);
    return 0;
}

void KPTTask::clearResourceRequests() {
    if (m_requests)
        m_requests->clear();
}

void KPTTask::addRequest(KPTResourceGroup *group, int numResources) {
    addRequest(new KPTResourceGroupRequest(group, numResources));
}

void KPTTask::addRequest(KPTResourceGroupRequest *request) {
    if (!m_requests)
        m_requests = new KPTResourceRequestCollection();
    m_requests->addRequest(request);
}

void KPTTask::takeRequest(KPTResourceGroupRequest *request) {
    if (m_requests) {
        m_requests->takeRequest(request);
        if (m_requests->isEmpty()) {
            delete m_requests;
            m_requests = 0;
        }
    }
}

int KPTTask::units() const {
    if (!m_requests)
        return 0;
    return m_requests->units();
}

int KPTTask::workUnits() const {
    if (!m_requests)
        return 0;
    return m_requests->workUnits();
}

void KPTTask::makeAppointments() {
    if (type() == KPTNode::Type_Task) {
        if (m_requests) 
            m_requests->makeAppointments(this);
    } else if (type() == KPTNode::Type_Summarytask) {
        QPtrListIterator<KPTNode> nit(m_nodes);
        for ( ; nit.current(); ++nit ) {
            nit.current()->makeAppointments();
        }
    } else if (type() == KPTNode::Type_Milestone) {
        //kdDebug()<<k_funcinfo<<"Milestone not implemented"<<endl;
        // Well, shouldn't have resources anyway...
    }
}

// A new constraint means start/end times and duration must be recalculated
void KPTTask::setConstraint(KPTNode::ConstraintType type) {
    if (m_constraint == type)
        return;
    m_constraint = type;
    // atm, the user must recalculate the project
//    calculateStartEndTime();
}


bool KPTTask::load(QDomElement &element) {
    // Load attributes (TODO: Handle different types of tasks, milestone, summary...)
    bool ok = false;
    m_id = mapNode(QString(element.attribute("id","-1")).toInt(&ok), this);
    m_name = element.attribute("name");
    m_leader = element.attribute("leader");
    m_description = element.attribute("description");

    earliestStart = KPTDateTime::fromString(element.attribute("earlieststart"));
    latestFinish = KPTDateTime::fromString(element.attribute("latestfinish"));
    m_startTime = KPTDateTime::fromString(element.attribute("start"));
    m_endTime = KPTDateTime::fromString(element.attribute("end"));
    m_duration = KPTDuration::fromString(element.attribute("duration"));

    // Allow for both numeric and text
    QString constraint = element.attribute("scheduling","0");
    m_constraint = (KPTNode::ConstraintType)constraint.toInt(&ok);
    if (!ok)
        KPTNode::setConstraint(constraint); // hmmm, why do I need KPTNode::?

    // Load the project children
    QDomNodeList list = element.childNodes();
    for (unsigned int i=0; i<list.count(); ++i) {
	if (list.item(i).isElement()) {
	    QDomElement e = list.item(i).toElement();

	    if (e.tagName() == "project") {
		// Load the subproject
		KPTProject *child = new KPTProject(this);
		if (child->load(e))
		    addChildNode(child);
		else
		    // TODO: Complain about this
		    delete child;
	    } else if (e.tagName() == "task") {
		// Load the task
		KPTTask *child = new KPTTask(this);
		if (child->load(e))
		    addChildNode(child);
		else
		    // TODO: Complain about this
		    delete child;
	    } else if (e.tagName() == "resource") {
		// TODO: Load the resource
	    } else if (e.tagName() == "effort") {
    		//  Load the effort
            m_effort->load(e);
	    } else if (e.tagName() == "resourcegroup-request") {
		    // Load the resource request
            KPTProject *p = dynamic_cast<KPTProject *>(projectNode());
            if (p == 0) {
                kdDebug()<<k_funcinfo<<"Project does not exist"<<endl;
            } else {
                KPTResourceGroupRequest *r = new KPTResourceGroupRequest();
                if (r->load(e, p))
                    addRequest(r);
                else {
                    kdError()<<k_funcinfo<<"Failed to load resource request"<<endl;
                    delete r;
                }
            }
        }
	}
    }

    return true;
}


void KPTTask::save(QDomElement &element)  {
    QDomElement me = element.ownerDocument().createElement("task");
    element.appendChild(me);

    //TODO: Handle different types of tasks, milestone, summary...
    if (m_id < 0)
        m_id = m_parent->mapNode(this);
    me.setAttribute("id", m_id);
    me.setAttribute("name", m_name);
    me.setAttribute("leader", m_leader);
    me.setAttribute("description", m_description);

    me.setAttribute("earlieststart",earliestStart.toString());
    me.setAttribute("latestfinish",latestFinish.toString());

    me.setAttribute("start",m_startTime.toString());
    me.setAttribute("end",m_endTime.toString());
    me.setAttribute("duration",m_duration.toString());
    me.setAttribute("scheduling",constraintToString());

    m_effort->save(me);


    if (m_requests) {
        m_requests->save(me);
    }

    for (int i=0; i<numChildren(); i++)
    	// First add the child
	    getChildNode(i)->save(me);
}

double KPTTask::plannedCost() {
    //kdDebug()<<k_funcinfo<<endl;
    double c = 0;
    if (type() == KPTNode::Type_Summarytask) {
        QPtrListIterator<KPTNode> it(childNodeIterator());
        for (; it.current(); ++it) {
            c += it.current()->plannedCost();
        }
    } else {
        QPtrList<KPTAppointment> list = appointments(this);
        QPtrListIterator<KPTAppointment> it(list);
        for (; it.current(); ++it) {
            c += it.current()->cost();
        }
    }
    return c;
}

double KPTTask::plannedCost(QDateTime &dt) {
    //kdDebug()<<k_funcinfo<<endl;
    double c = 0;
    if (type() == KPTNode::Type_Summarytask) {
        QPtrListIterator<KPTNode> it(childNodeIterator());
        for (; it.current(); ++it) {
            c += it.current()->plannedCost(dt);
        }
    } else {
        QPtrList<KPTAppointment> list = appointments(this);
        QPtrListIterator<KPTAppointment> it(list);
        for (; it.current(); ++it) {
            c += it.current()->cost(KPTDateTime(dt)); //FIXME
        }
    }
    return c;
}

double KPTTask::actualCost() {
    //kdDebug()<<k_funcinfo<<endl;
    double c = 0;
    if (type() == KPTNode::Type_Summarytask) {
        QPtrListIterator<KPTNode> it(childNodeIterator());
        for (; it.current(); ++it) {
            c += it.current()->actualCost();
        }
    } else {
        c = 1; //TODO
    }
    return c;
}

int KPTTask::plannedWork()  {
    //kdDebug()<<k_funcinfo<<endl;
    int w = 0;
    if (type() == KPTNode::Type_Summarytask) {
        QPtrListIterator<KPTNode> it(childNodeIterator());
        for (; it.current(); ++it) {
            w += it.current()->plannedWork();
        }
    } else {
        QPtrList<KPTAppointment> a = appointments(this);
        QPtrListIterator<KPTAppointment> it(a);
        for (; it.current(); ++it) {
            if (it.current()->resource()->type() == KPTResource::Type_Work) { // hmmm. show only work?
                w += it.current()->work();
                //TODO:overtime and non-proportional work
            }
        }
    }
    return w;
}

int KPTTask::plannedWork(QDateTime &dt)  {
    //kdDebug()<<k_funcinfo<<endl;
    int w = 0;
    if (type() == KPTNode::Type_Summarytask) {
        QPtrListIterator<KPTNode> it(childNodeIterator());
        for (; it.current(); ++it) {
            w += it.current()->plannedWork(dt);
        }
    } else {
        QPtrList<KPTAppointment> a = appointments(this);
        QPtrListIterator<KPTAppointment> it(a);
        for (; it.current(); ++it) {
            w += it.current()->work(KPTDateTime(dt)); //FIXME
        }
    }
    return w;
}
int KPTTask::actualWork() {
    return 0;
}

void KPTTask::initiateCalculationLists(QPtrList<KPTNode> &startnodes, QPtrList<KPTNode> &endnodes, QPtrList<KPTNode> &summarytasks/*, QPtrList<KPTNode> &milestones*/) {
    //kdDebug()<<k_funcinfo<<m_name<<endl;
    if (type() == KPTNode::Type_Summarytask) {
        summarytasks.append(this);
        // propagate my relations to my children and dependent nodes
        
        QPtrListIterator<KPTNode> nodes = m_nodes;
        for (; nodes.current(); ++nodes) {
            if (!dependParentNodes().isEmpty()) 
                nodes.current()->addParentProxyRelations(dependParentNodes());
            if (!dependChildNodes().isEmpty()) 
                nodes.current()->addChildProxyRelations(dependChildNodes());
            nodes.current()->initiateCalculationLists(startnodes, endnodes, summarytasks);
        }        
    } else {
        if (isEndNode()) {
            endnodes.append(this);
            //kdDebug()<<k_funcinfo<<"endnodes append: "<<m_name<<endl;
        }
        if (isStartNode()) {
            startnodes.append(this);
            //kdDebug()<<k_funcinfo<<"startnodes append: "<<m_name<<endl;
        }
    }
}

KPTDateTime KPTTask::calculatePredeccessors(const QPtrList<KPTRelation> &list, int use) {
    KPTDateTime time;
    QPtrListIterator<KPTRelation> it = list;
    for (; it.current(); ++it) {
        if (it.current()->parent()->type() == Type_Summarytask) {
            //kdDebug()<<k_funcinfo<<"Skip summarytask: "<<it.current()->parent()->name()<<endl;
            continue; // skip summarytasks
        }
        time = it.current()->parent()->calculateForward(use);
        switch (it.current()->timingRelation()) {
            case START_START:
                time = it.current()->parent()->getEarliestStart() + it.current()->lag();
                break;
            case FINISH_FINISH:
                // I can't finisg later than my predeccessor, so
                // I can't start later than it's latestfinish - my duration
                time -= duration(time + it.current()->lag(), use, true);
                break;
            default:
                time += it.current()->lag();
                break;
        }
    }
    return time;
}
KPTDateTime KPTTask::calculateForward(int use) {
    //kdDebug()<<k_funcinfo<<m_name<<endl;
    if (m_visitedForward)
        return earliestStart + m_durationForward;
    // First, calculate all predecessors
    KPTDateTime time = calculatePredeccessors(dependParentNodes(), use);
    if (time.isValid() && time > earliestStart) {
        earliestStart = time;
    }
    time = calculatePredeccessors(m_parentProxyRelations, use);
    if (time.isValid() && time > earliestStart) {
        earliestStart = time;
    }
    if (type() == KPTNode::Type_Summarytask) {
        kdWarning()<<k_funcinfo<<"Summarytasks should not be calculated here: "<<m_name<<endl;
    } else if (type() == KPTNode::Type_Task) {
        // Adjust duration to when resource(s) can start work
        if (m_requests) {
            earliestStart = m_requests->availableAfter(earliestStart);
        }
        if (m_visitedBackward && (latestFinish - m_durationBackward) == earliestStart) {
            m_durationForward = m_durationBackward;
        } else {
            m_durationForward = duration(earliestStart, use, false);
        }
        if (m_requests) {
            // Adjust duration to when resource(s) can do (end) work
            m_durationForward = m_requests->availableBefore(earliestStart + m_durationForward) - earliestStart;
        }
    } else if (type() == KPTNode::Type_Milestone) {
        m_durationForward = KPTDuration::zeroDuration;
        //kdDebug()<<k_funcinfo<<m_name<<" "<<earliestStart.toString()<<endl
    } else { // ???
        m_durationForward = KPTDuration::zeroDuration;
    }
    //kdDebug()<<k_funcinfo<<m_name<<": "<<earliestStart.toString()<<" dur="<<m_durationForward.toString()<<endl;
    m_visitedForward = true;
    return earliestStart + m_durationForward;
}

KPTDateTime KPTTask::calculateSuccessors(const QPtrList<KPTRelation> &list, int use) {
    KPTDateTime time;
    QPtrListIterator<KPTRelation> it = list;
    for (; it.current(); ++it) {
        if (it.current()->child()->type() == Type_Summarytask) {
            //kdDebug()<<k_funcinfo<<"Skip summarytask: "<<it.current()->parent()->name()<<endl;
            continue; // skip summarytasks
        }
        time = it.current()->child()->calculateBackward(use);
        switch (it.current()->timingRelation()) {
            case START_START:
                // I can't start before my successor, so
                // I can't finish later than it's starttime + my duration
                time += duration(time -  it.current()->lag(), use, false);
                break;
            case FINISH_FINISH:
                time = it.current()->parent()->getLatestFinish() -  it.current()->lag();
                break;
            default:
                time -= it.current()->lag();
                break;
        }
    
    }
    //kdDebug()<<k_funcinfo<<m_name<<" time="<<time.toString()<<endl;
    return time;
}
KPTDateTime KPTTask::calculateBackward(int use) {
    //kdDebug()<<k_funcinfo<<m_name<<endl;
    if (m_visitedBackward)
        return latestFinish - m_durationBackward;
    // First, calculate all successors
    KPTDateTime time = calculateSuccessors(dependChildNodes(), use);
    if (time.isValid() && time < latestFinish) {
        latestFinish = time;
    }
    time = calculateSuccessors(m_childProxyRelations, use);
    if (time.isValid() && time < latestFinish) {
        latestFinish = time;
    }
    if (type() == KPTNode::Type_Summarytask) {
        kdWarning()<<k_funcinfo<<"Summarytasks should not be calculated here: "<<m_name<<endl;
    } else if (type() == KPTNode::Type_Task) {
        if (m_requests) {
            // Move to when resource(s) can do (end) work
            latestFinish = m_requests->availableBefore(latestFinish);
        }
        if (m_visitedForward && (earliestStart + m_durationForward) == latestFinish) {
            m_durationBackward = m_durationForward;
        } else {
            m_durationBackward = duration(latestFinish, use, true);            
        }
        if (m_requests) {
            // Adjust duration to when resource(s) can start work
            m_durationBackward = latestFinish - m_requests->availableAfter(latestFinish - m_durationBackward);
        }
    } else if (type() == KPTNode::Type_Milestone) {
        m_durationBackward = KPTDuration::zeroDuration;
        //kdDebug()<<k_funcinfo<<m_name<<" "<<latestFinish.toString()<<" : "<<m_endTime.toString()<<endl;
    } else { // ???
        m_durationBackward = KPTDuration::zeroDuration;
    }
    //kdDebug()<<k_funcinfo<<m_name<<": latestFinish="<<latestFinish.toString()<<" dur="<<m_durationBackward.toString()<<endl;
    m_visitedBackward = true;
    return latestFinish - m_durationBackward;
}

KPTDateTime KPTTask::schedulePredeccessors(const QPtrList<KPTRelation> &list, int use) {
    KPTDateTime time;
    QPtrListIterator<KPTRelation> it = list;
    for (; it.current(); ++it) {
        if (it.current()->parent()->type() == Type_Summarytask) {
            //kdDebug()<<k_funcinfo<<"Skip summarytask: "<<it.current()->parent()->name()<<endl;
            continue; // skip summarytasks
        }
        // schedule the predecessors
        KPTDateTime earliest = it.current()->parent()->getEarliestStart();
        time = it.current()->parent()->scheduleForward(earliest, use);
        switch (it.current()->timingRelation()) {
            case START_START:
                time = it.current()->parent()->startTime() + it.current()->lag();
                break;
            case FINISH_FINISH:
                // I can't end before my predecessor, so
                // I can't start before it's endtime - my duration
                time -= duration(time + it.current()->lag(), use, true);
                break;
            default:
                time += it.current()->lag();
                break;
        }
    }
    //kdDebug()<<k_funcinfo<<m_name<<" time="<<time.toString()<<endl;
    return time;
}

KPTDateTime &KPTTask::scheduleForward(KPTDateTime &earliest, int use) {
    //kdDebug()<<k_funcinfo<<m_name<<" earliest="<<earliest<<endl;
    if (m_visitedForward)
        return m_endTime;
    m_startTime = earliest > earliestStart ? earliest : earliestStart;
    // First, calculate all my own predecessors
    KPTDateTime time = schedulePredeccessors(dependParentNodes(), use);
    if (time.isValid() && time > m_startTime) {
        m_startTime = time;
        //kdDebug()<<k_funcinfo<<m_name<<" new startime="<<m_startTime<<endl;
    }
    // Then my parents
    time = schedulePredeccessors(m_parentProxyRelations, use);
    if (time.isValid() && time > m_startTime) {
        m_startTime = time;
        //kdDebug()<<k_funcinfo<<m_name<<" new startime="<<m_startTime<<endl;
    }

    if(type() == KPTNode::Type_Task) {
        switch (m_constraint) {
        case KPTNode::ASAP:
            // m_startTime calculated above
            if (m_startTime == earliestStart)
                m_duration = m_durationForward;
            else if (m_startTime == latestFinish - m_duration)
                m_duration = m_durationBackward;
            else
                m_duration = duration(m_startTime, use, false);
            
            m_endTime = m_startTime + m_duration;
            break;
        case KPTNode::ALAP:
            m_startTime = latestFinish - m_durationBackward;
            m_endTime = latestFinish;
            m_duration = m_durationBackward;
            break;
        case KPTNode::StartNotEarlier:
            //TODO
            break;
        case KPTNode::FinishNotLater:
            //TODO
            break;
        case KPTNode::MustStartOn:
            break;
        default:
            break;
        }
        if (m_requests) {
            m_requests->reserve(m_startTime, m_duration);
        }
    } else if(type() == KPTNode::Type_Milestone) {
        // milestones generally wants to stick to their dependant parent
        KPTDateTime time = schedulePredeccessors(dependParentNodes(), use);
        if (time.isValid() && time < m_endTime) {
            m_endTime = time;
        }
        // Then my parents
        time = scheduleSuccessors(m_parentProxyRelations, use);
        if (time.isValid() && time < m_endTime) {
            m_endTime = time;
        }
        m_endTime = m_startTime;
        m_duration = KPTDuration::zeroDuration;
    } else if (type() == KPTNode::Type_Summarytask) {
        //shouldn't come here
        m_endTime = m_startTime;
        m_duration = m_endTime - m_startTime;
        kdWarning()<<k_funcinfo<<"Summarytasks should not be calculated here: "<<m_name<<endl;
    }
    //kdDebug()<<k_funcinfo<<m_name<<": "<<m_startTime.toString()<<" : "<<m_endTime.toString()<<endl;
    m_visitedForward = true;
    return m_endTime;
}

KPTDateTime KPTTask::scheduleSuccessors(const QPtrList<KPTRelation> &list, int use) {
    KPTDateTime time;
    QPtrListIterator<KPTRelation> it = list;
    for (; it.current(); ++it) {
        if (it.current()->child()->type() == Type_Summarytask) {
            //kdDebug()<<k_funcinfo<<"Skip summarytask: "<<it.current()->child()->name()<<endl;
            continue;
        }
        // get the successors starttime
        KPTDateTime latest = it.current()->child()->getLatestFinish();
        time = it.current()->child()->scheduleBackward(latest, use);
        if (it.current()->timingRelation() != FINISH_START) {
            time = it.current()->child()->endTime();
        }
        switch (it.current()->timingRelation()) {
            case START_START:
                // I can't start before my successor, so
                // I can't finish later than it's starttime + my duration
                time += duration(time - it.current()->lag(), use, false);
                break;
            case FINISH_FINISH:
                time = it.current()->parent()->endTime() - it.current()->lag();
                break;
            default:
                time -= it.current()->lag();
                break;
        }
   }
   return time;
}
KPTDateTime &KPTTask::scheduleBackward(KPTDateTime &latest, int use) {
    //kdDebug()<<k_funcinfo<<m_name<<": latest="<<latest<<endl;
    if (m_visitedBackward)
        return m_startTime;
    m_endTime = latest < latestFinish ? latest : latestFinish;
    // First, calculate all my own successors
    KPTDateTime time = scheduleSuccessors(dependChildNodes(), use);
    if (time.isValid() && time < m_endTime) {
        m_endTime = time;
    }
    // Then my parents
    time = scheduleSuccessors(m_childProxyRelations, use);
    if (time.isValid() && time < m_endTime) {
        m_endTime = time;
    }

    if (type() == KPTNode::Type_Task) {
        switch (m_constraint) {
        case KPTNode::ASAP:
            m_startTime = earliestStart;
            m_endTime = earliestStart + m_durationForward;
            m_duration = m_durationForward;
            break;
        case KPTNode::ALAP:
            // m_endTime calculated above
            if (m_endTime == latestFinish)
                m_duration = m_durationBackward;
            else if (m_endTime == earliestStart + m_duration)
                m_duration = m_durationForward;
            else
                m_duration = duration(m_endTime, use, true);
            
            m_startTime = m_endTime - m_duration;
            break;
        case KPTNode::StartNotEarlier:
            //TODO
            break;
        case KPTNode::FinishNotLater:
            //TODO
            break;
        case KPTNode::MustStartOn:
            break;
        default:
            break;
        }
        if (m_requests) {
            m_requests->reserve(m_startTime, m_duration);
        }
    } else if (type() == KPTNode::Type_Milestone) {
        // milestones generally wants to stick to their dependant parent
        KPTDateTime time = schedulePredeccessors(dependParentNodes(), use);
        if (time.isValid() && time < m_endTime) {
            m_endTime = time;
        }
        // Then my parents
        time = scheduleSuccessors(m_parentProxyRelations, use);
        if (time.isValid() && time < m_endTime) {
            m_endTime = time;
        }
        m_startTime = m_endTime;
        m_duration = KPTDuration::zeroDuration;
    } else if (type() == KPTNode::Type_Summarytask) {
        //shouldn't come here
        m_startTime = m_endTime;
        m_duration = m_endTime - m_startTime;
        kdWarning()<<k_funcinfo<<"Summarytasks should not be calculated here: "<<m_name<<endl;
    }
    //kdDebug()<<k_funcinfo<<m_name<<": "<<m_startTime.toString()<<" : "<<m_endTime.toString()<<endl;
    m_visitedBackward = true;
    return m_startTime;
}

void KPTTask::adjustSummarytask() {
    if (type() == Type_Summarytask) {
        KPTDateTime start = latestFinish;
        KPTDateTime end = earliestStart;
        QPtrListIterator<KPTNode> it(m_nodes);
        for (; it.current(); ++it) {
            it.current()->adjustSummarytask();
            if (it.current()->startTime() < start)
                start = it.current()->startTime();
            if (it.current()->endTime() > end)
                end = it.current()->endTime();
        }
        m_startTime = start;
        m_endTime = end;
        m_duration = end - start;
        //kdDebug()<<k_funcinfo<<m_name<<": "<<m_startTime.toString()<<" : "<<m_endTime.toString()<<endl;
    }
}

// Assumes all subtasks are calculated
KPTDuration KPTTask::summarytaskDurationForward(const KPTDateTime &time) {
    //kdDebug()<<k_funcinfo<<m_name<<endl;
    KPTDuration dur;
    if(type() == KPTNode::Type_Summarytask) {
        KPTDuration tmp;
        QPtrListIterator<KPTNode> it = childNodeIterator();
        for (; it.current(); ++it) {
            tmp = it.current()->summarytaskDurationForward(time);
            if (tmp > dur)
                dur = tmp;
        }
    } else {
        dur = earliestStart + m_durationForward - time;
    }
    //kdDebug()<<k_funcinfo<<m_name<<" dur="<<dur.toString()<<endl;
    return dur;
}

// Assumes all subtasks are calculated
KPTDateTime KPTTask::summarytaskEarliestStart() {
    KPTDateTime time;
    if(type() == KPTNode::Type_Summarytask) {
        KPTDateTime tmp;
        QPtrListIterator<KPTNode> it = childNodeIterator();
        for (; it.current(); ++it) {
            tmp = it.current()->summarytaskEarliestStart();
            if (tmp < time || !time.isValid())
                time = tmp;
        }
    } else {
        time = earliestStart;
    }
    return time;
}

// Assumes all subtasks are calculated
KPTDuration KPTTask::summarytaskDurationBackward(const KPTDateTime &time) {
    //kdDebug()<<k_funcinfo<<m_name<<endl;
    KPTDuration dur;
    if(type() == KPTNode::Type_Summarytask) {
        KPTDuration tmp;
        QPtrListIterator<KPTNode> it = childNodeIterator();
        for (; it.current(); ++it) {
            tmp = it.current()->summarytaskDurationBackward(time);
            if (tmp > dur)
                dur = tmp;
        }
    } else {
        dur = time - (latestFinish - m_durationBackward);
    }
    //kdDebug()<<k_funcinfo<<m_name<<" dur="<<dur.toString()<<endl;
    return dur;
}

// Assumes all subtasks are calculated
KPTDateTime KPTTask::summarytaskLatestFinish() {
    KPTDateTime time;
    if(type() == KPTNode::Type_Summarytask) {
        KPTDateTime tmp;
        QPtrListIterator<KPTNode> it = childNodeIterator();
        for (; it.current(); ++it) {
            tmp = it.current()->summarytaskLatestFinish();
            if (tmp > time || !time.isValid())
                time = tmp;
        }
    } else {
        time = latestFinish;
    }
    return time;
}

KPTDuration KPTTask::calcDuration(const KPTDateTime &time, const KPTDuration &effort, bool backward) {
    //kdDebug()<<"calcDuration "<<m_name<<endl;
    if (!m_requests) {
        m_resourceError = true;
        return effort;
    }
    KPTDuration dur = m_requests->duration(time, effort, backward);
    //kdDebug()<<"calcDuration "<<m_name<<": "<<time.toString()<<" to "<<(time+dur).toString()<<" = "<<dur.toString()<<endl;
    return dur;
}

void KPTTask::clearProxyRelations() {
    m_parentProxyRelations.clear();
    m_childProxyRelations.clear();
}

void KPTTask::addParentProxyRelations(QPtrList<KPTRelation> &list) {
    //kdDebug()<<k_funcinfo<<m_name<<endl;
    if (type() == Type_Summarytask) {
        // propagate to my children
        QPtrListIterator<KPTNode> nodes = m_nodes;
        for (; nodes.current(); ++nodes) {
            nodes.current()->addParentProxyRelations(list);
            nodes.current()->addParentProxyRelations(dependParentNodes());
        }        
    } else {
        // add 'this' as child relation to the relations parent
        QPtrListIterator<KPTRelation> it = list;
        for (; it.current(); ++it) {
            it.current()->parent()->addChildProxyRelation(this, it.current());
            // add a parent relation to myself
            addParentProxyRelation(it.current()->parent(), it.current());
        }
    }
}

void KPTTask::addChildProxyRelations(QPtrList<KPTRelation> &list) {
    //kdDebug()<<k_funcinfo<<m_name<<endl;
    if (type() == Type_Summarytask) {
        // propagate to my children
        QPtrListIterator<KPTNode> nodes = m_nodes;
        for (; nodes.current(); ++nodes) {
            nodes.current()->addChildProxyRelations(list);
            nodes.current()->addChildProxyRelations(dependChildNodes());
        }        
    } else {
        // add 'this' as parent relation to the relations child
        QPtrListIterator<KPTRelation> it = list;
        for (; it.current(); ++it) {
            it.current()->child()->addParentProxyRelation(this, it.current());
            // add a child relation to myself
            addChildProxyRelation(it.current()->child(), it.current());
        }
    }
}

void KPTTask::addParentProxyRelation(KPTNode *node, const KPTRelation *rel) {
    if (node->type() != Type_Summarytask) {
        //kdDebug()<<"Add parent proxy from "<<node->name()<<" to (me) "<<m_name<<endl;
        m_parentProxyRelations.append(new KPTProxyRelation(node, this, rel->timingType(), rel->timingRelation(), rel->lag()));
    }
}

void KPTTask::addChildProxyRelation(KPTNode *node, const KPTRelation *rel) {
    if (node->type() != Type_Summarytask) {
        //kdDebug()<<"Add child proxy from (me) "<<m_name<<" to "<<node->name()<<endl;
        m_childProxyRelations.append(new KPTProxyRelation(this, node, rel->timingType(), rel->timingRelation(), rel->lag()));
    }
}

bool KPTTask::isEndNode() const {
    QPtrListIterator<KPTRelation> it = m_dependChildNodes;
    for (; it.current(); ++it) {
        if (it.current()->timingRelation() == FINISH_START)
            return false;
    }
    QPtrListIterator<KPTRelation> pit = m_childProxyRelations;
    for (; pit.current(); ++pit) {
        if (pit.current()->timingRelation() == FINISH_START)
            return false;
    }
    return true;
}
bool KPTTask::isStartNode() const {
    QPtrListIterator<KPTRelation> it = m_dependParentNodes;
    for (; it.current(); ++it) {
        if (it.current()->timingRelation() == FINISH_START ||
            it.current()->timingRelation() == START_START)
            return false;
    }
    QPtrListIterator<KPTRelation> pit = m_parentProxyRelations;
    for (; pit.current(); ++pit) {
        if (pit.current()->timingRelation() == FINISH_START ||
            pit.current()->timingRelation() == START_START)
            return false;
    }
    return true;
}

#ifndef NDEBUG
void KPTTask::printDebug(bool children, QCString indent) {
    kdDebug()<<indent<<"+ Task node: "<<name()<<" type="<<type()<<endl;
    indent += "!  ";
    kdDebug()<<indent<<"Requested resources (total): "<<units()<<"%"<<endl;
    kdDebug()<<indent<<"Requested resources (work): "<<workUnits()<<"%"<<endl;
    kdDebug()<<indent<<"Resource overbooked="<<resourceOverbooked()<<endl;
    kdDebug()<<indent<<"resourceError="<<resourceError()<<endl;
    if (m_requests)
        m_requests->printDebug(indent);
    kdDebug()<<indent<<endl;
    KPTNode::printDebug(children, indent);

}

#endif

}  //KPlato namespace
