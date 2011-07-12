/* This file is part of the KDE project
 * Copyright (C) 2011 Casper Boemann <cbo@boemann.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "ReferencesTool.h"
#include "TextShape.h"
#include "dialogs/SimpleTableOfContentsWidget.h"
#include "dialogs/SimpleCitationWidget.h"
#include "dialogs/SimpleFootEndNotesWidget.h"
#include "dialogs/SimpleCaptionsWidget.h"
#include "dialogs/NotesConfigurationDialog.h"

#include <KoTextLayoutRootArea.h>
#include <KoCanvasBase.h>
#include <KoTextEditor.h>
#include <KoBookmark.h>
#include <KoInlineNote.h>
#include <KoTextDocumentLayout.h>
#include <KoInlineTextObjectManager.h>

#include <kdebug.h>

#include <KLocale>
#include <KAction>
#include <QTextDocument>


ReferencesTool::ReferencesTool(KoCanvasBase* canvas): TextTool(canvas)
{
    createActions();
}

ReferencesTool::~ReferencesTool()
{
}

void ReferencesTool::createActions()
{
    KAction *action = new KAction(i18n("Insert"), this);
    KAction *action1;
    KAction *action2;
    KAction *action3;

    action = new KAction(i18n("Add ToC"), this);
    addAction("insert_tableofcentents", action);
    action->setToolTip(i18n("Insert a Table of Contents into the document."));
    connect(action, SIGNAL(triggered()), this, SLOT(insertTableOfContents()));

    action = new KAction(i18n("Configure..."), this);
    addAction("format_tableofcentents", action);
    action->setToolTip(i18n("Configure the Table of Contents"));
    connect(action, SIGNAL(triggered()), this, SLOT(formatTableOfContents()));

    action = new KAction(i18n("Footnote"),this);
    addAction("insert_footnote",action);
    action->setToolTip(i18n("Insert a Foot Note into the document."));
    connect(action, SIGNAL(triggered()), this, SLOT(insertFootNote()));

    action = new KAction(i18n("Endnote"),this);
    addAction("insert_endnote",action);
    action->setToolTip(i18n("Insert an End Note into the document."));
    connect(action, SIGNAL(triggered()), this, SLOT(insertEndNote()));

    action = new KAction(this);
    QIcon *icon = new QIcon("/home/brijesh/kde/src/calligra/plugins/textshape/pics/settings-icon1_1.png");
    action->setIcon( *icon );
    addAction("notes_settings",action);
    action->setToolTip(i18n("Footnote/Endnote Settings"));
    connect(action, SIGNAL(triggered()), this, SLOT(openSettings()));
}

void ReferencesTool::activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes)
{
    TextTool::activate(toolActivation, shapes);
}

void ReferencesTool::deactivate()
{
    TextTool::deactivate();
    canvas()->canvasWidget()->setFocus();
}

QList<QWidget*> ReferencesTool::createOptionWidgets()
{
    QList<QWidget *> widgets;
    stocw = new SimpleTableOfContentsWidget(this, 0);
    //SimpleCitationWidget *scw = new SimpleCitationWidget(0);
    sfenw = new SimpleFootEndNotesWidget(this,0);
    //SimpleCaptionsWidget *scapw = new SimpleCaptionsWidget(0);

    // Connect to/with simple table of contents option widget
    connect(stocw, SIGNAL(doneWithFocus()), this, SLOT(returnFocusToCanvas()));

    // Connect to/with simple citation index option widget
    //connect(scw, SIGNAL(doneWithFocus()), this, SLOT(returnFocusToCanvas()));

    // Connect to/with simple citation index option widget
    connect(sfenw, SIGNAL(doneWithFocus()), this, SLOT(returnFocusToCanvas()));

    stocw->setWindowTitle(i18n("Table of Contents"));
    widgets.append(stocw);
    sfenw->setWindowTitle(i18n("Footnotes & Endnotes"));
    widgets.append(sfenw);
    //widgets.insert(i18n("Citations"), scw);
    //widgets.insert(i18n("Captions"), scapw);
    return widgets;
}

void ReferencesTool::insertFootNote()
{
    connect(textEditor()->document(),SIGNAL(cursorPositionChanged(QTextCursor)),this,SLOT(disableButtons(QTextCursor)));
    note = textEditor()->insertFootNote();
    note->setAutoNumbering(sfenw->widget.autoNumbering->isChecked());
    if(note->autoNumbering()) {
        note->setLabel(QString().number(note->manager()->footNotes().count()));
    }
    else {
        note->setLabel(sfenw->widget.characterEdit->text());
    }

    QTextCursor cursor(note->textCursor());
    QString s;
    s.append("Foot");
    s.append(note->label());
    KoBookmark *bookmark = new KoBookmark(note->textFrame()->document());
    bookmark->setType(KoBookmark::SinglePosition);
    bookmark->setName(s);
    note->manager()->insertInlineObject(cursor, bookmark);
    QTextCharFormat *fmat = new QTextCharFormat();
    fmat->setVerticalAlignment(QTextCharFormat::AlignSuperScript);
    cursor.insertText(note->label(),*fmat);

    fmat->setVerticalAlignment(QTextCharFormat::AlignNormal);
    cursor.insertText(" ",*fmat);

}

void ReferencesTool::insertEndNote()
{
    note = textEditor()->insertEndNote();
    note->setAutoNumbering(sfenw->widget.autoNumbering->isChecked());
    if(note->autoNumbering()) {
        note->setLabel(note->toRoman(note->manager()->endNotes().count()));
    }
    else {
        note->setLabel(sfenw->widget.characterEdit->text());
    }

    QTextCursor cursor(note->textCursor());

    QString s;
    s.append("End");
    s.append(note->label());
    KoBookmark *bookmark = new KoBookmark(note->textFrame()->document());
    bookmark->setType(KoBookmark::SinglePosition);
    bookmark->setName(s);
    note->manager()->insertInlineObject(cursor, bookmark);

    QTextCharFormat *fmat = new QTextCharFormat();
    fmat->setVerticalAlignment(QTextCharFormat::AlignSuperScript);
    cursor.insertText(note->label(),*fmat);
    fmat->setVerticalAlignment(QTextCharFormat::AlignNormal);
    cursor.insertText(" ",*fmat);
}

void ReferencesTool::openSettings()
{
    NotesConfigurationDialog *dialog = new NotesConfigurationDialog(textEditor()->document(),0);
    dialog->exec();
}

void ReferencesTool::disableButtons(QTextCursor cursor)
{
    if(cursor.currentFrame()->format().intProperty(KoText::SubFrameType) == KoText::NoteFrameType) {
        sfenw->widget.addFootnote->setEnabled(false);
        sfenw->widget.addEndnote->setEnabled(false);
    } else {
        sfenw->widget.addFootnote->setEnabled(true);
        sfenw->widget.addEndnote->setEnabled(true);
    }
}

void ReferencesTool::insertTableOfContents()
{
    textEditor()->insertTableOfContents();
}

void ReferencesTool::formatTableOfContents()
{
    connect(textEditor()->document(),SIGNAL(cursorPositionChanged(QTextCursor)),this,SLOT(disableButtons(QTextCursor)));
    note = textEditor()->insertFootNote();
    note->setAutoNumbering(sfenw->widget.autoNumbering->isChecked());
    if(note->autoNumbering()) {
        note->setLabel(QString().number(note->manager()->footNotes().count()));
    }
    else {
        note->setLabel(sfenw->widget.characterEdit->text());
    }

    QTextCursor cursor(note->textCursor());
    QString s;
    s.append("Foot");
    s.append(note->label());
    KoBookmark *bookmark = new KoBookmark(note->textFrame()->document());
    bookmark->setType(KoBookmark::SinglePosition);
    bookmark->setName(s);
    note->manager()->insertInlineObject(cursor, bookmark);
    QTextCharFormat *fmat = new QTextCharFormat();
    fmat->setVerticalAlignment(QTextCharFormat::AlignSuperScript);
    cursor.insertText(note->label(),*fmat);

    fmat->setVerticalAlignment(QTextCharFormat::AlignNormal);
    cursor.insertText(" ",*fmat);

}

void ReferencesTool::insertEndNote()
{
    note = textEditor()->insertEndNote();
    note->setAutoNumbering(sfenw->widget.autoNumbering->isChecked());
    if(note->autoNumbering()) {
        note->setLabel(note->toRoman(note->manager()->endNotes().count()));
    }
    else {
        note->setLabel(sfenw->widget.characterEdit->text());
    }

    QTextCursor cursor(note->textCursor());

    QString s;
    s.append("End");
    s.append(note->label());
    KoBookmark *bookmark = new KoBookmark(note->textFrame()->document());
    bookmark->setType(KoBookmark::SinglePosition);
    bookmark->setName(s);
    note->manager()->insertInlineObject(cursor, bookmark);

    QTextCharFormat *fmat = new QTextCharFormat();
    fmat->setVerticalAlignment(QTextCharFormat::AlignSuperScript);
    cursor.insertText(note->label(),*fmat);

    fmat->setVerticalAlignment(QTextCharFormat::AlignNormal);
    cursor.insertText(" ",*fmat);

}

void ReferencesTool::openSettings()
{
    NotesConfigurationDialog *dialog = new NotesConfigurationDialog(textEditor()->document(),0);
    dialog->exec();
}

void ReferencesTool::disableButtons(QTextCursor cursor)
{
    if(cursor.currentFrame()->format().intProperty(KoText::SubFrameType) == KoText::NoteFrameType) {
        sfenw->widget.addFootnote->setEnabled(false);
        sfenw->widget.addEndnote->setEnabled(false);
    } else {
        sfenw->widget.addFootnote->setEnabled(true);
        sfenw->widget.addEndnote->setEnabled(true);
    }
}

#include <ReferencesTool.moc>
