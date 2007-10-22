/* This file is part of the KDE project
  Copyright (C) 2007 Dag Andersen <danders@get2net.dk>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
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

#ifndef TASKSTATUSVIEW_H
#define TASKSTATUSVIEW_H

#include "kptitemmodelbase.h"
#include "kptnodeitemmodel.h"
#include "kptviewbase.h"


class KAction;

namespace KPlato
{

class Project;
class Node;
class Task;

typedef QList<Node*> NodeList;

class TaskStatusItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit TaskStatusItemModel( Part *part, QObject *parent = 0 );
    ~TaskStatusItemModel();
    
    virtual void setProject( Project *project );
    void setManager( ScheduleManager *sm );
    
    virtual Qt::ItemFlags flags( const QModelIndex & index ) const;
    
    virtual QModelIndex parent( const QModelIndex & index ) const;
    virtual bool hasChildren( const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index( const Node *node ) const;
    virtual QModelIndex index( const NodeList *lst ) const;
    
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const; 
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const; 
    
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const; 
    virtual bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    
    virtual QMimeData * mimeData( const QModelIndexList & indexes ) const;
    virtual QStringList mimeTypes () const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );

    NodeList *list( const QModelIndex &index ) const;
    Node *node( const QModelIndex &index ) const;
    QItemDelegate *createDelegate( int column, QWidget *parent ) const;
    
    NodeList nodeList( QDataStream &stream );
    static NodeList removeChildNodes( const NodeList nodes );
    bool dropAllowed( Node *on, const QMimeData *data );
    
    void clear();
    void refresh();
    
    void setNow( const QDate &date ) { m_nodemodel.setNow( date ); }
    void setPeriod( int days ) { m_period = days; }
    
protected slots:
    void slotAboutToBeReset();
    void slotReset();

    void slotNodeChanged( Node* );
    void slotNodeToBeInserted( Node *node, int row );
    void slotNodeInserted( Node *node );
    void slotNodeToBeRemoved( Node *node );
    void slotNodeRemoved( Node *node );

protected:
    QVariant alignment( int column ) const;
    
    QVariant name( int row, int role ) const;

private:
    NodeModel m_nodemodel;
    QStringList m_topNames;
    QList<NodeList*> m_top;
    NodeList m_notstarted;
    NodeList m_running;
    NodeList m_finished;
    NodeList m_upcomming;
    
    long m_id; // schedule id
    int m_period;
};


class TaskStatusTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    TaskStatusTreeView( Part *part, QWidget *parent );
    
    //void setSelectionModel( QItemSelectionModel *selectionModel );

    TaskStatusItemModel *itemModel() const { return static_cast<TaskStatusItemModel*>( model() ); }
    
    Project *project() const { return itemModel()->project(); }
    void setProject( Project *project ) { itemModel()->setProject( project ); }
    
protected slots:
    void slotActivated( const QModelIndex index );
    
protected:
    void dragMoveEvent(QDragMoveEvent *event);

};


class TaskStatusView : public ViewBase
{
    Q_OBJECT
public:
    TaskStatusView( Part *part, QWidget *parent );
    
    void setupGui();
    virtual void draw( Project &project );
    virtual void draw();

    virtual void updateReadWrite( bool readwrite );
    virtual Node *currentNode() const;
    
    /// Loads context info into this view. Reimplement.
    virtual bool loadContext( const KoXmlElement &/*context*/ );
    /// Save context info from this view. Reimplement.
    virtual void saveContext( QDomElement &/*context*/ ) const;

signals:
    void requestPopupMenu( const QString&, const QPoint & );
    void openNode();

public slots:
    /// Activate/deactivate the gui
    virtual void setGuiActive( bool activate );

    void slotCurrentScheduleManagerChanged( ScheduleManager *sm );

protected:
    void updateActionsEnabled( bool on );

private slots:
    void slotContextMenuRequested( const QModelIndex &index, const QPoint& pos );
    void slotContextMenuRequested( Node *node, const QPoint& pos );
    void slotHeaderContextMenuRequested( const QPoint& );
    void slotOptions();
    
private:
    Project *m_project;
    int m_id;
    TaskStatusTreeView *m_view;

    // View options context menu
    KAction *actionOptions;
};


} //namespace KPlato


#endif
