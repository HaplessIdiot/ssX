/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xf86spoly.c,v 3.2 1998/03/20 21:07:30 hohndel Exp $ */
 

#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "mi.h"
#define PSZ 8	/* PSZ doesn't matter */
#include "cfb.h"

#include "xf86.h"
#include "xf86xaa.h"
#include "xf86pcache.h"
#include "xf86local.h"

extern void RotatePattern(unsigned char*, int, int);

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

static int  adjLeftX, adjTopY;
static int  patternx, patterny;


static void 
NonExpandingPROGRAMMED_ORIGIN_SCREEN_ORIGIN(x, y, w, h)
   int x, y, w, h;
{
    xf86AccelInfoRec.SubsequentFill8x8Pattern(
				(- adjLeftX) & 7,
				(- adjTopY) & 7, 
				x, y, w, h);
}

static void 
NonExpandingPROGRAMMED_ORIGIN(x, y, w, h)
   int x, y, w, h;
{
    xf86AccelInfoRec.SubsequentFill8x8Pattern(
				(x - adjLeftX) & 7,
				(y - adjTopY) & 7, 
				x, y, w, h);
}

static void 
NonExpandingSCREEN_ORIGIN(x, y, w, h)
   int x, y, w, h;
{
    xf86AccelInfoRec.SubsequentFill8x8Pattern(
				patternx,
				patterny, 
				x, y, w, h);
}

static void 
NonExpandingOTHER(x, y, w, h)
   int x, y, w, h;
{
    xf86AccelInfoRec.SubsequentFill8x8Pattern(
				patternx + ((y - adjTopY) & 7) * 8,
				patterny + ((x - adjLeftX) & 7),
				x, y, w, h);
}

	
/***********************/

static void 
ExpandingPROGRAMMED_ORIGIN_SCREEN_ORIGIN(x, y, w, h)
   int x, y, w, h;
{
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand(
				(- adjLeftX) & 7,
				(- adjTopY) & 7, 
				x, y, w, h);
}

static void 
ExpandingPROGRAMMED_ORIGIN (x, y, w, h)
   int x, y, w, h;
{
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand(
				(x - adjLeftX) & 7,
				(y - adjTopY) & 7, 
				x, y, w, h);
}

static void 
ExpandingSCREEN_ORIGIN (x, y, w, h)
   int x, y, w, h;
{
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand(
				patternx,
				patterny, 
				x, y, w, h);
}

static void 
ExpandingPROGRAMMED_BITS (x, y, w, h)
   int x, y, w, h;
{
    unsigned int pattern[2];
    pattern[0] = patternx;
    pattern[1] = patterny;
    RotatePattern((unsigned char *)&pattern,
	                    (x - adjLeftX) & 7,
	                    (y - adjTopY) & 7);
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand(
		            pattern[0], pattern[1],
		            x, y, w, h);
}


static void 
ExpandingOTHER (x, y, w, h)
   int x, y, w, h;
{
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand(
				patternx + ((y - adjTopY) & 7) * 64,
		            	patterny + ((x - adjLeftX) & 7),
				x, y, w, h);
}


/**************************/

static void
ExpandingTrapPROGRAMMED_ORIGIN_SCREEN_ORIGIN(y, h, left, dxl, dyl, el, right, dxr, dyr, er)
	int y, h, left, dxl, dyl, el, right, dxr, dyr, er;
{
    xf86AccelInfoRec.Subsequent8x8TrapezoidColorExpand(
			(- adjLeftX) & 7, (- adjTopY) & 7, 
			y, h, left, dxl, dyl, el, right, dxr, dyr, er);
}


void
xf86FillPolygonStippled(pDrawable, pGC, shape, mode, count, ptsIn)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		shape;
    int		mode;
    int		count;
    DDXPointPtr	ptsIn;
{
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
    extern CacheInfoPtr xf86CacheInfo; 
    CacheInfoPtr    pci = NULL;
    PixmapPtr	    pPixmap = NULL; 
    void (*SubsequentRectFunction)(); 
    void (*SubsequentTrapFunction)() = NULL; 

#ifdef NO_ONE_RECT
    if (REGION_NUM_RECTS(pGC->pCompositeClip) != 1) {
	miFillPolygon (pDrawable, pGC, shape, mode, count, ptsIn);
	return;
    }
#endif
 

   if(pGC->fillStyle == FillTiled) {
       if ((xf86AccelInfoRec.Flags & PIXMAP_CACHE)
	&& (!(xf86AccelInfoRec.PatternFlags & HARDWARE_PATTERN_NO_PLANEMASK)
	|| (pGC->planemask & xf86AccelInfoRec.FullPlanemask) == 
	       xf86AccelInfoRec.FullPlanemask)
        && xf86CacheTile(pGC->tile.pixmap)) 
	{
       		pPixmap = pGC->tile.pixmap;
        }
    } else { /* FillStippled or FillOpaqueStippled */
        if ((xf86AccelInfoRec.Flags & PIXMAP_CACHE)
        && (!(xf86AccelInfoRec.Flags & DO_NOT_CACHE_STIPPLES))
	&& (!(xf86AccelInfoRec.PatternFlags & HARDWARE_PATTERN_NO_PLANEMASK)
	|| (pGC->planemask & xf86AccelInfoRec.FullPlanemask) == 
	       xf86AccelInfoRec.FullPlanemask)
        && xf86CacheStipple(pDrawable, pGC)) 
	{
       		pPixmap = pGC->stipple;
        }
    }

    if(pPixmap) pci = 
&xf86CacheInfo[((xf86PixPrivPtr)(pPixmap->devPrivates[xf86PixmapIndex].ptr))->slot];

    if(!pci || !pci->flags) { /* no hardware pattern */
	miFillPolygon (pDrawable, pGC, shape, mode, count, ptsIn);
	return;
    }

    if (mode == CoordModePrevious) {
	register DDXPointPtr ppt = ptsIn + 1;

	for (c = 1; c < count; c++, ppt++) {
	    ppt->x += (ppt-1)->x;
	    ppt->y += (ppt-1)->y;
	}
        mode = CoordModeOrigin;
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
    if (shape == Convex) {
    	while (count--) {
	    c = *vertex2p;
	    clip |= (c - vertex1) | (vertex2 - c);
	    c = intToY(c);
	    if (c < y) {
	    	y = c;
	    	vertex1p = vertex2p;
	    }
	    vertex2p++;
	    if (c > maxy)
	    	maxy = c;
    	}
    } else {
	int yFlip = 0;
	dx1 = 1;
	x2 = -1;
	x1 = -1;
    	while (count--) {
	    c = *vertex2p;
	    clip |= (c - vertex1) | (vertex2 - c);
	    c = intToY(c);
	    if (c < y) {
	    	y = c;
	    	vertex1p = vertex2p;
	    }
	    vertex2p++;
	    if (c > maxy)
	    	maxy = c;
	    if (c == x1)
		continue;
	    if (dx1 > 0) {
		if (x2 < 0)
		    x2 = c;
		else
		    dx2 = dx1 = (c - x1) >> 31;
	    }
	    else
		if ((c - x1) >> 31 != dx1) {
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

    if (clip & 0x80008000) {
	miFillPolygon (pDrawable, pGC, shape, mode, vertex2p - (int *) ptsIn, ptsIn);
	return;
    }

    adjLeftX = ((pGC->patOrg.x + pDrawable->x) & 0x07);
    adjTopY = ((pGC->patOrg.y + pDrawable->y) & 0x07);


    if(pci->flags == 1) {
	if(xf86AccelInfoRec.PatternFlags & 
				HARDWARE_PATTERN_PROGRAMMED_ORIGIN) {
	    if(xf86AccelInfoRec.PatternFlags
                        & HARDWARE_PATTERN_SCREEN_ORIGIN)
      		SubsequentRectFunction = 
			NonExpandingPROGRAMMED_ORIGIN_SCREEN_ORIGIN;
	    else
      		SubsequentRectFunction = NonExpandingPROGRAMMED_ORIGIN;
	} else if (xf86AccelInfoRec.PatternFlags
		    & HARDWARE_PATTERN_SCREEN_ORIGIN)
      		SubsequentRectFunction = NonExpandingSCREEN_ORIGIN;
	else
     		SubsequentRectFunction = NonExpandingOTHER;

	patternx = pci->x;
	patterny = pci->y;
	if(xf86AccelInfoRec.PatternFlags & HARDWARE_PATTERN_SCREEN_ORIGIN) {
	        patterny += (- adjLeftX) & 7;
	        patternx += ((- adjTopY) & 7) * 8;
	}
        xf86AccelInfoRec.SetupForFill8x8Pattern(patternx, patterny, 
		pGC->alu, pGC->planemask,
                (pGC->fillStyle == FillStippled) ? pci->bg_color : -1);
     } else { /* pci->flags == 2 */
	if(xf86AccelInfoRec.PatternFlags & 
				HARDWARE_PATTERN_PROGRAMMED_ORIGIN) {
	    if(xf86AccelInfoRec.PatternFlags
                        & HARDWARE_PATTERN_SCREEN_ORIGIN) {
		if(xf86AccelInfoRec.Subsequent8x8TrapezoidColorExpand)
		    SubsequentTrapFunction = 
			ExpandingTrapPROGRAMMED_ORIGIN_SCREEN_ORIGIN;
      		SubsequentRectFunction = 
			ExpandingPROGRAMMED_ORIGIN_SCREEN_ORIGIN;
	    } else
      		SubsequentRectFunction = ExpandingPROGRAMMED_ORIGIN;
	} else if (xf86AccelInfoRec.PatternFlags
		    & HARDWARE_PATTERN_SCREEN_ORIGIN)
      		SubsequentRectFunction = ExpandingSCREEN_ORIGIN;
	else if (xf86AccelInfoRec.PatternFlags
		    & HARDWARE_PATTERN_PROGRAMMED_BITS)
     		SubsequentRectFunction = ExpandingPROGRAMMED_BITS;
	else
     		SubsequentRectFunction = ExpandingOTHER;

	if(xf86AccelInfoRec.PatternFlags & HARDWARE_PATTERN_PROGRAMMED_BITS) {
	    if(xf86AccelInfoRec.PatternFlags & 
				HARDWARE_PATTERN_PROGRAMMED_ORIGIN){
	            patternx = pci->pattern0;
	            patterny = pci->pattern1;
	    } else if (xf86AccelInfoRec.PatternFlags &
				HARDWARE_PATTERN_SCREEN_ORIGIN) {
		unsigned int pattern[2];
		pattern[0] = pci->pattern0;
		pattern[1] = pci->pattern1;
		RotatePattern((unsigned char *)&pattern,
	                    (- adjLeftX) & 7,
	                    (- adjTopY) & 7);
		patternx = pattern[0];
		patterny = pattern[1];
	    } else {
	    /* Otherwise, rotate every time. */
	        patternx = pci->pattern0;
	        patterny = pci->pattern1;
            }
	} else if (xf86AccelInfoRec.PatternFlags
	        & HARDWARE_PATTERN_NOT_LINEAR) {
		patternx = pci->x + (((-adjLeftX) & 7) << 3);
		patterny = pci->y + ((-adjTopY) & 7);
  	} else {
                /*
                 * Video memory location of pattern data.
                 * Note that the x-coordinate is defined in "bit" or
                 * "stencil" units for a monochrome pattern.
                 */
	        patternx = pci->x * xf86AccelInfoRec.BitsPerPixel;
	        patterny = pci->y;
	        if ((xf86AccelInfoRec.PatternFlags 
		    & HARDWARE_PATTERN_SCREEN_ORIGIN) 
		    && !(xf86AccelInfoRec.PatternFlags & 
			 HARDWARE_PATTERN_PROGRAMMED_ORIGIN)) {
	            patterny += (- adjLeftX) & 7;
	            patternx += ((- adjTopY) & 7) * 64;
	        }
	}     
        if (pGC->fillStyle == FillStippled)
    	        /* Setup for transparency compare. */
                xf86AccelInfoRec.SetupFor8x8PatternColorExpand(
                    patternx, patterny, -1, pGC->fgPixel,
                    pGC->alu, pGC->planemask);
	else if (pGC->fillStyle == FillTiled)
                /* Tiled. Colors from cache info. */
                xf86AccelInfoRec.SetupFor8x8PatternColorExpand(
                    patternx, patterny, pci->bg_color, pci->fg_color,
                    pGC->alu, pGC->planemask);
        else    /* Opaque stippled, no transparency. */
                xf86AccelInfoRec.SetupFor8x8PatternColorExpand(
                    patternx, patterny, pGC->bgPixel, pGC->fgPixel,
                    pGC->alu, pGC->planemask);
    }


    origin = intToX(origin);
    vertex2p = vertex1p;
    vertex2 = vertex1 = *vertex2p++;
    if (vertex2p == endp)
	vertex2p = (int *) ptsIn;

    yoffset = pDrawable->y;
    for (;;) {
	if (y == intToY(vertex1)) {
	    do {
	    	if (vertex1p == (int *) ptsIn)
		    vertex1p = endp;
	    	c = *--vertex1p;
	    	Setup (c,x1,vertex1,dx1,dy1,e1,sign1,step1,DX1)
	    } while (y >= intToY(vertex1));
	    h = dy1;
	} else {
	    Step(x1,dx1,dy1,e1,sign1,step1)
	    h = intToY(vertex1) - y;
	}
	if (y == intToY(vertex2)) {
	    do {
	    	c = *vertex2p++;
	    	if (vertex2p == endp)
		    vertex2p = (int *) ptsIn;
	    	Setup (c,x2,vertex2,dx2,dy2,e2,sign2,step2,DX2)
	    } while (y >= intToY(vertex2));
	    if (dy2 < h)
		h = dy2;
	} else {
	    Step(x2,dx2,dy2,e2,sign2,step2)
	    if ((c = (intToY(vertex2) - y)) < h)
		h = c;
	}

	/* fill spans for this segment */
        if(DX1 | DX2) {
      	  if(SubsequentTrapFunction && (h > 6)) {
	     if(x1 == x2) {
		while(x1 == x2) {
	     	   y++;
	    	   if (!--h)
		   	break;
	    	   Step(x1,dx1,dy1,e1,sign1,step1)
	    	   Step(x2,dx2,dy2,e2,sign2,step2)
		}
		if(y == maxy) break;
    		if(!h) continue;
	     }

             if(x1 < x2)
 	     	(*SubsequentTrapFunction)(y + yoffset, h,
					x1, DX1, dy1, e1, 
					x2 - 1, DX2, dy2, e2);
	     else
	     	(*SubsequentTrapFunction)(y + yoffset, h,
					x2, DX2, dy2, e2, 
					x1 - 1, DX1, dy1, e1);
	     y += h;	
             if(--h) {
	     	FixError(x1,dx1,dy1,e1,sign1,step1,h);
	     	FixError(x2,dx2,dy2,e2,sign2,step2,h);
		h = 0;
	     }  	
	  } else {
	     for (;;) {
	    	if (x2 > x1)
	            (*SubsequentRectFunction)(x1, y + yoffset, x2 - x1, 1);
	        else if (x1 > x2)
	            (*SubsequentRectFunction)(x2, y + yoffset, x1 - x2, 1);
	     	y++;
	    	if (!--h)
		   break;
	    	Step(x1,dx1,dy1,e1,sign1,step1)
	    	Step(x2,dx2,dy2,e2,sign2,step2)
	     }
	  }
	} else {
	    if (x2 > x1)
	        (*SubsequentRectFunction)(x1, y + yoffset, x2 - x1, h);
	    else if (x1 > x2)
	        (*SubsequentRectFunction)(x2, y + yoffset, x1 - x2, h);

	    y += h;
	    h = 0;
        } 
	if (y == maxy)
	    break;
    }
    if (xf86AccelInfoRec.Flags & BACKGROUND_OPERATIONS)
        SET_SYNC_FLAG;
}
