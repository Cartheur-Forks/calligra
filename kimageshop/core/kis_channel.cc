/*
 *  kis_channel.cc - part of KImageShop
 *
 *  Copyright (c) 1999 Andrew Richards <A.Richards@phys.canterbury.ac.nz>
 *                1999-2000 Matthias ELter  <elter@kde.org>
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

#include "kis_channel.h"

KisChannel::KisChannel(cId id) : m_id( id )
{
  m_xTiles = 0;
  m_yTiles = 0;
  m_tiles = 0;

  m_imgRect = m_tileRect = QRect(0, 0, 0, 0);
}

KisChannel::~KisChannel()
{
  // FIXME: delete data
}

void KisChannel::setPixel(uint x, uint y, uchar pixel)
{
  // no sanity checks for performance reasons!

  // find the point in tile coordinates
  x = x - m_tileRect.x();
  y = y - m_tileRect.y();
  
  // find the tile
  int tileNo = (y / TILE_SIZE) * m_xTiles + x / TILE_SIZE;
  
  // does the tile exist?
  if (m_tiles[tileNo] == 0)
    return;
  
  // get a pointer to the points tile data
  uchar *ptr = m_tiles[tileNo] + ((y % TILE_SIZE) * TILE_SIZE + x % TILE_SIZE);
  *ptr = pixel;
}

uchar KisChannel::pixel(uint x, uint y)
{
  // again no sanity checks for performance reasons!

  // find the point in tile coordinates
  x = x - m_tileRect.x();
  y = y - m_tileRect.y();
  
  // find the tile
  int tileNo = (y / TILE_SIZE) * m_xTiles + x / TILE_SIZE;

  // does the tile exist?
  if (m_tiles[tileNo] == 0)
    return(0); // FIXME: fix this return some sort of undef (or bg) via KisColor
  
  // get a pointer to the points tile data
  uchar *ptr = m_tiles[tileNo] + ((y % TILE_SIZE) * TILE_SIZE + x % TILE_SIZE);
  return *ptr;
}

uint KisChannel::lastTileOffsetX()
{
  uint lastTileXOffset = TILE_SIZE - ( m_tileRect.right() - m_imgRect.right());
  return((lastTileXOffset) ? lastTileXOffset :  TILE_SIZE);
}

uint KisChannel::lastTileOffsetY()
{
  uint lastTileYOffset = TILE_SIZE - (m_tileRect.bottom() - m_imgRect.bottom());
  return((lastTileYOffset) ? lastTileYOffset :  TILE_SIZE);
}

void KisChannel::moveBy(int dx, int dy)
{
  m_imgRect.moveBy(dx, dy);
  m_tileRect.moveBy(dx, dy);
}

void KisChannel::moveTo(int x, int y)
{
  int dx = x - m_imgRect.x();
  int dy = y - m_imgRect.y();
  m_imgRect.moveTopLeft(QPoint(x, y));
  m_tileRect.moveBy(dx,dy);
}

QRect KisChannel::tileRect(int tileNo)
{
  int xTile = tileNo % m_xTiles;
  int yTile = tileNo / m_xTiles;
  QRect tr(xTile * TILE_SIZE, yTile * TILE_SIZE, TILE_SIZE, TILE_SIZE);
  tr.moveBy(m_tileRect.x(), m_tileRect.y());
  return(tr);
}


// Resize the channel so that it includes the rectangle newRect (canvasCoords)
// and allocates space for all the pixels in newRect

void KisChannel::allocateRect(QRect newRect)
{
  if (newRect.isNull())
    return;
  
  if (m_imgRect.contains(newRect))
    return;
  
  if (m_tileRect.contains(newRect))
	{
	  m_imgRect = m_imgRect.unite(newRect);
	  return;
	}
  
  // make a newTileExtents rect which contains m_imgRect and newRect
  // now make it fall on the closest multiple of TILE_SIZE which contains it
  if (m_tileRect.isNull())
	m_tileRect=QRect(newRect.topLeft(), QSize(1, 1));

  QRect newTileExtents = m_tileRect;
  newTileExtents = newTileExtents.unite(newRect);

  if (newTileExtents.left() < m_tileRect.left())
	newTileExtents.setLeft(m_tileRect.left() - ((m_tileRect.left() - newTileExtents.left()
												 + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE);
  if (newTileExtents.top() < m_tileRect.top())
	newTileExtents.setTop(m_tileRect.top() - ((m_tileRect.top() - newTileExtents.top()
											   + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE);
	  
  newTileExtents.setWidth(((newTileExtents.width() + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE);
  newTileExtents.setHeight(((newTileExtents.height() + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE);
  
  // calculate new tile counts
  int newXTiles = newTileExtents.width() / TILE_SIZE;
  int newYTiles = newTileExtents.height() / TILE_SIZE;
	  
  // allocate new tile data pointers
  uchar **newData = new uchar* [newXTiles*newYTiles];

  // zero new tile pointers
  for(int yTile = 0; yTile < newYTiles; yTile++) {
	for(int xTile = 0; xTile < newXTiles; xTile++) {
	  newData[yTile * newXTiles + xTile]=0;
	}
  }
  
  // these are where the old tiles start in the new tile block
  int oldXTilePos = (m_tileRect.left() - newTileExtents.left()) / TILE_SIZE;
  int oldYTilePos = (m_tileRect.top() - newTileExtents.top()) / TILE_SIZE;
  
  // copy the old tile pointers into the new array
  for(uint y = 0; y < m_yTiles; y++)
	for(uint x = 0; x < m_xTiles; x++)
	  newData[(y + oldYTilePos) * newXTiles + (x + oldXTilePos)] = m_tiles[y * m_xTiles + x];
  
  // delete old tile pointers
  delete m_tiles;
  
  // set calculated values
  m_imgRect = m_imgRect.unite(newRect);
  m_tiles = newData;
  m_xTiles = newXTiles;
  m_yTiles = newYTiles;
  m_tileRect = newTileExtents;
	  
  // allocate any unallocated tiles in the newRect
  QRect allocRect = newRect;
  allocRect.moveBy(-m_tileRect.x(), -m_tileRect.y());

  int minYTile = allocRect.top() / TILE_SIZE;
  int maxYTile = allocRect.bottom() / TILE_SIZE;
  int minXTile = allocRect.left() / TILE_SIZE;
  int maxXTile = allocRect.right() / TILE_SIZE;
  
  for(int y = minYTile; y <= maxYTile; y++)
    for(int x = minXTile; x <= maxXTile; x++)
      if (m_tiles[(y * m_xTiles) + x] == 0)
		{
		  m_tiles[(y * m_xTiles) + x] = new uchar [TILE_SIZE * TILE_SIZE];
		  if (m_id == ci_Alpha) // FIXME: set good init values for all cId's
			memset(m_tiles[(y * m_xTiles) + x], 0, TILE_SIZE * TILE_SIZE);
		  else
			memset(m_tiles[(y * m_xTiles) + x], 255, TILE_SIZE * TILE_SIZE);
      }
}
