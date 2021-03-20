/* This file is part of the KDE project
 * Copyright (C) 2007 Thomas Zander <zander@kde.org>
 * Copyright (C) 2008 Fredy Yanardi <fyanardi@gmail.com>
 * Copyright (C) 2008 Thorsten Zachmann <zachmann@kde.org>
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

#include "KoFind.h"
#include "KoFind_p.h"

#include <KoCanvasResourceManager.h>
#include <kactioncollection.h>
#include <QAction>


KoFind::KoFind(QWidget *parent, KoCanvasResourceManager *canvasResourceManager, KActionCollection *ac)
        : QObject(parent)
        , d(new KoFindPrivate(this, canvasResourceManager, parent))
{
    connect(canvasResourceManager, &KoCanvasResourceManager::canvasResourceChanged,
            this, [this] (int a, const QVariant &b) {
                d->resourceChanged(a, b);
            });
    ac->addAction(KStandardAction::Find, "edit_find", this, SLOT(findActivated()));
    d->findNext = ac->addAction(KStandardAction::FindNext, "edit_findnext", this, SLOT(findNextActivated()));
    d->findNext->setEnabled(false);
    d->findPrev = ac->addAction(KStandardAction::FindPrev, "edit_findprevious", this, SLOT(findPreviousActivated()));
    d->findPrev->setEnabled(false);
    ac->addAction(KStandardAction::Replace, "edit_replace", this, SLOT(replaceActivated()));
}

KoFind::~KoFind()
{
    delete d;
}

//have to include this because of Q_PRIVATE_SLOT
#include "moc_KoFind.cpp"
