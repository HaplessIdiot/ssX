/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaFillPoly.c,v 1.1.2.1 1998/06/22 13:54:02 dawes Exp $ */

/*
 * Copyright 1996  The XFree86 Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * HARM HANEMAAYER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Written by Harm Hanemaayer (H.Hanemaayer@inter.nl.net).
 */
 
/*
 * Filled solid polygons, based on cfbply1rct.c.
 *
 * Fill polygons using calls to low-level span fill. Because the math
 * between spans can be done concurrently with the drawing of the span
 * with a graphics coprocessor operation, this is faster than just
 * using miFillPoly, which first calculates all the spans and then
 * calls FillSpans.
 *
 * The function accepts one clipping rectangle.
 */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "xf86str.h"
#include "mi.h"
#define PSZ 8   /* PSZ doesn't matter */
#include "cfb.h"

#include "xaa.h"
#include "xaalocal.h"


#define Setup(c,x,vertex,dx,dy,e,sign,step,DX) {\
    x = intToX(vertex); \
    if (dy = intToY(c) - y) { \
    	DX = dx = intToX(c) - x; \
	step = 0; \
    	if (dx >= 0) \
    	{ \
	    e = 0; \
	    sign = 1; \
	    if (dx >= dy) {\
	    	step = dx / dy; \
	    	dx %= dy; \
	    } \
    	} \
    	else \
    	{ \
	    e = 1 - dy; \
	    sign = -1; \
	    dx = -dx; \
	    if (dx >= dy) { \
		step = - (dx / dy); \
		dx %= dy; \
	    } \
    	} \
    } \
    x += origin; \
    vertex = c; \
}

#define Step(x,dx,dy,e,sign,step) {\
    x += step; \
    if ((e += dx) > 0) \
    { \
	x += sign; \
	e -= dy; \
    } \
}

#define FixError(x, dx, dy, e, sign, step, h)	{	\
	   e += (h) * dx;				\
	   x += (h) * step;				\
	   if(e > 0) {					\
		x += e * sign/dy;			\
		e %= dy;				\
	   	if(e) {					\
		   x += sign;				\
		   e -= dy;				\
		}					\
	   } 	 					\
}


void
XAAFillPolygonSolid(
    DrawablePtr	pDrawable,
    GCPtr	pGC,
    int		shape,
    int		mode,
    int		count,
    DDXPointPtr	ptsIn )
{
    XAAInfoRecPtr   infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int		    maxy;
    int		    origin;
    register int    vertex1, vertex2;
    int		    c;
    BoxPtr	    extents;
    int		    clip;
    int		    y;
    int		    *vertex1p, *vertex2p;
    int		    *endp;
    int		    x1, x2;
    int		    dx1, dx2;
    int		    dy1, dy2;
    int		    e1, e2;
    int		    step1, step2;
    int		    sign1, sign2;
    int		    h;
    int		    yoffset;
    int		    DX1, DX2;  /* for trapezoid fills */

    if (mode == CoordModePrevious) {
	register DDXPointPtr ppt = ptsIn + 1;

	for (c = 1; c < count; c++, ppt++) {
	    ppt->x += (ppt-1)->x;
	    ppt->y += (ppt-1)->y;
	}
        mode = CoordModeOrigin;
    }
    
    if (REGION_NUM_RECTS(pGC->pCompositeClip) != 1) {
	miFillPolygon (pDrawable, pGC, shape, mode, count, ptsIn);
	return;
    }

    origin = *((int *) &pDrawable->x);
    origin -= (origin & 0x8000) << 1;
    extents = &pGC->pCompositeClip->extents;
    vertex1 = *((int *) &extents->x1) - origin;
    vertex2 = *((int *) &extents->x2) - origin - 0x00010001;
    clip = 0;
    y = 32767;
    maxy = 0;
    vertex2p = (int *) ptsIn;
    endp = vertex2p + count;
    if (shape == Convex)
    {
    	while (count--)
    	{
	    c = *vertex2p;
	    clip |= (c - vertex1) | (vertex2 - c);
	    c = intToY(c);
	    if (c < y) 
	    {
	    	y = c;
	    	vertex1p = vertex2p;
	    }
	    vertex2p++;
	    if (c > maxy)
	    	maxy = c;
    	}
    }
    else
    {
	int yFlip = 0;
	dx1 = 1;
	x2 = -1;
	x1 = -1;
    	while (count--)
    	{
	    c = *vertex2p;
	    clip |= (c - vertex1) | (vertex2 - c);
	    c = intToY(c);
	    if (c < y) 
	    {
	    	y = c;
	    	vertex1p = vertex2p;
	    }
	    vertex2p++;
	    if (c > maxy)
	    	maxy = c;
	    if (c == x1)
		continue;
	    if (dx1 > 0)
	    {
		if (x2 < 0)
		    x2 = c;
		else
		    dx2 = dx1 = (c - x1) >> 31;
	    }
	    else
		if ((c - x1) >> 31 != dx1) 
		{
		    dx1 = ~dx1;
		    yFlip++;
		}
	    x1 = c;
       	}
	x1 = (x2 - c) >> 31;
	if (x1 != dx1)
	    yFlip++;
	if (x1 != dx2)
	    yFlip++;
	if (yFlip != 2) 
	    clip = 0x8000;
    }
    if (y == maxy)
	return;

    if (clip & 0x80008000)
    {
	miFillPolygon (pDrawable, pGC, shape, mode, vertex2p - (int *) ptsIn, ptsIn);
	return;
    }

     (*infoRec->SetupForSolidFill)(infoRec->pScrn, pGC->fgPixel, pGC->alu,
        pGC->planemask);

    origin = intToX(origin);
    vertex2p = vertex1p;
    vertex2 = vertex1 = *vertex2p++;
    if (vertex2p == endp)
	vertex2p = (int *) ptsIn;

    yoffset = pDrawable->y;
    while(1)
    {
	if (y == intToY(vertex1))
	{
	    do
	    {
	    	if (vertex1p == (int *) ptsIn)
		    vertex1p = endp;
	    	c = *--vertex1p;
	    	Setup (c,x1,vertex1,dx1,dy1,e1,sign1,step1,DX1)
	    } while (y >= intToY(vertex1));
	    h = dy1;
	}
	else
	{
	    Step(x1,dx1,dy1,e1,sign1,step1)
	    h = intToY(vertex1) - y;
	}
	if (y == intToY(vertex2))
	{
	    do
	    {
	    	c = *vertex2p++;
	    	if (vertex2p == endp)
		    vertex2p = (int *) ptsIn;
	    	Setup (c,x2,vertex2,dx2,dy2,e2,sign2,step2,DX2)
	    } while (y >= intToY(vertex2));
	    if (dy2 < h)
		h = dy2;
	}
	else
	{
	    Step(x2,dx2,dy2,e2,sign2,step2)
	    if ((c = (intToY(vertex2) - y)) < h)
		h = c;
	}

	/* fill spans for this segment */
        if(DX1 | DX2) {
      	  if(infoRec->SubsequentSolidFillTrap && (h > 6)) {
	     if(x1 == x2) {
		while(x1 == x2) {
	     	   y++;
	    	   if (!--h) break;
	    	   Step(x1,dx1,dy1,e1,sign1,step1)
	    	   Step(x2,dx2,dy2,e2,sign2,step2)
		}
		if(y == maxy) break;
    		if(!h) continue;
	     }

             if(x1 < x2)
 	     	(*infoRec->SubsequentSolidFillTrap)(infoRec->pScrn,
					y + yoffset, h,
					x1, DX1, dy1, e1, 
					x2 - 1, DX2, dy2, e2);
	     else
 	     	(*infoRec->SubsequentSolidFillTrap)(infoRec->pScrn,
					y + yoffset, h,
					x2, DX2, dy2, e2, 
					x1 - 1, DX1, dy1, e1);
	     y += h;	
             if(--h) {
	     	FixError(x1,dx1,dy1,e1,sign1,step1,h);
	     	FixError(x2,dx2,dy2,e2,sign2,step2,h);
		h = 0;
	     }  	
	  } else {
	     while(1) {
	    	if (x2 > x1)
	            (*infoRec->SubsequentSolidFillRect)(infoRec->pScrn,
	            		x1, y + yoffset, x2 - x1, 1);
	        else if (x1 > x2)
	            (*infoRec->SubsequentSolidFillRect)(infoRec->pScrn,
	                    x2, y + yoffset, x1 - x2, 1);
	     	y++;
	    	if (!--h) break;
	    	Step(x1,dx1,dy1,e1,sign1,step1)
	    	Step(x2,dx2,dy2,e2,sign2,step2)
	     }
	  }
	} else {
	    if (x2 > x1)
	        (*infoRec->SubsequentSolidFillRect)(infoRec->pScrn,
	            x1, y + yoffset, x2 - x1, h);
	    else if (x1 > x2)
	        (*infoRec->SubsequentSolidFillRect)(infoRec->pScrn,
	                x2, y + yoffset, x1 - x2, h);

	    y += h;
	    h = 0;
        } 
	if (y == maxy) break;
    }
    SET_SYNC_FLAG(infoRec);
}
