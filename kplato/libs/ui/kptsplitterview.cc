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
  Boston, MA 02110-1301, USA.
*/

#include "kptsplitterview.h"

#include "kptpart.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTabWidget>

namespace KPlato
{

SplitterView::SplitterView(Part *doc, QWidget *parent)
    : ViewBase( doc, parent ),
    m_activeview( 0 )
{
    QVBoxLayout *b = new QVBoxLayout( this );
    b->setContentsMargins( 0, 0, 0, 0 );
    m_splitter = new QSplitter( this );
    m_splitter->setOrientation( Qt::Vertical );
    b->addWidget( m_splitter );
}
    
QTabWidget *SplitterView::addTabWidget(  )
{
    QTabWidget *w = new QTabWidget( m_splitter );
    m_splitter->addWidget( w );
    return w;
}

void SplitterView::addView( ViewBase *view )
{
    m_splitter->addWidget( view );
    connect( view, SIGNAL( guiActivated( ViewBase*, bool ) ), this, SLOT( slotGuiActivated( ViewBase*, bool ) ) );
}

void SplitterView::addView( ViewBase *view, QTabWidget *tab, const QString &label )
{
    tab->addTab( view, label );
    connect( view, SIGNAL( guiActivated( ViewBase*, bool ) ), this, SLOT( slotGuiActivated( ViewBase*, bool ) ) );
}

// reimp
void SplitterView::setGuiActive( bool active ) // virtual slot
{
    if ( m_activeview ) {
        m_activeview->setGuiActive( active );
    } else {
        emit guiActivated( this, active );
    }
}

void SplitterView::slotGuiActivated( ViewBase *v, bool active )
{
    if ( active ) {
        if ( m_activeview ) {
            emit guiActivated( m_activeview, false );
        }
        m_activeview = v;
    } else {
        m_activeview = 0;
    }
    emit guiActivated( v, active );
}

ViewBase *SplitterView::findView( const QPoint &pos ) const
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *w = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( w && w->frameGeometry().contains( pos ) ) {
            return w;
        }
        QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
        if (tw && tw->frameGeometry().contains( pos ) ) {
            w = dynamic_cast<ViewBase*>( tw->currentWidget() );
            if ( w ) {
                return w;
            }
        }
    }
    return const_cast<SplitterView*>( this );
}

void SplitterView::setZoom(double zoom)
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *v = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( v ) {
            v->setZoom( zoom );
        } else {
            QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
            if (tw ) {
                for ( int j = 0; j < tw->count(); ++j ) {
                    v = dynamic_cast<ViewBase*>( tw->currentWidget() );
                    if ( v ) {
                        v->setZoom( zoom );
                    }
                }
            }
        }
    }
}
    
void SplitterView::setProject( Project *project )
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *v = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( v ) {
            v->setProject( project );
        } else {
            QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
            if (tw ) {
                for ( int j = 0; j < tw->count(); ++j ) {
                    v = dynamic_cast<ViewBase*>( tw->currentWidget() );
                    if ( v ) {
                        v->setProject( project );
                    }
                }
            }
        }
    }
}
    
void SplitterView::draw()
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *v = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( v ) {
            v->draw();
        } else {
            QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
            if (tw ) {
                for ( int j = 0; j < tw->count(); ++j ) {
                    v = dynamic_cast<ViewBase*>( tw->currentWidget() );
                    if ( v ) {
                        v->draw();
                    }
                }
            }
        }
    }
}

void SplitterView::draw( Project &project )
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *v = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( v ) {
            v->draw( project );
        } else {
            QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
            if (tw ) {
                for ( int j = 0; j < tw->count(); ++j ) {
                    v = dynamic_cast<ViewBase*>( tw->currentWidget() );
                    if ( v ) {
                        v->draw( project );
                    }
                }
            }
        }
    }
}


void SplitterView::updateReadWrite( bool mode )
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *v = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( v ) {
            v->updateReadWrite( mode );
        } else {
            QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
            if (tw ) {
                for ( int j = 0; j < tw->count(); ++j ) {
                    v = dynamic_cast<ViewBase*>( tw->currentWidget() );
                    if ( v ) {
                        v->updateReadWrite( mode );
                    }
                }
            }
        }
    }
}

QStringList SplitterView::actionListNames() const
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *v = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( v  ) {
            if ( v->hasFocus() ) {
                return v->actionListNames();
            }
            continue;
        } else {
            QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
            if (tw ) {
                v = dynamic_cast<ViewBase*>( tw->currentWidget() );
                if ( v && v->hasFocus() ) {
                    return v->actionListNames();
                }
                continue;
            }
        }
    }
    return QStringList();
}
    
QList<QAction*> SplitterView::actionList( const QString name ) const
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *v = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( v ) {
            if ( v->hasFocus() ) {
                return v->actionList( name );
            }
            continue;
        } else {
            QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
            if (tw ) {
                v = dynamic_cast<ViewBase*>( tw->currentWidget() );
                if ( v && v->hasFocus() ) {
                    return v->actionList( name );
                }
                continue;
            }
        }
    }
    return QList<QAction*>();
}
    
bool SplitterView::loadContext( const KoXmlElement &context )
{
    kDebug()<<endl;
    KoXmlElement e = context.namedItem( "views" ).toElement();
    if ( e.isNull() ) {
        return true;
    }
    foreach ( QString s, e.attributeNames() ) {
        ViewBase *v = findChildren<ViewBase*>( s ).first();
        if ( v == 0 ) {
            continue;
        }
        KoXmlElement e1 = e.namedItem( s ).toElement();
        if ( e1.isNull() ) {
            continue;
        }
        v->loadContext( e1 );
    }
    return true;
}

void SplitterView::saveContext( QDomElement &context ) const
{
    QList<ViewBase*> lst = findChildren<ViewBase*>();
    if ( lst.isEmpty() ) {
        return;
    }
    QDomElement e = context.ownerDocument().createElement( "views" );
    context.appendChild( e );
    foreach ( ViewBase *v, lst ) {
        e.setAttribute( v->objectName(), "" );
    }
    foreach ( ViewBase *v, lst ) {
        QDomElement e1 = e.ownerDocument().createElement( v->objectName() );
        e.appendChild( e1 );
        v->saveContext( e1 );
    }
}

} // namespace KPlato

#include "kptsplitterview.moc"
