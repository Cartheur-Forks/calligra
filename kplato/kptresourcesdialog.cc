/* This file is part of the KDE project
   Copyright (C) 2005 Dag Andersen <danders@get2net.dk>

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

#include <kdebug.h>

#include "kptresourcesdialog.h"
#include "kptproject.h"
#include "kptresourcespanel.h"
#include "kptresource.h"
#include "kptcommand.h"

namespace KPlato
{

ResourcesDialog::ResourcesDialog(Project &p, QWidget *parent)
    : KDialog( parent),
      project(p)
{
    setCaption( i18n("Resources") );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );
    showButtonSeparator( true );
    panel = new ResourcesPanel(this, &project);

    setMainWidget(panel);
    enableButtonOk(false);

    connect(this, SIGNAL(okClicked()), SLOT(slotOk()));
    connect(panel, SIGNAL(changed()), SLOT(slotChanged()));
}


void ResourcesDialog::slotChanged() {
    enableButtonOk(true);
}

void ResourcesDialog::slotOk() {
    if (!panel->ok())
        return;
    accept();
}

MacroCommand *ResourcesDialog::buildCommand(Part *part) {
    MacroCommand *m = 0;
    QString c = i18n("Modify resources");
    MacroCommand *cmd = panel->buildCommand(part);
    if (cmd) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(cmd);
    }
    return m;
}

}  //KPlato namespace

#include "kptresourcesdialog.moc"
