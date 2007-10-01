/* This file is part of the KDE project
   Copyright (C) 2007 Dag Andersen <danders@get2net.dk>

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

#include "kptviewlistdialog.h"
#include "viewlist/kptviewlist.h"
#include "kptview.h"

#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>

namespace KPlato
{

// little helper stolen from kmail/kword
static inline QPixmap loadIcon( const char * name ) {
  return KIconLoader::global()->loadIcon( QString::fromLatin1(name), KIconLoader::NoGroup, KIconLoader::SizeMedium );
}


ViewListDialog::ViewListDialog( View *view, ViewListWidget &viewlist, QWidget *parent )
    : KDialog(parent)
{
    setCaption( i18n("Add View") );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( Ok );

    m_panel = new AddViewPanel( view, viewlist, this );

    setMainWidget( m_panel );
    
    enableButtonOk(false);

    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
    connect( m_panel, SIGNAL( enableButtonOk( bool ) ), SLOT( enableButtonOk( bool ) ) );
}


void ViewListDialog::slotOk() {
    if ( m_panel->ok() ) {
        accept();
    }
}

//------------------------
AddViewPanel::AddViewPanel( View *view, ViewListWidget &viewlist, QWidget *parent )
    : QWidget( parent ),
      m_view( view ),
      m_viewlist( viewlist )
{
    widget.setupUi( this );
    
    // NOTE: must match switch in ok()
    QStringList lst;
    lst << i18n( "Resource Editor" )
            << i18n( "Task Editor" )
            << i18n( "Work & Vacation Editor" )
            << i18n( "Accounts Editor" )
            << i18n( "Dependency Editor" )
            << i18n( "Schedule Handler" )
            << i18n( "Task Status" )
            << i18n( "Gantt View" )
            << i18n( "Milestone Gantt View" )
            << i18n( "Resource Assignments" )
            << i18n( "Cost Breakdown" )
            << i18n( "Performance Chart" )
            << i18n( "Tasks by Resources" );
    widget.viewtype->addItems( lst );
    
    foreach ( ViewListItem *item, m_viewlist.categories() ) {
        m_categories.insert( item->text( 0 ), item->tag() );
    }
    widget.category->addItems( m_categories.keys() );
    
    connect( widget.viewname, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.tooltip, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
}

bool AddViewPanel::ok()
{
    QString n = widget.category->currentText();
    QString c = m_categories.value( n );
    if ( c.isEmpty() ) {
        c = n;
    }
    ViewListItem *cat = m_viewlist.addCategory( c, n );
    if ( cat == 0 ) {
        return false;
    }
    switch ( widget.viewtype->currentIndex() ) {
        case 0: // Resource editor
            m_view->createResourcEditor( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 1: // Task editor
            m_view->createTaskEditor( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 2: // Work & Vacation Editor
            m_view->createCalendarEditor( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 3: // Accounts Editor
            m_view->createAccountsEditor( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 4: // Dependency Editor
            m_view->createDependencyEditor( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 5: // Schedules Handler
            m_view->createScheduleHandler( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 6: // Task status
            m_view->createTaskStatusView( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 7: // Gantt View
            m_view->createGanttView( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 8: // Milestone Gantt View
            m_view->createMilestoneGanttView( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 9: // Resource Assignments
            m_view->createResourceAppointmentsView( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 10: // Cost Breakdown
            m_view->createAccountsView( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 11: // Performance Chart
            m_view->createTaskStatusView( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        case 12: // Tasks by Resources
            m_view->createResourceAssignmentView( cat, widget.viewname->text(), widget.viewname->text(), widget.tooltip->text() );
            break;
        default:
            kError()<<"Unknown view type!";
            break;
    }
    return true;
}

void AddViewPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty() | widget.viewtype->currentText().isEmpty() | widget.category->currentText().isEmpty();
    emit enableButtonOk( ! disable );
}

//------------------------
ViewListEditViewDialog::ViewListEditViewDialog( ViewListWidget &viewlist, ViewListItem *item, QWidget *parent )
    : KDialog(parent)
{
    setCaption( i18n("Configure View") );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( Ok );

    m_panel = new EditViewPanel( viewlist, item, this );

    setMainWidget( m_panel );
    
    enableButtonOk(false);

    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
    connect( m_panel, SIGNAL( enableButtonOk( bool ) ), SLOT( enableButtonOk( bool ) ) );
}


void ViewListEditViewDialog::slotOk() {
    if ( m_panel->ok() ) {
        accept();
    }
}

EditViewPanel::EditViewPanel( ViewListWidget &viewlist, ViewListItem *item, QWidget *parent )
    : QWidget( parent ),
      m_item( item ),
      m_viewlist( viewlist )
{
    widget.setupUi( this );
    
    widget.viewname->setText( item->text( 0 ) );
    widget.tooltip->setText( item->toolTip( 0 ) );
    
    QList<ViewListItem*> lst = m_viewlist.categories();
    if ( ! lst.isEmpty() ) {
        int idx = 0;
        for ( int i = 0; i < lst.count(); ++i ) {
            ViewListItem *citem = lst[ i ];
            widget.category->addItem( citem->text( 0 ), QVariant::fromValue( (void*)citem ) );
            if ( citem == item->parent() ) {
                idx = i;
                fillAfter( citem );
            }
        }
        widget.category->setCurrentIndex( idx );
    }
    
    connect( widget.viewname, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.tooltip, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.insertAfter, SIGNAL( currentIndexChanged( int ) ), SLOT( changed() ) );
    connect( widget.category, SIGNAL( currentIndexChanged( int ) ), SLOT( categoryChanged( int ) ) );
}

bool EditViewPanel::ok()
{
    ViewListItem *cat = static_cast<ViewListItem*>( widget.category->itemData( widget.category->currentIndex() ).value<void*>() );
    if ( cat == 0 ) {
        return false;
    }
    m_item->setText( 0, widget.viewname->text() );
    m_item->setToolTip( 0, widget.tooltip->text() );
    m_viewlist.removeViewListItem( m_item );
    int index = widget.insertAfter->currentIndex();
    m_viewlist.addViewListItem( m_item, cat, index );
    
    return true;
}

void EditViewPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty() | widget.category->currentText().isEmpty();
    emit enableButtonOk( ! disable );
}

void EditViewPanel::categoryChanged( int index )
{
    kDebug();
    ViewListItem *cat = static_cast<ViewListItem*>( widget.category->itemData( index ).value<void*>() );
    fillAfter( cat );
    changed();
}

void EditViewPanel::fillAfter( ViewListItem *cat )
{
    kDebug();
    widget.insertAfter->clear();
    if ( cat ) {
        widget.insertAfter->addItem( i18n( "Top" ) );
        int idx = 0;
        for ( int i = 0; i < cat->childCount(); ++i ) {
            ViewListItem *itm = static_cast<ViewListItem*>( cat->child( i ) );
            if ( m_item == itm ) {
                idx = i;
            } else {
                widget.insertAfter->addItem( itm->text( 0 ), QVariant::fromValue( (void*)itm ) );
            }
        }
        widget.insertAfter->setCurrentIndex( idx );
    }
}

//------------------------
ViewListEditCategoryDialog::ViewListEditCategoryDialog( ViewListWidget &viewlist, ViewListItem *item, QWidget *parent )
    : KDialog(parent)
{
    setCaption( i18n("Configure View") );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( Ok );

    m_panel = new EditCategoryPanel( viewlist, item, this );

    setMainWidget( m_panel );
    
    enableButtonOk(false);

    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
    connect( m_panel, SIGNAL( enableButtonOk( bool ) ), SLOT( enableButtonOk( bool ) ) );
}


void ViewListEditCategoryDialog::slotOk() {
    if ( m_panel->ok() ) {
        accept();
    }
}

EditCategoryPanel::EditCategoryPanel( ViewListWidget &viewlist, ViewListItem *item, QWidget *parent )
    : QWidget( parent ),
      m_item( item ),
      m_viewlist( viewlist )
{
    widget.setupUi( this );
    
    widget.viewname->setText( item->text( 0 ) );
    widget.tooltip->setText( item->toolTip( 0 ) );
    
    fillAfter();
    
    connect( widget.viewname, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.tooltip, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.insertAfter, SIGNAL( currentIndexChanged( int ) ), SLOT( changed() ) );
}

bool EditCategoryPanel::ok()
{
    m_item->setText( 0, widget.viewname->text() );
    m_item->setToolTip( 0, widget.tooltip->text() );
    m_viewlist.removeViewListItem( m_item );
    int index = widget.insertAfter->currentIndex();
    m_viewlist.addViewListItem( m_item, 0, index );
    
    return true;
}

void EditCategoryPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty();
    emit enableButtonOk( ! disable );
}

void EditCategoryPanel::fillAfter()
{
    kDebug();
    widget.insertAfter->clear();
    widget.insertAfter->addItem( i18n( "Top" ) );
    int idx = 0;
    QList<ViewListItem*> lst = m_viewlist.categories();
    for ( int i = 0; i < lst.count(); ++i ) {
        ViewListItem *itm = lst[ i ];
        if ( m_item == itm ) {
            idx = i;
        } else {
            widget.insertAfter->addItem( itm->text( 0 ), QVariant::fromValue( (void*)itm ) );
        }
    }
    widget.insertAfter->setCurrentIndex( idx );
}



}  //KPlato namespace

#include "kptviewlistdialog.moc"
