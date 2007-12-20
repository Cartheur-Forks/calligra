/*
 *  Copyright (c) 2005 Bart Coppens <kde@bartcoppens.be>
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

#ifndef KIS_CUSTOM_BRUSH_H_
#define KIS_CUSTOM_BRUSH_H_

#include <QObject>
#include <QShowEvent>

#include "ui_wdgcustombrush.h"
#include "kis_resourceserver_adapter.h"

class KoResource;
class KisView2;

class KisWdgCustomBrush : public QWidget, public Ui::KisWdgCustomBrush
{
    Q_OBJECT

    public:
        KisWdgCustomBrush(QWidget *parent, const char *name) : QWidget(parent) { setObjectName(name); setupUi(this); }
};

class KisCustomBrush : public KisWdgCustomBrush
{
    Q_OBJECT
public:
    KisCustomBrush(QWidget *parent, const char* name, const QString& caption, KisView2* view);
    virtual ~KisCustomBrush();

public slots:
    void slotUseBrush();

signals:
    void activatedResource(KoResource *);

protected:
    virtual void showEvent(QShowEvent *);

private slots:
    void slotExport();
    void slotAddPredefined();
    void slotUpdateCurrentBrush(int); // To connect with activated(int)

private:
    void createBrush();
    KisView2* m_view;
    KisBrush* m_brush;
    KisResourceServerAdapter<KisBrush>* m_rServerAdapter;
};


#endif // KIS_CUSTOM_BRUSH_H_
