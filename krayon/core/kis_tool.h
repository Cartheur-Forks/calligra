/*
 *  kis_tool.h - part of KImageShop
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __kis_tool_h__
#define __kis_tool_h__

#include <qobject.h>
#include <qcursor.h>
#include <qpixmap.h>
#include <qimage.h>

#include "kis_brush.h"

class KisDoc;
class KisView;

class KisTool : public QObject
{
  Q_OBJECT

 public:
    KisTool(KisDoc *doc, KisView *view = 0L);
    ~KisTool();

    QString toolName();
    QCursor cursor();

    virtual void setCursor(const QCursor& cursor);
    virtual void setCursor();
    virtual void setBrush(KisBrush *b);
    virtual void optionsDialog();
    virtual void clearOld(){}

    void setSelectCursor();
    void setMoveCursor();
    
 public slots:

    virtual void mousePress(QMouseEvent*); 
    virtual void mouseMove(QMouseEvent*);
    virtual void mouseRelease(QMouseEvent*);
    
 protected:

    int zoomed(int n);
    int zoomedX(int n);  
    int zoomedY(int n);
    QPoint zoomed(QPoint & point);

    QRect getDrawRect( QPointArray & points );
    QPointArray zoomPointArray( QPointArray & points );

    void setClipImage();
    void dragSelectImage( QPoint dragPoint, QPoint m_hotSpot );
    bool pasteClipImage( QPoint pos );
  
    KisDoc  *m_pDoc;
    KisView *m_pView;
    QCursor  m_Cursor;

    QPixmap clipPixmap;
    QImage  clipImage;
};

#endif

