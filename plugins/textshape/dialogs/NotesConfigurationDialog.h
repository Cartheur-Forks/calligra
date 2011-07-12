/* This file is part of the KDE project
 * Copyright (C) 2011 Brijesh Patel <brijesh3105@gmail.com>
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
#ifndef NOTESCONFIGURATIONDIALOG_H
#define NOTESCONFIGURATIONDIALOG_H

#include <ui_NotesConfigurationDialog.h>
#include <KoListStyle.h>
#include <KoOdfNotesConfiguration.h>

#include <QDialog>
#include <QTextDocument>

class KoStyleManager;

class NotesConfigurationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NotesConfigurationDialog(QTextDocument *doc,QWidget *parent = 0);
    Ui::NotesConfigurationDialog widget;
    KoOdfNotesConfiguration *notesConfiguration( void );

public slots:
    void setStyleManager(KoStyleManager *sm);
    void footnoteSetup( bool on );
    void endnoteSetup( bool on );
    void apply(QAbstractButton*);

signals:
    void doneWithFocus();

private:

    KoOdfNotesConfiguration *notesConfig;
    KoStyleManager *m_styleManager;
    QTextDocument *document;
};

#endif
