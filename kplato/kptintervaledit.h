/* This file is part of the KDE project
   Copyright (C) 2004 Dag Andersen <danders@get2net.dk>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KPTINTERVALEDIT_H
#define KPTINTERVALEDIT_H

#include "kptintervaleditbase.h"

#include <kdialogbase.h>

#include <qstring.h>
#include <qptrlist.h>
#include <qpair.h>

namespace KPlato
{

class KPTIntervalEditImpl : public KPTIntervalEditBase {
    Q_OBJECT
public:
    KPTIntervalEditImpl(const QPtrList<QPair<QTime, QTime> > &intervals, QWidget *parent);
    
    QPtrList<QPair<QTime, QTime> > intervals() const;
    
private slots:
    void slotCheckAllFieldsFilled();
    void slotEnableButtonOk(bool on);
    
    void slotClearClicked();
    void slotAddIntervalClicked();
    void slotIntervalSelectionChanged(QListViewItem *item);
signals:
    void obligatedFieldsFilled(bool yes);
    void enableButtonOk(bool);
    
};

class KPTIntervalEdit : public KDialogBase {
    Q_OBJECT
public:
    KPTIntervalEdit(const QPtrList<QPair<QTime, QTime> > &intervals, QWidget *parent=0, const char *name=0);
    
    QPtrList<QPair<QTime, QTime> > intervals() const;
    
private:
    KPTIntervalEditImpl *dia;
};

}  //KPlato namespace

#endif // KPTINTERVALEDIT_H
