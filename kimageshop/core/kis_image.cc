/*
 *  kis_image.cc - part of KImageShop
 *
 *  Copyright (c) 1999 Andrew Richards <A.Richards@phys.canterbury.ac.nz>
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

#include <string.h>
#include <stdio.h>
#include <iostream.h>
#include <sys/time.h>
#include <unistd.h>

#include <qpainter.h>
#include <qregexp.h>
#include <qfileinfo.h>

#include <kstddirs.h>
#include <kglobal.h>
#include <kmimetype.h>

#include "kis_factory.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_util.h"
#include "kis_brush.h"
#include "kis_util.h"

#define KIS_DEBUG(AREA, CMD)

KisImage::KisImage( const QString& _name, int width, int height )
  : w (width)
  , h (height)
  , m_name (_name)
{
  viewportRect = QRect(0,0,w,h);

  QRect tileExtents = KisUtil::findTileExtents( viewportRect );

  xTiles = tileExtents.width() / TILE_SIZE;	
  yTiles = tileExtents.height() / TILE_SIZE;

  setUpVisual();
  QPixmap::setDefaultOptimization( QPixmap::NoOptim );

  tiles = new QPixmap* [xTiles*yTiles];

  for( int y = 0; y < yTiles; y++)
    for( int x = 0; x < xTiles; x++) {
      tiles[y * xTiles + x] = new QPixmap(TILE_SIZE, TILE_SIZE);
      tiles[y * xTiles + x]->fill();
    }
  
  img.create(TILE_SIZE, TILE_SIZE, 32);
  
  channels=3;
  currentLayer=0;
  
  compose = new KisLayer( channels );
  compose->allocateRect( QRect( 0, 0, TILE_SIZE, TILE_SIZE ) );
  
  // XXX make this like the compose layer
  // make it work with other colour spaces
  background = new uchar[channels * TILE_SIZE * TILE_SIZE];

  for( int y = 0; y < TILE_SIZE; y++)
    for(int x = 0; x < TILE_SIZE; x++) {
      *(background+channels*(y*TILE_SIZE+x))  =128+63*((x/16+y/16)%2);
      *(background+channels*(y*TILE_SIZE+x)+1)=128+63*((x/16+y/16)%2);
      *(background+channels*(y*TILE_SIZE+x)+2)=128+63*((x/16+y/16)%2);
    }

  // load some test layers
  //QString _image = locate("kis_images", "cam9b.jpg", KisFactory::global());
  //addRGBLayer(_image);
  //setLayerOpacity(255);

  //_image = locate("kis_images", "cambw12.jpg", KisFactory::global());
  //addRGBLayer(_image);
  //moveLayer(256,384);
  //setLayerOpacity(180);

  /*
  _image = locate("kis_images", "cam05.jpg", KisFactory::global());
  addRGBLayer(_image);
  setLayerOpacity(255);

  _image = locate("kis_images", "cam6.jpg", KisFactory::global());
  addRGBLayer(_image);
  moveLayer(240,280);
  setLayerOpacity(255);

  _image = locate("kis_images", "img2.jpg", KisFactory::global());
  addRGBLayer(_image);
   setLayerOpacity(80);
  */

  compositeImage(QRect());
}

KisImage::~KisImage()
{
  qDebug("~KisImage()");

  // XXX delete individual tiles
  delete tiles;

  if ((visual!=unknown) && (visual!=rgb888x))
    free(imageData);
}

void KisImage::paintContent( QPainter& painter, const QRect& rect, bool /*transparent*/)
{
  paintPixmap( &painter, rect);
}

void KisImage::paintPixmap(QPainter *p, QRect area)
{
  if (!p) return;

 KisUtil::startTimer();
 int startX, startY, pixX, pixY, clipX, clipY = 0;

 int l = area.left();
 int t = area.top();
 int r = area.right();
 int b = area.bottom();

 //qDebug("KisImage::paintPixmap l: %d; t: %d; r: %d; b: %d", l, t, r, b);
 
 for(int y=0;y<yTiles;y++)
   {
      for(int x=0;x<xTiles;x++)
	{
	  QRect tileRect(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE);
	  int xt = x*TILE_SIZE;
	  int yt = y*TILE_SIZE;
	  //qDebug("tile: %d", y*xTiles+x);

	  if (tileRect.intersects(viewportRect) && tileRect.intersects(area))
	    {
	      if (tiles[y*xTiles+x]->isNull())
		continue;
	      
	      if (xt < l)
		{
		  startX = 0;
		  pixX = l - xt;
		}
	      else
		{
		  startX = xt - l;
		  pixX = 0;
		}

	      clipX = r - xt - pixX;
	      
	      if (yt < t)
		{
		  startY = 0;
		  pixY = t - yt;
		}
	      else
		{
		  startY = yt - t;
		  pixY = 0;
		}
	      clipY = b - yt - pixY;

	      if (clipX <= 0)
		clipX = -1;
	      if (clipY <= 0)
		clipY = -1;

	      //qDebug("clipX %d clipY %d", clipX, clipY);
	      //qDebug("pixX %d pixY %d", pixX, pixY);
	      
	      p->drawPixmap(startX, startY, *tiles[y*xTiles+x], pixX, pixY, clipX, clipY);
	    }
	}
    }
 KisUtil::stopTimer("paintImage ");
}

void KisImage::setUpVisual()
{
  QPixmap p;
  Display *dpy    =p.x11Display();
  int displayDepth=p.x11Depth();
  Visual *vis     =(Visual*)p.x11Visual();
  bool trueColour = (vis->c_class == TrueColor);

  // change the false to true to test the faster image converters
  visual=unknown;
  if (true && trueColour) { // do they have a worthy display
    uint red_mask  =(uint)vis->red_mask;
    uint green_mask=(uint)vis->green_mask;
    uint blue_mask =(uint)vis->blue_mask;

    if ((red_mask==0xf800) && (green_mask==0x7e0) && (blue_mask==0x1f))
      visual=rgb565;
    if ((red_mask==0xff0000) && (green_mask==0xff00) && (blue_mask==0xff))
      visual=rgb888x;

    if (visual==unknown) {
      puts("Unoptimized visual - want to write an optimised routine?");
      printf("red=%8x green=%8x blue=%8x\n",red_mask,green_mask,blue_mask);
    } else {
      puts("Using optimized visual");
      xi=XCreateImage( dpy, vis, displayDepth, ZPixmap, 0,0, TILE_SIZE,TILE_SIZE, 32, 0 );
      printf("ximage: bytes_per_line=%d\n",xi->bytes_per_line);
      if (visual!=rgb888x) {
				imageData=new char[xi->bytes_per_line*TILE_SIZE];
				xi->data=imageData;
      }
    }
  }
}	

void KisImage::addRGBLayer(QString file)
{
  QImage img(file);
 
  QString alphaName = file;
  alphaName.replace(QRegExp("\\.jpg$"),"-alpha.jpg");
  qDebug("alphaname=%s\n",alphaName.latin1());
  QImage alpha(alphaName);

  addRGBLayer(img, alpha, QFileInfo(file).fileName());
}

void KisImage::addRGBLayer(QImage& img, QImage &alpha, const QString name)
{
  // XXX currently assumes the alpha image IS a greyscale and the same size as
  // the other channels
  
  qDebug("KisImage::addRGBLayer: %s\n",name.latin1());

  if (img.isNull())
    {
      qDebug("Unable to load image: %s\n",name.latin1());
      return;
    }

  img = img.convertDepth(32);
  
  if (alpha.isNull() || (img.size()!= alpha.size()))
    {
      qDebug("Incorrect sized alpha channel - not loaded");
      alpha = QImage();
    }

  KisLayer *lay = new KisLayer(3);
  lay->setName(name);
  lay->loadRGBImage(img, alpha);
  layers.append(lay);
  currentLayer=lay;
}

void KisImage::addRGBLayer(const QRect& rect, const QColor& c, const QString& name)
{
  KisLayer *lay = new KisLayer(3);
  lay->setName(name);

  lay->allocateRect(rect);
  lay->clear(c);

  layers.append(lay);
  currentLayer=lay;
}

void KisImage::removeLayer( unsigned int _layer )
{
  if( _layer >= layers.count() )
    return;

  KisLayer* lay = layers.take( _layer );

  if( currentLayer == lay )
  {
    if( layers.count() != 0 )
      currentLayer = layers.at( 0 );
    else
      currentLayer = NULL;
  }

  delete lay;
}


// Constructs the composite image in the tile at x,y and updates the relevant
// pixmap
void KisImage::compositeTile(int x, int y, KisLayer *dstLay, int dstTile)
{
  // work out which tile to render into unless directed to a specific tile
  if (dstTile==-1)
    dstTile=y*xTiles+x;
  if (dstLay==0)
    dstLay=compose;

  KIS_DEBUG(tile, printf("\n*** compositeTile %d,%d\n",x,y); );

  //printf("compositeTile: dstLay=%p dstTile=%d\n",dstLay, dstTile);

  // Set the background
	memcpy(dstLay->channelMem(dstTile,0,0), background,
				 channels*TILE_SIZE*TILE_SIZE);

  // Find the tiles boundary in KisImage coordinates
  QRect tileBoundary(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE);

  int l=0;
  KisLayer *lay=layers.first();
  while(lay) { // Go through each layer and find its contribution to this tile
    l++;
    //printf("layer: %s opacity=%d\n",lay->name().data(), lay->opacity());
    if ((lay->isVisible()) &&
				(tileBoundary.intersects(lay->imageExtents()))) {
      // The layer is part of the tile. Find out the 1-4 tiles of the channel
      // which are in it and render the appropriate proportions of each
      //TIME_START;
      //printf("*** compositeTile %d,%d\n",x,y);
      renderLayerIntoTile(tileBoundary, lay, dstLay, dstTile);
      //TIME_END("renderLayerIntoTile");
    }
    lay=layers.next();
  }
}

void KisImage::compositeImage(QRect r)
{
  KisUtil::startTimer();
  for(int y=0;y<yTiles;y++)
    for(int x=0;x<xTiles;x++)
      if (r.isNull() ||
	  r.intersects(QRect(x*TILE_SIZE, y*TILE_SIZE,TILE_SIZE,TILE_SIZE))) {
				// set the alpha channel to opaque
	memset(compose->channelMem(0, 0,0, true),255, 
	       TILE_SIZE*TILE_SIZE);
	compositeTile(x,y, compose, 0);
	
	convertTileToPixmap(compose, 0, tiles[y*xTiles+x]);
      }
  KisUtil::stopTimer("compositeImage");

  emit updated(r);
}


// Renders the part of srcLay which resides in dstTile of dstLay

void KisImage::renderLayerIntoTile(QRect tileBoundary, const KisLayer *srcLay,
																 KisLayer *dstLay, int dstTile)
{
  int tileNo, tileOffsetX, tileOffsetY, xTile, yTile;

	//puts("renderLayerIntoTile");

  srcLay->findTileNumberAndPos(tileBoundary.topLeft(), &tileNo,
															 &tileOffsetX, &tileOffsetY);
  xTile=tileNo%srcLay->xTiles();
  yTile=tileNo/srcLay->xTiles();
  KIS_DEBUG(render, showi(tileNo); );

  bool renderQ1=true, renderQ2=true, renderQ3=true, renderQ4=true;
  if (tileOffsetX<0)
    renderQ1=renderQ3=false;
  if (tileOffsetY<0)
    renderQ2=renderQ1=false;

  KIS_DEBUG(render, showi(tileOffsetX); );
  KIS_DEBUG(render, showi(tileOffsetY); );

  int maxLayerX=TILE_SIZE, maxLayerY=TILE_SIZE;
  if (srcLay->boundryTileX(tileNo)) {
    maxLayerX=srcLay->channelLastTileOffsetX();
    if (tileOffsetX>=0)
      renderQ2=false;
    KIS_DEBUG(render, showi(maxLayerX); );
  }
  if (tileOffsetX==0)
    renderQ4=false;

  if (srcLay->boundryTileY(tileNo)) {
    maxLayerY=srcLay->channelLastTileOffsetY();
		if (tileOffsetY>=0)
		  renderQ3=false;
		KIS_DEBUG(render, showi(maxLayerX); );
  }
  if (tileOffsetY==0)
    renderQ4=false;


  KIS_DEBUG(render, showi(renderQ1); );
  KIS_DEBUG(render, showi(renderQ2); );
  KIS_DEBUG(render, showi(renderQ3); );
  KIS_DEBUG(render, showi(renderQ4); );

  // Render quadrants of each tile (either 1, 2 or 4 quadrants get rendered)
  //
  //  ---------
  //  | 1 | 2 |
  //  ---------
  //  | 3 | 4 |
  //  ---------
  //

  KIS_DEBUG(render, {
    SHOW_POINT(tileBoundary.topLeft());
    printf("tileNo %d, tileOffsetX %d, tileOffsetY %d\n",
					 tileNo, tileOffsetX, tileOffsetY);
  });
	
  int renderedToX, renderedToY;

  KIS_DEBUG(render, printf("Test 1: "); );
  if (renderQ1) {
    // true => render 1
    renderTileQuadrant(srcLay, tileNo, dstLay, dstTile,
											 tileOffsetX, tileOffsetY, 0, 0,
											 TILE_SIZE, TILE_SIZE);
    renderedToX=maxLayerX-tileOffsetX;
    renderedToY=maxLayerY-tileOffsetY;
  } else
    KIS_DEBUG(render, puts("ignore"); );
	
  KIS_DEBUG(render, printf("Test 2:"); );
  if (renderQ2) {
    // true => render 2
    if (renderQ1)
      renderTileQuadrant(srcLay, tileNo+1, dstLay, dstTile,
			 0, tileOffsetY, maxLayerX-tileOffsetX, 0,
			 TILE_SIZE, TILE_SIZE);
    else
      renderTileQuadrant(srcLay, tileNo, dstLay, dstTile,
			 0, tileOffsetY, -tileOffsetX,0,
			 TILE_SIZE, TILE_SIZE);
  } else
    KIS_DEBUG(render, puts("ignore"));

  KIS_DEBUG(render, printf("Test 3:"); );
  if (renderQ3) {
    // true => render 3
    if (renderQ1)
      renderTileQuadrant(srcLay, tileNo+srcLay->xTiles(), dstLay, dstTile,
												 tileOffsetX, 0, 0, maxLayerY-tileOffsetY,
												 TILE_SIZE, TILE_SIZE);
    else
      renderTileQuadrant(srcLay, tileNo, dstLay, dstTile,
												 tileOffsetX, 0, 0, -tileOffsetY,
												 TILE_SIZE, TILE_SIZE);
  } else
    KIS_DEBUG(render, puts("ignore"); );

  KIS_DEBUG(render, printf("Test 4:"); );
  // true => render 4
  if (renderQ4) {
    int newTile=tileNo;
    KIS_DEBUG(render, showi(xTile); );
    KIS_DEBUG(render, showi(yTile); );
    if (renderQ1) {
      xTile++; yTile++; newTile+=srcLay->xTiles()+1;
    } else {
      if (renderQ2) { yTile++; newTile+=srcLay->xTiles(); }
			if (renderQ3) { xTile++; newTile+=1; }
    }
    KIS_DEBUG(render, showi(xTile); );
    KIS_DEBUG(render, showi(yTile); );
    if ((xTile<srcLay->xTiles()) && (yTile<srcLay->yTiles())) {
      KIS_DEBUG(render, showi(newTile); );
      if (!(renderQ1 && !renderQ2 && !renderQ3)) {
				if (tileOffsetX>0) tileOffsetX=tileOffsetX-TILE_SIZE;
				if (tileOffsetY>0) tileOffsetY=tileOffsetY-TILE_SIZE;
				renderTileQuadrant(srcLay, newTile, dstLay, dstTile,
													 0, 0, -tileOffsetX, -tileOffsetY,
													 TILE_SIZE, TILE_SIZE);
      }
    }	else
      KIS_DEBUG(render, puts("ignore"); );
  }	else
    KIS_DEBUG(render, puts("ignore"); );
}

void KisImage::renderTileQuadrant(const KisLayer *srcLay, int srcTile,
				KisLayer *dstLay, int dstTile,
				int srcX, int srcY,
				int dstX, int dstY, int w, int h)
{
  if (srcLay->channelMem(srcTile,0,0)==0) return;

  uchar opacity=srcLay->opacity();
	
  // Constrain the width so that the copy is clipped to the overlap
  w = min(min(w, TILE_SIZE-srcX), TILE_SIZE-dstX);
  h = min(min(h, TILE_SIZE-srcY), TILE_SIZE-dstY);
  // now constrain if on the boundry of the layer
  if (srcLay->boundryTileX(srcTile))
    w = min(w, srcLay->channelLastTileOffsetX()-srcX);
  if (srcLay->boundryTileY(srcTile))
    h = min(h, srcLay->channelLastTileOffsetY()-srcY);
  // XXX now constrain for the boundry of the Canvas

  //printf("renderTileQuadrant: srcTile=%d src=(%d,%d) dstTile=%d dst=(%d,%d) size=(%d,%d)\n", srcTile, srcX, srcY, dstTile, dstX, dstY, w,h);


  uchar one=255;
  int leadIn=(TILE_SIZE-w);

	uchar *composeD=dstLay->channelMem(dstTile, dstX, dstY);
	uchar *composeA=dstLay->channelMem(dstTile, dstX, dstY, true);
	uchar *channelD=srcLay->channelMem(srcTile, srcX, srcY);
	uchar *alpha   =srcLay->channelMem(srcTile, srcX, srcY, true);

	int alphaInc, alphaLeadIn;

	if (alpha) {
		alphaInc=1;
		alphaLeadIn=leadIn;
	} else {
		puts("************* no alpha");
		alpha=&one;
		alphaInc=0;
		alphaLeadIn=0;
	}

	leadIn*=3;

	uchar opac,invOpac;
	for(int y=h; y; y--) {
		for(int x=w; x; x--) {
			// for prepultiply => invOpac=255-(*alpha*opacity)/255;
			opac=(*alpha*opacity)/255;
			invOpac=255-opac;
			// fix this (in a fast way for colour spaces

			*composeD++ = (((*composeD * *composeA)/255) * invOpac + 
										 *channelD++ * opac)/255;
			*composeD++ = (((*composeD * *composeA)/255) * invOpac + 
										 *channelD++ * opac)/255;
			*composeD++ = (((*composeD * *composeA)/255) * invOpac + 
										 *channelD++ * opac)/255;
			*composeA=*alpha + *composeA - (*alpha * *composeA)/255;
			composeA+=alphaInc;
			alpha+=alphaInc;
		}
		composeD+=leadIn;
		composeA+=alphaLeadIn;
		channelD+=leadIn;
		alpha+=alphaLeadIn;
	}
}

KisLayer* KisImage::layerPtr( KisLayer *_layer )
{
  if( _layer == 0 )
    return( currentLayer );
  return( _layer );
}

void KisImage::setCurrentLayer( int _layer )
{
  currentLayer = layers.at( _layer );
}

void KisImage::setLayerOpacity( uchar _opacity, KisLayer *_layer )
{
  _layer = layerPtr( _layer );
  _layer->setOpacity( _opacity );

  // FIXME: repaint
}

void KisImage::moveLayer( int _dx, int _dy, KisLayer *_lay )
{
  _lay = layerPtr( _lay );
  _lay->moveBy( _dx, _dy );
}

void KisImage::moveLayerTo( int _x, int _y, KisLayer *_lay )
{
  _lay = layerPtr( _lay );
  _lay->moveTo( _x, _y );
}

void KisImage::convertImageToPixmap(QImage *image, QPixmap *pix)
{
  if (visual==unknown) {
    //TIME_START;
    pix->convertFromImage(*image);
    //TIME_END("convertFromImage");
  } else {
    //TIME_START;
    switch(visual) {
    case rgb565: {
      ushort s;
      ushort *ptr=(ushort *)imageData;
      uchar *qimg=image->bits();
      for(int y=0;y<TILE_SIZE;y++)
				for(int x=0;x<TILE_SIZE;x++) {
					s =(*qimg++)>>3;
					s|=(*qimg++ & 252)<<3;
					s|=(*qimg++ & 248)<<8;
					qimg++;
					*ptr++=s;
				}
    }
    break;

    case rgb888x:
      xi->data=(char*)image->bits();
      break;

    default: break;
    }
    XPutImage(pix->x11Display(), pix->handle(), qt_xget_readonly_gc(),
	      xi, 0,0, 0,0, TILE_SIZE, TILE_SIZE);
    //TIME_END("fast convertImageToPixmap");
  }
}

void KisImage::convertTileToPixmap(KisLayer *lay, int tileNo, QPixmap *pix)
{
  // Copy the composite image into a QImage so it can be converted to a
  // QPixmap.
  // Note: surprisingly it is not quicker to render directly into a QImage
  // probably due to the CPU cache, it's also useless wrt to other colour
  // spaces

  // For RGB images XXX
	uchar *comp=lay->channelMem(tileNo, 0,0);
	for(int yy=0;yy<TILE_SIZE;yy++) {
		uchar *ptr=img.scanLine(yy);
		for(int xx=TILE_SIZE;xx;xx--) {
			*ptr++=*comp++;
			*ptr++=*comp++;
			*ptr++=*comp++;
			ptr++;
		}
	}

  // Construct the relevant pixmap
  convertImageToPixmap(&img, pix);
}

void KisImage::mergeAllLayers()
{
  QList<KisLayer> l;

  KisLayer *lay = layers.first();

  while(lay)
    {
      l.append(lay);
      lay = layers.next();
    }
  mergeLayers(l);
}

void KisImage::mergeVisibleLayers()
{
  QList<KisLayer> l;

  KisLayer *lay = layers.first();

  while(lay)
    {
      if(lay->isVisible())
	l.append(lay);
      lay = layers.next();
    }
  mergeLayers(l);
}

void KisImage::mergeLinkedLayers()
{
  QList<KisLayer> l;

  KisLayer *lay = layers.first();

  while(lay)
    {
      if (lay->isLinked())
	l.append(lay);
      lay = layers.next();
    }
  mergeLayers(l);
}

void KisImage::mergeLayers(QList<KisLayer> list)
{
  list.setAutoDelete(false);

  KisLayer *a, *b;
  QRect newRect;

  a = list.first();
  while(a)
    {
      newRect.unite(a->imageExtents());
      a->renderOpacityToAlpha();
      a = list.next();
    }

  while((a = list.first()) && (b = list.next()))
    {
      if (!a || !b)
	break;
      QRect ar = a->imageExtents();
      QRect br = b->imageExtents();
      
      QRect urect = ar.unite(br);
      
      // allocate out tiles if required
      a->allocateRect(urect);
      b->allocateRect(urect);
      
      // rect in layer coords (offset from tileExtents.topLeft())
      QRect rect = urect;
      rect.moveTopLeft(urect.topLeft() - a->tileExtents().topLeft());
      
      // workout which tiles in the layer need to be updated
      int minYTile=rect.top() / TILE_SIZE;
      int maxYTile=rect.bottom() / TILE_SIZE;
      int minXTile=rect.left() / TILE_SIZE;
      int maxXTile=rect.right() / TILE_SIZE;
      
      QRect tileBoundary;
  
      for(int y=minYTile; y<=maxYTile; y++)
	{
	  for(int x=minXTile; x<=maxXTile; x++)
	    {
	      int dstTile = y * a->xTiles() + x;
	      tileBoundary = a->tileRect(dstTile);
	      renderLayerIntoTile(tileBoundary, b, a, dstTile);
	    }
	}
      
      list.remove(b);
      layers.remove(b);
      if( currentLayer == b )
	{
	  if(layers.count() != 0)
	    currentLayer = layers.at(0);
	  else
	    currentLayer = NULL;
	}
      delete b;
    }
  emit layersUpdated();
  compositeImage(newRect);
}

void KisImage::upperLayer( unsigned int _layer )
{
  ASSERT( _layer < layers.count() );

  if( _layer > 0 )
  {
    KisLayer *pLayer = layers.take( _layer );
    layers.insert( _layer - 1, pLayer );
  }
}

void KisImage::lowerLayer( unsigned int _layer )
{
  ASSERT( _layer < layers.count() );

  if( _layer < ( layers.count() - 1 ) )
  {
    KisLayer *pLayer = layers.take( _layer );
    layers.insert( _layer + 1, pLayer );
  }
}

void KisImage::setFrontLayer( unsigned int _layer )
{
  ASSERT( _layer < layers.count() );

  if( _layer < ( layers.count() - 1 ) )
  {
    KisLayer *pLayer = layers.take( _layer );
    layers.append( pLayer );
  }
}

void KisImage::setBackgroundLayer( unsigned int _layer )
{
  ASSERT( _layer < layers.count() );

  if( _layer > 0 )
  {
    KisLayer *pLayer = layers.take( _layer );
    layers.insert( 0, pLayer );
  }
}

void KisImage::rotateLayer180(KisLayer *_layer)
{
  _layer = layerPtr( _layer );
  _layer->rotate180();
  compositeImage(_layer->imageExtents());
}

void KisImage::rotateLayerLeft90(KisLayer *_layer)
{
  _layer = layerPtr( _layer );
  _layer->rotateLeft90();
  compositeImage(_layer->imageExtents());
}

void KisImage::rotateLayerRight90(KisLayer *_layer)
{
  _layer = layerPtr( _layer );
  _layer->rotateRight90();
  compositeImage(_layer->imageExtents());
}

void KisImage::mirrorLayerX(KisLayer *_layer)
{
  _layer = layerPtr( _layer );
  _layer->mirrorX();
  compositeImage(_layer->imageExtents());
}

void KisImage::mirrorLayerY(KisLayer *_layer)
{
  _layer = layerPtr( _layer );
  _layer->mirrorY();
  compositeImage(_layer->imageExtents());
}

#include "kis_image.moc"
