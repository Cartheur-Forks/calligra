/*
 *  Copyright (c) 2003 Patrick Julien <freak@codepimps.org>
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
#ifndef KIS_RESOURCE_MEDIATOR_H_
#define KIS_RESOURCE_MEDIATOR_H_

#include <QObject>
#include <QMap>
#include <QWidget>

class QTableWidgetItem;
class KisItemChooser;
class KisIconItem;
class KoResource;
class KisResourceServerBase;

/**
 * A resource mediator manages access to resources like
 * gradients. brushes, patterns and palettes.
 * For every view, a new resource mediator is created for every
 * resource type.
 */
class KisResourceMediator : public QObject {
    Q_OBJECT
    typedef QObject super;

public:
    KisResourceMediator(KisItemChooser *chooser,
                QObject *parent = 0,
                const char *name = 0);
    virtual ~KisResourceMediator();

public:
    void connectServer(KisResourceServerBase* rServer);
    KoResource *currentResource() const;
    KisIconItem *itemFor(KoResource *r) const;
    KoResource *resourceFor(QTableWidgetItem *item) const;
    KoResource *resourceFor(KisIconItem *item) const;
    QWidget *chooserWidget() const;

public slots:
    
    void setActiveItem(QTableWidgetItem *item);

signals:
    void activatedResource(KoResource *r);

private slots:
    void rServerAddedResource(KoResource *resource);

private:
    KisItemChooser *m_chooser;
    QMap<KoResource*, KisIconItem*> m_items;
    QTableWidgetItem *m_activeItem;
};

#endif // KIS_RESOURCE_MEDIATOR_H_

