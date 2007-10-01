/* This file is part of the KDE project
  Copyright (C) 2007 Dag Andersen <danders@get2net.dk>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation;
  version 2 of the License, or (at your option) any later version..

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#ifndef NODEITEMMODEL_H
#define NODEITEMMODEL_H

#include "kptitemmodelbase.h"
#include "kptschedule.h"

#include <QDate>
#include <QItemDelegate>

class QMimeData;
class QModelIndex;

namespace KPlato
{

class Project;
class Node;
class Estimate;

class NodeModel : public QObject
{
    Q_OBJECT
public:
    NodeModel()
        : QObject(), 
        m_project( 0 ), 
        m_manager( 0 ), 
        m_now( QDate::currentDate() ),
        m_prec( 1 )
     {}
    ~NodeModel() {}
    
    void setProject( Project *project );
    void setManager( ScheduleManager *sm );
    Project *project() const { return m_project; }
    ScheduleManager *manager() const { return m_manager; }
    long id() const { return m_manager == 0 ? -1 : m_manager->id(); }
    
    QVariant data( const Node *node, int property, int role = Qt::DisplayRole ) const; 
    static bool setData( Node *node, int property, const QVariant & value, int role = Qt::EditRole );
    
    static QVariant headerData( int section, int role = Qt::DisplayRole );

    static int propertyCount();
    
    void setNow( const QDate &now ) { m_now = now; }
    QDate now() const { return m_now; }
    
    QVariant name( const Node *node, int role ) const;
    QVariant leader( const Node *node, int role ) const;
    QVariant allocation( const Node *node, int role ) const;
    QVariant description( const Node *node, int role ) const;
    QVariant type( const Node *node, int role ) const;
    QVariant constraint( const Node *node, int role ) const;
    QVariant constraintStartTime( const Node *node, int role ) const;
    QVariant constraintEndTime( const Node *node, int role ) const;
    QVariant estimateType( const Node *node, int role ) const;
    QVariant estimate( const Node *node, int role ) const;
    QVariant optimisticRatio( const Node *node, int role ) const;
    QVariant pessimisticRatio( const Node *node, int role ) const;
    QVariant riskType( const Node *node, int role ) const;
    QVariant runningAccount( const Node *node, int role ) const;
    QVariant startupAccount( const Node *node, int role ) const;
    QVariant startupCost( const Node *node, int role ) const;
    QVariant shutdownAccount( const Node *node, int role ) const;
    QVariant shutdownCost( const Node *node, int role ) const;
    
    QVariant startTime( const Node *node, int role ) const;
    QVariant endTime( const Node *node, int role ) const;

    QVariant duration( const Node *node, int role ) const;
    QVariant varianceDuration( const Node *node, int role ) const;
    QVariant varianceEstimate( const Estimate *est, int role ) const;
    QVariant optimisticDuration( const Node *node, int role ) const;
    QVariant optimisticEstimate( const Estimate *est, int role ) const;
    QVariant pertExpected( const Estimate *est, int role ) const;
    QVariant pessimisticDuration( const Node *node, int role ) const;
    QVariant pessimisticEstimate( const Estimate *est, int role ) const;

    QVariant earlyStart( const Node *node, int role ) const;
    QVariant earlyFinish( const Node *node, int role ) const;
    QVariant lateStart( const Node *node, int role ) const;
    QVariant lateFinish( const Node *node, int role ) const;
    QVariant positiveFloat( const Node *node, int role ) const;
    QVariant freeFloat( const Node *node, int role ) const;
    QVariant negativeFloat( const Node *node, int role ) const;
    QVariant startFloat( const Node *node, int role ) const;
    QVariant finishFloat( const Node *node, int role ) const;

    QVariant status( const Node *node, int role ) const;
    QVariant completed( const Node *node, int role ) const;
    QVariant startedTime( const Node *node, int role ) const;
    QVariant finishedTime( const Node *node, int role ) const;
    QVariant plannedEffortTo( const Node *node, int role ) const;
    QVariant actualEffortTo( const Node *node, int role ) const;
    QVariant remainingEffort( const Node *node, int role ) const;
    QVariant plannedCostTo( const Node *node, int role ) const;
    QVariant actualCostTo( const Node *node, int role ) const;
    QVariant note( const Node *node, int role ) const;

private:
    Project *m_project;
    ScheduleManager *m_manager;
    QDate m_now;
    int m_prec;
};

class NodeItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit NodeItemModel( Part *part, QObject *parent = 0 );
    ~NodeItemModel();
    
    virtual void setProject( Project *project );
    void setManager( ScheduleManager *sm );
    
    virtual Qt::ItemFlags flags( const QModelIndex & index ) const;
    
    virtual QModelIndex parent( const QModelIndex & index ) const;
    virtual bool hasChildren( const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index( const Node *node ) const;
    
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const; 
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const; 
    
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const; 
    virtual bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    
    virtual QMimeData * mimeData( const QModelIndexList & indexes ) const;
    virtual QStringList mimeTypes () const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );

    Node *node( const QModelIndex &index ) const;
    QItemDelegate *createDelegate( int column, QWidget *parent ) const;

    QModelIndex insertTask( Node *node, Node *after );
    QModelIndex insertSubtask( Node *node, Node *parent );
    
    QList<Node*> nodeList( QDataStream &stream );
    static QList<Node*> removeChildNodes( const QList<Node*> nodes );
    bool dropAllowed( Node *on, const QMimeData *data );
    
    virtual bool dropAllowed( const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data );
    
protected slots:
    void slotNodeChanged( Node* );
    void slotNodeToBeInserted( Node *node, int row );
    void slotNodeInserted( Node *node );
    void slotNodeToBeRemoved( Node *node );
    void slotNodeRemoved( Node *node );

    void slotLayoutChanged();
    
protected:
    bool setName( Node *node, const QVariant &value, int role );
    bool setLeader( Node *node, const QVariant &value, int role );
    QVariant allocation( const Node *node, int role ) const;
    bool setAllocation( Node *node, const QVariant &value, int role );
    bool setDescription( Node *node, const QVariant &value, int role );
    bool setType( Node *node, const QVariant &value, int role );
    bool setConstraint( Node *node, const QVariant &value, int role );
    bool setConstraintStartTime( Node *node, const QVariant &value, int role );
    bool setConstraintEndTime( Node *node, const QVariant &value, int role );
    bool setEstimateType( Node *node, const QVariant &value, int role );
    bool setEstimate( Node *node, const QVariant &value, int role );
    bool setOptimisticRatio( Node *node, const QVariant &value, int role );
    bool setPessimisticRatio( Node *node, const QVariant &value, int role );
    bool setRiskType( Node *node, const QVariant &value, int role );
    bool setRunningAccount( Node *node, const QVariant &value, int role );
    bool setStartupAccount( Node *node, const QVariant &value, int role );
    bool setStartupCost( Node *node, const QVariant &value, int role );
    bool setShutdownAccount( Node *node, const QVariant &value, int role );
    bool setShutdownCost( Node *node, const QVariant &value, int role );
    
private:
    Node *m_node; // for sanety check
    NodeModel m_nodemodel;
};

class MilestoneItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit MilestoneItemModel( Part *part, QObject *parent = 0 );
    ~MilestoneItemModel();

    virtual void setProject( Project *project );
    void setManager( ScheduleManager *sm );

    virtual Qt::ItemFlags flags( const QModelIndex & index ) const;

    virtual QModelIndex parent( const QModelIndex & index ) const;
    virtual bool hasChildren( const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index( const Node *node ) const;

    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const; 
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const; 

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const; 
    virtual bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );


    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    virtual QMimeData * mimeData( const QModelIndexList & indexes ) const;
    virtual QStringList mimeTypes () const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );

    Node *node( const QModelIndex &index ) const;
    QItemDelegate *createDelegate( int column, QWidget *parent ) const;

    QModelIndex insertTask( Node *node, Node *after );
    QModelIndex insertSubtask( Node *node, Node *parent );

    QList<Node*> nodeList( QDataStream &stream );
    static QList<Node*> removeChildNodes( const QList<Node*> nodes );
    bool dropAllowed( Node *on, const QMimeData *data );

    virtual bool dropAllowed( const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data );

protected slots:
    void slotNodeChanged( Node* );
    void slotNodeToBeInserted( Node *node, int row );
    void slotNodeInserted( Node *node );
    void slotNodeToBeRemoved( Node *node );
    void slotNodeRemoved( Node *node );

    void slotLayoutChanged();

protected:
    bool setName( Node *node, const QVariant &value, int role );
    bool setLeader( Node *node, const QVariant &value, int role );
    QVariant allocation( const Node *node, int role ) const;
    bool setAllocation( Node *node, const QVariant &value, int role );
    bool setDescription( Node *node, const QVariant &value, int role );
    bool setType( Node *node, const QVariant &value, int role );
    bool setConstraint( Node *node, const QVariant &value, int role );
    bool setConstraintStartTime( Node *node, const QVariant &value, int role );
    bool setConstraintEndTime( Node *node, const QVariant &value, int role );
    bool setEstimateType( Node *node, const QVariant &value, int role );
    bool setEstimate( Node *node, const QVariant &value, int role );
    bool setOptimisticRatio( Node *node, const QVariant &value, int role );
    bool setPessimisticRatio( Node *node, const QVariant &value, int role );
    bool setRiskType( Node *node, const QVariant &value, int role );
    bool setRunningAccount( Node *node, const QVariant &value, int role );
    bool setStartupAccount( Node *node, const QVariant &value, int role );
    bool setStartupCost( Node *node, const QVariant &value, int role );
    bool setShutdownAccount( Node *node, const QVariant &value, int role );
    bool setShutdownCost( Node *node, const QVariant &value, int role );

protected:
    void resetModel();
    
private:
    NodeModel m_nodemodel;
    QList<Node*> m_mslist;
};

} //namespace KPlato

#endif //NODEITEMMODEL_H
