/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *   02111-1307, USA.
 *

 Some code is derived from GraphicsMagick/magick/composite.c and is 
 subject to the following license and copyright:

  Copyright (C) 2002 GraphicsMagick Group, an organization dedicated
  to making software imaging solutions freely available.

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files
  ("GraphicsMagick"), to deal in GraphicsMagick without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of GraphicsMagick,
  and to permit persons to whom GraphicsMagick is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of GraphicsMagick.

  The software is provided "as is", without warranty of any kind, express
  or implied, including but not limited to the warranties of
  merchantability, fitness for a particular purpose and noninfringement.
  In no event shall GraphicsMagick Group be liable for any claim,
  damages or other liability, whether in an action of contract, tort or
  otherwise, arising from, out of or in connection with GraphicsMagick
  or the use or other dealings in GraphicsMagick.

  Except as contained in this notice, the name of the GraphicsMagick
  Group shall not be used in advertising or otherwise to promote the
  sale, use or other dealings in GraphicsMagick without prior written
  authorization from the GraphicsMagick Group.

 Other code is derived from gwenview/src/qxcfi.* - this is released under
  the terms of the LGPL

 */

#ifndef COMPOSITE_H_
#define COMPOSITE_H_

#include <kdebug.h>

#include <kis_global.h>

/**
 * Image composition functions that can be used by the colour strategies. 
 *
 * XXX: perhaps each composition function ought to be a strategy of itself.
 * Krita is still missing something like a capabilities database that ties 
 * together image formats, colour systems, composition functions etc., that
 * determines which goes with which and defines user visible text for all this.
 * 
 * For now, this is a quick hack; once things are working again, I'll investigate
 * doing this nicely (famous last words...)
 *
 * XXX: Except for Over, none of the operators uses the opacity parameter
 */


// Straight from image.h

#define PixelIntensity(pixel) ((unsigned int) \
   (((double)306.0 * (pixel[PIXEL_RED]) + \
     (double)601.0 * (pixel[PIXEL_GREEN]) + \
     (double)117.0 * (pixel[PIXEL_BLUE)) \
    / 1024.0))

#define PixelIntensityToQuantum(pixel) ((QUANTUM)PixelIntensity(pixel))

#define PixelIntensityToDouble(pixel) ((double)PixelIntensity(pixel))

#define RoundSignedToQuantum(value) ((QUANTUM) (value < 0 ? 0 : \
  (value > QUANTUM_MAX) ? QUANTUM_MAX : value + 0.5))

#define RoundToQuantum(value) ((QUANTUM) (value > QUANTUM_MAX ? QUANTUM_MAX : \
  value + 0.5))

// And from studio.h
#define AbsoluteValue(x)  ((x) < 0 ? -(x) : (x))

void compositeIn(Q_INT32 pixelSize,
		 QUANTUM *dst, 
		 Q_INT32 dstRowSize,
		 const QUANTUM *src, 
		 Q_INT32 srcRowSize,
		 Q_INT32 rows, 
		 Q_INT32 cols, 
		 QUANTUM opacity = OPACITY_OPAQUE)
{

	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;
	
	double sAlpha, dAlpha;
	double alpha;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			
			if (s[PIXEL_ALPHA] == OPACITY_TRANSPARENT)
			{
				memcpy(d, s, pixelSize * sizeof(QUANTUM));
				continue;
			}
			if (d[PIXEL_ALPHA] == OPACITY_TRANSPARENT)
				continue;
 
			sAlpha = QUANTUM_MAX - s[PIXEL_ALPHA];
			dAlpha = QUANTUM_MAX - d[PIXEL_ALPHA];

			alpha=(double) (((double) QUANTUM_MAX - sAlpha) * (QUANTUM_MAX - dAlpha) / QUANTUM_MAX);
			d[PIXEL_RED]=(QUANTUM) (((double) QUANTUM_MAX - sAlpha) *
						(QUANTUM_MAX-dAlpha) * s[PIXEL_RED] / QUANTUM_MAX / alpha + 0.5);
			d[PIXEL_GREEN]=(QUANTUM) (((double) QUANTUM_MAX - sAlpha)*
						  (QUANTUM_MAX-dAlpha) * s[PIXEL_GREEN] / QUANTUM_MAX / alpha + 0.5);
			d[PIXEL_BLUE]=(QUANTUM) (((double) QUANTUM_MAX - sAlpha)*
						 (QUANTUM_MAX - dAlpha) * s[PIXEL_BLUE] / QUANTUM_MAX / alpha + 0.5);
			d[PIXEL_ALPHA]=(QUANTUM) ((d[PIXEL_ALPHA] * (QUANTUM_MAX - alpha) / QUANTUM_MAX) + 0.5);
			
		}
		dst += dstRowSize;
		src += srcRowSize;
	}

}

void compositeOut(Q_INT32 pixelSize,
		   QUANTUM *dst, 
		   Q_INT32 dstRowSize,
		   const QUANTUM *src, 
		   Q_INT32 srcRowSize,
		   Q_INT32 rows, 
		   Q_INT32 cols, 
		   QUANTUM opacity = OPACITY_OPAQUE)
{
	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double sAlpha, dAlpha;
	double alpha;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			if (s[PIXEL_ALPHA] == OPACITY_TRANSPARENT)
			{
				memcpy(d, s, pixelSize * sizeof(QUANTUM));
				break;
			}
			if (d[PIXEL_ALPHA] == OPACITY_OPAQUE)
			{
				d[PIXEL_ALPHA]=OPACITY_TRANSPARENT;
				break;
			}
			sAlpha = QUANTUM_MAX - s[PIXEL_ALPHA];
			dAlpha = QUANTUM_MAX - d[PIXEL_ALPHA];

			alpha=(double) (QUANTUM_MAX - sAlpha) * d[PIXEL_ALPHA]/QUANTUM_MAX;
			d[PIXEL_RED] = (QUANTUM) (((double) QUANTUM_MAX - sAlpha) * dAlpha * s[PIXEL_RED] / QUANTUM_MAX / alpha + 0.5);
			d[PIXEL_GREEN] = (QUANTUM) (((double) QUANTUM_MAX - sAlpha) * dAlpha * s[PIXEL_GREEN] / QUANTUM_MAX / alpha + 0.5);
			d[PIXEL_BLUE] = (QUANTUM) (((double) QUANTUM_MAX - sAlpha) * dAlpha * s[PIXEL_BLUE] / QUANTUM_MAX / alpha + 0.5);
			d[PIXEL_ALPHA]=(QUANTUM) ((d[PIXEL_ALPHA] * (QUANTUM_MAX - alpha) / QUANTUM_MAX) + 0.5);
		}
		dst += dstRowSize;
		src += srcRowSize;
	}
		
}

void compositeAtop(Q_INT32 pixelSize,
		   QUANTUM *dst, 
		   Q_INT32 dstRowSize,
		   const QUANTUM *src, 
		   Q_INT32 srcRowSize,
		   Q_INT32 rows, 
		   Q_INT32 cols, 
		   QUANTUM opacity = OPACITY_OPAQUE)
{

	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double sAlpha, dAlpha;
	double alpha, red, green, blue;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			sAlpha = QUANTUM_MAX - s[PIXEL_ALPHA];
			dAlpha = QUANTUM_MAX - d[PIXEL_ALPHA];
			
			alpha = ((double)(QUANTUM_MAX - sAlpha) *
			       (QUANTUM_MAX - dAlpha) + (double) sAlpha *
			       (QUANTUM_MAX - dAlpha)) / QUANTUM_MAX;

			red = ((double)(QUANTUM_MAX - sAlpha) * (QUANTUM_MAX - dAlpha) *  s[PIXEL_RED] / QUANTUM_MAX +
			     (double) sAlpha * (QUANTUM_MAX-dAlpha) * d[PIXEL_RED]/QUANTUM_MAX) / alpha;
			d[PIXEL_RED] = (QUANTUM) (red > QUANTUM_MAX ? QUANTUM_MAX : red + 0.5);

			green = ((double) (QUANTUM_MAX - sAlpha) * (QUANTUM_MAX - dAlpha) * s[PIXEL_GREEN] / QUANTUM_MAX + 
				 (double) sAlpha * (QUANTUM_MAX-dAlpha) * d[PIXEL_GREEN]/QUANTUM_MAX)/alpha;
			d[PIXEL_GREEN] = (QUANTUM) (green > QUANTUM_MAX ? QUANTUM_MAX : green + 0.5);

			blue = ((double) (QUANTUM_MAX - sAlpha) * (QUANTUM_MAX- dAlpha) * s[PIXEL_BLUE] / QUANTUM_MAX +
				     (double) sAlpha * (QUANTUM_MAX - dAlpha) * d[PIXEL_BLUE]/QUANTUM_MAX) / alpha;
			d[PIXEL_BLUE] = (QUANTUM) (blue > QUANTUM_MAX ? QUANTUM_MAX : blue + 0.5);
			d[PIXEL_ALPHA]=(QUANTUM) (QUANTUM_MAX - (alpha > QUANTUM_MAX ? QUANTUM_MAX : alpha) + 0.5);
		}
		dst += dstRowSize;
		src += srcRowSize;
	}
}


void compositeXor(Q_INT32 pixelSize,
		  QUANTUM *dst, 
		  Q_INT32 dstRowSize,
		  const QUANTUM *src, 
		  Q_INT32 srcRowSize,
		  Q_INT32 rows, 
		  Q_INT32 cols, 
		  QUANTUM opacity = OPACITY_OPAQUE)
{
	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double sAlpha, dAlpha;
	double alpha, red, green, blue;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			sAlpha = QUANTUM_MAX - s[PIXEL_ALPHA];
			dAlpha = QUANTUM_MAX - d[PIXEL_ALPHA];
			
			alpha =((double) (QUANTUM_MAX -sAlpha)*
				dAlpha+(double) (QUANTUM_MAX -dAlpha)*
				sAlpha)/QUANTUM_MAX ;
			red=((double) (QUANTUM_MAX -sAlpha)*dAlpha*
			     s[PIXEL_RED]/QUANTUM_MAX +(double) (QUANTUM_MAX -dAlpha)*
			     sAlpha*d[PIXEL_RED]/QUANTUM_MAX )/alpha ;
			d[PIXEL_RED]=RoundSignedToQuantum(red);
			green=((double) (QUANTUM_MAX -sAlpha)*dAlpha*
			       s[PIXEL_GREEN]/QUANTUM_MAX +(double) (QUANTUM_MAX -dAlpha)*
			       sAlpha*d[PIXEL_GREEN]/QUANTUM_MAX )/alpha ;
			d[PIXEL_GREEN]=RoundSignedToQuantum(green);
			blue=((double) (QUANTUM_MAX -sAlpha)*dAlpha*
			      s[PIXEL_BLUE]/QUANTUM_MAX +(double) (QUANTUM_MAX -dAlpha)*
			      sAlpha*d[PIXEL_BLUE]/QUANTUM_MAX )/alpha ;
			d[PIXEL_BLUE]=RoundSignedToQuantum(blue);
			d[PIXEL_ALPHA]=QUANTUM_MAX -RoundSignedToQuantum(alpha );
		}
		dst += dstRowSize;
		src += srcRowSize;
	}

}


void compositePlus(Q_INT32 pixelSize,
		   QUANTUM *dst, 
		   Q_INT32 dstRowSize,
		   const QUANTUM *src, 
		   Q_INT32 srcRowSize,
		   Q_INT32 rows, 
		   Q_INT32 cols, 
		   QUANTUM opacity = OPACITY_OPAQUE)
{
	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double sAlpha, dAlpha;
	double alpha, red, green, blue;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			sAlpha = QUANTUM_MAX - s[PIXEL_ALPHA];
			dAlpha = QUANTUM_MAX - d[PIXEL_ALPHA];

			red=((double) (QUANTUM_MAX -sAlpha)*s[PIXEL_RED]+(double)
			     (QUANTUM_MAX -dAlpha)*d[PIXEL_RED])/QUANTUM_MAX ;
			d[PIXEL_RED]=RoundSignedToQuantum(red);
			green=((double) (QUANTUM_MAX -sAlpha)*s[PIXEL_GREEN]+(double)
			       (QUANTUM_MAX -dAlpha)*d[PIXEL_GREEN])/QUANTUM_MAX ;
			d[PIXEL_GREEN]=RoundSignedToQuantum(green);
			blue=((double) (QUANTUM_MAX -sAlpha)*s[PIXEL_BLUE]+(double)
			      (QUANTUM_MAX -dAlpha)*d[PIXEL_BLUE])/QUANTUM_MAX ;
			d[PIXEL_BLUE]=RoundSignedToQuantum(blue);
			alpha =((double) (QUANTUM_MAX -sAlpha)+
				(double) (QUANTUM_MAX -dAlpha))/QUANTUM_MAX ;
			d[PIXEL_ALPHA]=QUANTUM_MAX -RoundSignedToQuantum(alpha );	
		}
		dst += dstRowSize;
		src += srcRowSize;
	}
}



void compositeMinus(Q_INT32 pixelSize,
		    QUANTUM *dst, 
		    Q_INT32 dstRowSize,
		    const QUANTUM *src, 
		    Q_INT32 srcRowSize,
		    Q_INT32 rows, 
		    Q_INT32 cols,
		    QUANTUM opacity = OPACITY_OPAQUE)
{
	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double sAlpha, dAlpha;
	double alpha, red, green, blue;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			sAlpha = QUANTUM_MAX - s[PIXEL_ALPHA];
			dAlpha = QUANTUM_MAX - d[PIXEL_ALPHA];

			red=((double) (QUANTUM_MAX -dAlpha)*d[PIXEL_RED]-
			     (double) (QUANTUM_MAX -sAlpha)*s[PIXEL_RED])/QUANTUM_MAX ;
			d[PIXEL_RED]=RoundSignedToQuantum(red);
			green=((double) (QUANTUM_MAX -dAlpha)*d[PIXEL_GREEN]-
			       (double) (QUANTUM_MAX -sAlpha)*s[PIXEL_GREEN])/QUANTUM_MAX ;
			d[PIXEL_GREEN]=RoundSignedToQuantum(green);
			blue=((double) (QUANTUM_MAX -dAlpha)*d[PIXEL_BLUE]-
			      (double) (QUANTUM_MAX -sAlpha)*s[PIXEL_BLUE])/QUANTUM_MAX ;
			d[PIXEL_BLUE]=RoundSignedToQuantum(blue);
			alpha =((double) (QUANTUM_MAX -dAlpha)-
				(double) (QUANTUM_MAX -sAlpha))/QUANTUM_MAX ;
			d[PIXEL_ALPHA]=QUANTUM_MAX -RoundSignedToQuantum(alpha );
			
		}
		dst += dstRowSize;
		src += srcRowSize;
	}

}

void compositeAdd(Q_INT32 pixelSize,
		  QUANTUM *dst, 
		  Q_INT32 dstRowSize,
		  const QUANTUM *src, 
		  Q_INT32 srcRowSize,
		  Q_INT32 rows, 
		  Q_INT32 cols, 
		  QUANTUM opacity = OPACITY_OPAQUE)
{
	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double sAlpha, dAlpha;
	double red, green, blue;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			sAlpha = QUANTUM_MAX - s[PIXEL_ALPHA];
			dAlpha = QUANTUM_MAX - d[PIXEL_ALPHA];

			red=(double) s[PIXEL_RED]+d[PIXEL_RED];
			d[PIXEL_RED]=(QUANTUM)
				(red > QUANTUM_MAX  ? red-=QUANTUM_MAX  : red+0.5);
			green=(double) s[PIXEL_GREEN]+d[PIXEL_GREEN];
			d[PIXEL_GREEN]=(QUANTUM)
				(green > QUANTUM_MAX  ? green-=QUANTUM_MAX  : green+0.5);
			blue=(double) s[PIXEL_BLUE]+d[PIXEL_BLUE];
			d[PIXEL_BLUE]=(QUANTUM)
				(blue > QUANTUM_MAX  ? blue-=QUANTUM_MAX  : blue+0.5);
			d[PIXEL_ALPHA]=OPACITY_OPAQUE;	
		}
		dst += dstRowSize;
		src += srcRowSize;
	}

}

void compositeSubtract(Q_INT32 pixelSize,
		       QUANTUM *dst, 
		       Q_INT32 dstRowSize,
		       const QUANTUM *src, 
		       Q_INT32 srcRowSize,
		       Q_INT32 rows, 
		       Q_INT32 cols, 
		       QUANTUM opacity = OPACITY_OPAQUE)
{
	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double red, green, blue;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {

			red=(double) s[PIXEL_RED]-d[PIXEL_RED];
			d[PIXEL_RED]=(QUANTUM)
				(red < 0 ? red+=QUANTUM_MAX  : red+0.5);
			green=(double) s[PIXEL_GREEN]-d[PIXEL_GREEN];
			d[PIXEL_GREEN]=(QUANTUM)
				(green < 0 ? green+=QUANTUM_MAX  : green+0.5);
			blue=(double) s[PIXEL_BLUE]-d[PIXEL_BLUE];
			d[PIXEL_BLUE]=(QUANTUM)
				(blue < 0 ? blue+=QUANTUM_MAX  : blue+0.5);
			d[PIXEL_ALPHA]=OPACITY_OPAQUE;
			
		}
		dst += dstRowSize;
		src += srcRowSize;
	}

}

void compositeDiff(Q_INT32 pixelSize,
		   QUANTUM *dst, 
		   Q_INT32 dstRowSize,
		   const QUANTUM *src, 
		   Q_INT32 srcRowSize,
		   Q_INT32 rows, 
		   Q_INT32 cols, 
		   QUANTUM opacity = OPACITY_OPAQUE)
{
	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double sAlpha, dAlpha;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			sAlpha = QUANTUM_MAX - s[PIXEL_ALPHA];
			dAlpha = QUANTUM_MAX - d[PIXEL_ALPHA];

			d[PIXEL_RED]=(QUANTUM)
				AbsoluteValue(s[PIXEL_RED]-(double) d[PIXEL_RED]);
			d[PIXEL_GREEN]=(QUANTUM)
				AbsoluteValue(s[PIXEL_GREEN]-(double) d[PIXEL_GREEN]);
			d[PIXEL_BLUE]=(QUANTUM)
				AbsoluteValue(s[PIXEL_BLUE]-(double) d[PIXEL_BLUE]);
			d[PIXEL_ALPHA]=QUANTUM_MAX - (QUANTUM)
				AbsoluteValue(sAlpha-(double) dAlpha);

		}
		dst += dstRowSize;
		src += srcRowSize;
	}

}

void compositeBumpmap(Q_INT32 pixelSize,
		      QUANTUM *dst, 
		      Q_INT32 dstRowSize,
		      const QUANTUM *src, 
		      Q_INT32 srcRowSize,
		      Q_INT32 rows, 
		      Q_INT32 cols, 
		      QUANTUM opacity = OPACITY_OPAQUE)
{
	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double intensity;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			// Is this correct? It's not this way in GM.
			if (s[PIXEL_ALPHA] == OPACITY_TRANSPARENT)
				continue;

			// And I'm not sure whether this is correct, either.
			intensity = ((double)306.0 * s[PIXEL_RED] + 
				     (double)601.0 * s[PIXEL_GREEN] + 
				     (double)117.0 * s[PIXEL_BLUE]) / 1024.0;
			
			d[PIXEL_RED]=(QUANTUM) (((double)
						 intensity * d[PIXEL_RED])/QUANTUM_MAX +0.5);
			d[PIXEL_GREEN]=(QUANTUM) (((double)
						   intensity * d[PIXEL_GREEN])/QUANTUM_MAX +0.5);
			d[PIXEL_BLUE]=(QUANTUM) (((double)
						  intensity * d[PIXEL_BLUE])/QUANTUM_MAX +0.5);
			d[PIXEL_ALPHA]= (QUANTUM) (((double)
						   intensity * d[PIXEL_ALPHA])/QUANTUM_MAX +0.5);
			
			
		}
		dst += dstRowSize;
		src += srcRowSize;
	}

}

void compositeCopy(Q_INT32 pixelSize,
		   QUANTUM *dst, 
		   Q_INT32 dstRowSize,
		   const QUANTUM *src, 
		   Q_INT32 srcRowSize,
		   Q_INT32 rows, 
		   Q_INT32 cols, 
		   QUANTUM /*opacity*/ = OPACITY_OPAQUE)
{
	Q_INT32 linesize = pixelSize * sizeof(QUANTUM) * cols;
	QUANTUM *d;
	const QUANTUM *s;
	d = dst;
	s = src;
	
	while (rows-- > 0) {
		memcpy(d, s, linesize);
		d += dstRowSize;
		s += srcRowSize;
	}
}

void compositeCopyChannel(PIXELTYPE pixel, 
			   Q_INT32 pixelSize,
			   QUANTUM *dst, 
			   Q_INT32 dstRowSize,
			   const QUANTUM *src, 
			   Q_INT32 srcRowSize,
			   Q_INT32 rows, 
			   Q_INT32 cols, 
			  QUANTUM /*opacity*/ = OPACITY_OPAQUE)
{
	QUANTUM *d;
	const QUANTUM *s;
	Q_INT32 i;

	while (rows-- > 0) {
		d = dst;
		s = src;
		
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			d[pixel] = s[pixel];
		}
		
		dst += dstRowSize;
		src += srcRowSize;
	}

}

void compositeCopyRed(Q_INT32 pixelSize,
		      QUANTUM *dst, 
		      Q_INT32 dstRowSize,
		      const QUANTUM *src, 
		      Q_INT32 srcRowSize,
		      Q_INT32 rows, 
		      Q_INT32 cols, 
		      QUANTUM opacity = OPACITY_OPAQUE)
{
	compositeCopyChannel(PIXEL_RED, pixelSize, dst, dstRowSize, src, srcRowSize, rows, cols, opacity);
}

void compositeCopyGreen(Q_INT32 pixelSize,
			QUANTUM *dst, 
			Q_INT32 dstRowSize,
			const QUANTUM *src, 
			Q_INT32 srcRowSize,
			Q_INT32 rows, 
			Q_INT32 cols, 
			QUANTUM opacity = OPACITY_OPAQUE)
{
	compositeCopyChannel(PIXEL_GREEN, pixelSize, dst, dstRowSize, src, srcRowSize, rows, cols, opacity);
}

void compositeCopyBlue(Q_INT32 pixelSize,
		       QUANTUM *dst, 
		       Q_INT32 dstRowSize,
		       const QUANTUM *src, 
		       Q_INT32 srcRowSize,
		       Q_INT32 rows, 
		       Q_INT32 cols, 
		       QUANTUM opacity = OPACITY_OPAQUE)
{
	compositeCopyChannel(PIXEL_BLUE, pixelSize, dst, dstRowSize, src, srcRowSize, rows, cols, opacity);
}


void compositeCopyOpacity(Q_INT32 pixelSize,
			  QUANTUM *dst, 
			  Q_INT32 dstRowSize,
			  const QUANTUM *src, 
			  Q_INT32 srcRowSize,
			  Q_INT32 rows, 
			  Q_INT32 cols, 
			  QUANTUM opacity = OPACITY_OPAQUE)
{

	// XXX: mess with intensity if there isn't an alpha channel, according to GM.
	compositeCopyChannel(PIXEL_ALPHA, pixelSize, dst, dstRowSize, src, srcRowSize, rows, cols, opacity);

}


void compositeClear(Q_INT32 pixelSize,
		    QUANTUM *dst, 
		    Q_INT32 dstRowSize,
		    const QUANTUM *src, 
		    Q_INT32 /*srcRowSize*/,
		    Q_INT32 rows, 
		    Q_INT32 cols,
		    QUANTUM /*opacity*/ = OPACITY_OPAQUE)
{

	Q_INT32 linesize = pixelSize * sizeof(QUANTUM) * cols;
	QUANTUM *d;
	const QUANTUM *s;

	d = dst;
	s = src;
	
	while (rows-- > 0) {
		memset(d, 0, linesize);
		d += dstRowSize;
	}
	
}


void compositeDissolve(Q_INT32 pixelSize,
		       QUANTUM *dst, 
		       Q_INT32 dstRowSize,
		       const QUANTUM *src, 
		       Q_INT32 srcRowSize,
		       Q_INT32 rows, 
		       Q_INT32 cols, 
		       QUANTUM opacity = OPACITY_OPAQUE)
{
	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double sAlpha, dAlpha;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			// XXX: correct?
			if (s[PIXEL_ALPHA] == OPACITY_TRANSPARENT) continue;

			sAlpha = QUANTUM_MAX - s[PIXEL_ALPHA];
			dAlpha = QUANTUM_MAX - d[PIXEL_ALPHA];
			
			d[PIXEL_RED]=(QUANTUM) (((double) sAlpha*s[PIXEL_RED]+
					 	 (QUANTUM_MAX -sAlpha)*d[PIXEL_RED])/QUANTUM_MAX +0.5);
			d[PIXEL_GREEN]= (QUANTUM) (((double) sAlpha*s[PIXEL_GREEN]+
						   (QUANTUM_MAX -sAlpha)*d[PIXEL_GREEN])/QUANTUM_MAX +0.5);
			d[PIXEL_BLUE] = (QUANTUM) (((double) sAlpha*s[PIXEL_BLUE]+
						  (QUANTUM_MAX -sAlpha)*d[PIXEL_BLUE])/QUANTUM_MAX +0.5);
			d[PIXEL_ALPHA] = OPACITY_OPAQUE;
		}
		dst += dstRowSize;
		src += srcRowSize;
	}

}


void compositeDisplace(Q_INT32 pixelSize,
		       QUANTUM *dst, 
		       Q_INT32 dstRowSize,
		       const QUANTUM *src, 
		       Q_INT32 srcRowSize,
		       Q_INT32 rows, 
		       Q_INT32 cols, 
		       QUANTUM /*opacity*/ = OPACITY_OPAQUE)
{
	Q_INT32 linesize = pixelSize * sizeof(QUANTUM) * cols;
	QUANTUM *d;
	const QUANTUM *s;
	d = dst;
	s = src;
	
	while (rows-- > 0) {
		memcpy(d, s, linesize);
		d += dstRowSize;
		s += srcRowSize;
	}

}

#if 0
void compositeModulate(Q_INT32 pixelSize,
		       QUANTUM *dst, 
		       Q_INT32 dstRowSize,
		       const QUANTUM *src, 
		       Q_INT32 srcRowSize,
		       Q_INT32 rows, 
		       Q_INT32 cols, 
		       QUANTUM opacity = OPACITY_OPAQUE)
{
	if (opacity == OPACITY_TRANSPARENT) 
		return;

	QUANTUM *d;
	const QUANTUM *s;

	Q_INT32 i;

	double sAlpha, dAlpha;
	long offset;

	while (rows-- > 0) {
		d = dst;
		s = src;
		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			// XXX: correct?
			if (s[PIXEL_ALPHA] == OPACITY_TRANSPARENT) continue;

			sAlpha = QUANTUM_MAX - s[PIXEL_ALPHA];
			dAlpha = QUANTUM_MAX - d[PIXEL_ALPHA];
			

			offset=(long) (PixelIntensityToQuantum(&source)-midpoint);
			if (offset == 0)
				continue;
			TransformHSL(d[PIXEL_RED],d[PIXEL_GREEN],d[PIXEL_BLUE],
				     &hue,&saturation,&brightness);
			brightness+=(percent_brightness*offset)/midpoint;
			if (brightness < 0.0)
				brightness=0.0;
			else
				if (brightness > 1.0)
					brightness=1.0;
			HSLTransform(hue,saturation,brightness,&d[PIXEL_RED],
				     &d[PIXEL_GREEN],&d[PIXEL_BLUE]);
			
			
		}
		dst += dstRowSize;
		src += srcRowSize;
	}


}


void compositeThreshold(Q_INT32 pixelSize,
			QUANTUM *dst, 
			Q_INT32 dstRowSize,
			const QUANTUM *src, 
			Q_INT32 srcRowSize,
			Q_INT32 rows, 
			Q_INT32 cols, 
			QUANTUM opacity = OPACITY_OPAQUE)
{
	Q_INT32 linesize = pixelSize * sizeof(QUANTUM) * cols;
	QUANTUM *d;
	const QUANTUM *s;
	QUANTUM alpha;
	QUANTUM invAlpha;
	Q_INT32 i;

}

#endif

void compositeColorize(Q_INT32 pixelSize,
		       QUANTUM *dst, 
		       Q_INT32 dstRowSize,
		       const QUANTUM *src, 
		       Q_INT32 srcRowSize,
		       Q_INT32 rows, 
		       Q_INT32 cols, 
		       QUANTUM opacity = OPACITY_OPAQUE)
{
	Q_INT32 linesize = pixelSize * sizeof(QUANTUM) * cols;
	QUANTUM *d;
	const QUANTUM *s;
	QUANTUM alpha;
	QUANTUM invAlpha;
	Q_INT32 i;


}


void compositeLuminize(Q_INT32 pixelSize,
		       QUANTUM *dst, 
		       Q_INT32 dstRowSize,
		       const QUANTUM *src, 
		       Q_INT32 srcRowSize,
		       Q_INT32 rows, 
		       Q_INT32 cols, 
		       QUANTUM opacity = OPACITY_OPAQUE)
{
	Q_INT32 linesize = pixelSize * sizeof(QUANTUM) * cols;
	QUANTUM *d;
	const QUANTUM *s;
	QUANTUM alpha;
	QUANTUM invAlpha;
	Q_INT32 i;

}

void compositeErase(Q_INT32 pixelSize,
		    QUANTUM *dst, 
		    Q_INT32 dstRowSize,
		    const QUANTUM *src, 
		    Q_INT32 srcRowSize,
		    Q_INT32 rows, 
		    Q_INT32 cols, 
		    QUANTUM /*opacity*/ = OPACITY_OPAQUE)
{
	QUANTUM *d;
	const QUANTUM *s;
	Q_INT32 i;

	while (rows-- > 0) {
		d = dst;
		s = src;

		for (i = cols; i > 0; i--, d += pixelSize, s += pixelSize) {
			if (d[PIXEL_ALPHA] < s[PIXEL_ALPHA]) {
				continue;
			}
			else {
				d[PIXEL_ALPHA] = s[PIXEL_ALPHA];
			}

		}

		dst += dstRowSize;
		src += srcRowSize;
	}


}

#endif

