/*
 *  kis_brush.h - part of KImageShop
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

#ifndef __kis_brush_h__
#define __kis_brush_h__

#include <qsize.h>

#include "iconitem.h"

class QPoint;
class QPixmap;

class KisBrush : public IconItem
{
 public:
  KisBrush(QString file);
  virtual ~KisBrush();

  void 	    setSpacing(int s)                 { m_spacing = s;     }
  int       spacing()   	      const { return m_spacing;  }
  bool      isValid()   	      const { return m_valid;    }
  void      setHotSpot(QPoint);
  QPoint    hotSpot()   	      const { return m_hotSpot;  }
  QPixmap&  pixmap();
  QSize     size()                    const { return QSize(m_w, m_h); } 
  int       width()                   const { return m_w; }
  int       height()                  const { return m_h; }
  uchar     value(int x, int y) const;
  uchar*    scanline(int) const;
  uchar*    bits() const;

  // debug
  void      dump() const;

 private:
  void      loadViaQImage(QString file);

  bool      m_valid;
  int       m_spacing;
  QPoint    m_hotSpot;

  int       m_w, m_h;
  uchar*    m_pData;

  QPixmap  *m_pPixmap;
};

#endif

