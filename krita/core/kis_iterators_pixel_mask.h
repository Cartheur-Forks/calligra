/* This file is part of the KDE project
   Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KIS_ITERATORS_PIXEL_MASK_H_
#define _KIS_ITERATORS_PIXEL_MASK_H_

#include "kis_iterators.h"

class KisIteratorPixelMask : public KisIteratorUnit
{
public:
	KisIteratorPixelMask( KisPaintDeviceSP ndevice, KisTileCommand* command, Q_INT32 nypos = 0, Q_INT32 nxpos = 0);
	virtual ~KisIteratorPixelMask() {}
public:
	inline operator KisPixel();
	inline KisPixelRO oldValue();
	inline KisQuantum operator[](int index);
private:
};


/**
 * XXX: document
 */
class KisIteratorLinePixelMask : public KisIteratorLine<KisIteratorPixel>
{
public:
	KisIteratorLinePixelMask( KisPaintDeviceSP ndevice, 
			      KisTileCommand* command, 
			      Q_INT32 nypos = 0,
			      Q_INT32 nxstart = -1, 
			      Q_INT32 nxend = -1);
public:
	virtual KisIteratorPixel operator*();
	virtual operator KisIteratorPixel* ();
	virtual KisIteratorPixel begin();
	virtual KisIteratorPixel end();
};

#endif
