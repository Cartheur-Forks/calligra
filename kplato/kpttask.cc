/* This file is part of the KDE project
   Copyright (C) 2001 Thomas zander <zander@kde.org>

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

#include <qdom.h>
#include <qbrush.h>
#include <kdebug.h>
#include <koRect.h> //DEBUGRECT


KPTTask::KPTTask(KPTNode *parent) : KPTNode(parent), m_resource() {
    m_resource.setAutoDelete(true);
    m_requests.setAutoDelete(true);
    
    KPTDuration d(24, 0);
    m_effort = new KPTEffort(d) ;

    if (m_parent)
        m_leader = m_parent->leader();
}


KPTTask::~KPTTask() {
    delete m_effort;
}

int KPTTask::type() const {
	if ( numChildren() > 0) {
	  return KPTNode::Type_Summarytask;
	}
	else if ( 0 == effort()->expected().duration() ) {
		return KPTNode::Type_Milestone;
	}
	else {
		return KPTNode::Type_Task;
	}
}



KPTDuration *KPTTask::getExpectedDuration() {
    //kdDebug()<<k_funcinfo<<endl;
    // Duration should already be calculated
    KPTDuration *ed= new KPTDuration(m_duration.dateTime());
    return ed;
}

KPTDuration *KPTTask::getRandomDuration() {
    return 0L;
}

void KPTTask::setStartTime() {
    //kdDebug()<<k_funcinfo<<endl;
    if(numChildren() == 0) {
        switch (m_constraint)
        {
        case KPTNode::ASAP:
            //TODO: it must be room for parents/children also
            m_startTime.set(earliestStart);
            break;
        case KPTNode::ALAP:
        {
            //TODO: it must be room for parents/children also
            m_startTime.set(latestFinish);
            m_startTime.subtract(m_duration);
            break;
        }
        case KPTNode::StartNotEarlier:
            constraintTime() > earliestStart ? m_startTime.set(constraintTime()) : m_startTime.set(earliestStart);
            break;
        case KPTNode::FinishNotLater:
            m_startTime.set(constraintTime()); // FIXME
            break;
        case KPTNode::MustStartOn:
        {
            KPTDuration t(constraintTime().dateTime());
            t.add(m_duration);
            if (constraintTime() >= earliestStart && t <= latestFinish)
                m_startTime.set(constraintTime());
            else {
                // TODO: conflict
                m_startTime.set(earliestStart);
            }
            break;
        }
        default:
            break;
        }
    } else {
        // summary task
        m_startTime.set(KPTDuration::zeroDuration);
        KPTDuration *time = 0;
        QPtrListIterator<KPTNode> it(m_nodes);
        for ( ; it.current(); ++it ) {
            it.current()->setStartTime();
            time = it.current()->getStartTime();
            if (m_startTime < *time || *time == KPTDuration::zeroDuration)
                m_startTime.set(*time);
        }
        delete time;
    }
}

void KPTTask::setEndTime() {
    //kdDebug()<<k_funcinfo<<endl;
    if(numChildren() == 0) {
        m_endTime.set(m_startTime);
        m_endTime.add(m_duration);
    } else {
        // summary task
        m_endTime.set(KPTDuration::zeroDuration);
        KPTDuration *time = 0;
        QPtrListIterator<KPTNode> it(m_nodes);
        for ( ; it.current(); ++it ) {
            it.current()->setEndTime();
            time = it.current()->getEndTime();
            if (*time > m_endTime)
                m_endTime.set(*time);

        }
        delete time;
    }
}

KPTDuration *KPTTask::getStartTime() {
    //kdDebug()<<k_funcinfo<<endl;
    KPTDuration *time = new KPTDuration();
    if(numChildren() == 0) {
        time->set(m_startTime.dateTime());
    } else {
        // summary task
        KPTDuration *start = 0;
        QPtrListIterator<KPTNode> it(m_nodes);
        for ( ; it.current(); ++it ) {
            start = it.current()->getStartTime();
            if (*start < *time || *time == KPTDuration::zeroDuration) {
                time->set(*start);
            }
            delete start;
        }
    }
    return time;
}

KPTDuration *KPTTask::getEndTime() {
    //kdDebug()<<k_funcinfo<<endl;
    KPTDuration *time = new KPTDuration();
    if(numChildren() == 0) {
        time->set(m_startTime.dateTime());
    } else {
        // summary task
        KPTDuration *end;
        QPtrListIterator<KPTNode> it(m_nodes);
        for ( ; it.current(); ++it ) {
            end = it.current()->getEndTime();
            if (*end < *time || *time == KPTDuration::zeroDuration)
                time->set(*end);
            delete end;
        }
    }
    return time;
}

KPTDuration *KPTTask::getFloat() {
    return new KPTDuration;
}

const KPTDuration& KPTTask::expectedDuration(const KPTDuration &start) {
    //kdDebug()<<k_funcinfo<<endl;
    calculateDuration(start);
    return m_duration;
 }

void KPTTask::calculateDuration(const KPTDuration &start) {
    //kdDebug()<<k_funcinfo<<endl;

    int duration = m_effort->expected().duration();

    if (m_effort->type() == KPTEffort::Type_WorkBased) {
        // Here we assume that all requested resources are present
        // which means that the plan will be an ideal one.
        // The ideal plan must later be matched against available resources (elswhere)
        // and conflicts reported to user which must take action to resolve them.
        int num = numWorkResources(); // We assume duration is only dependent on resources that do work
        if (num == 0) {
            if ((num = numResources()) == 0) {
                // This must be reported as an error to user, but we assign a resource to get a decent plan
                m_resourceError = true;
                num = 1;
            } else {
                // Hmm, no work resource, but we have some other type so we use that. Is this sensible at all?
                m_resourceError = false;
            }
        } else {
            m_resourceError = false;
        }
        duration = duration/num;
    } else if (m_effort->type() == KPTEffort::Type_FixedDuration) {
        // The amount of resource doesn't matter
        m_resourceError = false;
    } else {
        // error
        kdError()<<k_funcinfo<<"Unsupported effort type"<<endl;
    }
    KPTDuration d = KPTDuration(duration); //TODO: handle workdays/holidays
    m_duration.set(d);

    // TODO: handle risc

}

KPTResourceGroupRequest *KPTTask::resourceGroupRequest(KPTResourceGroup *group) const {
    QPtrListIterator<KPTResourceGroupRequest> it(m_requests);
    for (; it.current(); ++it) {
        if (it.current()->group() == group)
            return it.current(); // we assume only one request to the same group
    }
    return 0;
}

void KPTTask::clearResourceRequests() {
    m_requests.clear();
}

void KPTTask::addResourceRequest(KPTResourceGroup *group, int numResources) {
    m_requests.append(new KPTResourceGroupRequest(group, numResources));
}

void KPTTask::addResourceRequest(KPTResourceGroupRequest *request) {
    m_requests.append(request);
}

int KPTTask::numResources() const {
    QPtrListIterator<KPTResourceGroupRequest> it(m_requests);
    int num = 0;
    for (; it.current(); ++it) {
        //if (it.current()->isWork())
            num += it.current()->numResources();
    }
    //kdDebug()<<k_funcinfo<<" num="<<num<<endl;
    return num;
}

int KPTTask::numWorkResources() const {
    QPtrListIterator<KPTResourceGroupRequest> it(m_requests);
    int num = 0;
    for (; it.current(); ++it) {
        //if (it.current()->isWork())
            num += it.current()->numResources();
    }
    //kdDebug()<<k_funcinfo<<m_name<<": num="<<num<<endl;
    return num;
}

void KPTTask::requestResources() const {
    //kdDebug()<<k_funcinfo<<name()<<endl;
    if (type() == KPTNode::Type_Task) {
        QPtrListIterator<KPTResourceGroupRequest> it(m_requests);
        for (; it.current(); ++it) {
            // Tell the resource group I want resource(s)
            it.current()->group()->addNode(this);
            //kdDebug()<<k_funcinfo<<name()<<" made request to: "<<it.current()->group()->name()<<endl;
        }
    } else if (type() == KPTNode::Type_Summarytask) {
        QPtrListIterator<KPTNode> nit(m_nodes);
        for ( ; nit.current(); ++nit ) {
            nit.current()->requestResources();
        }
    } else {
        kdDebug()<<k_funcinfo<<"Not yet implemented"<<endl;
    }
}


void KPTTask::addResource(KPTResourceGroup * resource) {
}


void KPTTask::removeResource(KPTResourceGroup * /* resource */){
   // always auto remove
}


void KPTTask::removeResource(int /* number */){
   // always auto remove
}


void KPTTask::insertResource( unsigned int /* index */,
			      KPTResourceGroup * /* resource */) {
}

// A new constraint means start/end times must be recalculated
void KPTTask::setConstraint(KPTNode::ConstraintType type) {
    if (m_constraint == type)
        return;
    m_constraint = type;
    calculateStartEndTime();
}

void KPTTask::calculateStartEndTime() {
    //kdDebug()<<k_funcinfo<<endl;
    setStartEndTime();
    QPtrListIterator<KPTRelation> it(m_dependChildNodes);
    for (; it.current(); ++it) {
        it.current()->child()->calculateStartEndTime(m_endTime); // adjust for all dependant children
    }
}

void KPTTask::calculateStartEndTime(const KPTDuration &start) {
    //kdDebug()<<k_funcinfo<<endl;
    if (start > m_startTime) { //TODO: handle different constraints
        m_startTime.set(start);
        setEndTime();
    }

    QPtrListIterator<KPTRelation> it(m_dependChildNodes);
    for (; it.current(); ++it) {
        it.current()->child()->calculateStartEndTime(m_endTime); // adjust for all dependant children
    }
}

bool KPTTask::load(QDomElement &element) {
    // Load attributes (TODO: Handle different types of tasks, milestone, summary...)
    bool ok = false;
    m_id = mapNode(QString(element.attribute("id","-1")).toInt(&ok), this);
    m_name = element.attribute("name");
    m_leader = element.attribute("leader");
    m_description = element.attribute("description");

    earliestStart = KPTDuration(QDateTime::fromString(element.attribute("earlieststart")));
    latestFinish = KPTDuration(QDateTime::fromString(element.attribute("latestfinish")));
    m_startTime = KPTDuration(QDateTime::fromString(element.attribute("start")));
    m_endTime = KPTDuration(QDateTime::fromString(element.attribute("end")));
    m_duration = KPTDuration(QDateTime::fromString(element.attribute("duration")));

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
                    addResourceRequest(r);
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


    QPtrListIterator<KPTResourceGroupRequest> it(m_requests);
    for ( ; it.current(); ++it ) {
        it.current()->save(me);
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
            int h = KPTDuration::zeroDuration.hoursTo(it.current()->duration().dateTime());
            c +=  h * it.current()->resource()->normalRate();
            //TODO: overtime
            c += it.current()->resource()->fixedCost();
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
            if (it.current()->startTime() < dt) {
                KPTDuration d(it.current()->startTime());
                d.add(it.current()->duration());
                int h = 0;
                dt > d.dateTime() ? h = it.current()->startTime().hoursTo(d.dateTime()) : h = it.current()->startTime().hoursTo(dt);
                c +=  h * it.current()->resource()->normalRate();
                //TODO:overtime
                c += it.current()->resource()->fixedCost();
            }
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
                w += KPTDuration::zeroDuration.hoursTo(it.current()->duration().dateTime());
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
            if (it.current()->startTime() < dt &&
                it.current()->resource()->type() == KPTResource::Type_Work) { // hmmm. show only work?
                KPTDuration d(it.current()->startTime());
                d.add(it.current()->duration());
                dt > d.dateTime() ? w += it.current()->startTime().hoursTo(d.dateTime()) : w += it.current()->startTime().hoursTo(dt);
                //TODO:overtime and non-proportional work
            }
        }
    }
    return w;
}
int KPTTask::actualWork() {
    return 0;
}

#ifndef NDEBUG
void KPTTask::printDebug(bool children, QCString indent) {
    kdDebug()<<indent<<"+ Task node: "<<name()<<" type="<<type()<<endl;
    indent += "!  ";
    kdDebug()<<indent<<"Requested work resources (total): "<<numWorkResources()<<endl;
    kdDebug()<<indent<<"Resource overbooked="<<resourceOverbooked()<<endl;
    kdDebug()<<indent<<"resourceError="<<resourceError()<<endl;
    QPtrListIterator<KPTResourceGroupRequest> it(m_requests);
    for (; it.current(); ++it) {
        it.current()->printDebug(indent);
    }
    kdDebug()<<indent<<endl;
    KPTNode::printDebug(children, indent);

}
#endif
