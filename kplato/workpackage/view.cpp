/* This file is part of the KDE project
  Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>
  Copyright (C) 2002 - 2007 Dag Andersen <danders@get2net.dk>

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

#include "view.h"

#include <kmessagebox.h>

#include "KoDocumentInfo.h"
#include <KoMainWindow.h>
#include <KoToolManager.h>
#include <KoToolBox.h>
#include <KoDocumentChild.h>

#include <QApplication>
#include <QIcon>
#include <QLayout>
#include <QColor>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <qsize.h>
#include <QTabWidget>
#include <QRect>
#include <QVBoxLayout>
#include <QStyle>
#include <QPrinter>
#include <QPrintDialog>

#include <kicon.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kmenu.h>
#include <kstandardaction.h>
#include <klocale.h>
#include <kdebug.h>
#include <ktoolbar.h>
#include <kstandardshortcut.h>
#include <kaccelgen.h>
#include <kdeversion.h>
#include <kstatusbar.h>
#include <kxmlguifactory.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <k3command.h>
#include <ktoggleaction.h>
#include <kfiledialog.h>
#include <kparts/event.h>
#include <kparts/partmanager.h>
#include <KoQueryTrader.h>

#include "part.h"
#include "factory.h"

#include "kptviewbase.h"
#include "kptdocumentseditor.h"
#include "kptworkpackageview.h"

#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptcommand.h"

#include <assert.h>

namespace KPlatoWork
{

//-------------------------------
View::View( Part* part, QWidget* parent )
        : KoView( part, parent ),
        m_scheduleActionGroup( new QActionGroup( this ) ),
        m_manager( 0 ),
        m_readWrite( false )
{
    //kDebug();

    setComponentData( Factory::global() );
    if ( !part->isReadWrite() )
        setXMLFile( "kplatowork_readonly.rc" );
    else
        setXMLFile( "kplatowork.rc" );

//    m_dbus = new ViewAdaptor( this );
//    QDBusConnection::sessionBus().registerObject( '/' + objectName(), this );
    
    m_tab = new QTabWidget( this );
    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setMargin(0);
    layout->addWidget( m_tab );

////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Add sub views
    createViews();

    // The menu items
    // ------ Edit
    actionCut = actionCollection()->addAction(KStandardAction::Cut,  "edit_cut", this, SLOT( slotEditCut() ));
    actionCopy = actionCollection()->addAction(KStandardAction::Copy,  "edit_copy", this, SLOT( slotEditCopy() ));
    actionPaste = actionCollection()->addAction(KStandardAction::Paste,  "edit_paste", this, SLOT( slotEditPaste() ));

    // ------ Settings
    actionConfigure  = new KAction(KIcon( "configure" ), i18n("Configure KPlatoWork..."), this);
    actionCollection()->addAction("configure", actionConfigure );
    connect( actionConfigure, SIGNAL( triggered( bool ) ), SLOT( slotConfigure() ) );

    m_progress = 0;
    m_estlabel = new QLabel( "", 0 );
    if ( statusBar() ) {
        addStatusBarItem( m_estlabel, 0, true );
        //m_progress = new QProgressBar();
        //addStatusBarItem( m_progress, 0, true );
        //m_progress->hide();
    }
    connect( &getProject(), SIGNAL( scheduleChanged( MainSchedule* ) ), SLOT( slotScheduleChanged( MainSchedule* ) ) );

    connect( &getProject(), SIGNAL( scheduleAdded( const MainSchedule* ) ), SLOT( slotScheduleAdded( const MainSchedule* ) ) );
    connect( &getProject(), SIGNAL( scheduleRemoved( const MainSchedule* ) ), SLOT( slotScheduleRemoved( const MainSchedule* ) ) );
    slotPlugScheduleActions();

    loadContext();
    
    connect( part, SIGNAL( changed() ), SLOT( slotUpdate() ) );
    
    connect( m_scheduleActionGroup, SIGNAL( triggered( QAction* ) ), SLOT( slotViewSchedule( QAction* ) ) );
    
    //kDebug()<<" end";
}

View::~View()
{
    removeStatusBarItem( m_estlabel );
    delete m_estlabel;
}

void View::createViews()
{
    createTaskInfoView();
    createDocumentsView();
}

ViewBase *View::createTaskInfoView()
{
    kDebug();
    WorkPackageInfoView *v = new WorkPackageInfoView( getPart(), m_tab );
    m_tab->addTab( v, i18n( "Information" ) );

    Project &p = getProject();
    v->setProject( &p );
    Task *t = 0;
    if ( p.numChildren() > 0 ) { // should be 1
        t = dynamic_cast<Task*>( p.childNode( 0 ) );
    }
    v->setTask( t );
    
    kDebug()<<p.allScheduleManagers();
    v->setScheduleManager( p.allScheduleManagers().value( 0 ) );
    
    connect( v, SIGNAL( guiActivated( ViewBase*, bool ) ), SLOT( slotGuiActivated( ViewBase*, bool ) ) );

    connect( v, SIGNAL( requestPopupMenu( const QString&, const QPoint & ) ), this, SLOT( slotPopupMenu( const QString&, const QPoint& ) ) );
    v->updateReadWrite( false );
    return v;
}

ViewBase *View::createDocumentsView()
{
    kDebug();
    DocumentsEditor *v = new DocumentsEditor( getPart(), m_tab );
    m_tab->addTab( v, i18n( "Documents" ) );

    Project &p = getProject();
    Task *t = 0;
    if ( p.numChildren() > 0 ) { // should be 1
        Node *n = p.childNode( 0 );
        kDebug()<<"Node: "<<n->name();
        v->draw( n->documents() );
    }
    
    connect( v, SIGNAL( guiActivated( ViewBase*, bool ) ), SLOT( slotGuiActivated( ViewBase*, bool ) ) );

    connect( v, SIGNAL( requestPopupMenu( const QString&, const QPoint & ) ), this, SLOT( slotPopupMenu( const QString&, const QPoint& ) ) );
    v->updateReadWrite( false );
    return v;
}

Project& View::getProject() const
{
    return getPart() ->getProject();
}

void View::setZoom( double )
{
    //TODO
}

void View::setupPrinter( QPrinter &printer, QPrintDialog &printDialog )
{
    //kDebug();
}

void View::print( QPrinter &printer, QPrintDialog &printDialog )
{
}

void View::slotEditCut()
{
    //kDebug();
}

void View::slotEditCopy()
{
    //kDebug();
}

void View::slotEditPaste()
{
    //kDebug();
}

void View::slotScheduleRemoved( const MainSchedule *sch )
{
    QAction *a = 0;
    QAction *checked = m_scheduleActionGroup->checkedAction();
    QMapIterator<QAction*, Schedule*> i( m_scheduleActions );
    while (i.hasNext()) {
        i.next();
        if ( i.value() == sch ) {
            a = i.key();
            break;
        }
    }
    if ( a ) {
        unplugActionList( "view_schedule_list" );
        delete a;
        plugActionList( "view_schedule_list", m_scheduleActions.keys() );
        if ( checked && checked != a ) {
            checked->setChecked( true );
        } else if ( ! m_scheduleActions.isEmpty() ) {
            m_scheduleActions.keys().first()->setChecked( true );
        }
    }
    slotViewSchedule( m_scheduleActionGroup->checkedAction() );
}

void View::slotScheduleAdded( const MainSchedule *sch )
{
    if ( sch->type() != Schedule::Expected ) {
        return; // Only view expected
    }
    MainSchedule *s = const_cast<MainSchedule*>( sch ); // FIXME
    //kDebug()<<sch->name()<<" deleted="<<sch->isDeleted();
    QAction *checked = m_scheduleActionGroup->checkedAction();
    if ( ! sch->isDeleted() && sch->isScheduled() ) {
        unplugActionList( "view_schedule_list" );
        QAction *act = addScheduleAction( s );
        plugActionList( "view_schedule_list", m_scheduleActions.keys() );
        if ( checked ) {
            checked->setChecked( true );
        } else if ( act ) {
            act->setChecked( true );
        } else if ( ! m_scheduleActions.isEmpty() ) {
            m_scheduleActions.keys().first()->setChecked( true );
        }
    }
    slotViewSchedule( m_scheduleActionGroup->checkedAction() );
}

void View::slotScheduleChanged( MainSchedule *sch )
{
    //kDebug()<<sch->name()<<" deleted="<<sch->isDeleted();
    if ( sch->isDeleted() || ! sch->isScheduled() ) {
        slotScheduleRemoved( sch );
        return;
    }
    if ( m_scheduleActions.values().contains( sch ) ) {
        slotScheduleRemoved( sch ); // hmmm, how to avoid this?
    }
    slotScheduleAdded( sch );
}

QAction *View::addScheduleAction( Schedule *sch )
{
    QAction *act = 0;
    if ( ! sch->isDeleted() ) {
        QString n = sch->name();
        QAction *act = new KToggleAction( n, this);
        actionCollection()->addAction(n, act );
        m_scheduleActions.insert( act, sch );
        m_scheduleActionGroup->addAction( act );
        //kDebug()<<"Add:"<<n;
        connect( act, SIGNAL(destroyed( QObject* ) ), SLOT( slotActionDestroyed( QObject* ) ) );
    }
    return act;
}

void View::slotViewSchedule( QAction *act )
{
    //kDebug();
    if ( act != 0 ) {
        Schedule *sch = m_scheduleActions.value( act, 0 );
        m_manager = sch->manager();
    } else {
        m_manager = 0;
    }
    kDebug()<<m_manager<<endl;
    setLabel();
    emit currentScheduleManagerChanged( m_manager );
}

void View::slotActionDestroyed( QObject *o )
{
    //kDebug()<<o->name();
    m_scheduleActions.remove( static_cast<QAction*>( o ) );
    slotViewSchedule( m_scheduleActionGroup->checkedAction() );
}

void View::slotPlugScheduleActions()
{
    //kDebug();
    unplugActionList( "view_schedule_list" );
    foreach( QAction *act, m_scheduleActions.keys() ) {
        m_scheduleActionGroup->removeAction( act );
        delete act;
    }
    m_scheduleActions.clear();
    Schedule *cs = getProject().currentSchedule();
    QAction *ca = 0;
    foreach( ScheduleManager *sm, getProject().allScheduleManagers() ) {
        Schedule *sch = sm->expected();
        if ( sch == 0 ) {
            continue;
        }
        QAction *act = addScheduleAction( sch );
        if ( act ) {
            if ( ca == 0 && cs == sch ) {
                ca = act;
            }
        }
    }
    plugActionList( "view_schedule_list", m_scheduleActions.keys() );
    if ( ca == 0 && m_scheduleActionGroup->actions().count() > 0 ) {
        ca = m_scheduleActionGroup->actions().first();
    }
    if ( ca ) {
        ca->setChecked( true );
    }
    slotViewSchedule( ca );
}

void View::slotProgressChanged( int )
{
}

void View::slotAddScheduleManager( Project *project )
{
    if ( project == 0 ) {
        return;
    }
    ScheduleManager *sm = project->createScheduleManager();
    AddScheduleManagerCmd *cmd =  new AddScheduleManagerCmd( *project, sm, i18n( "Add Schedule %1", sm->name() ) );
    getPart() ->addCommand( cmd );
}

void View::slotDeleteScheduleManager( Project *project, ScheduleManager *sm )
{
    if ( project == 0 || sm == 0) {
        return;
    }
    DeleteScheduleManagerCmd *cmd =  new DeleteScheduleManagerCmd( *project, sm, i18n( "Delete Schedule %1", sm->name() ) );
    getPart() ->addCommand( cmd );
}

void View::slotConfigure()
{
}

ScheduleManager *View::currentScheduleManager() const
{
    Schedule *s = m_scheduleActions.value( m_scheduleActionGroup->checkedAction() );
    return s == 0 ? 0 : s->manager();
}

long View::currentScheduleId() const
{
    Schedule *s = m_scheduleActions.value( m_scheduleActionGroup->checkedAction() );
    return s == 0 ? -1 : s->id();
}

void View::updateReadWrite( bool readwrite )
{
    m_readWrite = readwrite;
}

Part *View::getPart() const
{
    return ( Part * ) koDocument();
}

QMenu * View::popupMenu( const QString& name )
{
    //kDebug();
    Q_ASSERT( factory() );
    if ( factory() )
        return ( ( QMenu* ) factory() ->container( name, this ) );
    return 0L;
}

void View::slotUpdate()
{
    updateView( m_tab->currentWidget() );
}

void View::slotGuiActivated( ViewBase *view, bool activate )
{
    if ( activate ) {
        foreach( QString name, view->actionListNames() ) {
            //kDebug()<<"activate"<<name<<","<<view->actionList( name ).count();
            plugActionList( name, view->actionList( name ) );
        }
    } else {
        foreach( QString name, view->actionListNames() ) {
            //kDebug()<<"deactivate"<<name;
            unplugActionList( name );
        }
    }
}

void View::guiActivateEvent( KParts::GUIActivateEvent *ev )
{
    //kDebug()<<ev->activated();
    KoView::guiActivateEvent( ev );
    if ( ev->activated() ) {
        // plug my own actionlists, they may be gone
        slotPlugScheduleActions();
    }
    // propagate to sub-view
    ViewBase *v = dynamic_cast<ViewBase*>( m_tab->currentWidget() );
    if ( v ) {
        v->setGuiActive( ev->activated() );
    }
}

void View::slotCurrentChanged( int )
{
}

void View::updateView( QWidget * )
{
    QApplication::setOverrideCursor( Qt::WaitCursor );

    QApplication::restoreOverrideCursor();
}

void View::slotPopupMenu( const QString& menuname, const QPoint & pos )
{
    QMenu * menu = this->popupMenu( menuname );
    if ( menu ) {
        //kDebug()<<menu<<":"<<menu->actions().count();
        menu->exec( pos );
    }
}

bool View::loadContext()
{
    //kDebug()<<endl;
    return true;
}

void View::saveContext( QDomElement &me ) const
{
    //kDebug()<<endl;
}

void View::setLabel()
{
    //kDebug();
    Schedule *s = m_manager == 0 ? 0 : m_manager->expected();
    if ( s && !s->isDeleted() && s->isScheduled() ) {
        m_estlabel->setText( m_manager->name() );
        return;
    }
    m_estlabel->setText( i18n( "Not scheduled" ) );
}

}  //KPlatoWork namespace

#include "view.moc"
