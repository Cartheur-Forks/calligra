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

#include "kptdocumentspanel.h"

#include "kptdocumentmodel.h"
#include "kptnode.h"
#include "kptcommand.h"

#include <QDialog>
#include <QList>
#include <QString>

#include <klocale.h>
#include <kurlrequesterdialog.h>
#include <kmessagebox.h>

#include <kdebug.h>

namespace KPlato
{

DocumentsPanel::DocumentsPanel( Node &node, QWidget *parent )
    : QWidget( parent ),
    m_node( node ),
    m_docs( node.documents() )
{
    widget.setupUi( this );

    currentChanged( QModelIndex() );
    
    foreach ( Document *doc, m_docs.documents() ) {
        m_orgurl.insert( doc, doc->url() );
    }
    
    DocumentItemModel *m = new DocumentItemModel( 0, this );
    m->setDocuments( &m_docs );
    widget.itemView->setModel( m );

    connect( widget.pbAdd, SIGNAL( clicked() ), SLOT( slotAddUrl() ) );
    connect( widget.pbChange, SIGNAL( clicked() ), SLOT( slotChangeUrl() ) );
    connect( widget.pbRemove, SIGNAL( clicked() ), SLOT( slotRemoveUrl() ) );
    connect( widget.pbView, SIGNAL( clicked() ), SLOT( slotViewUrl() ) );
    
    connect( widget.itemView->selectionModel(), SIGNAL( currentChanged ( const QModelIndex&, const QModelIndex& ) ), SLOT( currentChanged( const QModelIndex& ) ) );
}

DocumentItemModel *DocumentsPanel::model() const
{
    return static_cast<DocumentItemModel*>( widget.itemView->model() );
}

void DocumentsPanel::currentChanged( const QModelIndex &index ) const
{
    widget.pbChange->setEnabled( index.isValid() );
    widget.pbRemove->setEnabled( index.isValid() );
    widget.pbView->setEnabled( index.isValid() );
}

Document *DocumentsPanel::currentDocument() const
{
    return model()->document( widget.itemView->selectionModel()->currentIndex() );
}

void DocumentsPanel::slotAddUrl()
{
    KUrlRequesterDialog *dlg = new KUrlRequesterDialog( QString(), QString(), this );
    dlg->setWindowTitle( i18n( "Attach Document" ) );
    if ( dlg->exec() == QDialog::Accepted ) {
        if ( m_docs.findDocument( dlg->selectedUrl() ) ) {
            kWarning()<<"Document (url) already exists: "<<dlg->selectedUrl();
            KMessageBox::sorry( this, i18n( "Document url already attached: %1", dlg->selectedUrl().prettyUrl() ), i18n( "Cannot Attach Document" ) );
        } else {
            Document *doc = new Document( dlg->selectedUrl() );
            //DocumentAddCmd *cmd = new DocumentAddCmd( m_docs, doc, i18n( "Add Document" ) );
            //m_cmds.push( cmd );
            m_docs.addDocument( doc );
            m_state.insert( doc, Added );
            model()->setDocuments( &m_docs );
            emit changed();
        }
    }
    delete dlg;
}

void DocumentsPanel::slotChangeUrl()
{
    Document *doc = currentDocument();
    if ( doc == 0 ) {
        return slotAddUrl();
    }
    KUrlRequesterDialog *dlg = new KUrlRequesterDialog( doc->url().url(), QString(), this );
    dlg->setWindowTitle( i18n( "Modify Url" ) );
    if ( dlg->exec() == QDialog::Accepted ) {
        if ( doc->url() != dlg->selectedUrl() ) {
            if ( m_docs.findDocument( dlg->selectedUrl() ) ) {
                kWarning()<<"Document url already exists";
                KMessageBox::sorry( this, i18n( "Document url already exists: %1", dlg->selectedUrl().prettyUrl() ), i18n( "Cannot Modify Url" ) );
            } else {
                kDebug()<<"Modify url: "<<doc->url()<<" : "<<dlg->selectedUrl();
                doc->setUrl( dlg->selectedUrl() );
                m_state.insert( doc, (State)( m_state[ doc ] | Modified ) );
                model()->setDocuments( &m_docs );
                emit changed();
                kDebug()<<"State: "<<doc->url()<<" : "<<m_state[ doc ];
            }
        }
    }
    delete dlg;
}

void DocumentsPanel::slotRemoveUrl()
{
    Document *doc = currentDocument();
    if ( doc == 0 ) {
        return;
    }
    m_docs.takeDocument( doc );
    if ( m_state.contains( doc ) && m_state[ doc ] & Added ) {
        m_state.remove( doc );
    } else {
        m_state.insert( doc, Removed );
    }
    model()->setDocuments( &m_docs );
    emit changed();
}

void DocumentsPanel::slotViewUrl()
{
}

MacroCommand *DocumentsPanel::buildCommand()
{
    if ( m_docs == m_node.documents() ) {
        kDebug()<<"No changes to save";
        return 0;
    }
    Documents &docs = m_node.documents();
    Document *d = 0;
    QString txt = i18n( "Modify Documents" );
    MacroCommand *m = 0;
    QMap<Document*, State>::const_iterator i = m_state.constBegin();
    for ( ; i != m_state.constEnd(); ++i) {
        kDebug()<<i.value();
        if ( i.value() &  Removed ) {
            if ( m == 0 ) m = new MacroCommand( txt );
            kDebug()<<"remove document "<<i.key();
            //m->addCommand( new DocumentRemoveCmd( docs, i.key(), i18n( "Remove Document" ) ) );
            kDebug()<<1;
        } else if ( ( i.value() & Added ) == 0 && i.value()  &  Modified ) {
            // do plain modifications before additions
            d = docs.findDocument( m_orgurl[ i.key() ] );
            Q_ASSERT( d );
            kDebug()<<"modify document "<<d;
            if ( i.key()->url() != d->url() ) {
                if ( m == 0 ) m = new MacroCommand( txt );
                m->addCommand( new DocumentModifyUrlCmd( d, i.key()->url(), i18n( "Modify Document Url" ) ) );
            }
            if ( i.key()->type() != d->type() ) {
                if ( m == 0 ) m = new MacroCommand( txt );
                m->addCommand( new DocumentModifyTypeCmd( d, i.key()->type(), i18n( "Modify Document Type" ) ) );
            }
            if ( i.key()->status() != d->status() ) {
                if ( m == 0 ) m = new MacroCommand( txt );
                m->addCommand( new DocumentModifyStatusCmd( d, i.key()->status(), i18n( "Modify Document Status" ) ) );
            }
            kDebug()<<2;
        } else if ( i.value() &  Added ) {
            if ( m == 0 ) m = new MacroCommand( txt );
            kDebug()<<i.key()<<m_docs.documents();
            d = m_docs.takeDocument( i.key() );
            kDebug()<<"add document "<<d;
            m->addCommand( new DocumentAddCmd( docs, d, i18n( "Add Document" ) ) );
            kDebug()<<2;
        }
    }
    kDebug()<<3;
    return m;
}


} //namespace KPlato

#include "kptdocumentspanel.h"
