/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <qcombobox.h>
#include <klocale.h>
#include <qbuttongroup.h>

#include "kis_colorspace_registry.h"
#include "kis_types.h"
#include "kis_profile.h"
#include "kis_abstract_colorspace.h"
#include "kis_dlg_apply_profile.h"
#include "kis_config.h"
#include "kis_id.h"
#include "kis_cmb_idlist.h"

// XXX: Hardcode RGBA name. This should be a constant, somewhere.
KisDlgApplyProfile::KisDlgApplyProfile(QWidget *parent, const char *name)
    : super(parent, name, true, "", Ok | Cancel)
{

    setCaption(i18n("Apply Image Profile to Clipboard Data"));
    m_page = new WdgApplyProfile(this);

    setMainWidget(m_page);
    resize(m_page -> sizeHint());

    // XXX: This is BAD! (bsar)
    fillCmbProfiles(KisID("RGBA", ""));
    KisConfig cfg;
    m_page -> grpRenderIntent -> setButton(cfg.renderIntent());

}

KisDlgApplyProfile::~KisDlgApplyProfile()
{
    delete m_page;
}


KisProfileSP KisDlgApplyProfile::profile() const
{
    QString profileName;

    if (m_page -> cmbProfile -> currentItem() == 0) {
        // Use import profile
        KisConfig cfg;
        profileName = cfg.importProfile();
    } else {
        profileName = m_page -> cmbProfile -> currentText();
    }
    
    return KisColorSpaceRegistry::instance()->getProfileByName(profileName);
}

int KisDlgApplyProfile::renderIntent() const
{
    return m_page -> grpRenderIntent -> selectedId();
}


// XXX: Copy & paste from kis_dlg_create_img -- refactor to separate class
void KisDlgApplyProfile::fillCmbProfiles(const KisID & s)
{

    KisAbstractColorSpace * cs = KisColorSpaceRegistry::instance() -> get(s);
    m_page -> cmbProfile -> clear();
    m_page -> cmbProfile -> insertItem(i18n("None"));
    vKisProfileSP profileList = cs -> profiles();
        vKisProfileSP::iterator it;
        for ( it = profileList.begin(); it != profileList.end(); ++it ) {
        m_page -> cmbProfile -> insertItem((*it) -> productName());
    }
    

}

#include "kis_dlg_apply_profile.moc"

