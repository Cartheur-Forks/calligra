/*
 *  Copyright (c) 2004,2007 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_AUTOBRUSH_H_
#define _KIS_AUTOBRUSH_H_

#include <QObject>
#include <QResizeEvent>

#include "ui_wdgautobrush.h"
#include <kis_autobrush_resource.h>

class KisWdgAutobrush : public QWidget, public Ui::KisWdgAutobrush
{
    Q_OBJECT

    public:
        KisWdgAutobrush(QWidget *parent, const char *name) : QWidget(parent) { setObjectName(name); setupUi(this); }
};

class KisAutobrush : public KisWdgAutobrush
{
    Q_OBJECT
public:
    KisAutobrush(QWidget *parent, const char* name, const QString& caption);
    void activate();

signals:
    void activatedResource(KoResource *r);
    
private slots:
    void paramChanged();
    void spinBoxWidthChanged(int );
    void spinBoxHeigthChanged(int );
    void spinBoxHorizontalChanged(int);
    void spinBoxVerticalChanged(int);
    void linkSizeToggled(bool);
    void linkFadeToggled(bool);

protected:
    virtual void resizeEvent ( QResizeEvent * );

private:
    QImage m_brush;
    bool m_linkSize;
    bool m_linkFade;
};


#endif
