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

#include <klocale.h>
#include <kdebug.h>

namespace KPlato
{

Node::Node(Node *parent) : m_nodes(), m_dependChildNodes(), m_dependParentNodes() {
    //kDebug()<<k_funcinfo<<"("<<this<<")"<<endl;
    m_parent = parent;
    init();
    m_id = QString(); // Not mapped
}

Node::Node(Node &node, Node *parent) 
    : QObject( 0 ), // Don't set parent, we handle parent/child ourselves
      m_nodes(), 
      m_dependChildNodes(), 
      m_dependParentNodes() {
    //kDebug()<<k_funcinfo<<"("<<this<<")"<<endl;
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
    //kDebug()<<k_funcinfo<<"("<<this<<") "<<m_name<<endl;
    while (!m_nodes.isEmpty())
        delete m_nodes.takeFirst();
    
    if (findNode() == this) {
        removeId(); // only remove myself (I may be just a working copy)
    }
    Relation *rel = 0;
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
    m_effort = 0;
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
        case Type_Project: return trans ? i18n("") : "Project";
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

    kError()<<k_funcinfo<<"Ooops, no parent and no project found"<<endl;
    return 0;
}

void Node::takeChildNode( Node *node) {
    //kDebug()<<k_funcinfo<<"find="<<m_nodes.indexOf(node)<<endl;
    int i = m_nodes.indexOf(node);
    if ( i != -1 ) {
        m_nodes.removeAt(i);
    }
    node->setParent(0);
}

void Node::takeChildNode( int number ) {
    if (number >= 0 && number < m_nodes.size()) {
        Node *n = m_nodes.takeAt(number);
        //kDebug()<<k_funcinfo<<(n?n->id():"null")<<" : "<<(n?n->name():"")<<endl;
        if (n) {
            n->setParent(0);
        }
    }
}

void Node::insertChildNode( unsigned int index, Node *node) {
    if (index == -1)
        m_nodes.append(node);
    else
        m_nodes.insert(index,node);
    node->setParent(this);
}

void Node::addChildNode( Node *node, Node *after) {
    int index = m_nodes.indexOf(after);
    if (index == -1) {
        m_nodes.append(node);
        node->setParent(this);
        return;
    }
    m_nodes.insert(index+1, node);
    node->setParent(this);
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


const Node* Node::childNode(int number) const {
    return m_nodes.at(number);
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
        //kDebug()<<k_funcinfo<<m_name<<": ("<<rel<<")"<<endl;
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
        //kDebug()<<k_funcinfo<<m_name<<": ("<<rel<<")"<<endl;
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
    //kDebug()<<k_funcinfo<<" '"<<m_name<<"' checking against '"<<node->name()<<"'"<<endl;
    for (int i=0; i<numDependParentNodes(); i++) {
        Relation *rel = getDependParentNode(i);
        if (rel->parent() == node)
            return true;
		if (rel->parent()->isDependChildOf(node))
		    return true;
    }
	return false;
}

Duration Node::duration(const DateTime &time, int use, bool backward) {
    //kDebug()<<k_funcinfo<<endl;
    // TODO: handle risc
    if (!time.isValid()) {
        kError()<<k_funcinfo<<"Time is invalid"<<endl;
        return Duration::zeroDuration;
    }
    if (m_effort == 0) {
        kError()<<k_funcinfo<<"m_effort == 0"<<endl;
        return Duration::zeroDuration;
    }
    if (m_currentSchedule == 0) {
        return Duration::zeroDuration;
        kError()<<k_funcinfo<<"No current schedule"<<endl;
    }
    //kDebug()<<k_funcinfo<<m_name<<": Use="<<use<<endl;
    return calcDuration(time, m_effort->effort(use, m_currentSchedule->usePert()), backward);
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
const Duration& Node::duration( long id )
{
    Schedule *s = m_currentSchedule;
    if ( id != -1 ) {
        s = findSchedule( id );
    }
    return s ? s->duration : Duration::zeroDuration;
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
    m_currentSchedule->earliestStart = time;
    //kDebug()<<k_funcinfo<<m_name<<": "<<m_currentSchedule->earliestStart.toString()<<endl;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->propagateEarliestStart(time);
    }
}

void Node::propagateLatestFinish(DateTime &time) {
    if (m_currentSchedule == 0)
        return;
    m_currentSchedule->latestFinish = time;
    //kDebug()<<k_funcinfo<<m_name<<": "<<m_currentSchedule->latestFinish<<endl;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->propagateLatestFinish(time);
    }
}

void Node::moveEarliestStart(DateTime &time) {
    if (m_currentSchedule == 0)
        return;
    if (m_currentSchedule->earliestStart < time)
        m_currentSchedule->earliestStart = time;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->moveEarliestStart(time);
    }
}

void Node::moveLatestFinish(DateTime &time) {
    if (m_currentSchedule == 0)
        return;
    if (m_currentSchedule->latestFinish > time)
        m_currentSchedule->latestFinish = time;
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
    //kDebug()<<k_funcinfo<<endl;
    if (getParent())
        return getParent()->childBefore(this);
    return 0;
}

Node *Node::childBefore(Node *node) {
    //kDebug()<<k_funcinfo<<endl;
    int index = m_nodes.indexOf(node);
    if (index > 0){
        return m_nodes.at(index-1);
    }
    return 0;
}

Node *Node::siblingAfter() {
    //kDebug()<<k_funcinfo<<endl;
    if (getParent())
        return getParent()->childAfter(this);
    return 0;
}

Node *Node::childAfter(Node *node)
{
    //kDebug()<<k_funcinfo<<endl;
    uint index = m_nodes.indexOf(node);
    if (index < m_nodes.count()-1) {
        return m_nodes.at(index+1);    }
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
    //kDebug()<<k_funcinfo<<id<<endl;
    if (id.isEmpty()) {
        kError()<<k_funcinfo<<"id is empty"<<endl;
        m_id = id;
        return false;
    }
    if (!m_id.isEmpty()) {
        Node *n = findNode();
        if (n == this) {
            //kDebug()<<k_funcinfo<<"My id found, remove it"<<endl;
            removeId();
        } else if (n) {
            //Hmmm, shouldn't happen
            kError()<<k_funcinfo<<"My id '"<<m_id<<"' already used for different node: "<<n->name()<<endl;
        }
    }
    if (findNode(id)) {
        kError()<<k_funcinfo<<"id '"<<id<<"' is already used for different node: "<<findNode(id)->name()<<endl;
        m_id = QString(); // hmmm
        return false;
    }
    m_id = id;
    insertId(id);
    //kDebug()<<k_funcinfo<<m_name<<": inserted id="<<id<<endl;
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
    //kDebug()<<k_funcinfo<<m_name<<" id="<<id<<endl;
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
    //kDebug()<<k_funcinfo<<this<<": "<<appointment<<", "<<s<<", "<<s->id()<<", "<<main.id()<<endl;
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
    //kDebug()<<k_funcinfo<<name<<" type="<<type<<" id="<<(int)id<<endl;
    NodeSchedule *sch = new NodeSchedule(this, name, type, id);
    addSchedule(sch);
    return sch;
}

Schedule *Node::createSchedule(Schedule *parent) {
    //kDebug()<<k_funcinfo<<name<<" type="<<type<<" id="<<(int)id<<endl;
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
    //kDebug()<<k_funcinfo<<m_name<<" find type="<<type<<" nr="<<m_schedules.count()<<endl;
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
        kError()<<k_funcinfo<<m_name<<" Could not find schedule with id="<<id<<endl;
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
    //kDebug()<<k_funcinfo<<m_name<<endl;
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

int Node::level() {
    Node *n = getParent();
    return n ? n->level() + 1 : 0;
}

void Node::generateWBS(int count, WBSDefinition &def, const QString& wbs) {
    m_wbs = wbs + def.code(count, level());
    //kDebug()<<k_funcinfo<<m_name<<" wbs: "<<m_wbs<<endl;
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
    //kDebug()<<k_funcinfo<<m_name<<" id: "<<id<<"="<<m_currentSchedule<<endl;
}

void Node::changed(Node *node) {
    if (m_parent)
        m_parent->changed(this);
}


//////////////////////////   Effort   /////////////////////////////////

Effort::Effort( Duration e, Duration p, Duration o) {
  m_expectedEffort = e;
  m_pessimisticEffort = p;
  m_optimisticEffort = o;
  m_type = Type_Effort;
  m_risktype = Risk_None;
}

Effort::Effort(const Effort &effort) {
    set(effort.expected(), effort.pessimistic(), effort.optimistic());
    setType(effort.type());
    setRisktype(effort.risktype());
}

Effort::~Effort() {
}

const Effort Effort::zeroEffort( Duration::zeroDuration,
                       Duration::zeroDuration,
                       Duration::zeroDuration );

void Effort::set( Duration e, Duration p, Duration o ) {
    m_expectedEffort = e;
    m_pessimisticEffort = (p == Duration::zeroDuration) ? e :  p;
    m_optimisticEffort = (o == Duration::zeroDuration) ? e :  o;
    //kDebug()<<k_funcinfo<<"   Expected: "<<m_expectedEffort.toString()<<endl;
}

void Effort::set( int e, int p, int o ) {
    m_expectedEffort = Duration(e);
    m_pessimisticEffort = (p < 0) ? Duration(e) :  Duration(p);
    m_optimisticEffort = (o < 0) ? Duration(e) :  Duration(o);
    //kDebug()<<k_funcinfo<<"   Expected: "<<m_expectedEffort.toString()<<endl;
    //kDebug()<<k_funcinfo<<"   Optimistic: "<<m_optimisticEffort.toString()<<endl;
    //kDebug()<<k_funcinfo<<"   Pessimistic: "<<m_pessimisticEffort.toString()<<endl;

    //kDebug()<<k_funcinfo<<"   Expected: "<<m_expectedEffort.duration()<<" manseconds"<<endl;
}

//TODO (?): effort is not really a duration, should maybe not use Duration for storage
void Effort::set(unsigned days, unsigned hours, unsigned minutes) {
    Duration dur(days, hours, minutes);
    set(dur);
    //kDebug()<<k_funcinfo<<"effort="<<dur.toString()<<endl;
}

void Effort::expectedEffort(unsigned *days, unsigned *hours, unsigned *minutes) {
    m_expectedEffort.get(days, hours, minutes);
}

Duration Effort::variance() const {
    return (m_pessimisticEffort - m_optimisticEffort)/6;
}
Duration Effort::pertExpected() const {
    if (m_risktype == Risk_Low) {
        return (m_optimisticEffort + m_pessimisticEffort + (m_expectedEffort*4))/6;
    } else if (m_risktype == Risk_High) {
        return (m_optimisticEffort + (m_pessimisticEffort*2) + (m_expectedEffort*4))/7;
    }
    return m_expectedEffort; // risk==none
}
Duration Effort::pertOptimistic() const {
    if (m_risktype != Risk_None) {
        return pertExpected() - variance();
    }
    return m_optimisticEffort;
}
Duration Effort::pertPessimistic() const {
    if (m_risktype != Risk_None) {
        return pertExpected() + variance();
    }
    return m_pessimisticEffort;
}

Duration Effort::effort(int valueType, bool pert) const {
    if (valueType == Effort::Use_Expected) {
        return pert ? pertExpected() : m_expectedEffort;
    } else if (valueType == Effort::Use_Optimistic) {
        return pert ? pertOptimistic() : m_optimisticEffort;
    } else if (valueType == Effort::Use_Pessimistic)
        return pert ? pertPessimistic() : m_pessimisticEffort;
    
    return m_expectedEffort; // default
}

bool Effort::load(QDomElement &element) {
    m_expectedEffort = Duration::fromString(element.attribute("expected"));
    m_optimisticEffort = Duration::fromString(element.attribute("optimistic"));
    m_pessimisticEffort = Duration::fromString(element.attribute("pessimistic"));
    setType(element.attribute("type", "WorkBased"));
    setRisktype(element.attribute("risk"));
    return true;
}

void Effort::save(QDomElement &element) const {
    QDomElement me = element.ownerDocument().createElement("effort");
    element.appendChild(me);
    me.setAttribute("expected", m_expectedEffort.toString());
    me.setAttribute("optimistic", m_optimisticEffort.toString());
    me.setAttribute("pessimistic", m_pessimisticEffort.toString());
    me.setAttribute("type", typeToString());
    me.setAttribute("risk", risktypeToString());
}

QString Effort::typeToString( bool trans ) const {
    return typeToStringList( trans ).at( m_type );
}

QStringList Effort::typeToStringList( bool trans ) {
    return QStringList() 
            << (trans ? i18n("Effort") : QString("Effort"))
            << (trans ? i18n("FixedDuration") : QString("FixedDuration"));
}

void Effort::setType(const QString& type) {
    if (type == "Effort")
        setType(Type_Effort);
    else if (type == "FixedDuration")
        setType(Type_FixedDuration);
    else if (type == "Type_FixedDuration") // Typo, keep old xml files working
        setType(Type_FixedDuration);
    else
        setType(Type_Effort); // default
}

QString Effort::risktypeToString( bool trans ) const {
    return risktypeToStringList( trans ).at( m_risktype );
}

QStringList Effort::risktypeToStringList( bool trans ) {
    return QStringList() 
            << (trans ? i18n("None") : QString("None"))
            << (trans ? i18n("Low") : QString("Low"))
            << (trans ? i18n("High") : QString("High"));
}

void Effort::setRisktype(const QString& type) {
    if (type == "High")
        setRisktype(Risk_High);
    else if (type == "Low")
        setRisktype(Risk_Low);
    else
        setRisktype(Risk_None); // default
}

void Effort::setOptimisticRatio(int percent)
{
    int p = percent>0 ? -percent : percent;
    m_optimisticEffort = m_expectedEffort*(100+p)/100;
}

int Effort::optimisticRatio() const {
    if (m_expectedEffort == Duration::zeroDuration)
        return 0;
    return (m_optimisticEffort.milliseconds()*100/m_expectedEffort.milliseconds())-100;
}

void Effort::setPessimisticRatio(int percent) 
{
    int p = percent<0 ? -percent : percent;
    m_pessimisticEffort = m_expectedEffort*(100+p)/100;
}
int Effort::pessimisticRatio() const {
    if (m_expectedEffort == Duration::zeroDuration)
        return 0;
    return m_pessimisticEffort.milliseconds()*100/m_expectedEffort.milliseconds()-100;
}

// Debugging
#ifndef NDEBUG
void Node::printDebug(bool children, const QByteArray& _indent) {
    QByteArray indent = _indent;
    kDebug()<<indent<<"  Unique node identity="<<m_id<<endl;
    if (m_effort) m_effort->printDebug(indent);
    QString s = "  Constraint: " + constraintToString();
    if (m_constraint == MustStartOn || m_constraint == StartNotEarlier || m_constraint == FixedInterval)
        kDebug()<<indent<<s<<" ("<<constraintStartTime().toString()<<")"<<endl;
    if (m_constraint == MustFinishOn || m_constraint == FinishNotLater || m_constraint == FixedInterval)
        kDebug()<<indent<<s<<" ("<<constraintEndTime().toString()<<")"<<endl;
    Schedule *cs = m_currentSchedule; 
    if (cs) {
        kDebug()<<indent<<"  Current schedule: "<<"id="<<cs->id()<<" '"<<cs->name()<<"' type: "<<cs->type()<<endl;
    } else {
        kDebug()<<indent<<"  Current schedule: None"<<endl;
    }
    foreach (Schedule *sch, m_schedules.values()) {
        sch->printDebug(indent+"  ");
    }
    kDebug()<<indent<<"  Parent: "<<(m_parent ? m_parent->name() : QString("None"))<<endl;
    kDebug()<<indent<<"  Level: "<<level()<<endl;
    kDebug()<<indent<<"  No of predecessors: "<<m_dependParentNodes.count()<<endl;
    QListIterator<Relation*> pit(m_dependParentNodes);
    //kDebug()<<indent<<"  Dependent parents="<<pit.count()<<endl;
    while (pit.hasNext()) {
        pit.next()->printDebug(indent);
    }
    kDebug()<<indent<<"  No of successors: "<<m_dependChildNodes.count()<<endl;
    QListIterator<Relation*> cit(m_dependChildNodes);
    //kDebug()<<indent<<"  Dependent children="<<cit.count()<<endl;
    while (cit.hasNext()) {
        cit.next()->printDebug(indent);
    }

    //kDebug()<<indent<<endl;
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
void Effort::printDebug(const QByteArray& _indent) {
    QByteArray indent = _indent;
    kDebug()<<indent<<"  Effort:"<<endl;
    indent += "  ";
    kDebug()<<indent<<"  Expected:    "<<m_expectedEffort.toString()<<endl;
    kDebug()<<indent<<"  Optimistic:  "<<m_optimisticEffort.toString()<<endl;
    kDebug()<<indent<<"  Pessimistic: "<<m_pessimisticEffort.toString()<<endl;
    
    kDebug()<<indent<<"  Risk: "<<risktypeToString()<<endl;
    kDebug()<<indent<<"  Pert expected:    "<<pertExpected().toString()<<endl;
    kDebug()<<indent<<"  Pert optimistic:  "<<pertOptimistic().toString()<<endl;
    kDebug()<<indent<<"  Pert pessimistic: "<<pertPessimistic().toString()<<endl;
    kDebug()<<indent<<"  Pert variance:    "<<variance().toString()<<endl;
}
#endif

}  //KPlato namespace

#include "kptnode.moc"
