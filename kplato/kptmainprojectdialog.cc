/* This file is part of the KDE project
   Copyright (C) 2003 - 2005 Dag Andersen <danders@get2net.dk>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include <klocale.h>
#include <kcommand.h>

#include <kdebug.h>

#include "kptmainprojectdialog.h"
#include "kptproject.h"
#include "kptmainprojectpanel.h"

namespace KPlato
{

MainProjectDialog::MainProjectDialog(Project &p, QWidget *parent, const char *name)
    : KDialog( parent),
      project(p)
{
    setCaption( i18n("Project Settings") );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );
    showButtonSeparator( true );
    panel = new MainProjectPanel(project, this);

    setMainWidget(panel);
    enableButtonOk(false);
    resize( QSize(500, 410).expandedTo(minimumSizeHint()));
    
    connect(this, SIGNAL(okClicked()), SLOT(slotOk()));
    connect(panel, SIGNAL(obligatedFieldsFilled(bool)), SLOT(enableButtonOk(bool)));
}


void MainProjectDialog::slotOk() {
    if (!panel->ok())
        return;

    accept();
}

KCommand *MainProjectDialog::buildCommand(Part *part) {
    KMacroCommand *m = 0;
    QString c = i18n("Modify main project");
    KCommand *cmd = panel->buildCommand(part);
    if (cmd) {
        if (!m) m = new KMacroCommand(c);
        m->addCommand(cmd);
    }
    return m;
}

}  //KPlato namespace

#include "kptmainprojectdialog.moc"
