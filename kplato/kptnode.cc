/* This file is part of the KDE project
   Copyright (C) 2001 Thomas zander <zander@kde.org>
   Copyright (C) 200 - 2007 Dag Andersen <danders@get2net.dk>

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
#include "kptnode.h"

#include "kptappointment.h"
#include "kptaccount.h"
#include "kptwbsdefinition.h"
#include "kptresource.h"
#include "kptschedule.h"

#include <QList>
#include <QListIterator>
#include <qdom.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>

namespace KPlato
{

Node::Node(Node *parent) 
    : QObject( 0 ), // We don't use qobjects parent
      m_nodes(), m_dependChildNodes(), m_dependParentNodes(),
      m_estimate( 0 )
{
    //kDebug()<<"("<<this<<")";
    m_parent = parent;
    init();
    m_id = QString(); // Not mapped
}

Node::Node(Node &node, Node *parent) 
    : QObject( 0 ), // Don't set parent, we handle parent/child ourselves
      m_nodes(), 
      m_dependChildNodes(), 
      m_dependParentNodes() {
    //kDebug()<<"("<<this<<")";
    m_parent = parent;
    init();
    m_name = node.name();
    m_leader = node.leader();
    m_description = node.description();
    m_constraint = (ConstraintType) node.constraint();
    m_constraintStartTime = node.constraintStartTime();
    m_constraintEndTime = node.constraintEndTime();
    
    m_runningAccount = node.runningAccount();
    m_startupAccount = node.startupAccount();
    m_shutdownAccount = node.shutdownAccount();

    m_startupCost = node.startupCost();
    m_shutdownCost = node.shutdownCost();
    
}

Node::~Node() {
    //kDebug()<<"("<<this<<")"<<m_name;
    delete m_estimate;
    while (!m_nodes.isEmpty())
        delete m_nodes.takeFirst();
    
    if (findNode() == this) {
        removeId(); // only remove myself (I may be just a working copy)
    }
    while (!m_dependParentNodes.isEmpty()) {
        delete m_dependParentNodes.takeFirst();
    }
    while (!m_dependChildNodes.isEmpty()) {
        delete m_dependChildNodes.takeFirst();
    }
    if (m_runningAccount)
        m_runningAccount->removeRunning(*this);
    if (m_startupAccount)
        m_startupAccount->removeStartup(*this);
    if (m_shutdownAccount)
        m_shutdownAccount->removeShutdown(*this);

    foreach (long key, m_schedules.keys()) {
        delete m_schedules.take(key);
    }
    m_parent = 0; //safety
}

void Node::init() {
    m_currentSchedule = 0;
    m_name="";
    m_constraint = Node::ASAP;
    m_estimate = 0;
    m_visitedForward = false;
    m_visitedBackward = false;
    
    m_runningAccount = 0;
    m_startupAccount = 0;
    m_shutdownAccount = 0;
    m_startupCost = 0.0;
    m_shutdownCost = 0.0;
}

QString Node::typeToString( bool trans ) const {
    switch ( type() ) {
        case Type_Node: return trans ? i18n("Node") : "Node";
        case Type_Project: return trans ? i18n("Project") : "Project";
        case Type_Subproject: return trans ? i18n("Sub-Project") : "Sub-Project";
        case Type_Task: return trans ? i18n("Task") : "Task";
        case Type_Milestone: return trans ? i18n("Milestone") : "Milestone";
        case Type_Periodic: return trans ? i18n("Periodic") : "Periodic";
        case Type_Summarytask: return trans ? i18n("Summary") : "Summary-Task";
    }
    return QString();
}

void Node::setName(const QString &n) 
{
     m_name = n;
     changed(this);
}

void Node::setLeader(const QString &l)
{
     m_leader = l;
     changed(this);
}

void Node::setDescription(const QString &d)
{
     m_description = d;
     changed(this);
}

Node *Node::projectNode() {
    if ((type() == Type_Project) || (type() == Type_Subproject)) {
        return this;
    }
    if (m_parent)
        return m_parent->projectNode();

    kError()<<"Ooops, no parent and no project found"<<endl;
    return 0;
}

void Node::takeChildNode( Node *node) {
    //kDebug()<<"find="<<m_nodes.indexOf(node);
    int t = type();
    int i = m_nodes.indexOf(node);
    if ( i != -1 ) {
        m_nodes.removeAt(i);
    }
    node->setParentNode(0);
    if ( t != type() ) {
        changed();
    }
}

void Node::takeChildNode( int number ) {
    int t = type();
    if (number >= 0 && number < m_nodes.size()) {
        Node *n = m_nodes.takeAt(number);
        //kDebug()<<(n?n->id():"null")<<" :"<<(n?n->name():"");
        if (n) {
            n->setParentNode( 0 );
        }
    }
    if ( t != type() ) {
        changed();
    }
}

void Node::insertChildNode( int index, Node *node ) {
    int t = type();
    if (index == -1)
        m_nodes.append(node);
    else
        m_nodes.insert(index,node);
    node->setParentNode( this );
    if ( t != type() ) {
        changed();
    }
}

void Node::addChildNode( Node *node, Node *after) {
    int t = type();
    int index = m_nodes.indexOf(after);
    if (index == -1) {
        m_nodes.append(node);
        node->setParentNode( this );
        if ( t != type() ) {
            changed();
        }
        return;
    }
    m_nodes.insert(index+1, node);
    node->setParentNode(this);
    if ( t != type() ) {
        changed();
    }
}

int Node::findChildNode( Node* node )
{
    return m_nodes.indexOf( node );
}

bool Node::isChildOf( Node* node )
{
    if ( node == 0 || m_parent == 0 ) {
        return false;
    }
    if ( node == m_parent ) {
        return true;
    }
    return m_parent->isChildOf( node );
}


Node* Node::childNode(int number)
{
    //kDebug()<<number;
    return m_nodes.value( number );
}

const Node* Node::childNode(int number) const
{
    if ( number < 0 || number >= m_nodes.count() ) {
        return 0;
    }
    return m_nodes.at( number );
}

int Node::indexOf( const Node *node ) const
{
    return m_nodes.indexOf( const_cast<Node*>(node) );
}


Duration *Node::getDelay() {
    /* TODO
       Calculate the delay of this node. Use the calculated startTime and the set startTime.
    */
    return 0L;
}

void Node::addDependChildNode( Node *node, Relation::Type p) {
    addDependChildNode(node,p,Duration());
}

void Node::addDependChildNode( Node *node, Relation::Type p, Duration lag) {
    Relation *relation = new Relation(this, node, p, lag);
    if (node->addDependParentNode(relation))
        m_dependChildNodes.append(relation);
    else
        delete relation;
}

void Node::insertDependChildNode( unsigned int index, Node *node, Relation::Type p) {
    Relation *relation = new Relation(this, node, p, Duration());
    if (node->addDependParentNode(relation))
        m_dependChildNodes.insert(index, relation);
    else
        delete relation;
}

bool Node::addDependChildNode( Relation *relation) {
    if(m_dependChildNodes.indexOf(relation) != -1)
        return false;
    m_dependChildNodes.append(relation);
    return true;
}

void Node::takeDependChildNode( Relation *rel ) {
    int i = m_dependChildNodes.indexOf(rel);
    if ( i != -1 ) {
        //kDebug()<<m_name<<": ("<<rel<<")";
        m_dependChildNodes.removeAt(i);
    }
}

void Node::addDependParentNode( Node *node, Relation::Type p) {
    addDependParentNode(node,p,Duration());
}

void Node::addDependParentNode( Node *node, Relation::Type p, Duration lag) {
    Relation *relation = new Relation(node, this, p, lag);
    if (node->addDependChildNode(relation))
        m_dependParentNodes.append(relation);
    else
        delete relation;
}

void Node::insertDependParentNode( unsigned int index, Node *node, Relation::Type p) {
    Relation *relation = new Relation(this, node, p, Duration());
    if (node->addDependChildNode(relation))
        m_dependParentNodes.insert(index,relation);
    else
        delete relation;
}

bool Node::addDependParentNode( Relation *relation) {
    if(m_dependParentNodes.indexOf(relation) != -1)
        return false;
    m_dependParentNodes.append(relation);
    return true;
}

void Node::takeDependParentNode( Relation *rel ) {
    int i = m_dependParentNodes.indexOf(rel);
    if ( i != -1 ) {
        //kDebug()<<m_name<<": ("<<rel<<")";
        m_dependParentNodes.removeAt(i);
    }
}

bool Node::isParentOf(Node *node) {
    if (m_nodes.indexOf(node) != -1)
        return true;

    QListIterator<Node*> nit(childNodeIterator());
    while (nit.hasNext()) {
        if (nit.next()->isParentOf(node))
            return true;
    }
    return false;
}

Relation *Node::findParentRelation(Node *node) {
    for (int i=0; i<numDependParentNodes(); i++) {
        Relation *rel = getDependParentNode(i);
        if (rel->parent() == node)
            return rel;
    }
    return (Relation *)0;
}

Relation *Node::findChildRelation(Node *node) {
    for (int i=0; i<numDependChildNodes(); i++) {
        Relation *rel = getDependChildNode(i);
        if (rel->child() == node)
            return rel;
    }
    return (Relation *)0;
}

Relation *Node::findRelation(Node *node) {
    Relation *rel = findParentRelation(node);
    if (!rel)
        rel = findChildRelation(node);
    return rel;
}

bool Node::isDependChildOf(Node *node) {
    //kDebug()<<" '"<<m_name<<"' checking against '"<<node->name()<<"'";
    for (int i=0; i<numDependParentNodes(); i++) {
        Relation *rel = getDependParentNode(i);
        if (rel->parent() == node)
            return true;
		if (rel->parent()->isDependChildOf(node))
		    return true;
    }
	return false;
}

 QList<Node*> Node::getParentNodes()
{
    this->m_parentNodes.clear();
    foreach(Relation * currentRelation, this->dependParentNodes())
    {
        if (!this->m_parentNodes.contains(currentRelation->parent())) 
        {
            this->m_parentNodes.append(currentRelation->parent());
        }
    }
    return this->m_parentNodes;
}

bool Node::canMoveTo( Node *newParent )
{
    if ( m_parent == newParent ) {
        return true;
    }
    if ( newParent->isChildOf( this ) ) {
        return false;
    }
    if ( isDependChildOf( newParent ) || newParent->isDependChildOf( this ) ) {
        kDebug()<<"Can't move, node is dependent on new parent";
        return false;
    }
    foreach ( Node *n, m_nodes ) {
        if ( !n->canMoveTo( newParent ) ) {
            return false;
        }
    }
    return true;
}

void Node::makeAppointments() {
    QListIterator<Node*> nit(m_nodes);
    while (nit.hasNext()) {
        nit.next()->makeAppointments();
    }
}

void Node::calcResourceOverbooked() {
    QListIterator<Node*> nit(m_nodes);
    while (nit.hasNext()) {
        nit.next()->calcResourceOverbooked();
    }
}

// Returns the (previously) calculated duration
const Duration& Node::duration( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->duration : Duration::zeroDuration;
}

double Node::variance( long id, Duration::Unit unit ) const
{
    double d = deviation( id, unit );
    return d * d;
}

double Node::deviation( long id, Duration::Unit unit ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    double d = 0.0;
    if ( s && m_estimate ) {
        d = s->duration.toDouble( unit );
        double o = ( d *  ( 100 + m_estimate->optimisticRatio() ) ) / 100;
        double p = ( d * ( 100 + m_estimate->pessimisticRatio() ) ) / 100;
        d =  ( p - o ) / 6;
    }
    return d;
}

DateTime Node::startTime( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->startTime : DateTime();
}
DateTime Node::endTime( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->endTime : DateTime();
}
    
void Node::setEarlyStart(const DateTime &dt, long id )
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    if ( s ) s->earlyStart = dt;
}

DateTime Node::earlyStart( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->earlyStart : DateTime();
}

void Node::setLateStart(const DateTime &dt, long id )
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    if ( s ) s->lateStart = dt;
}

DateTime Node::lateStart( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->lateStart : DateTime();
}
    
void Node::setEarlyFinish(const DateTime &dt, long id )
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    if ( s ) s->earlyFinish = dt;
}

DateTime Node::earlyFinish( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->earlyFinish : DateTime();
}

void Node::setLateFinish(const DateTime &dt, long id )
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    if ( s ) s->lateFinish = dt;
}

DateTime Node::lateFinish( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->lateFinish : DateTime();
}
    
DateTime Node::workStartTime( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->workStartTime : DateTime();
}

void Node::setWorkStartTime(const DateTime &dt, long id )
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }

     if ( s ) s->workStartTime = dt;
}

DateTime Node::workEndTime( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->workEndTime : DateTime();
}

void Node::setWorkEndTime(const DateTime &dt, long id )
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    if ( s ) s->workEndTime = dt;
}
    
bool Node::inCriticalPath( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->inCriticalPath : false;
}

bool Node::resourceError( long id ) const 
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->resourceError : false;
}
    
bool Node::resourceOverbooked( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->resourceOverbooked : false;
}
    
bool Node::resourceNotAvailable( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->resourceNotAvailable : false;
}
    
bool Node::schedulingError( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->schedulingError : false;
}

bool Node::notScheduled( long id ) const
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s == 0 || s->isDeleted() || s->notScheduled;
}

QStringList Node::overbookedResources( long id ) const {
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->overbookedResources() : QStringList();
}

void Node::saveRelations(QDomElement &element) const {
    QListIterator<Relation*> it(m_dependChildNodes);
    while (it.hasNext()) {
        it.next()->save(element);
    }
    QListIterator<Node*> nodes(m_nodes);
    while (nodes.hasNext()) {
        nodes.next()->saveRelations(element);
    }
}

void Node::setConstraint(Node::ConstraintType type)
{ 
    m_constraint = type;
    changed( this );
}

void Node::setConstraint(QString &type) {
    // Do not i18n these, they are used in load()
    if (type == "ASAP")
        setConstraint(ASAP);
    else if (type == "ALAP")
        setConstraint(ALAP);
    else if (type == "MustStartOn")
        setConstraint(MustStartOn);
    else if (type == "MustFinishOn")
        setConstraint(MustFinishOn);
    else if (type == "StartNotEarlier")
        setConstraint(StartNotEarlier);
    else if (type == "FinishNotLater")
        setConstraint(FinishNotLater);
    else if (type == "FixedInterval")
        setConstraint(FixedInterval);
    else
        setConstraint(ASAP);  // default
}

QString Node::constraintToString( bool trans ) const {
    return constraintList( trans ).at( m_constraint );
}

QStringList Node::constraintList( bool trans ) {
    // keep theses in the same order as the enum!
    return QStringList() 
            << (trans ? i18n("As Soon As Possible") : QString("ASAP"))
            << (trans ? i18n("As Late As Possible") : QString("ALAP"))
            << (trans ? i18n("Must Start On") : QString("MustStartOn"))
            << (trans ? i18n("Must Finish On") : QString("MustFinishOn"))
            << (trans ? i18n("Start Not Earlier") : QString("StartNotEarlier"))
            << (trans ? i18n("Finish Not Later") : QString("FinishNotLater"))
            << (trans ? i18n("Fixed Interval") : QString("FixedInterval"));
}

void Node::propagateEarliestStart(DateTime &time) {
    if (m_currentSchedule == 0)
        return;
    m_currentSchedule->earlyStart = time;
    //kDebug()<<m_name<<":"<<m_currentSchedule->earlyStart.toString();
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->propagateEarliestStart(time);
    }
}

void Node::propagateLatestFinish(DateTime &time) {
    if (m_currentSchedule == 0)
        return;
    m_currentSchedule->lateFinish = time;
    //kDebug()<<m_name<<":"<<m_currentSchedule->lateFinish;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->propagateLatestFinish(time);
    }
}

void Node::moveEarliestStart(DateTime &time) {
    if (m_currentSchedule == 0)
        return;
    if (m_currentSchedule->earlyStart < time)
        m_currentSchedule->earlyStart = time;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->moveEarliestStart(time);
    }
}

void Node::moveLatestFinish(DateTime &time) {
    if (m_currentSchedule == 0)
        return;
    if (m_currentSchedule->lateFinish > time)
        m_currentSchedule->lateFinish = time;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->moveLatestFinish(time);
    }
}

void Node::initiateCalculation(MainSchedule &sch) {
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->initiateCalculation(sch);
    }
}

void Node::resetVisited() {
    m_visitedForward = false;
    m_visitedBackward = false;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->resetVisited();
    }
}

Node *Node::siblingBefore() {
    //kDebug();
    if (parentNode())
        return parentNode()->childBefore(this);
    return 0;
}

Node *Node::childBefore(Node *node) {
    //kDebug();
    int index = m_nodes.indexOf(node);
    if (index > 0){
        return m_nodes.at(index-1);
    }
    return 0;
}

Node *Node::siblingAfter() {
    //kDebug();
    if (parentNode())
        return parentNode()->childAfter(this);
    return 0;
}

Node *Node::childAfter(Node *node)
{
    //kDebug();
    int index = m_nodes.indexOf(node);
    if (index < m_nodes.count()-1) {
        return m_nodes.at(index+1);
    }
    return 0;
}

bool Node::moveChildUp(Node* node)
{
    if (findChildNode(node) == -1)
        return false; // not my node!
    Node *sib = node->siblingBefore();
    if (!sib)
        return false;
    sib = sib->siblingBefore();
    takeChildNode(node);
    if (sib) {
        addChildNode(node, sib);
    } else {
        insertChildNode(0, node);
    }        
    return true;
}

bool Node::moveChildDown(Node* node)
{
    if (findChildNode(node) == -1)
        return false; // not my node!
    Node *sib = node->siblingAfter();
    if (!sib)
        return false;
    takeChildNode(node);
    addChildNode(node, sib);
    return true;
}

bool Node::legalToLink(Node *node) {
    Node *p = projectNode();
    if (p)
        return p->legalToLink(this, node);
    return false;
}

bool Node::isEndNode() const {
    return m_dependChildNodes.isEmpty();
}
bool Node::isStartNode() const {
    return m_dependParentNodes.isEmpty();
}

bool Node::setId(const QString& id) {
    //kDebug()<<id;
    if (id.isEmpty()) {
        kError()<<"id is empty"<<endl;
        m_id = id;
        return false;
    }
    if (!m_id.isEmpty()) {
        Node *n = findNode();
        if (n == this) {
            //kDebug()<<"My id found, remove it";
            removeId();
        } else if (n) {
            //Hmmm, shouldn't happen
            kError()<<"My id '"<<m_id<<"' already used for different node: "<<n->name()<<endl;
        }
    }
    if (findNode(id)) {
        kError()<<"id '"<<id<<"' is already used for different node: "<<findNode(id)->name()<<endl;
        m_id = QString(); // hmmm
        return false;
    }
    m_id = id;
    insertId(id);
    //kDebug()<<m_name<<": inserted id="<<id;
    return true;
}

void Node::setStartTime(DateTime startTime, long id ) { 
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    if ( s )
        s->startTime = startTime;
}

void Node::setEndTime(DateTime endTime, long id ) { 
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    if ( s )
        s->endTime = endTime;
}

void Node::saveAppointments(QDomElement &element, long id) const {
    //kDebug()<<m_name<<" id="<<id;
    QListIterator<Node*> it(m_nodes);
    while (it.hasNext()) {
        it.next()->saveAppointments(element, id);
    }
}

QList<Appointment*> Node::appointments( long id ) {
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    QList<Appointment*> lst;
    if ( s )
        lst = s->appointments();
    return lst;
}

// Appointment *Node::findAppointment(Resource *resource) {
//     if (m_currentSchedule)
//         return m_currentSchedule->findAppointment(resource);
//     return 0;
// }
// bool Node::addAppointment(Appointment *appointment) {
//     if ( m_currentSchedule )
//         return m_currentSchedule->add(appointment);
//     return false;
// }
// 
// called from resource side when resource adds appointment
bool Node::addAppointment(Appointment *appointment, Schedule &main) {
    Schedule *s = findSchedule(main.id());
    if (s == 0) {
        s = createSchedule(&main);
    }
    appointment->setNode(s);
    //kDebug()<<this<<":"<<appointment<<","<<s<<","<<s->id()<<","<<main.id();
    return s->add(appointment);
}

void Node::addAppointment(ResourceSchedule *resource, DateTime &start, DateTime &end, double load) {
    Schedule *node = findSchedule(resource->id());
    if (node == 0) {
        node = createSchedule(resource->parent());
    }
    node->setCalculationMode( resource->calculationMode() );
    node->addAppointment(resource, start, end, load);
}

void Node::takeSchedule(const Schedule *schedule) {
    if (schedule == 0)
        return;
    if (m_currentSchedule == schedule)
        m_currentSchedule = 0;
    m_schedules.take(schedule->id());
}

void Node::addSchedule(Schedule *schedule) {
    if (schedule == 0)
        return;
    m_schedules.insert(schedule->id(), schedule);
}

Schedule *Node::createSchedule(const QString& name, Schedule::Type type, long id) {
    //kDebug()<<name<<" type="<<type<<" id="<<(int)id;
    NodeSchedule *sch = new NodeSchedule(this, name, type, id);
    addSchedule(sch);
    return sch;
}

Schedule *Node::createSchedule(Schedule *parent) {
    //kDebug()<<name<<" type="<<type<<" id="<<(int)id;
    NodeSchedule *sch = new NodeSchedule(parent, this);
    addSchedule(sch);
    return sch;
}

Schedule *Node::findSchedule( long id ) const
{
    return m_schedules.value( id );
}

Schedule *Node::findSchedule(const QString name, const Schedule::Type type) {
    QHash<long, Schedule*> it;
    foreach (Schedule *sch, it) {
        if (!sch->isDeleted() && 
            sch->name() == name && sch->type() == type)
            return sch;
    }
    return 0;
}

Schedule *Node::findSchedule(const QString name) {
    QList<Schedule*> it = m_schedules.values();
    foreach (Schedule *sch, it) {
        if (!sch->isDeleted() && sch->name() == name)
            return sch;
    }
    return 0;
}


Schedule *Node::findSchedule(const Schedule::Type type) {
    //kDebug()<<m_name<<" find type="<<type<<" nr="<<m_schedules.count();
    QHash<long, Schedule*> hash;
    foreach (Schedule *sch, hash) {
        if (!sch->isDeleted() && sch->type() == type) {
            return sch;
        }
    }
    return 0;
}

void Node::setScheduleDeleted(long id, bool on) {
    Schedule *ns = findSchedule(id);
    if (ns == 0) {
        kError()<<m_name<<" Could not find schedule with id="<<id<<endl;
    } else {
        ns->setDeleted(on);
    }
}

void Node::setParentSchedule(Schedule *sch) {
    Schedule *s = findSchedule(sch->id());
    if (s) {
        s->setParent(sch);
    }
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->setParentSchedule(sch);
    }
}

bool Node::calcCriticalPath(bool fromEnd) {
    if (m_currentSchedule == 0)
        return false;
    //kDebug()<<m_name;
    if (!isCritical()) {
        return false;
    }
    if (!fromEnd && isStartNode()) {
        m_currentSchedule->inCriticalPath = true;
        return true;
    }
    if (fromEnd && isEndNode()) {
        m_currentSchedule->inCriticalPath = true;
        return true;
    }
    QListIterator<Relation*> pit(m_dependParentNodes);
    while (pit.hasNext()) {
        if (pit.next()->parent()->calcCriticalPath(fromEnd)) {
            m_currentSchedule->inCriticalPath = true;
        }
    }
    return m_currentSchedule->inCriticalPath;
}

void Node::calcFreeFloat() {
    foreach ( Node *n, m_nodes ) {
        n->calcFreeFloat();
    }
    return;
}

int Node::level() {
    Node *n = parentNode();
    return n ? n->level() + 1 : 0;
}

void Node::generateWBS(int count, WBSDefinition &def, const QString& wbs) {
    m_wbs = wbs + def.code(count, level());
    //kDebug()<<m_name<<" wbs:"<<m_wbs;
    QString w = wbs + def.wbs(count, level());
    QListIterator<Node*> it = m_nodes;
    int i=0;
    while (it.hasNext()) {
        it.next()->generateWBS(++i, def, w);
    }

}

void Node::setCurrentSchedule(long id) {
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->setCurrentSchedule(id);
    }
    //kDebug()<<m_name<<" id:"<<id<<"="<<m_currentSchedule;
}

void Node::setStartupCost(double cost)
{
    m_startupCost = cost;
    changed();
}

void Node::setStartupAccount(Account *acc)
{
    //kDebug()<<m_name<<"="<<acc;
    m_startupAccount = acc;
    changed();
}

void Node::setShutdownCost(double cost)
{
    m_shutdownCost = cost;
    changed();
}

void Node::setShutdownAccount(Account *acc)
{
    //kDebug()<<m_name<<"="<<acc;
    m_shutdownAccount = acc;
    changed();
}

void Node::setRunningAccount(Account *acc)
{
    //kDebug()<<m_name<<"="<<acc;
    m_runningAccount = acc;
    changed();
}

void Node::changed(Node *node) {
    if (m_parent)
        m_parent->changed(node);
}


//////////////////////////   Estimate   /////////////////////////////////

Estimate::Estimate( Duration e, Duration p, Duration o) 
{
    m_parent = 0;
    m_expectedEstimate = e;
    m_pessimisticEstimate = p;
    m_optimisticEstimate = o;
    m_type = Type_Effort;
    m_risktype = Risk_None;
    m_displayUnit = Duration::Unit_h;
}

Estimate::Estimate(const Estimate &estimate) 
{
    m_parent = 0; // don't copy
    set(estimate.expected(), estimate.pessimistic(), estimate.optimistic());
    setType(estimate.type());
    setRisktype(estimate.risktype());
    setDisplayUnit( estimate.displayUnit() );
}

Estimate::~Estimate() {
}

void Estimate::set( Duration e, Duration p, Duration o ) {
    m_expectedEstimate = e;
    m_pessimisticEstimate = (p == Duration::zeroDuration) ? e :  p;
    m_optimisticEstimate = (o == Duration::zeroDuration) ? e :  o;
    //kDebug()<<"   Expected:"<<m_expectedEstimate.toString();
    changed();
}

void Estimate::set( int e, int p, int o ) {
    m_expectedEstimate = Duration((qint64)(e)*1000);
    m_pessimisticEstimate = (p < 0) ? Duration((qint64)(e)*1000) :  Duration((qint64)(p)*1000);
    m_optimisticEstimate = (o < 0) ? Duration((qint64)(e)*1000) :  Duration((qint64)(o)*1000);
    //kDebug()<<"   Expected:"<<m_expectedEstimate.toString();
    //kDebug()<<"   Optimistic:"<<m_optimisticEstimate.toString();
    //kDebug()<<"   Pessimistic:"<<m_pessimisticEstimate.toString();

    //kDebug()<<"   Expected:"<<m_expectedEstimate.duration()<<" manseconds";
    changed();
}

void Estimate::set(unsigned days, unsigned hours, unsigned minutes) {
    Duration dur(days, hours, minutes);
    set(dur);
    //kDebug()<<"estimate="<<dur.toString();
    changed();
}

void Estimate::expectedEstimate(unsigned *days, unsigned *hours, unsigned *minutes) {
    m_expectedEstimate.get(days, hours, minutes);
}

double Estimate::variance( Duration::Unit unit ) const {
    double d = deviation( unit );
    return d * d;
}

double Estimate::deviation( Duration::Unit unit ) const {
    double p = m_pessimisticEstimate.toDouble( unit );
    double o = m_optimisticEstimate.toDouble( unit );
    double v = ( p - o ) / 6;
    return v;
}

Duration Estimate::pertExpected() const {
    if (m_risktype == Risk_Low) {
        return (m_optimisticEstimate + m_pessimisticEstimate + (m_expectedEstimate*4))/6;
    } else if (m_risktype == Risk_High) {
        return (m_optimisticEstimate + (m_pessimisticEstimate*2) + (m_expectedEstimate*4))/7;
    }
    return m_expectedEstimate; // risk==none
}
Duration Estimate::pertOptimistic() const {
    if (m_risktype != Risk_None) {
        return pertExpected() - Duration( variance() );
    }
    return m_optimisticEstimate;
}
Duration Estimate::pertPessimistic() const {
    if (m_risktype != Risk_None) {
        return pertExpected() + Duration( variance() );
    }
    return m_pessimisticEstimate;
}

Duration Estimate::value(int valueType, bool pert) const {
    if (valueType == Estimate::Use_Expected) {
        return pert ? pertExpected() : m_expectedEstimate;
    } else if (valueType == Estimate::Use_Optimistic) {
        return pert ? pertOptimistic() : m_optimisticEstimate;
    } else if (valueType == Estimate::Use_Pessimistic)
        return pert ? pertPessimistic() : m_pessimisticEstimate;
    
    return m_expectedEstimate; // default
}

bool Estimate::load(KoXmlElement &element) {
    m_expectedEstimate = Duration::fromString(element.attribute("expected"));
    m_optimisticEstimate = Duration::fromString(element.attribute("optimistic"));
    m_pessimisticEstimate = Duration::fromString(element.attribute("pessimistic"));
    setType(element.attribute("type", "WorkBased"));
    setRisktype(element.attribute("risk"));
    m_displayUnit = (Duration::Unit)(element.attribute("display-unit", QString().number(Duration::Unit_h) ).toInt());
    return true;
}

void Estimate::save(QDomElement &element) const {
    QDomElement me = element.ownerDocument().createElement("estimate");
    element.appendChild(me);
    me.setAttribute("expected", m_expectedEstimate.toString());
    me.setAttribute("optimistic", m_optimisticEstimate.toString());
    me.setAttribute("pessimistic", m_pessimisticEstimate.toString());
    me.setAttribute("type", typeToString());
    me.setAttribute("risk", risktypeToString());
    me.setAttribute("display-unit", m_displayUnit);
}

QString Estimate::typeToString( bool trans ) const {
    return typeToStringList( trans ).at( m_type );
}

QStringList Estimate::typeToStringList( bool trans ) {
    return QStringList() 
            << (trans ? i18n("Effort") : QString("Effort"))
            << (trans ? i18n("FixedDuration") : QString("FixedDuration"));
}

void Estimate::setType(const QString& type) {
    if (type == "Effort")
        setType(Type_Effort);
    else if (type == "FixedDuration")
        setType(Type_FixedDuration);
    else if (type == "Type_FixedDuration") // Typo, keep old xml files working
        setType(Type_FixedDuration);
    else
        setType(Type_Effort); // default
}

QString Estimate::risktypeToString( bool trans ) const {
    return risktypeToStringList( trans ).at( m_risktype );
}

QStringList Estimate::risktypeToStringList( bool trans ) {
    return QStringList() 
            << (trans ? i18n("None") : QString("None"))
            << (trans ? i18n("Low") : QString("Low"))
            << (trans ? i18n("High") : QString("High"));
}

void Estimate::setRisktype(const QString& type) {
    if (type == "High")
        setRisktype(Risk_High);
    else if (type == "Low")
        setRisktype(Risk_Low);
    else
        setRisktype(Risk_None); // default
}

void Estimate::setOptimisticRatio(int percent)
{
    int p = percent>0 ? -percent : percent;
    m_optimisticEstimate = m_expectedEstimate*(100+p)/100;
    changed();
}

int Estimate::optimisticRatio() const {
    if (m_expectedEstimate == Duration::zeroDuration)
        return 0;
    return (m_optimisticEstimate.milliseconds()*100/m_expectedEstimate.milliseconds())-100;
}

void Estimate::setPessimisticRatio(int percent) 
{
    int p = percent<0 ? -percent : percent;
    m_pessimisticEstimate = m_expectedEstimate*(100+p)/100;
    changed();
}
int Estimate::pessimisticRatio() const {
    if (m_expectedEstimate == Duration::zeroDuration)
        return 0;
    return m_pessimisticEstimate.milliseconds()*100/m_expectedEstimate.milliseconds()-100;
}

double Estimate::scale( const Duration &value, Duration::Unit unit, QList<double> scales )
{
    QList<double> lst = scales;
    switch ( lst.count() ) {
        case 0:
            lst << 24.0; // add hours in day
        case 1:
            lst << 60.0; // add minutes in hour
        case 2:
            lst << 60.0; // add seconds in minute
        case 3:
            lst << 1000.0; // add milliseconds in second
        default:
            break;
    }
    double v = (double)value.milliseconds();
    if (unit == Duration::Unit_ms) return v;
    v /= lst[3];
    if (unit == Duration::Unit_s) return v;
    v /= lst[2];
    if (unit == Duration::Unit_m) return v;
    v /= lst[1];
    if (unit == Duration::Unit_h) return v;
    v /= lst[0];
    //kDebug()<<value.toString()<<","<<unit<<"="<<v;
    return v;
}

Duration Estimate::scale( double value, Duration::Unit unit, const QList<double> scales )
{
    QList<double> lst = scales;
    switch ( lst.count() ) {
        case 0:
            lst << 24.0; // add hours in day
        case 1:
            lst << 60.0; // add minutes in hour
        case 2:
            lst << 60.0; // add seconds in minute
        case 3:
            lst << 1000.0; // add milliseconds in second
        default:
            break;
    }
    double v = value;
    switch ( unit ) {
        case Duration::Unit_d:
            v *= lst[0];
        case Duration::Unit_h:
            v *= lst[1];
        case Duration::Unit_m:
            v *= lst[2];
        case Duration::Unit_s:
            v *= lst[3];
        case Duration::Unit_ms:
            break; // nothing
    }
    //kDebug()<<value<<","<<unit<<"="<<v;
    return Duration( v, Duration::Unit_ms );
}


// Debugging
#ifndef NDEBUG
void Node::printDebug(bool children, const QByteArray& _indent) {
    QByteArray indent = _indent;
    kDebug()<<indent<<"  Unique node identity="<<m_id;
    m_estimate->printDebug(indent);
    QString s = "  Constraint: " + constraintToString();
    if (m_constraint == MustStartOn || m_constraint == StartNotEarlier || m_constraint == FixedInterval)
        kDebug()<<indent<<s<<" ("<<constraintStartTime().toString()<<")";
    if (m_constraint == MustFinishOn || m_constraint == FinishNotLater || m_constraint == FixedInterval)
        kDebug()<<indent<<s<<" ("<<constraintEndTime().toString()<<")";
    Schedule *cs = m_currentSchedule; 
    if (cs) {
        kDebug()<<indent<<"  Current schedule:"<<"id="<<cs->id()<<" '"<<cs->name()<<"' type:"<<cs->type();
    } else {
        kDebug()<<indent<<"  Current schedule: None";
    }
    foreach (Schedule *sch, m_schedules.values()) {
        sch->printDebug(indent+"  ");
    }
    kDebug()<<indent<<"  Parent:"<<(m_parent ? m_parent->name() : QString("None"));
    kDebug()<<indent<<"  Level:"<<level();
    kDebug()<<indent<<"  No of predecessors:"<<m_dependParentNodes.count();
    QListIterator<Relation*> pit(m_dependParentNodes);
    //kDebug()<<indent<<"  Dependent parents="<<pit.count();
    while (pit.hasNext()) {
        pit.next()->printDebug(indent);
    }
    kDebug()<<indent<<"  No of successors:"<<m_dependChildNodes.count();
    QListIterator<Relation*> cit(m_dependChildNodes);
    //kDebug()<<indent<<"  Dependent children="<<cit.count();
    while (cit.hasNext()) {
        cit.next()->printDebug(indent);
    }

    //kDebug()<<indent;
    indent += "  ";
    if (children) {
        QListIterator<Node*> it(m_nodes);
        while (it.hasNext()) {
            it.next()->printDebug(true,indent);
        }
    }

}
#endif


#ifndef NDEBUG
void Estimate::printDebug(const QByteArray& _indent) {
    QByteArray indent = _indent;
    kDebug()<<indent<<"  Estimate:";
    indent += "  ";
    kDebug()<<indent<<"  Expected:"<<m_expectedEstimate.toString();
    kDebug()<<indent<<"  Optimistic:"<<m_optimisticEstimate.toString();
    kDebug()<<indent<<"  Pessimistic:"<<m_pessimisticEstimate.toString();
    
    kDebug()<<indent<<"  Risk:"<<risktypeToString();
    kDebug()<<indent<<"  Pert expected:      "<<pertExpected().toString()<<endl;
    kDebug()<<indent<<"  Pert optimistic:    "<<pertOptimistic().toString()<<endl;
    kDebug()<<indent<<"  Pert pessimistic:   "<<pertPessimistic().toString()<<endl;
    kDebug()<<indent<<"  Pert variance:      "<<variance()<<endl;
    kDebug()<<indent<<"  Pert std deviation: "<<deviation()<<endl;
}
#endif

}  //KPlato namespace

#include "kptnode.moc"
