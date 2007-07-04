/*
 *  Copyright (c) 2007 Cyrille Berger (cberger@cberger.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "together.h"

#include <stdlib.h>

#include <QApplication>

#include <kaction.h>
#include <kactioncollection.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <kis_action_recorder.h>
#include <kis_config.h>
#include <kis_cursor.h>
#include <kis_global.h>
#include <kis_image.h>
#include <kis_recorded_action.h>
#include <kis_types.h>
#include <kis_view2.h>

typedef KGenericFactory<TogetherPlugin> TogetherPluginFactory;
K_EXPORT_COMPONENT_FACTORY( kritatogether, TogetherPluginFactory( "krita" ) )


TogetherPlugin::TogetherPlugin(QObject *parent, const QStringList &)
    : KParts::Plugin(parent)
{
    if ( parent->inherits("KisView2") )
    {
        m_view = (KisView2*) parent;

        setComponentData(TogetherPluginFactory::componentData());

        setXMLFile(KStandardDirs::locate("data","kritaplugins/together.rc"), true);

        // Replay recording action
        KAction* action  = new KAction(i18n("Replay"), this);
        actionCollection()->addAction("Recording_Replay", action );
        connect(action, SIGNAL(triggered()), this, SLOT(slotReplay()));
    }
}

TogetherPlugin::~TogetherPlugin()
{
    m_view = 0;
}

void TogetherPlugin::slotReplay()
{
    KisActionRecorder* actionRecorder = m_view->image()->actionRecorder();
    QList<KisRecordedAction*> actions = actionRecorder->actions();
    for( QList<KisRecordedAction*>::iterator it = actions.begin();
         it != actions.end(); ++it)
    {
        (*it)->play();
    }
}

#include "together.moc"
