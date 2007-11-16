/* This file is part of the KDE project
  Copyright (C) 2006 Dag Andersen <danders@get2net.dk>

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
  Boston, MA 02110-1301, USA.
*/

#include "kptviewbase.h"
#include "kptitemmodelbase.h"
#include "kptproject.h"

#include <kaction.h>
#include <kicon.h>
#include <kparts/event.h>
#include <kxmlguifactory.h>

#include <KoDocument.h>

#include <QHeaderView>
#include <QPoint>

namespace KPlato
{
    
//--------------
ViewBase::ViewBase(KoDocument *doc, QWidget *parent)
    : KoView( doc, parent )
{
}
    
KoDocument *ViewBase::part() const
{
     return koDocument();
}

void ViewBase::updateReadWrite( bool /*readwrite*/ )
{
}

void ViewBase::setGuiActive( bool active ) // virtual slot
{
    //kDebug()<<active;
    emit guiActivated( this, active );
}

ViewBase *ViewBase::hitView( const QPoint &glpos )
{
    kDebug()<<glpos;
    return this;
}


//------------------------------------------------

TreeViewBase::TreeViewBase( QWidget *parent )
    : QTreeView( parent ),
    m_arrowKeyNavigation( true ),
    m_acceptDropsOnView( false ),
    m_readWrite( false )

{
    setAlternatingRowColors ( true );

    header()->setContextMenuPolicy( Qt::CustomContextMenu );

    connect( header(), SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );
}

void TreeViewBase::setReadWrite( bool rw )
{
    if ( model() ) {
        model()->setReadWrite( rw );
    }
}

void TreeViewBase::slotHeaderContextMenuRequested( const QPoint& pos )
{
    kDebug();
    emit headerContextMenuRequested( header()->mapToGlobal( pos ) );
}

void TreeViewBase::setColumnsHidden( const QList<int> &lst )
{
    //kDebug()<<m_hideList<<endl;
    int prev = 0;
    foreach ( int c, lst ) {
        if ( c == -1 ) {
            // hide rest
            for ( int i = prev+1; i < model()->columnCount(); ++i ) {
                if ( ! lst.contains( i ) ) {
                    hideColumn( i );
                }
            }
            break;
        }
        hideColumn( c );
        prev = c;
    }
}

QModelIndex TreeViewBase::nextColumn( const QModelIndex &curr ) const
{
    int s = header()->visualIndex( curr.column() ) + 1;
    while ( isColumnHidden( header()->logicalIndex( s ) ) ) {
        if ( s >= header()->count() ) {
            kDebug()<<curr.column()<<": -1";
            return QModelIndex();
        }
        ++s;
    }
    kDebug()<<curr.column()<<": next="<<header()->logicalIndex( s )<<","<<s;
    return model()->index( curr.row(), header()->logicalIndex( s ), curr.parent() );
}

QModelIndex TreeViewBase::previousColumn( const QModelIndex &curr ) const
{
    int s = header()->visualIndex( curr.column() ) - 1;
    while ( isColumnHidden( header()->logicalIndex( s ) ) ) {
        if ( s < 0 ) {
            kDebug()<<curr.column()<<": -1";
            return QModelIndex();
        }
        --s;
    }
    kDebug()<<curr.column()<<": prev="<<header()->logicalIndex( s )<<","<<s;
    return model()->index( curr.row(), header()->logicalIndex( s ), curr.parent() );
}

/*!
    \reimp
 */
void TreeViewBase::keyPressEvent(QKeyEvent *event)
{
    kDebug()<<event->key()<<","<<m_arrowKeyNavigation;
    if ( !m_arrowKeyNavigation ) {
        QTreeView::keyPressEvent( event );
        return;
    }
    QModelIndex current = currentIndex();
    if ( current.isValid() ) {
        switch (event->key()) {
            case Qt::Key_Right: {
                QModelIndex nxt = nextColumn( current );
                if ( nxt.isValid() ) {
                    selectionModel()->setCurrentIndex( nxt, QItemSelectionModel::NoUpdate );
                } else {
                    emit moveAfterLastColumn( current );
                }
                event->accept();
                return;
                break;
            }
            case Qt::Key_Left: {
                QModelIndex prv = previousColumn( current );
                if ( prv.isValid() ) {
                    selectionModel()->setCurrentIndex( prv, QItemSelectionModel::NoUpdate );
                } else {
                    emit moveBeforeFirstColumn( current );
                }
                event->accept();
                return;
                break;
            }
            case Qt::Key_Down: {
                QModelIndex i = indexBelow( current );
                if ( i.isValid() ) {
                    i = model()->index( i.row(), current.column(), i.parent() );
                    selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
                }
                event->accept();
                return;
                break;
            }
            case Qt::Key_Up: {
                QModelIndex i = indexAbove( current );
                if ( i.isValid() ) {
                    i = model()->index( i.row(), current.column(), i.parent() );
                    selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
                }
                event->accept();
                return;
                break;
            }
            case Qt::Key_Plus:
                if ( itemsExpandable()) {
                    if ( model()->hasChildren( current ) ) {
                        QTreeView::keyPressEvent( event );
                    //HACK: Bug in qt??
                        selectionModel()->setCurrentIndex(current, QItemSelectionModel::NoUpdate);
                    }
                    event->accept();
                    return;
                }
                break;
            case Qt::Key_Minus:
                if ( itemsExpandable() ) {
                    if ( model()->hasChildren( current ) ) {
                        QTreeView::keyPressEvent( event );
                    //HACK: Bug in qt??
                        selectionModel()->setCurrentIndex(current, QItemSelectionModel::NoUpdate);
                    }
                    event->accept();
                    return;
                }
                break;
        }
    }
    QTreeView::keyPressEvent(event);
}

QItemSelectionModel::SelectionFlags TreeViewBase::selectionCommand(const QModelIndex &index, const QEvent *event) const
{
    /*    if ( event && event->type() == QEvent::KeyPress && selectionMode() == QAbstractItemView::ExtendedSelection ) {
    if ( static_cast<const QKeyEvent*>(event)->key() == Qt::Key_Space ) {
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    QItemSelectionModel::SelectionFlags bflags = QItemSelectionModel::Rows;
    // 
    if ( modifiers && Qt::ShiftModifier ) {
    return QItemSelectionModel::SelectCurrent|bflags;
}
    // Toggle on Ctrl-Qt::Key_Space
    if ( modifiers & Qt::ControlModifier ) {
    return QItemSelectionModel::Toggle|bflags;
}
    // Select on Space alone
    return QItemSelectionModel::ClearAndSelect|bflags;
}
}*/
    return QTreeView::selectionCommand( index, event );
}

void TreeViewBase::contextMenuEvent ( QContextMenuEvent *event )
{
    kDebug();
    emit contextMenuRequested( indexAt(event->pos()), event->globalPos() );
}

void TreeViewBase::slotCurrentChanged( const QModelIndex &current, const QModelIndex & )
{
    if ( current.isValid() ) {
        scrollTo( current );
    }
}

void TreeViewBase::setModel( QAbstractItemModel *model )
{
    if ( selectionModel() ) {
        disconnect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( slotCurrentChanged( const QModelIndex&, const QModelIndex& ) ) );
    }
    QTreeView::setModel( model );
    if ( selectionModel() ) {
        connect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( slotCurrentChanged( const QModelIndex&, const QModelIndex& ) ) );
    }
    setReadWrite( m_readWrite );
}

void TreeViewBase::setSelectionModel( QItemSelectionModel *model )
{
    if ( selectionModel() ) {
        disconnect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( slotCurrentChanged( const QModelIndex&, const QModelIndex& ) ) );
    }
    QTreeView::setSelectionModel( model );
    if ( selectionModel() ) {
        connect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( slotCurrentChanged( const QModelIndex&, const QModelIndex& ) ) );
    }
}


ItemModelBase *TreeViewBase::model() const
{
    return static_cast<ItemModelBase*>( QTreeView::model() );
}

void TreeViewBase::setStretchLastSection( bool mode )
{
    header()->setStretchLastSection( mode );
}

void TreeViewBase::mapToSection( int col, int section )
{
    header()->moveSection( header()->visualIndex( col ), section );
}

int TreeViewBase::section( int col ) const
{
    return header()->visualIndex( col );
}

void TreeViewBase::dragMoveEvent(QDragMoveEvent *event)
{
    //kDebug();
    if (dragDropMode() == InternalMove
        && (event->source() != this || !(event->possibleActions() & Qt::MoveAction))) {
        //kDebug()<<"Internal:"<<event->isAccepted();
        return;
        }
        QTreeView::dragMoveEvent( event );
        event->ignore();
        if ( dropIndicatorPosition() == QAbstractItemView::OnViewport ) {
            if ( m_acceptDropsOnView ) {
                event->accept();
            }
        //kDebug()<<"On viewport:"<<event->isAccepted();
            return;
        }
        QModelIndex index = indexAt( event->pos() );
        if ( ! index.isValid() ) {
        //kDebug()<<"Invalid index:"<<event->isAccepted();
            return;
        }
        if ( model()->dropAllowed( index, dropIndicatorPosition(), event->mimeData() ) ) {
            event->accept();
        }
    //kDebug()<<event->isAccepted();
}

bool TreeViewBase::loadContext( const KoXmlElement &element )
{
    kDebug()<<objectName()<<endl;
    KoXmlElement e = element.namedItem( "columns" ).toElement();
    if ( ! e.isNull() ) {
        for ( int i = model()->columnCount() - 1; i >= 0; --i ) {
            if ( e.attribute( QString( "column-%1" ).arg( i ), "" ) == "hidden" ) {
                hideColumn( i );
            } else {
                showColumn( i );
            }
        }
    }
    e = element.namedItem( "sections" ).toElement();
    if ( ! e.isNull() ) {
        QHeaderView *h = header();
        QString s( "section-%1" );
        for ( int i = 0; i < h->count(); ++i ) {
            if ( e.hasAttribute( s.arg( i ) ) ) {
                int pos = h->visualIndex( e.attribute( s.arg( i ), "-1" ).toInt() );
                header()->moveSection( pos, i );
            }
        }
    }
    return true;
}

void TreeViewBase::saveContext( QDomElement &element ) const
{
    QDomElement e = element.ownerDocument().createElement( "columns" );
    element.appendChild( e );
    for ( int i = 0; i < model()->columnCount(); ++i ) {
        if ( isColumnHidden( i ) ) {
            e.setAttribute( QString( "column-%1" ).arg( i ), "hidden" );
        }
    }
    e = element.ownerDocument().createElement( "sections" );
    element.appendChild( e );
    QHeaderView *h = header();
    for ( int i = 0; i < h->count(); ++i ) {
        if ( ! isColumnHidden( h->logicalIndex( i ) ) ) {
            e.setAttribute( QString( "section-%1" ).arg( i ), h->logicalIndex( i ) );
        }
    }
}


//--------------------------------
DoubleTreeViewBase::DoubleTreeViewBase( bool mode, QWidget *parent )
    : QSplitter( parent ),
    m_rightview( 0 ),
    m_model( 0 ),
    m_selectionmodel( 0 ),
    m_readWrite( false ),
    m_mode( false )
{
    init();
}

DoubleTreeViewBase::DoubleTreeViewBase( QWidget *parent )
    : QSplitter( parent ),
    m_rightview( 0 ),
    m_model( 0 ),
    m_selectionmodel( 0 ),
    m_mode( false )
{
    init();
}

DoubleTreeViewBase::~DoubleTreeViewBase()
{
}

void DoubleTreeViewBase::init()
{
    setOrientation( Qt::Horizontal );
    setHandleWidth( 3 );
    m_leftview = new TreeViewBase( this );
    m_rightview = new TreeViewBase( this );

    connect( m_leftview, SIGNAL( contextMenuRequested( QModelIndex, const QPoint& ) ), SIGNAL( contextMenuRequested( QModelIndex, const QPoint& ) ) );
    connect( m_leftview, SIGNAL( headerContextMenuRequested( const QPoint& ) ), SLOT( slotLeftHeaderContextMenuRequested( const QPoint& ) ) );
    
    connect( m_rightview, SIGNAL( contextMenuRequested( QModelIndex, const QPoint& ) ), SIGNAL( contextMenuRequested( QModelIndex, const QPoint& ) ) );
    connect( m_rightview, SIGNAL( headerContextMenuRequested( const QPoint& ) ), SLOT( slotRightHeaderContextMenuRequested( const QPoint& ) ) );

    connect( m_rightview->verticalScrollBar(), SIGNAL( valueChanged( int ) ), m_leftview->verticalScrollBar(), SLOT( setValue( int ) ) );

    connect( m_leftview, SIGNAL( moveAfterLastColumn( const QModelIndex & ) ), this, SLOT( slotToRightView( const QModelIndex & ) ) );
    connect( m_rightview, SIGNAL( moveBeforeFirstColumn( const QModelIndex & ) ), this, SLOT( slotToLeftView( const QModelIndex & ) ) );

    connect( m_leftview, SIGNAL( expanded( const QModelIndex & ) ), m_rightview, SLOT( expand( const QModelIndex & ) ) );
    connect( m_leftview, SIGNAL( collapsed( const QModelIndex & ) ), m_rightview, SLOT( collapse( const QModelIndex & ) ) );
    
    m_actionSplitView = new KAction(KIcon("splitview"), "Hmmmm", this);
    kDebug()<<m_actionSplitView;
    setViewSplitMode( true );
}

QList<int> DoubleTreeViewBase::expandColumnList( const QList<int> lst ) const
{
    QList<int> mlst = lst;
    if ( ! mlst.isEmpty() ) {
        int v = 0;
        if ( mlst.last() == -1 && mlst.count() > 1 ) {
            v = mlst[ mlst.count() - 2 ] + 1;
            mlst.removeLast();
        }
        for ( int c = v; c < model()->columnCount(); ++c ) {
            mlst << c;
        }
    }
    return mlst;
}

void DoubleTreeViewBase::hideColumns( const QList<int> &masterList, QList<int> slaveList )
{
    m_leftview->setColumnsHidden( masterList );
    m_rightview->setColumnsHidden( slaveList );
    if ( m_rightview->isHidden() ) {
        QList<int> mlst = expandColumnList( masterList );
        QList<int> slst = expandColumnList( slaveList );
        QList<int> lst;
        for ( int c = 0; c < model()->columnCount(); ++c ) {
            // only hide columns hidden in *both* views
            kDebug()<<c<<(mlst.indexOf( c ))<<(slst.indexOf( c ));
            if ( (mlst.indexOf( c ) >= 0) && (slst.indexOf( c ) >= 0) ) {
                lst << c;
            }
        }
        kDebug()<<lst;
        m_leftview->setColumnsHidden( lst );
    }
}

void DoubleTreeViewBase::slotToRightView( const QModelIndex &index )
{
    kDebug()<<index.column()<<endl;
    if ( m_rightview->isHidden() ) {
        return;
    }
    m_rightview->setFocus();
    QModelIndex nxt = m_rightview->nextColumn( index );
    if ( nxt.isValid() ) {
        m_selectionmodel->setCurrentIndex( nxt, QItemSelectionModel::NoUpdate );
    }
}

void DoubleTreeViewBase::slotToLeftView( const QModelIndex &index )
{
    kDebug()<<index.column();
    m_leftview->setFocus();
    QModelIndex prv = m_leftview->previousColumn( index );
    if ( prv.isValid() ) {
        m_selectionmodel->setCurrentIndex( prv, QItemSelectionModel::NoUpdate );
    }
}

void DoubleTreeViewBase::setReadWrite( bool rw )
{
    m_readWrite = rw;
    if ( model() ) {
        model()->setReadWrite( rw );
    }
}

void DoubleTreeViewBase::setModel( ItemModelBase *model )
{
    m_model = model;
    m_leftview->setModel( model );
    m_rightview->setModel( model );
    if ( m_selectionmodel ) {
        disconnect( m_selectionmodel, SIGNAL( selectionChanged ( const QItemSelection &, const QItemSelection & ) ), this, SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );
    
        disconnect( m_selectionmodel, SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ), this, SIGNAL( currentChanged ( const QModelIndex &, const QModelIndex & ) ) );
    }
    m_selectionmodel = m_leftview->selectionModel();
    m_rightview->setSelectionModel( m_selectionmodel );
    
    connect( m_selectionmodel, SIGNAL( selectionChanged ( const QItemSelection &, const QItemSelection & ) ), this, SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );

    connect( m_selectionmodel, SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ), this, SIGNAL( currentChanged ( const QModelIndex &, const QModelIndex & ) ) );
    
    setReadWrite( m_readWrite );
}

ItemModelBase *DoubleTreeViewBase::model() const
{
    return m_model;
}

void DoubleTreeViewBase::slotSelectionChanged( const QItemSelection &sel, const QItemSelection & )
{
    emit selectionChanged( sel.indexes() );
}

void DoubleTreeViewBase::setSelectionModel( QItemSelectionModel *model )
{
    m_leftview->setSelectionModel( model );
    m_rightview->setSelectionModel( model );
}

void DoubleTreeViewBase::setSelectionMode( QAbstractItemView::SelectionMode mode )
{
    m_leftview->setSelectionMode( mode );
    m_rightview->setSelectionMode( mode );
}

void DoubleTreeViewBase::setSelectionBehavior( QAbstractItemView::SelectionBehavior mode )
{
    m_leftview->setSelectionBehavior( mode );
    m_rightview->setSelectionBehavior( mode );
}

void DoubleTreeViewBase::setItemDelegateForColumn( int col, QAbstractItemDelegate * delegate )
{
    m_leftview->setItemDelegateForColumn( col, delegate );
    m_rightview->setItemDelegateForColumn( col, delegate );
}

void DoubleTreeViewBase::setEditTriggers( QAbstractItemView::EditTriggers mode )
{
    m_leftview->setEditTriggers( mode );
    m_rightview->setEditTriggers( mode );
}

QAbstractItemView::EditTriggers DoubleTreeViewBase::editTriggers() const
{
    return m_leftview->editTriggers();
}

void DoubleTreeViewBase::setStretchLastSection( bool mode )
{
    m_rightview->header()->setStretchLastSection( mode );
    if ( m_rightview->isHidden() ) {
        m_leftview->header()->setStretchLastSection( mode );
    }
}

void DoubleTreeViewBase::edit( const QModelIndex &index )
{
    if ( ! m_leftview->isColumnHidden( index.column() ) ) {
        m_leftview->edit( index );
    } else if ( ! m_rightview->isHidden() && ! m_rightview->isColumnHidden( index.column() ) ) {
        m_rightview->edit( index );
    }
}

void DoubleTreeViewBase::setDragDropMode( QAbstractItemView::DragDropMode mode )
{
    m_leftview->setDragDropMode( mode );
}

void DoubleTreeViewBase::setDropIndicatorShown( bool mode )
{
    m_leftview->setDropIndicatorShown( mode );
}

void DoubleTreeViewBase::setDragEnabled ( bool mode )
{
    m_leftview->setDragEnabled( mode );
}

void DoubleTreeViewBase::setAcceptDrops( bool mode )
{
    m_leftview->setAcceptDrops( mode );
}

void DoubleTreeViewBase::setAcceptDropsOnView( bool mode )
{
    m_leftview->setAcceptDropsOnView( mode );
}

void DoubleTreeViewBase::slotRightHeaderContextMenuRequested( const QPoint &pos )
{
    kDebug();
    emit slaveHeaderContextMenuRequested( pos );
    emit headerContextMenuRequested( pos );
}

void DoubleTreeViewBase::slotLeftHeaderContextMenuRequested( const QPoint &pos )
{
    kDebug();
    emit masterHeaderContextMenuRequested( pos );
    emit headerContextMenuRequested( pos );
}

bool DoubleTreeViewBase::loadContext( const KoXmlElement &element )
{
    kDebug()<<endl;
    QList<int> lst1;
    QList<int> lst2;
    KoXmlElement e = element.namedItem( "master" ).toElement();
    if ( ! e.isNull() ) {
        m_leftview->loadContext( e );
    }
    e = element.namedItem( "slave" ).toElement();
    if ( ! e.isNull() ) {
        m_rightview->loadContext( e );
        if ( e.attribute( "hidden", "false" ) == "true" ) {
            setViewSplitMode( false );
        }
    }
    return true;
}

void DoubleTreeViewBase::saveContext( QDomElement &element ) const
{
    QDomElement e = element.ownerDocument().createElement( "master" );
    element.appendChild( e );
    m_leftview->saveContext( e );
    e = element.ownerDocument().createElement( "slave" );
    element.appendChild( e );
    if ( m_rightview->isHidden() ) {
        e.setAttribute( "hidden", "true" );
    }
    m_rightview->saveContext( e );
}

void DoubleTreeViewBase::setViewSplitMode( bool split )
{
    m_actionSplitView->setText( split ? i18n( "Unsplit View" ) : i18n( "Split View" ) );
    
    if ( m_mode == split ) {
        return;
    }

    m_mode = split;
    kDebug()<<split;
    if ( split ) {
        m_leftview->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        m_leftview->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
        if ( model() ) {
            m_rightview->setColumnHidden( 0, true );
            m_leftview->resizeColumnToContents( 0 );
            for ( int c = 1; c < m_rightview->model()->columnCount(); ++c ) {
                if ( m_leftview->isColumnHidden( c ) ) {
                    m_rightview->setColumnHidden( c, true );
                } else {
                    m_rightview->setColumnHidden( c, false );
                    m_rightview->mapToSection( c, m_leftview->section( c ) );
                    m_leftview->setColumnHidden( c, true );
                    m_rightview->resizeColumnToContents( c );
                }
            }
        }
        m_rightview->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
        m_rightview->show();
    } else {
        m_rightview->hide();
        if ( model() ) {
            int offset = m_rightview->isColumnHidden( 0 ) ? 1 : 0;
            for ( int c = 0; c < model()->columnCount(); ++c ) {
                if ( ! m_rightview->isColumnHidden( c ) ) {
                    m_leftview->setColumnHidden( c, false );
                    m_leftview->mapToSection( c, m_rightview->section( c ) + 1 );
                    m_leftview->resizeColumnToContents( c );
                }
            }
        }
        m_leftview->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
        m_leftview->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    }
}


} // namespace KPlato

#include "kptviewbase.moc"
