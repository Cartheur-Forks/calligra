/*
 *  iconchooser.cc - part of KImageShop
 *
 *  A general chooserwidget, showing items represented by pixmaps
 *
 *  Copyright (c) 1999 Carsten Pfeiffer <pfeiffer@kde.org>
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

#include <qbrush.h>
#include <qcolor.h>
#include <qframe.h>
#include <qpainter.h>

#include "iconchooser.h"
#include "iconitem.h"


IconChooser::IconChooser( QWidget *parent, QSize iconSize, const char *name )
  : QTableView( parent, name )
{
  setTableFlags( Tbl_autoVScrollBar | Tbl_clipCellPainting | Tbl_cutCellsH |
		 Tbl_snapToGrid );

  margin = 2; // a cell is 2 * two pixels larger than an item
  setCellWidth( iconSize.width() + 2*margin );
  setCellHeight( iconSize.height() + 2*margin );
  updateTableSize();

  iconList.clear();
  pw           = 0L;
  nCols        = 0;
  curRow       = 0;
  curCol       = 0;
  itemCount    = 0;
  itemWidth    = iconSize.width();
  itemHeight   = iconSize.height();
  bgColor      = colorGroup().background();
}


IconChooser::~IconChooser()
{
  iconList.clear();
  if ( pw )
    delete pw;
}


void IconChooser::addItem( IconItem *item )
{
  ASSERT( item != 0L );
  iconList.insert( itemCount++, item );
  calculateCells();
}


bool IconChooser::removeItem( IconItem *item )
{
  bool ok = iconList.remove( item );
  if ( ok ) {
    itemCount--;
    calculateCells();
  }

  return ok;
}


void IconChooser::clear()
{
  itemCount = 0;
  iconList.clear();
  calculateCells();
}


// return the pointer of the item at (row,col) - beware, resizing disturbs
// rows and cols!
// return 0L if item is not found
IconItem* IconChooser::itemAt( int row, int col )
{
  return itemAt( cellIndex( row, col ) );
}


// return the pointer of the item at position index
// return 0L if item is not found
IconItem* IconChooser::itemAt( int index )
{
  if ( index == -1 || index >= itemCount )
    return 0L;
  else
    return iconList.at( index );
}


// returns 0L if there is no current item
IconItem* IconChooser::currentItem()
{
  return itemAt( curRow, curCol );
}


// sets the current item to item
// does NOT emit selected()  (should it?)
void IconChooser::setCurrentItem( IconItem *item )
{
  int index = iconList.find( item );
  if ( index != -1 && nCols > 0 ) { // item is avaiable
    int oldRow = curRow;
    int oldCol = curCol;

    curRow = index / nCols;
    curCol = index - (curRow * nCols);

    updateCell( oldRow, oldCol, true );
    updateCell( curRow, curCol, true );
  }
}


// calculate the grid and set the number of rows and columns
// reorder all items approrpriately
void IconChooser::calculateCells()
{
  if ( nCols == 0 )
    return;

  setAutoUpdate( false );

  int rows = itemCount / nCols;
  if ( (rows * nCols) < itemCount )
    rows++;

  setNumRows( rows );

  setAutoUpdate( true );
  repaint();
}



// recalculate the number of items that fit in one row
void IconChooser::resizeEvent ( QResizeEvent *e )
{
  ASSERT( cellWidth() > 0 );
  nCols = width() / cellWidth();

  setNumCols( nCols );
  calculateCells();
  QTableView::resizeEvent( e );
}


// paint one cell
// mark the current item and center items smaller than the cellSize
void IconChooser::paintCell( QPainter *p, int row, int col )
{
  IconItem *item = itemAt( row, col );

  if ( item ) {
    const QPixmap& pix = item->pixmap();
    int x  = margin; 		int y  = margin;
    int pw = pix.width(); 	int ph = pix.height();
    int cw = cellWidth(); 	int ch = cellHeight();

    // center small pixmaps
    if ( pw < itemWidth )
      x = (cw - pw) / 2;
    if ( ph < itemHeight )
      y = (cw - ph) / 2;

    if ( row == curRow && col == curCol ) { // highlight current item
      p->drawPixmap( x, y, pix, 0, 0, itemWidth, itemHeight );
      p->drawRect( 0, 0, cw, ch );
    }
    else
      p->drawPixmap( x, y, pix, 0, 0, itemWidth, itemHeight );
  }

  else { // empty cell
    p->fillRect( 0, 0, cellWidth(), cellHeight(), QBrush( bgColor ) );
  }
}


// return the index of a cell, given row and column position
// maps directly to the position in the itemlist
// return -1 on failure
int IconChooser::cellIndex( int row, int col )
{
  if ( row < 0 || col < 0 )
    return -1;

  return ( (row * nCols) + col );
}


// eventually select the item, clicked on
// FIXME: improve behavior for big items, larger than cellSize
// Photoshop scales down the pixmap and shows the scalingfactor
void IconChooser::mousePressEvent( QMouseEvent *e )
{
  QTableView::mousePressEvent( e );

  if ( e->button() == LeftButton ) {
    QPoint p = e->pos();

    int row = findRow( p.y() );
    int col = findCol( p.x() );

    int index = cellIndex( row, col );

    IconItem *item = itemAt( index );
    if ( item ) {
      const QPixmap& pix = item->pixmap();
      if ( pix.width() > itemWidth || pix.height() > itemHeight )
	showFullPixmap( pix, p );

      int oldRow = curRow;
      int oldCol = curCol;

      curRow = row;
      curCol = col;

      updateCell( oldRow, oldCol, true );
      updateCell( curRow, curCol, true );

      emit selected( item );
    }
  }
}


// when a big item is shown in full size, delete it on mouseRelease
void IconChooser::mouseReleaseEvent( QMouseEvent * )
{
  if ( pw ) {
    delete pw;
    pw = 0L;
  }
}


// show the full pixmap of a large item in an extra widget
void IconChooser::showFullPixmap( const QPixmap& pix, const QPoint& p )
{
  pw = new PixmapWidget( pix, 0L );
}


// FIXME: implement keyboard navigation
void IconChooser::keyPressEvent( QKeyEvent *e )
{
  QTableView::keyPressEvent( e ); // for now...
}





/////////////////////////////
/////
// helper class PixmapWidget
//
PixmapWidget::PixmapWidget( const QPixmap& pix, QWidget *parent,
			    const char *name )
  : QFrame( parent, name, WStyle_Customize | WStyle_NoBorder )
{
  setFrameStyle( QFrame::WinPanel | QFrame::Raised );
  pixmap = pix;
  int w = pix.width() + 2*lineWidth();
  int h = pix.height() + 2*lineWidth();
  resize( w, h );

  // center widget under mouse cursor
  QPoint p = QCursor::pos();
  move( p.x() - w/2, p.y() - h/2 );
  show();
}


// paint the centered pixmap; don't overpaint the frame
void PixmapWidget::paintEvent( QPaintEvent *e )
{
  QFrame::paintEvent( e );
  QPainter p( this );
  p.drawPixmap( lineWidth(), lineWidth(), pixmap );
}


#include "iconchooser.moc"
