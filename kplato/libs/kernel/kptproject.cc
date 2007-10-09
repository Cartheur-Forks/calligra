/* This file is part of the KDE project
 Copyright (C) 2001 Thomas zander <zander@kde.org>
 Copyright (C) 2004 - 2007 Dag Andersen <danders@get2net.dk>
 Copyright (C) 2007 Florian Piquemal <flotueur@yahoo.fr>
 Copyright (C) 2007 Alexis Ménard <darktears31@gmail.com>

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
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#include "kptproject.h"
#include "kptappointment.h"
#include "kpttask.h"
#include "kptdatetime.h"
#include "kpteffortcostmap.h"
#include "kptschedule.h"
#include "kptwbsdefinition.h"
#include "kptxmlloaderobject.h"

#include <KoXmlReader.h>

#include <qdom.h>
#include <QString>
#include <qdatetime.h>
#include <qbrush.h>
#include <q3canvas.h>
#include <QList>

#include <kdatetime.h>
#include <kdebug.h>
#include <ksystemtimezone.h>
#include <ktimezone.h>

namespace KPlato
{


Project::Project( Node *parent )
        : Node( parent ),
        m_accounts( *this ),
        m_defaultCalendar( 0 )
{
    //kDebug()<<"("<<this<<")";
    m_constraint = Node::MustStartOn;
    m_standardWorktime = new StandardWorktime();
    init();
}

void Project::init()
{
//    m_currentViewScheduleId = -1;
    m_spec = KDateTime::Spec::LocalZone();
    if ( !m_spec.timeZone().isValid() ) {
        m_spec.setType( KTimeZone() );
    }
    //kDebug()<<m_spec.timeZone();
    if ( m_parent == 0 ) {
        // set sensible defaults for a project wo parent
        m_constraintStartTime = DateTime( QDate::currentDate(), QTime(), m_spec );
        m_constraintEndTime = m_constraintStartTime.addDays( 1 );
    }
}


Project::~Project()
{
    disconnect(); // NOTE: may be a problem if sombody uses the destroyd() signal
    delete m_standardWorktime;
    while ( !m_resourceGroups.isEmpty() )
        delete m_resourceGroups.takeFirst();
    while ( !m_calendars.isEmpty() )
        delete m_calendars.takeFirst();
    while ( !m_managers.isEmpty() )
        delete m_managers.takeFirst();
}

int Project::type() const { return Node::Type_Project; }

void Project::calculate( Schedule *schedule, const DateTime &dt )
{
    if ( schedule == 0 ) {
        kError() << "Schedule == 0, cannot calculate" << endl;
        return ;
    }
    m_currentSchedule = schedule;
    calculate( dt );
}

void Project::calculate( const DateTime &dt )
{
    if ( m_currentSchedule == 0 ) {
        kError() << "No current schedule to calculate" << endl;
        return ;
    }
    MainSchedule *cs = static_cast<MainSchedule*>( m_currentSchedule );
    Estimate::Use estType = ( Estimate::Use ) cs->type();
    if ( type() == Type_Project ) {
        initiateCalculation( *cs );
        initiateCalculationLists( *cs ); // must be after initiateCalculation() !!
        //kDebug()<<"Node="<<m_name<<" Start="<<m_constraintStartTime.toString();
        cs->startTime = dt;
        cs->earlyStart = dt;
        // Calculate from start time
        propagateEarliestStart( cs->earlyStart );
        cs->lateFinish = calculateForward( estType );
        propagateLatestFinish( cs->lateFinish );
        cs->calculateBackward( estType );
        cs->endTime = scheduleForward( cs->startTime, estType );
        calcCriticalPath( false );
        //makeAppointments();
        calcResourceOverbooked();
        cs->notScheduled = false;
        calcFreeFloat();
        emit scheduleChanged( cs );
    } else if ( type() == Type_Subproject ) {
        kWarning() << "Subprojects not implemented" << endl;
    } else {
        kError() << "Illegal project type: " << type() << endl;
    }
}

void Project::calculate( ScheduleManager &sm )
{
    emit sigProgress( 0 );
    //kDebug();
    if ( sm.recalculate() ) {
        sm.setCalculateAll( false );
        sm.createSchedules();
        calculate( sm.expected(), sm.fromDateTime() );
    } else {
        sm.createSchedules();
        calculate( sm.expected() );
        emit scheduleChanged( sm.expected() );
        if ( sm.optimistic() ) {
            calculate( sm.optimistic() );
        }
        if ( sm.pessimistic() ) {
            calculate( sm.pessimistic() );
        }
        setCurrentSchedule( sm.expected()->id() );
    }
    emit sigProgress( 100 );
    emit sigProgress( -1 );
    emit projectCalculated( &sm );
}

void Project::calculate( Schedule *schedule )
{
    if ( schedule == 0 ) {
        kError() << "Schedule == 0, cannot calculate" << endl;
        return ;
    }
    m_currentSchedule = schedule;
    calculate();
}

void Project::calculate()
{
    if ( m_currentSchedule == 0 ) {
        kError() << "No current schedule to calculate" << endl;
        return ;
    }
    MainSchedule *cs = static_cast<MainSchedule*>( m_currentSchedule );
    Estimate::Use estType = ( Estimate::Use ) cs->type();
    if ( type() == Type_Project ) {
        initiateCalculation( *cs );
        initiateCalculationLists( *cs ); // must be after initiateCalculation() !!
        if ( m_constraint == Node::MustStartOn ) {
            //kDebug()<<"Node="<<m_name<<" Start="<<m_constraintStartTime.toString();
            cs->startTime = m_constraintStartTime;
            cs->earlyStart = m_constraintStartTime;
            // Calculate from start time
            propagateEarliestStart( cs->earlyStart );
            cs->lateFinish = calculateForward( estType );
            propagateLatestFinish( cs->lateFinish );
            cs->calculateBackward( estType );
            cs->endTime = scheduleForward( cs->startTime, estType );
            cs->duration = cs->endTime - cs->startTime;
            calcCriticalPath( false );
        } else {
            //kDebug()<<"Node="<<m_name<<" End="<<m_constraintEndTime.toString();
            cs->endTime = m_constraintEndTime;
            cs->lateFinish = m_constraintEndTime;
            // Calculate from end time
            propagateLatestFinish( cs->lateFinish );
            cs->earlyStart = calculateBackward( estType );
            propagateEarliestStart( cs->earlyStart );
            cs->calculateForward( estType );
            cs->startTime = scheduleBackward( cs->endTime, estType );
            cs->duration = cs->endTime - cs->startTime;
            calcCriticalPath( true );
        }
        //makeAppointments();
        calcResourceOverbooked();
        cs->notScheduled = false;
        calcFreeFloat();
        emit scheduleChanged( cs );
    } else if ( type() == Type_Subproject ) {
        kWarning() << "Subprojects not implemented" << endl;
    } else {
        kError() << "Illegal project type: " << type() << endl;
    }
}

bool Project::calcCriticalPath( bool fromEnd )
{
    //kDebug();
    MainSchedule *cs = static_cast<MainSchedule*>( m_currentSchedule );
    if ( cs == 0 ) {
        return false;
    }
    if ( fromEnd ) {
        QListIterator<Node*> startnodes = cs->startNodes();
        while ( startnodes.hasNext() ) {
            startnodes.next() ->calcCriticalPath( fromEnd );
        }
    } else {
        QListIterator<Node*> endnodes = cs->endNodes();
        while ( endnodes.hasNext() ) {
            endnodes.next() ->calcCriticalPath( fromEnd );
        }
    }
    calcCriticalPathList( cs );
    return false;
}

void Project::calcCriticalPathList( MainSchedule *cs )
{
    kDebug()<<m_name<<", "<<cs->name()<<endl;
    cs->clearCriticalPathList();
    foreach ( Node *n, allNodes() ) {
        if ( n->numDependParentNodes() == 0 && n->inCriticalPath( cs->id() ) ) {
            cs->addCriticalPath();
            cs->addCriticalPathNode( n );
            calcCriticalPathList( cs, n );
        }
    }
    cs->criticalPathListCached = true;
    kDebug()<<*(criticalPathList( cs->id() ))<<endl;
}

void Project::calcCriticalPathList( MainSchedule *cs, Node *node )
{
    kDebug()<<node->name()<<", "<<cs->id()<<endl;
    bool newPath = false;
    QList<Node*> lst = *( cs->currentCriticalPath() );
    foreach ( Relation *r, node->dependChildNodes() ) {
        if ( r->child()->inCriticalPath( cs->id() ) ) {
            if ( newPath ) {
                cs->addCriticalPath( &lst );
                kDebug()<<node->name()<<" new path"<<endl;
            }
            cs->addCriticalPathNode( r->child() );
            calcCriticalPathList( cs, r->child() );
            newPath = true;
        }
    }
}

const QList< QList<Node*> > *Project::criticalPathList( long id )
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    if ( s == 0 ) {
        kDebug()<<"No schedule with id="<<id<<endl;
        return 0;
    }
    MainSchedule *ms = static_cast<MainSchedule*>( s );
    if ( ! ms->criticalPathListCached ) {
        initiateCalculationLists( *ms );
        calcCriticalPathList( ms );
    }
    return ms->criticalPathList();
}

QList<Node*> Project::criticalPath( long id, int index )
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    if ( s == 0 ) {
        kDebug()<<"No schedule with id="<<id<<endl;
        return QList<Node*>();
    }
    MainSchedule *ms = static_cast<MainSchedule*>( s );
    if ( ! ms->criticalPathListCached ) {
        initiateCalculationLists( *ms );
        calcCriticalPathList( ms );
    }
    return ms->criticalPath( index );
}

DateTime Project::startTime( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->startTime : m_constraintStartTime;
}

DateTime Project::endTime(  long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->endTime : m_constraintEndTime;
}

Duration *Project::getRandomDuration()
{
    return 0L;
}

DateTime Project::calculateForward( int use )
{
    //kDebug()<<m_name;
    MainSchedule *cs = static_cast<MainSchedule*>( m_currentSchedule );
    if ( cs == 0 ) {
        return DateTime();
    }
    if ( type() == Node::Type_Project ) {
        // Do constrained first
        foreach ( Node *n, cs->hardConstraints() ) {
            n->calculateEarlyFinish( use );
        }
        // Follow *parent* relations back and
        // calculate forwards following the child relations
        DateTime finish;
        DateTime time;
        foreach ( Node *n, cs->endNodes() ) {
            time = n->calculateForward( use );
            if ( !finish.isValid() || time > finish )
                finish = time;
        }
        //kDebug()<<m_name<<" finish="<<finish.toString();
        return finish;
    } else {
        //TODO: subproject
    }
    return DateTime();
}

DateTime Project::calculateBackward( int use )
{
    //kDebug()<<m_name;
    MainSchedule *cs = static_cast<MainSchedule*>( m_currentSchedule );
    if ( cs == 0 ) {
        return DateTime();
    }
    if ( type() == Node::Type_Project ) {
        // Do constrained first
        foreach ( Node *n, cs->hardConstraints() ) {
            n->calculateLateStart( use );
        }
        // Follow *child* relations back and
        // calculate backwards following parent relation
        DateTime start;
        DateTime time;
        foreach ( Node *n, cs->startNodes() ) {
            time = n->calculateBackward( use );
            if ( !start.isValid() || time < start )
                start = time;
        }
        //kDebug()<<m_name<<" start="<<start.toString();
        return start;
    } else {
        //TODO: subproject
    }
    return DateTime();
}

DateTime Project::scheduleForward( const DateTime &earliest, int use )
{
    MainSchedule *cs = static_cast<MainSchedule*>( m_currentSchedule );
    if ( cs == 0 ) {
        return DateTime();
    }
    resetVisited();
    DateTime end = earliest;
    DateTime time;
    QListIterator<Node*> it( cs->endNodes() );
    while ( it.hasNext() ) {
        time = it.next() ->scheduleForward( earliest, use );
        if ( time > end )
            end = time;
    }
    // Fix summarytasks
    adjustSummarytask();
    return end;
}

DateTime Project::scheduleBackward( const DateTime &latest, int use )
{
    MainSchedule *cs = static_cast<MainSchedule*>( m_currentSchedule );
    if ( cs == 0 ) {
        return DateTime();
    }
    resetVisited();
    DateTime start = latest;
    DateTime time;
    QListIterator<Node*> it( cs->startNodes() );
    while ( it.hasNext() ) {
        time = it.next() ->scheduleBackward( latest, use );
        if ( time < start )
            start = time;
    }
    // Fix summarytasks
    adjustSummarytask();
    return start;
}

void Project::adjustSummarytask()
{
    MainSchedule *cs = static_cast<MainSchedule*>( m_currentSchedule );
    if ( cs == 0 ) {
        return;
    }
    QListIterator<Node*> it( cs->summaryTasks() );
    while ( it.hasNext() ) {
        it.next() ->adjustSummarytask();
    }
}

void Project::initiateCalculation( MainSchedule &sch )
{
    //kDebug()<<m_name;
    // clear all resource appointments
    m_visitedForward = false;
    m_visitedBackward = false;
    QListIterator<ResourceGroup*> git( m_resourceGroups );
    while ( git.hasNext() ) {
        git.next() ->initiateCalculation( sch );
    }
    Node::initiateCalculation( sch );
}

void Project::initiateCalculationLists( MainSchedule &sch )
{
    //kDebug()<<m_name;
    sch.clearNodes();
    if ( type() == Node::Type_Project ) {
        QListIterator<Node*> it = childNodeIterator();
        while ( it.hasNext() ) {
            it.next() ->initiateCalculationLists( sch );
        }
    } else {
        //TODO: subproject
    }
}

bool Project::load( KoXmlElement &element, XMLLoaderObject &status )
{
    //kDebug()<<"--->";
    QList<Calendar*> cals;
    QString s;
    bool ok = false;
    QString id = element.attribute( "id" );
    if ( !setId( id ) ) {
        kWarning() << "Id must be unique: " << id << endl;
    }
    m_name = element.attribute( "name" );
    m_leader = element.attribute( "leader" );
    m_description = element.attribute( "description" );
    KTimeZone tz = KSystemTimeZones::zone( element.attribute( "timezone" ) );
    if ( tz.isValid() ) {
        m_spec = KDateTime::Spec( tz );
    } else kWarning()<<"No timezone specified, using default (local)"<<endl;
    status.setProjectSpec( m_spec );
    
    // Allow for both numeric and text
    s = element.attribute( "scheduling", "0" );
    m_constraint = ( Node::ConstraintType ) s.toInt( &ok );
    if ( !ok )
        setConstraint( s );
    if ( m_constraint != Node::MustStartOn &&
            m_constraint != Node::MustFinishOn ) {
        kError() << "Illegal constraint: " << constraintToString() << endl;
        setConstraint( Node::MustStartOn );
    }
    s = element.attribute( "start-time" );
    if ( !s.isEmpty() )
        m_constraintStartTime = DateTime::fromString( s, m_spec );
    s = element.attribute( "end-time" );
    if ( !s.isEmpty() )
        m_constraintEndTime = DateTime::fromString( s, m_spec );

    // Load the project children
    // Do calendars first, they only refrence other calendars
    //kDebug()<<"Calendars--->";
    KoXmlNode n = element.firstChild();
    for ( ; ! n.isNull(); n = n.nextSibling() ) {
        if ( ! n.isElement() ) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if ( e.tagName() == "calendar" ) {
            // Load the calendar.
            // Referenced by resources
            Calendar * child = new Calendar();
            child->setProject( this );
            if ( child->load( e, status ) ) {
                cals.append( child ); // temporary, reorder later
            } else {
                // TODO: Complain about this
                kError() << "Failed to load calendar" << endl;
                delete child;
            }
        } else if ( e.tagName() == "standard-worktime" ) {
            // Load standard worktime
            StandardWorktime * child = new StandardWorktime();
            if ( child->load( e, status ) ) {
                setStandardWorktime( child );
            } else {
                kError() << "Failed to load standard worktime" << endl;
                delete child;
            }
        }
    }
    // calendars references calendars in arbritary saved order
    bool added = false;
    do {
        added = false;
        QList<Calendar*> lst;
        while ( !cals.isEmpty() ) {
            Calendar *c = cals.takeFirst();
            if ( c->parentId().isEmpty() ) {
                addCalendar( c, status.baseCalendar() ); // handle pre 0.6 version
                added = true;
                //kDebug()<<"added to project:"<<c->name();
            } else {
                Calendar *par = calendar( c->parentId() );
                if ( par ) {
                    addCalendar( c, par );
                    added = true;
                    //kDebug()<<"added:"<<c->name()<<" to parent:"<<par->name();
                } else {
                    lst.append( c ); // treat later
                    //kDebug()<<"treat later:"<<c->name();
                }
            }
        }
        cals = lst;
    } while ( added );
    if ( ! cals.isEmpty() ) {
        kError()<<"All calendars not saved!"<<endl;
    }
    //kDebug()<<"Calendars<---";
    // Resource groups and resources, can reference calendars
    n = element.firstChild();
    for ( ; ! n.isNull(); n = n.nextSibling() ) {
        if ( ! n.isElement() ) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if ( e.tagName() == "resource-group" ) {
            // Load the resources
            // References calendars
            ResourceGroup * child = new ResourceGroup();
            if ( child->load( e, status ) ) {
                addResourceGroup( child );
            } else {
                // TODO: Complain about this
                delete child;
            }
        }
    }
    // The main stuff
    n = element.firstChild();
    for ( ; ! n.isNull(); n = n.nextSibling() ) {
        if ( ! n.isElement() ) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if ( e.tagName() == "project" ) {
            //kDebug()<<"Sub project--->";
/*                // Load the subproject
            Project * child = new Project( this );
            if ( child->load( e ) ) {
                if ( !addTask( child, this ) ) {
                    delete child; // TODO: Complain about this
                }
            } else {
                // TODO: Complain about this
                delete child;
            }*/
        } else if ( e.tagName() == "task" ) {
            //kDebug()<<"Task--->";
            // Load the task (and resourcerequests).
            // Depends on resources already loaded
            Task * child = new Task( this );
            if ( child->load( e, status ) ) {
                if ( !addTask( child, this ) ) {
                    delete child; // TODO: Complain about this
                }
            } else {
                // TODO: Complain about this
                delete child;
            }
        }
    }
    // These go last
    n = element.firstChild();
    for ( ; ! n.isNull(); n = n.nextSibling() ) {
        if ( ! n.isElement() ) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if ( e.tagName() == "accounts" ) {
            //kDebug()<<"Accounts--->";
            // Load accounts
            // References tasks
            if ( !m_accounts.load( e, *this ) ) {
                kError() << "Failed to load accounts" << endl;
            }
        } else if ( e.tagName() == "relation" ) {
            //kDebug()<<"Relation--->";
            // Load the relation
            // References tasks
            Relation * child = new Relation();
            if ( !child->load( e, *this ) ) {
                // TODO: Complain about this
                kError() << "Failed to load relation" << endl;
                delete child;
            }
            //kDebug()<<"Relation<---";
        } else if ( e.tagName() == "schedules" ) {
            //kDebug()<<"Project schedules & task appointments--->";
            // References tasks and resources
            n = e.firstChild();
            for ( ; ! n.isNull(); n = n.nextSibling() ) {
                if ( ! n.isElement() ) {
                    continue;
                }
                KoXmlElement el = n.toElement();
                //kDebug()<<el.tagName()<<" Version="<<status.version();
                ScheduleManager *sm = 0;
                bool add = false;
                if ( status.version() <= "0.5" ) {
                    if ( el.tagName() == "schedule" ) {
                        sm = findScheduleManager( el.attribute( "name" ) );
                        if ( sm == 0 ) {
                            sm = new ScheduleManager( *this, el.attribute( "name" ) );
                            add = true;
                        }
                    }
                } else if ( el.tagName() == "plan" ) {
                    sm = new ScheduleManager( *this );
                    add = true;
                }
                if ( sm ) {
                    if ( sm->loadXML( el, status ) ) {
                        if ( add )
                            addScheduleManager( sm );
                    } else {
                        kError() << "Failed to load schedule manager" << endl;
                        delete sm;
                    }
                } else {
                    kDebug()<<"No schedule manager ?!";
                }
            }
            //kDebug()<<"Node schedules<---";
        }
    }
    //kDebug()<<"Project schedules--->";
    foreach ( Schedule * s, m_schedules ) {
        if ( m_constraint == Node::MustFinishOn )
            m_constraintEndTime = s->endTime;
        else
            m_constraintStartTime = s->startTime;
    }
    //kDebug()<<"Project schedules<---";
    //kDebug()<<"<---";
    return true;
}

void Project::save( QDomElement &element ) const
{
    QDomElement me = element.ownerDocument().createElement( "project" );
    element.appendChild( me );

    me.setAttribute( "name", m_name );
    me.setAttribute( "leader", m_leader );
    me.setAttribute( "id", m_id );
    me.setAttribute( "description", m_description );
    me.setAttribute( "timezone", m_spec.timeZone().name() );
    
    me.setAttribute( "scheduling", constraintToString() );
    me.setAttribute( "start-time", m_constraintStartTime.toString( KDateTime::ISODate ) );
    me.setAttribute( "end-time", m_constraintEndTime.toString( KDateTime::ISODate ) );

    m_accounts.save( me );

    // save calendars
    foreach ( Calendar *c, calendarIdDict.values() ) {
        c->save( me );
    }
    // save standard worktime
    if ( m_standardWorktime )
        m_standardWorktime->save( me );

    // save project resources, must be after calendars
    QListIterator<ResourceGroup*> git( m_resourceGroups );
    while ( git.hasNext() ) {
        git.next() ->save( me );
    }

    // Only save parent relations
    QListIterator<Relation*> it( m_dependParentNodes );
    while ( it.hasNext() ) {
        it.next() ->save( me );
    }

    for ( int i = 0; i < numChildren(); i++ )
        // Save all children
        childNode( i ) ->save( me );

    // Now we can save relations assuming no tasks have relations outside the project
    QListIterator<Node*> nodes( m_nodes );
    while ( nodes.hasNext() ) {
        nodes.next() ->saveRelations( me );
    }

    if ( !m_managers.isEmpty() ) {
        QDomElement el = me.ownerDocument().createElement( "schedules" );
        me.appendChild( el );
        foreach ( ScheduleManager *sm, m_managers ) {
            sm->saveXML( el );
        }
    }
}

// void Project::setCurrentViewScheduleId( long id )
// {
//     if ( id != m_currentViewScheduleId ) {
//         m_currentViewScheduleId = id;
//         emit currentViewScheduleIdChanged( id );
//     }
// }

void Project::setParentSchedule( Schedule *sch )
{
    QListIterator<Node*> it = m_nodes;
    while ( it.hasNext() ) {
        it.next() ->setParentSchedule( sch );
    }
}

void Project::addResourceGroup( ResourceGroup *group, int index )
{
    int i = index == -1 ? m_resourceGroups.count() : index;
    emit resourceGroupToBeAdded( group, i );
    m_resourceGroups.insert( i, group );
    setResourceGroupId( group );
    group->setProject( this );
    foreach ( Resource *r, group->resources() ) {
        setResourceId( r );
        r->setProject( this );
    }
    emit resourceGroupAdded( group );
}

ResourceGroup *Project::takeResourceGroup( ResourceGroup *group )
{
    int i = m_resourceGroups.indexOf( group );
    Q_ASSERT( i != -1 );
    if ( i == -1 ) {
        return 0;
    }
    emit resourceGroupToBeRemoved( group );
    ResourceGroup *g = m_resourceGroups.takeAt( i );
    Q_ASSERT( group == g );
    g->setProject( 0 );
    removeResourceGroupId( g->id() );
    foreach ( Resource *r, g->resources() ) {
        r->setProject( 0 );
        removeResourceId( r->id() );
    }
    emit resourceGroupRemoved( g );
    return g;
}

QList<ResourceGroup*> &Project::resourceGroups()
{
    return m_resourceGroups;
}

void Project::addResource( ResourceGroup *group, Resource *resource, int index )
{
    int i = index == -1 ? group->numResources() : index;
    emit resourceToBeAdded( group, i );
    group->addResource( i, resource, 0 );
    setResourceId( resource );
    emit resourceAdded( resource );
}

Resource *Project::takeResource( ResourceGroup *group, Resource *resource )
{
    emit resourceToBeRemoved( resource );
    Q_ASSERT( removeResourceId( resource->id() ) == true );
    Resource *r = group->takeResource( resource );
    Q_ASSERT( resource == r );
    emit resourceRemoved( resource );
    return r;
}


bool Project::addTask( Node* task, Node* position )
{
    // we want to add a task at the given position. => the new node will
    // become next sibling right after position.
    if ( 0 == position ) {
        return addSubTask( task, this );
    }
    //kDebug()<<"Add"<<task->name()<<" after"<<position->name();
    // in case we want to add to the main project, we make it child element
    // of the root element.
    if ( Node::Type_Project == position->type() ) {
        return addSubTask( task, position );
    }
    // find the position
    // we have to tell the parent that we want to delete one of its children
    Node* parentNode = position->parentNode();
    if ( !parentNode ) {
        kDebug() <<"parent node not found???";
        return false;
    }
    int index = parentNode->findChildNode( position );
    if ( -1 == index ) {
        // ok, it does not exist
        kDebug() <<"Task not found???";
        return false;
    }
    return addSubTask( task, index + 1, parentNode );
}

bool Project::addSubTask( Node* task, Node* parent )
{
    // append task to parent
    return addSubTask( task, -1, parent );
}

bool Project::addSubTask( Node* task, int index, Node* parent )
{
    // we want to add a subtask to the node "parent" at the given index.
    // If parent is 0, add to this
    Node *p = parent;
    if ( 0 == p ) {
        p = this;
    }
    if ( !registerNodeId( task ) ) {
        kError() << "Failed to register node id, can not add subtask: " << task->name() << endl;
        return false;
    }
    int i = index == -1 ? p->numChildren() : index;
    emit nodeToBeAdded( p, i );
    p->insertChildNode( i, task );
    emit nodeAdded( task );
    return true;
}

void Project::takeTask( Node *node )
{
    Node * parent = node->parentNode();
    if ( parent == 0 ) {
        kDebug() <<"Node must have a parent!";
        return;
    }
    removeId( node->id() );
    emit nodeToBeRemoved( node );
    parent->takeChildNode( node );
    emit nodeRemoved( node );
}

bool Project::canMoveTask( Node* node, Node *newParent )
{
    //kDebug()<<node->name()<<" to"<<newParent->name();
    Node *p = newParent;
    while ( p && p != this ) {
        if ( ! node->canMoveTo( p ) ) {
            return false;
        }
        p = p->parentNode();
    }
    return true;
}

bool Project::moveTask( Node* node, Node *newParent, int newPos )
{
    //kDebug()<<node->name()<<" to"<<newParent->name()<<","<<newPos;
    if ( ! canMoveTask( node, newParent ) ) {
        return false;
    }
    const Node *before = newParent->childNode( newPos );
    takeTask( node );
    int i = before == 0 ? newParent->numChildren() : newPos;
    addSubTask( node, i, newParent );
    return true;
}

bool Project::canIndentTask( Node* node )
{
    if ( 0 == node ) {
        // should always be != 0. At least we would get the Project,
        // but you never know who might change that, so better be careful
        return false;
    }
    if ( node->type() == Node::Type_Project ) {
        //kDebug()<<"The root node cannot be indented";
        return false;
    }
    // we have to find the parent of task to manipulate its list of children
    Node* parentNode = node->parentNode();
    if ( !parentNode ) {
        return false;
    }
    if ( parentNode->findChildNode( node ) == -1 ) {
        kError() << "Tasknot found???" << endl;
        return false;
    }
    Node *sib = node->siblingBefore();
    if ( !sib ) {
        //kDebug()<<"new parent node not found";
        return false;
    }
    if ( node->findParentRelation( sib ) || node->findChildRelation( sib ) ) {
        //kDebug()<<"Cannot have relations to parent";
        return false;
    }
    return true;
}

bool Project::indentTask( Node* node, int index )
{
    if ( canIndentTask( node ) ) {
        Node * newParent = node->siblingBefore();
        int i = index == -1 ? newParent->numChildren() : index;
        moveTask( node, newParent, i );
        //kDebug();
        return true;
    }
    return false;
}

bool Project::canUnindentTask( Node* node )
{
    if ( 0 == node ) {
        // is always != 0. At least we would get the Project, but you
        // never know who might change that, so better be careful
        return false;
    }
    if ( Node::Type_Project == node->type() ) {
        //kDebug()<<"The root node cannot be unindented";
        return false;
    }
    // we have to find the parent of task to manipulate its list of children
    // and we need the parent's parent too
    Node* parentNode = node->parentNode();
    if ( !parentNode ) {
        return false;
    }
    Node* grandParentNode = parentNode->parentNode();
    if ( !grandParentNode ) {
        //kDebug()<<"This node already is at the top level";
        return false;
    }
    int index = parentNode->findChildNode( node );
    if ( -1 == index ) {
        kError() << "Tasknot found???" << endl;
        return false;
    }
    return true;
}

bool Project::unindentTask( Node* node )
{
    if ( canUnindentTask( node ) ) {
        Node * parentNode = node->parentNode();
        Node *grandParentNode = parentNode->parentNode();
        int i = grandParentNode->indexOf( parentNode ) + 1;
        if ( i == 0 )  {
            i = grandParentNode->numChildren();
        }
        moveTask( node, grandParentNode, i );
        //kDebug();
        return true;
    }
    return false;
}

bool Project::canMoveTaskUp( Node* node )
{
    if ( node == 0 )
        return false; // safety
    // we have to find the parent of task to manipulate its list of children
    Node* parentNode = node->parentNode();
    if ( !parentNode ) {
        //kDebug()<<"No parent found";
        return false;
    }
    if ( parentNode->findChildNode( node ) == -1 ) {
        kError() << "Tasknot found???" << endl;
        return false;
    }
    if ( node->siblingBefore() ) {
        return true;
    }
    return false;
}

bool Project::moveTaskUp( Node* node )
{
    if ( canMoveTaskUp( node ) ) {
        moveTask( node, node->parentNode(), node->parentNode()->indexOf( node ) - 1 );
        return true;
    }
    return false;
}

bool Project::canMoveTaskDown( Node* node )
{
    if ( node == 0 )
        return false; // safety
    // we have to find the parent of task to manipulate its list of children
    Node* parentNode = node->parentNode();
    if ( !parentNode ) {
        return false;
    }
    if ( parentNode->findChildNode( node ) == -1 ) {
        kError() << "Tasknot found???" << endl;
        return false;
    }
    if ( node->siblingAfter() ) {
        return true;
    }
    return false;
}

bool Project::moveTaskDown( Node* node )
{
    if ( canMoveTaskDown( node ) ) {
        moveTask( node, node->parentNode(), node->parentNode()->indexOf( node ) + 1 );
        return true;
    }
    return false;
}

Task *Project::createTask( Node* parent )
{
    Task * node = new Task( parent );
    node->setId( uniqueNodeId() );
    return node;
}

Task *Project::createTask( Task &def, Node* parent )
{
    Task * node = new Task( def, parent );
    node->setId( uniqueNodeId() );
    return node;
}

QString Project::uniqueNodeId( int seed )
{
    int i = seed;
    while ( findNode( QString( "%1" ).arg( i ) ) ) {
        ++i;
    }
    return QString( "%1" ).arg( i );
}

bool Project::removeId( const QString &id )
{
    //kDebug() <<"id=" << id;
    return ( m_parent ? m_parent->removeId( id ) : nodeIdDict.remove( id ) );
}

void Project::insertId( const QString &id, Node *node )
{
    //kDebug() <<"id=" << id <<"" << node->name();
    if ( m_parent == 0 )
        return ( void ) nodeIdDict.insert( id, node );
    m_parent->insertId( id, node );
}

bool Project::registerNodeId( Node *node )
{
    if ( node->id().isEmpty() ) {
        kError() << "Id is empty." << endl;
        return false;
    }
    Node *rn = findNode( node->id() );
    if ( rn == 0 ) {
        insertId( node->id(), node );
        return true;
    }
    if ( rn != node ) {
        kError() << "Id already exists for different task: " << node->id() << endl;
        return false;
    }
    return true;
}

QList<Node*> Project::allNodes()
{
    QList<Node*> lst = nodeIdDict.values();
    int me = lst.indexOf( this );
    if ( me != -1 ) {
        lst.removeAt( me );
    }
    return lst;
}
    
bool Project::setResourceGroupId( ResourceGroup *group )
{
    if ( group == 0 ) {
        return false;
    }
    if ( ! group->id().isEmpty() ) {
        ResourceGroup *g = findResourceGroup( group->id() );
        if ( group == g ) {
            return true;
        } else if ( g == 0 ) {
            insertResourceGroupId( group->id(), group );
            return true;;
        }
    }
    QString id = uniqueResourceGroupId();
    group->setId( id );
    if ( id.isEmpty() ) {
        return false;
    }
    insertResourceGroupId( id, group );
    return true;
}

QString Project::uniqueResourceGroupId() const {
    QString id;
    for (int i=0; i<32000 ; ++i) {
        id = id.setNum(i);
        if ( ! resourceGroupIdDict.contains( id ) ) {
            return id;
        }
    }
    return QString();
}

ResourceGroup *Project::group( const QString& id )
{
    return findResourceGroup( id );
}

ResourceGroup *Project::groupByName( const QString& name ) const
{
    foreach ( QString k, resourceGroupIdDict.keys() ) {
        ResourceGroup *g = resourceGroupIdDict[ k ];
        if ( g->name() == name ) {
            Q_ASSERT( k == g->id() );
            return g;
        }
    }
    return 0;
}

void Project::insertResourceId( const QString &id, Resource *resource )
{
    resourceIdDict.insert( id, resource );
}

bool Project::removeResourceId( const QString &id )
{
    return resourceIdDict.remove( id );
}

bool Project::setResourceId( Resource *resource )
{
    if ( resource == 0 ) {
        return false;
    }
    if ( ! resource->id().isEmpty() ) {
        Resource *r = findResource( resource->id() );
        if ( resource == r ) {
            return true;
        } else if ( r == 0 ) {
            insertResourceId( resource->id(), resource );
            return true;;
        }
    }
    QString id = uniqueResourceId();
    resource->setId( id );
    if ( id.isEmpty() ) {
        return false;
    }
    insertResourceId( id, resource );
    return true;
}

QString Project::uniqueResourceId() const {
    QString id;
    for (int i=0; i<32000 ; ++i) {
        id = id.setNum(i);
        if ( ! resourceIdDict.contains( id ) ) {
            return id;
        }
    }
    return QString();
}

Resource *Project::resource( const QString& id )
{
    return findResource( id );
}

Resource *Project::resourceByName( const QString& name ) const
{
    foreach ( QString k, resourceIdDict.keys() ) {
        Resource *r = resourceIdDict[ k ];
        if ( r->name() == name ) {
            Q_ASSERT( k == r->id() );
            return r;
        }
    }
    return 0;
}

QStringList Project::resourceNameList() const
{
    QStringList lst;
    foreach ( Resource *r, resourceIdDict.values() ) {
        lst << r->name();
    }
    return lst;
}

// TODO
EffortCostMap Project::plannedEffortCostPrDay( const QDate & /*start*/, const QDate & /*end*/, long id ) const
{
    //kDebug();
    EffortCostMap ec;
    return ec;

}

// Returns the total planned effort for this project (or subproject)
Duration Project::plannedEffort( long id ) const
{
    //kDebug();
    Duration eff;
    QListIterator<Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        eff += it.next() ->plannedEffort(id);
    }
    return eff;
}

// Returns the total planned effort for this project (or subproject) on date
Duration Project::plannedEffort( const QDate &date, long id ) const
{
    //kDebug();
    Duration eff;
    QListIterator<Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        eff += it.next() ->plannedEffort( date, id );
    }
    return eff;
}

// Returns the total planned effort for this project (or subproject) upto and including date
Duration Project::plannedEffortTo( const QDate &date, long id ) const
{
    //kDebug();
    Duration eff;
    QListIterator<Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        eff += it.next() ->plannedEffortTo( date, id );
    }
    return eff;
}

// Returns the total actual effort for this project (or subproject)
Duration Project::actualEffort( long id ) const
{
    //kDebug();
    Duration eff;
    QListIterator<Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        eff += it.next() ->actualEffort(id);
    }
    return eff;
}

// Returns the total actual effort for this project (or subproject) on date
Duration Project::actualEffort( const QDate &date, long id ) const
{
    //kDebug();
    Duration eff;
    QListIterator<Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        eff += it.next() ->actualEffort( date, id );
    }
    return eff;
}

// Returns the total actual effort for this project (or subproject) upto and including date
Duration Project::actualEffortTo( const QDate &date, long id ) const
{
    //kDebug();
    Duration eff;
    QListIterator
    <Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        eff += it.next() ->actualEffortTo( date, id );
    }
    return eff;
}

double Project::plannedCost( long id ) const
{
    //kDebug();
    double c = 0;
    QListIterator
    <Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        c += it.next() ->plannedCost(id);
    }
    return c;
}

// Returns the total planned effort for this project (or subproject) on date
double Project::plannedCost( const QDate &date, long id ) const
{
    //kDebug();
    double c = 0;
    QListIterator
    <Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        c += it.next() ->plannedCost( date, id );
    }
    return c;
}

// Returns the total planned effort for this project (or subproject) upto and including date
double Project::plannedCostTo( const QDate &date, long id ) const
{
    //kDebug();
    double c = 0;
    QListIterator
    <Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        c += it.next() ->plannedCostTo( date, id );
    }
    return c;
}

double Project::actualCost( long id ) const
{
    //kDebug();
    double c = 0;
    QListIterator
    <Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        c += it.next() ->actualCost(id);
    }
    return c;
}

// Returns the total planned effort for this project (or subproject) on date
double Project::actualCost( const QDate &date, long id ) const
{
    //kDebug();
    double c = 0;
    QListIterator
    <Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        c += it.next() ->actualCost( date, id );
    }
    return c;
}

// Returns the total planned effort for this project (or subproject) upto and including date
double Project::actualCostTo( const QDate &date, long id ) const
{
    //kDebug();
    double c = 0;
    QListIterator
    <Node*> it( childNodeIterator() );
    while ( it.hasNext() ) {
        c += it.next() ->actualCostTo( date, id );
    }
    return c;
}

double Project::bcwp( long id ) const
{
    //kDebug();
    double c = 0;
    foreach (Node *n, childNodeIterator()) {
        c += n->bcwp(id);
    }
    return c;
}

double Project::bcwp( const QDate &date, long id ) const
{
    //kDebug();
    double c = 0;
    foreach (Node *n, childNodeIterator()) {
        c += n->bcwp( date, id );
    }
    return c;
}


void Project::addCalendar( Calendar *calendar, Calendar *parent )
{
    Q_ASSERT( calendar != 0 );
    //kDebug()<<calendar->name()<<","<<(parent?parent->name():"No parent");
    int row = parent == 0 ? m_calendars.count() : parent->calendars().count();
    emit calendarToBeAdded( parent, row );
    if ( parent == 0 ) {
        calendar->setParentCal( 0 ); // in case
        m_calendars.append( calendar );
    } else {
        calendar->setParentCal( parent );
    }
    if ( calendar->isDefault() ) {
        setDefaultCalendar( calendar );
    }
    setCalendarId( calendar );
    emit calendarAdded( calendar );
}

void Project::takeCalendar( Calendar *calendar )
{
    emit calendarToBeRemoved( calendar );
    removeCalendarId( calendar->id() );
    if ( calendar == m_defaultCalendar ) {
        m_defaultCalendar = 0;
    }
    if ( calendar->parentCal() == 0 ) {
        int i = indexOf( calendar );
        if ( i != -1 ) {
            m_calendars.removeAt( i );
        }
    } else {
        calendar->setParentCal( 0 );
    }
    emit calendarRemoved( calendar );
}

int Project::indexOf( const Calendar *calendar ) const
{
    return m_calendars.indexOf( const_cast<Calendar*>(calendar) );
}

Calendar *Project::calendar( const QString& id ) const
{
    return findCalendar( id );
}

Calendar *Project::calendarByName( const QString& name ) const
{
    foreach( Calendar *c, calendarIdDict.values() ) {
        if ( c->name() == name ) {
            return c;
        }
    }
    return 0;
}

const QList<Calendar*> &Project::calendars() const
{
    return m_calendars;
}

QList<Calendar*> Project::allCalendars() const
{
    return calendarIdDict.values();
}

QStringList Project::calendarNames() const
{
    QStringList lst;
    foreach( Calendar *c, calendarIdDict.values() ) {
        lst << c->name();
    }
    return lst;
}

bool Project::setCalendarId( Calendar *calendar )
{
    if ( calendar == 0 ) {
        return false;
    }
    if ( ! calendar->id().isEmpty() ) {
        Calendar *c = findCalendar( calendar->id() );
        if ( calendar == c ) {
            return true;
        } else if ( c == 0 ) {
            insertCalendarId( calendar->id(), calendar );
            return true;;
        }
    }
    QString id = uniqueCalendarId();
    calendar->setId( id );
    if ( id.isEmpty() ) {
        return false;
    }
    insertCalendarId( id, calendar );
    return true;
}

QString Project::uniqueCalendarId() const {
    QString id;
    for (int i=1; i<32000 ; ++i) {
        id = id.setNum(i);
        if ( ! calendarIdDict.contains( id ) ) {
            return id;
        }
    }
    return QString();
}

void Project::setDefaultCalendar( Calendar *cal )
{
    if ( m_defaultCalendar ) {
        m_defaultCalendar->setDefault( false );
    }
    m_defaultCalendar = cal;
    if ( cal ) {
        cal->setDefault( true );
    }
    emit defaultCalendarChanged( cal );
}

void Project::setStandardWorktime( StandardWorktime * worktime )
{
    if ( m_standardWorktime != worktime ) {
        delete m_standardWorktime;
        m_standardWorktime = worktime;
    }
}

bool Project::legalToLink( Node *par, Node *child )
{
    //kDebug()<<par.name()<<" ("<<par.numDependParentNodes()<<" parents)"<<child.name()<<" ("<<child.numDependChildNodes()<<" children)";

    if ( par == 0 || child == 0 || par == child || par->isDependChildOf( child ) ) {
        return false;
    }
    bool legal = true;
    // see if par/child is related
    if ( par->isParentOf( child ) || child->isParentOf( par ) ) {
        legal = false;
    }
    if ( legal )
        legal = legalChildren( par, child );
    if ( legal )
        legal = legalParents( par, child );

    return legal;
}

bool Project::legalParents( Node *par, Node *child )
{
    bool legal = true;
    //kDebug()<<par->name()<<" ("<<par->numDependParentNodes()<<" parents)"<<child->name()<<" ("<<child->numDependChildNodes()<<" children)";
    for ( int i = 0; i < par->numDependParentNodes() && legal; ++i ) {
        Node *pNode = par->getDependParentNode( i ) ->parent();
        if ( child->isParentOf( pNode ) || pNode->isParentOf( child ) ) {
            //kDebug()<<"Found:"<<pNode->name()<<" is related to"<<child->name();
            legal = false;
        } else {
            legal = legalChildren( pNode, child );
        }
        if ( legal )
            legal = legalParents( pNode, child );
    }
    return legal;
}

bool Project::legalChildren( Node *par, Node *child )
{
    bool legal = true;
    //kDebug()<<par->name()<<" ("<<par->numDependParentNodes()<<" parents)"<<child->name()<<" ("<<child->numDependChildNodes()<<" children)";
    for ( int j = 0; j < child->numDependChildNodes() && legal; ++j ) {
        Node *cNode = child->getDependChildNode( j ) ->child();
        if ( par->isParentOf( cNode ) || cNode->isParentOf( par ) ) {
            //kDebug()<<"Found:"<<par->name()<<" is related to"<<cNode->name();
            legal = false;
        } else {
            legal = legalChildren( par, cNode );
        }
    }
    return legal;
}

void Project::generateWBS( int count, WBSDefinition &def, const QString& wbs )
{
    if ( type() == Type_Subproject || def.level0Enabled() ) {
        Node::generateWBS( count, def, wbs );
    } else {
        QListIterator<Node*> it = m_nodes;
        int i = 0;
        while ( it.hasNext() ) {
            it.next() ->generateWBS( ++i, def, m_wbs );
        }
    }
}

void Project::setCurrentSchedule( long id )
{
    //kDebug();
    setCurrentSchedulePtr( findSchedule( id ) );
    Node::setCurrentSchedule( id );
    QHash<QString, Resource*> hash = resourceIdDict;
    foreach ( Resource * r, hash ) {
        r->setCurrentSchedule( id );
    }
    emit currentScheduleChanged();
}

ScheduleManager *Project::findScheduleManager( const QString &name ) const
{
    //kDebug();
    ScheduleManager *m = 0;
    foreach( ScheduleManager *sm, m_managers ) {
        m = sm->findManager( name );
        if ( m ) {
            break;
        }
    }
    return m;
}

QList<ScheduleManager*> Project::allScheduleManagers() const
{
    QList<ScheduleManager*> lst;
    lst << m_managers;
    foreach ( ScheduleManager *sm, m_managers ) {
        lst << sm->allChildren();
    }
    return lst;
}

QString Project::uniqueScheduleName() const {
    //kDebug();
    QString n = i18n( "Plan" );
    bool unique = findScheduleManager( n ) == 0;
    if ( unique ) {
        return n;
    }
    n += " %1";
    int i = 1;
    for ( ; true; ++i ) {
        unique = findScheduleManager( n.arg( i ) ) == 0;;
        if ( unique ) {
            break;
        }
    }
    return n.arg( i );
}

void Project::addScheduleManager( ScheduleManager *sm, ScheduleManager *parent )
{
    if ( parent == 0 ) {
        emit scheduleManagerToBeAdded( parent, m_managers.count() );
        m_managers.append( sm );
    } else {
        emit scheduleManagerToBeAdded( parent, parent->children().count() );
        sm->setParentManager( parent );
    }
    emit scheduleManagerAdded( sm );
    //kDebug()<<"Added:"<<sm->name()<<", now"<<m_managers.count();
}

int Project::takeScheduleManager( ScheduleManager *sm )
{
    int index = -1;
    if ( sm->parentManager() ) {
        int index = sm->parentManager()->indexOf( sm );
        if ( index >= 0 ) {
            emit scheduleManagerToBeRemoved( sm );
            sm->setParentManager( 0 );
            emit scheduleManagerRemoved( sm );
        }
    } else {
        index = indexOf( sm );
        if ( index >= 0 ) {
            emit scheduleManagerToBeRemoved( sm );
            m_managers.removeAt( indexOf( sm ) );
            emit scheduleManagerRemoved( sm );
        }
    }
    return index;
}

bool Project::isScheduleManager( void *ptr ) const
{
    const ScheduleManager *sm = static_cast<ScheduleManager*>( ptr );
    if ( indexOf( sm ) >= 0 ) {
        return true;
    }
    foreach ( ScheduleManager *p, m_managers ) {
        if ( p->isParentOf( sm ) ) {
            return true;
        }
    }
    return false;
}

ScheduleManager *Project::createScheduleManager( const QString name )
{
    //kDebug()<<name;
    ScheduleManager *sm = new ScheduleManager( *this, name );
    return sm;
}

ScheduleManager *Project::createScheduleManager()
{
    //kDebug();
    return createScheduleManager( uniqueScheduleName() );
}

MainSchedule *Project::createSchedule( const QString& name, Schedule::Type type )
{
    //kDebug()<<"No of schedules:"<<m_schedules.count();
    MainSchedule *sch = new MainSchedule();
    sch->setName( name );
    sch->setType( type );
    addMainSchedule( sch );
    return sch;
}

void Project::addMainSchedule( MainSchedule *sch )
{
    if ( sch == 0 ) {
        return;
    }
    //kDebug()<<"No of schedules:"<<m_schedules.count();
    long i = 1;
    while ( m_schedules.contains( i ) ) {
        ++i;
    }
    sch->setId( i );
    sch->setNode( this );
    addSchedule( sch );
}

bool Project::removeCalendarId( const QString &id )
{
    //kDebug() <<"id=" << id;
    return calendarIdDict.remove( id );
}

void Project::insertCalendarId( const QString &id, Calendar *calendar )
{
    //kDebug() <<"id=" << id <<":" << calendar->name();
    calendarIdDict.insert( id, calendar );
}

void Project::changed( Node *node )
{
    if ( m_parent == 0 ) {
        emit nodeChanged( node );
        return;
    }
    Node::changed( node );
}

void Project::changed( ResourceGroup *group )
{
    //kDebug();
    emit resourceGroupChanged( group );
}

void Project::changed( ScheduleManager *sm )
{
    emit scheduleManagerChanged( sm );
}

void Project::changed( MainSchedule *sch )
{
    //kDebug()<<sch->id();
    emit scheduleChanged( sch );
}

void Project::sendScheduleToBeAdded( const ScheduleManager *sm, int row )
{
    emit scheduleToBeAdded( sm, row );
}

void Project::sendScheduleAdded( const MainSchedule *sch )
{
    //kDebug()<<sch->id();
    emit scheduleAdded( sch );
}

void Project::sendScheduleToBeRemoved( const MainSchedule *sch )
{
    emit scheduleToBeRemoved( sch );
}

void Project::sendScheduleRemoved( const MainSchedule *sch )
{
    //kDebug()<<sch->id();
    emit scheduleRemoved( sch );
}

void Project::changed( Resource *resource )
{
    emit resourceChanged( resource );
}

void Project::changed( Calendar *cal )
{
    emit calendarChanged( cal );
}

bool Project::addRelation( Relation *rel, bool check )
{
    if ( rel->parent() == 0 || rel->child() == 0 ) {
        return false;
    }
    if ( check && !legalToLink( rel->parent(), rel->child() ) ) {
        return false;
    }
    emit relationToBeAdded( rel, rel->parent()->numDependChildNodes(), rel->child()->numDependParentNodes() );
    rel->parent()->addDependChildNode( rel );
    rel->child()->addDependParentNode( rel );
    emit relationAdded( rel );
    return true;
}

void Project::takeRelation( Relation *rel )
{
    emit relationToBeRemoved( rel );
    rel->parent() ->takeDependChildNode( rel );
    rel->child() ->takeDependParentNode( rel );
    emit relationRemoved( rel );
}

void Project::setRelationType( Relation *rel, Relation::Type type )
{
    rel->setType( type );
    emit relationModified( rel );
}

void Project::setRelationLag( Relation *rel, const Duration &lag )
{
    rel->setLag( lag );
    emit relationModified( rel );
}

#ifndef NDEBUG
void Project::printDebug( bool children, const QByteArray& _indent )
{
    QByteArray indent = _indent;
    kDebug() << indent <<"+ Project node:" << Node::name(); //FIXME: QT3 support
    indent += '!';
    QListIterator<ResourceGroup*> it( resourceGroups() );
    while ( it.hasNext() )
        it.next() ->printDebug( indent );

    Node::printDebug( children, indent );
}
void Project::printCalendarDebug( const QByteArray& _indent )
{
    QByteArray indent = _indent;
    kDebug() << indent <<"-------- Calendars debug printout --------";
    foreach ( Calendar *c, calendarIdDict.values() ) {
        c->printDebug( indent + "--" );
        kDebug();
    }
    if ( m_standardWorktime )
        m_standardWorktime->printDebug();
}
#endif
//use in pert to have the project slack
void Project::setProjectSlack(const int& theValue)
{
    m_projectSlack = theValue;
}

int Project::projectSlack() const
{
    return m_projectSlack;
}



}  //KPlato namespace

#include "kptproject.moc"
