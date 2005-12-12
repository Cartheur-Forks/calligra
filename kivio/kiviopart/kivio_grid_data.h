/*
 * Kivio - Visual Modelling and Flowcharting
 * Copyright (C) 2000-2001 theKompany.com & Dave Marotti
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef KIVIO_GRID_DATA_H
#define KIVIO_GRID_DATA_H

#include <KoSize.h>
#include <qcolor.h>
#include <koffice_export.h>

class QDomElement;

class KIVIO_EXPORT KivioGridData
{
public:
  KivioGridData();
  ~KivioGridData();

  void save(QDomElement&, const QString&);
  void load(const QDomElement&, const QString&);

  KoSize freq;
  KoSize snap;
  QColor color;
  bool isSnap;
  bool isShow;
};

#endif

