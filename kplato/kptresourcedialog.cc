/* This file is part of the KDE project
   Copyright (C) 2003, 2004 Dag Andersen <danders@get2net.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation;
   version 2 of the License.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kptresourcedialog.h"
#include "kptproject.h"
#include "kptresource.h"
#include "kptcalendar.h"

#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qdatetimeedit.h>
#include <qdatetime.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qptrlist.h>
#include <qstringlist.h>

#include <kabc/addressee.h>
#include <kabc/addresseedialog.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kglobal.h>
#include <kdebug.h>

namespace KPlato
{

KPTResourceDialogImpl::KPTResourceDialogImpl (QWidget *parent)
    : ResourceDialogBase(parent)
{

    connect(type, SIGNAL(activated(int)), SLOT(slotChanged()));
    connect(units, SIGNAL(valueChanged(int)), SLOT(slotChanged()));
    connect(nameEdit, SIGNAL(textChanged(const QString&)), SLOT(slotChanged()));
    connect(initialsEdit, SIGNAL(textChanged(const QString&)), SLOT(slotChanged()));
    connect(emailEdit, SIGNAL(textChanged(const QString&)), SLOT(slotChanged()));

    connect(calendarList, SIGNAL(activated(int)), SLOT(slotChanged()));

    connect(rateEdit, SIGNAL(textChanged(const QString&)), SLOT(slotChanged()));
    connect(overtimeEdit, SIGNAL(textChanged(const QString&)), SLOT(slotChanged()));
    connect(fixedCostEdit, SIGNAL(textChanged(const QString&)), SLOT(slotChanged()));

    connect(chooseBtn, SIGNAL(clicked()), SLOT(slotChooseResource()));
    
    connect(editCalendarBtn, SIGNAL(clicked()), SLOT(slotEditCalendarClicked()));
}


void KPTResourceDialogImpl::slotChanged() {
    emit changed();
}

void KPTResourceDialogImpl::slotCalculationNeeded(const QString&) {
    emit calculate();
    emit changed();
}

void KPTResourceDialogImpl::slotChooseResource()
{
    KABC::Addressee a = KABC::AddresseeDialog::getAddressee(this);
    if (!a.isEmpty()) {
        nameEdit->setText(a.assembledName());
        emailEdit->setText(a.preferredEmail());
        QStringList l = QStringList::split(' ', a.assembledName());
        QString in;
        QStringList::Iterator it = l.begin();
        for (int i=0; it != l.end(); ++it) {
            in += (*it)[0];
        }
        initialsEdit->setText(in);
    }
}

void KPTResourceDialogImpl::slotEditCalendarClicked()
{
}

//////////////////  KPTResourceDialog  ////////////////////////

KPTResourceDialog::KPTResourceDialog(KPTProject &project, KPTResource &resource, QWidget *parent, const char *name)
    : KDialogBase( Swallow, i18n("Resource Settings"), Ok|Cancel, Ok, parent, name, true, true),
      m_resource(resource),
      m_calculationNeeded(false)
{
    dia = new KPTResourceDialogImpl(this);
    setMainWidget(dia);
    enableButtonOK(false);

    dia->nameEdit->setText(resource.name());
    dia->initialsEdit->setText(resource.initials());
    dia->emailEdit->setText(resource.email());
    dia->type->setCurrentItem((int)resource.type()); // NOTE: must match enum
    dia->units->setValue(resource.units());
    dia->rateEdit->setText(KGlobal::locale()->formatMoney(resource.normalRate()));
    dia->overtimeEdit->setText(KGlobal::locale()->formatMoney(resource.overtimeRate()));
    dia->fixedCostEdit->setText(KGlobal::locale()->formatMoney(resource.fixedCost()));

    int cal = 0;
    dia->calendarList->insertItem("None");
    m_calendars.insert(0, 0);      
    QPtrListIterator<KPTCalendar> cit(project.calendars());
    for(int i=1; cit.current(); ++cit, ++i) {
        dia->calendarList->insertItem(cit.current()->name(), i);
        m_calendars.insert(i, cit.current());
        if (cit.current() == resource.calendar())
            cal = i;
    }
    dia->calendarList->setCurrentItem(cal);

    connect(dia, SIGNAL(changed()), SLOT(enableButtonOk()));
    connect(dia, SIGNAL(calculate()), SLOT(slotCalculationNeeded()));
    connect(dia->calendarList, SIGNAL(activated(int)), SLOT(slotCalendarChanged(int)));
}


void KPTResourceDialog::enableButtonOk() {
    enableButtonOK(true);
}

void KPTResourceDialog::slotCalculationNeeded() {
    m_calculationNeeded = true;
}

void KPTResourceDialog::slotOk() {
    m_resource.setName(dia->nameEdit->text());
    m_resource.setInitials(dia->initialsEdit->text());
    m_resource.setEmail(dia->emailEdit->text());
    m_resource.setType((KPTResource::Type)(dia->type->currentItem()));
    m_resource.setUnits(dia->units->value());
    //FIXME: readMoney() can't read formatMoney() format!!
    m_resource.setNormalRate(KGlobal::locale()->readMoney(dia->rateEdit->text()));
    m_resource.setOvertimeRate(KGlobal::locale()->readMoney(dia->overtimeEdit->text()));
    m_resource.setFixedCost(KGlobal::locale()->readMoney(dia->fixedCostEdit->text()));
    m_resource.setCalendar(m_calendars[dia->calendarList->currentItem()]);
    
    accept();
}

void KPTResourceDialog::slotCalendarChanged(int cal) {
    
}

}  //KPlato namespace

#include "kptresourcedialog.moc"
