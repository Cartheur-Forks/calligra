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

#include "kplatoui_export.h"

#include <kptviewbase.h>
#include "kptsplitterview.h"
#include "kptschedulemodel.h"

#include <KoXmlReaderForward.h>

#include <QTreeWidget>

class KoDocument;

class QPoint;


namespace KPlato
{

class View;
class Project;
class ScheduleManager;
class MainSchedule;
class Schedule;

class KPLATOUI_EXPORT ScheduleTreeView : public TreeViewBase
{
    Q_OBJECT
public:
    ScheduleTreeView( QWidget *parent );

    ScheduleItemModel *model() const { return static_cast<ScheduleItemModel*>( TreeViewBase::model() ); }

    void setArrowKeyNavigation( bool on ) { m_arrowKeyNavigation = on; }
    bool arrowKeyNavigation() const { return m_arrowKeyNavigation; }

    Project *project() const { return model()->project(); }
    void setProject( Project *project ) { model()->setProject( project ); }

    ScheduleManager *currentManager() const;

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
};

class KPLATOUI_EXPORT ScheduleEditor : public ViewBase
{
    Q_OBJECT
public:
    ScheduleEditor( KoDocument *part, QWidget *parent );
    
    void setupGui();
    virtual void draw( Project &project );
    virtual void draw();
    
    ScheduleItemModel *model() const { return m_view->model(); }
    
    virtual void updateReadWrite( bool readwrite );

    /// Loads context info into this view. Reimplement.
    virtual bool loadContext( const KoXmlElement &/*context*/ );
    /// Save context info from this view. Reimplement.
    virtual void saveContext( QDomElement &/*context*/ ) const;
    
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
    void slotHeaderContextMenuRequested( const QPoint &pos );
    
    void slotSelectionChanged( const QModelIndexList );
    void slotCurrentChanged( const QModelIndex& );
    void slotEnableActions( const ScheduleManager *sm );

    void slotCalculateSchedule();
    void slotAddSchedule();
    void slotAddSubSchedule();
    void slotDeleteSelection();
    
    void slotOptions();
    
private:
    ScheduleTreeView *m_view;

    KAction *actionCalculateSchedule;
    KAction *actionAddSchedule;
    KAction *actionAddSubSchedule;
    KAction *actionDeleteSelection;
    
    KAction *actionOptions;
};


//-----------------------------
class KPLATOUI_EXPORT ScheduleHandlerView : public SplitterView
{
    Q_OBJECT
public:
    ScheduleHandlerView( KoDocument *part, QWidget *parent );
    
    ScheduleEditor *scheduleEditor() const { return m_scheduleEditor; }

signals:
    void currentScheduleManagerChanged( ScheduleManager* );

private:
    ScheduleEditor *m_scheduleEditor;
};


}  //KPlato namespace

#endif
