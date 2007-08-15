/* This file is part of the KDE project
  Copyright (C) 2006-2007 Dag Andersen <kplato@kde.org>
  Copyright (C) 2006-2007 Menard Alexis <kplato@kde.org>
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

#ifndef KPTSCHEDULEEDITOR_H
#define KPTSCHEDULEEDITOR_H

#include "kptcontext.h"
#include <kptviewbase.h>
#include <kptitemmodelbase.h>
#include "kptsplitterview.h"

#include <QTreeWidget>



class QPoint;


namespace KPlato
{

class View;
class Project;
class ScheduleManager;
class MainSchedule;
class Schedule;


class ScheduleItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit ScheduleItemModel( Part *part, QObject *parent = 0 );
    ~ScheduleItemModel();

    virtual void setProject( Project *project );

    virtual Qt::ItemFlags flags( const QModelIndex & index ) const;

    virtual QModelIndex parent( const QModelIndex & index ) const;
    virtual bool hasChildren( const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex index( const ScheduleManager *manager ) const;

    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const; 
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const; 

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const; 
    virtual bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    QItemDelegate *createDelegate( int column, QWidget *parent ) const;
    
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder );

    virtual QMimeData * mimeData( const QModelIndexList & indexes ) const;
    virtual QStringList mimeTypes () const;

    ScheduleManager *manager( const QModelIndex &index ) const;
    
protected slots:
    void slotManagerChanged( ScheduleManager *sch );
    void slotScheduleChanged( MainSchedule *sch );

    void slotScheduleManagerToBeInserted( const ScheduleManager *manager, int row );
    void slotScheduleManagerInserted( const ScheduleManager *manager );
    void slotScheduleManagerToBeRemoved( const ScheduleManager *manager );
    void slotScheduleManagerRemoved( const ScheduleManager *manager );
    void slotScheduleToBeInserted( const ScheduleManager *manager, int row );
    void slotScheduleInserted( const MainSchedule *schedule );
    void slotScheduleToBeRemoved( const MainSchedule *schedule );
    void slotScheduleRemoved( const MainSchedule *schedule );

protected:
    int row( const Schedule *sch ) const;
    
    QVariant name( const QModelIndex &index, int role ) const;
    bool setName( const QModelIndex &index, const QVariant &value, int role );
    
    QVariant state( const QModelIndex &index, int role ) const;
    bool setState( const QModelIndex &index, const QVariant &value, int role );

    QVariant allowOverbooking( const QModelIndex &index, int role ) const;
    bool setAllowOverbooking( const QModelIndex &index, const QVariant &value, int role );
    
    QVariant usePert( const QModelIndex &index, int role ) const;
    bool setUsePert( const QModelIndex &index, const QVariant &value, int role );

    QVariant calculateAll( const QModelIndex &index, int role ) const;
    bool setCalculateAll( const QModelIndex &index, const QVariant &value, int role );

    QVariant projectStart( const QModelIndex &index, int role ) const;
    QVariant projectEnd( const QModelIndex &index, int role ) const;

private:
    ScheduleManager *m_manager; // for sanety check
    
};

class ScheduleTreeView : public TreeViewBase
{
    Q_OBJECT
public:
    ScheduleTreeView( Part *part, QWidget *parent );

    ScheduleItemModel *itemModel() const { return static_cast<ScheduleItemModel*>( model() ); }

    void setArrowKeyNavigation( bool on ) { m_arrowKeyNavigation = on; }
    bool arrowKeyNavigation() const { return m_arrowKeyNavigation; }

    Project *project() const { return itemModel()->project(); }
    void setProject( Project *project ) { itemModel()->setProject( project ); }

    ScheduleManager *currentManager() const;

    Part *part() const { return m_part; }
    
signals:
    void currentChanged( const QModelIndex& );
    void currentColumnChanged( QModelIndex, QModelIndex );
    void selectionChanged( const QModelIndexList );

    void contextMenuRequested( QModelIndex, const QPoint& );
    
protected slots:
    void headerContextMenuRequested( const QPoint &pos );
    void slotActivated( const QModelIndex index );
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    virtual void currentChanged ( const QModelIndex & current, const QModelIndex & previous );
    
protected:

private:
    bool m_arrowKeyNavigation;
    Part *m_part;
};

class ScheduleEditor : public ViewBase
{
    Q_OBJECT
public:
    ScheduleEditor( Part *part, QWidget *parent );
    
    void setupGui();
    virtual void draw( Project &project );
    virtual void draw();
    virtual void updateReadWrite( bool /*readwrite*/ ) {}

signals:
    void requestPopupMenu( const QString&, const QPoint& );
    void calculateSchedule( Project*, ScheduleManager* );
    void addScheduleManager( Project* );
    void deleteScheduleManager( Project*, ScheduleManager* );
    void SelectionScheduleChanged();

    /**
     * Emitted when schedule selection changes.
     * @param sm is the new schedule manager. If @p is 0, no schedule is selected.
    */
    void scheduleSelectionChanged( ScheduleManager *sm );
    
public slots:
    /// Activate/deactivate the gui
    virtual void setGuiActive( bool activate );

private slots:
    void slotContextMenuRequested( QModelIndex index, const QPoint& pos );
    
    void slotSelectionChanged( const QModelIndexList );
    void slotCurrentChanged( const QModelIndex& );
    void slotEnableActions( const ScheduleManager *sm );

    void slotCalculateSchedule();
    void slotAddSchedule();
    void slotAddSubSchedule();
    void slotDeleteSelection();
    
private:
    ScheduleTreeView *m_editor;

    KAction *actionCalculateSchedule;
    KAction *actionAddSchedule;
    KAction *actionAddSubSchedule;
    KAction *actionDeleteSelection;
};


//-----------------------------
class ScheduleHandlerView : public SplitterView
{
    Q_OBJECT
public:
    ScheduleHandlerView( Part *part, QWidget *parent );
    
    ScheduleEditor *scheduleEditor() const { return m_scheduleEditor; }

signals:
    void currentScheduleManagerChanged( ScheduleManager* );

private:
    ScheduleEditor *m_scheduleEditor;
};


}  //KPlato namespace

#endif
