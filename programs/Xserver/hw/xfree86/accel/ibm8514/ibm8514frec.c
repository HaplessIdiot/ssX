/* $XConsortium: ibm8514frec.c,v 1.2 94/04/17 20:30:32 dpw Exp $ */
/*
 * Fill rectangles.
 */

/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

Modified for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)

KEVIN E. MARTIN AND TIAGO GONS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL KEVIN E. MARTIN OR TIAGO GONS BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Modified by Tiago Gons (tiago@comosjn.hobby.nl)
*/


/* $XConsortium: ibm8514frec.c,v 1.2 94/04/17 20:30:32 dpw Exp $ */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "cfb.h"
#include "cfbmskbits.h"
#include "mergerop.h"

#include "reg8514.h"
#include "ibm8514.h"

#define NUM_STACK_RECTS	1024

void
ibm8514PolyFillRect(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr pDrawable;
    register GCPtr pGC;
    int		nrectFill; 	/* number of rectangles to fill */
    xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    xRectangle	    *prect;
    RegionPtr	    prgnClip;
    register BoxPtr pbox;
    register BoxPtr pboxClipped;
    BoxPtr	    pboxClippedBase;
    BoxPtr	    pextent;
    BoxRec	    stackRects[NUM_STACK_RECTS];
    cfbPrivGC	    *priv;
    int		    numRects;
    int		    n;
    int		    xorg, yorg;
    int		    width, height;
    PixmapPtr	    pPix;
    int		    pixWidth;
    int		    xrot, yrot;

/* 4-5-93 TCG : is VT visible */
    if (!xf86VTSema)
    {
        cfbPolyFillRect(pDrawable, pGC, nrectFill, prectInit);
	return;
    }

    priv = (cfbPrivGC *) pGC->devPrivates[cfbGCPrivateIndex].ptr;
    prgnClip = priv->pCompositeClip;

    prect = prectInit;
    xorg = pDrawable->x;
    yorg = pDrawable->y;
    if (xorg || yorg)
    {
	prect = prectInit;
	n = nrectFill;
	while(n--)
	{
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
	}
    }

    prect = prectInit;

    numRects = REGION_NUM_RECTS(prgnClip) * nrectFill;
    if (numRects > NUM_STACK_RECTS)
    {
	pboxClippedBase = (BoxPtr)ALLOCATE_LOCAL(numRects * sizeof(BoxRec));
	if (!pboxClippedBase)
	    return;
    }
    else
	pboxClippedBase = stackRects;

    pboxClipped = pboxClippedBase;
	
    if (REGION_NUM_RECTS(prgnClip) == 1)
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = REGION_RECTS(prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    if ((pboxClipped->x1 = prect->x) < x1)
		pboxClipped->x1 = x1;
    
	    if ((pboxClipped->y1 = prect->y) < y1)
		pboxClipped->y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    pboxClipped->x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    pboxClipped->y2 = by2;

	    prect++;
	    if ((pboxClipped->x1 < pboxClipped->x2) &&
		(pboxClipped->y1 < pboxClipped->y2))
	    {
		pboxClipped++;
	    }
    	}
    }
    else
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    BoxRec box;
    
	    if ((box.x1 = prect->x) < x1)
		box.x1 = x1;
    
	    if ((box.y1 = prect->y) < y1)
		box.y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    box.x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    box.y2 = by2;
    
	    prect++;
    
	    if ((box.x1 >= box.x2) || (box.y1 >= box.y2))
	    	continue;
    
	    n = REGION_NUM_RECTS (prgnClip);
	    pbox = REGION_RECTS(prgnClip);
    
	    /* clip the rectangle to each box in the clip region
	       this is logically equivalent to calling Intersect()
	    */
	    while(n--)
	    {
		pboxClipped->x1 = max(box.x1, pbox->x1);
		pboxClipped->y1 = max(box.y1, pbox->y1);
		pboxClipped->x2 = min(box.x2, pbox->x2);
		pboxClipped->y2 = min(box.y2, pbox->y2);
		pbox++;

		/* see if clipping left anything */
		if(pboxClipped->x1 < pboxClipped->x2 && 
		   pboxClipped->y1 < pboxClipped->y2)
		{
		    pboxClipped++;
		}
	    }
    	}
    }

    if (pboxClipped != pboxClippedBase) {
	n = pboxClipped-pboxClippedBase;
	switch (pGC->fillStyle) {
	  case FillSolid:
	    WaitQueue(3);
	    outw(FRGD_COLOR, (short)(pGC->fgPixel));
	    outw(FRGD_MIX, FSS_FRGDCOL | ibm8514alu[pGC->alu]);
	    outw(WRT_MASK, (short)pGC->planemask);

	    pboxClipped = pboxClippedBase;
	    while (n--) {
		WaitQueue(5);
		outw(CUR_X, (short)(pboxClipped->x1));
		outw(CUR_Y, (short)(pboxClipped->y1));
		outw(MAJ_AXIS_PCNT,
		      (short)(pboxClipped->x2 - pboxClipped->x1 - 1));
		outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT |
		      (short)(pboxClipped->y2 - pboxClipped->y1 - 1));
		outw(CMD, CMD_RECT | INC_Y | INC_X | DRAW | PLANAR | WRTDATA);

		pboxClipped++;
	    }

	    WaitQueue(2);
	    outw(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
	    outw(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
 	    break;
	  case FillTiled:
	    xrot = pDrawable->x + pGC->patOrg.x;
	    yrot = pDrawable->y + pGC->patOrg.y;

	    pPix = pGC->tile.pixmap;
	    width = pPix->drawable.width;
	    height = pPix->drawable.height;
	    pixWidth = PixmapBytePad(width, pPix->drawable.depth);

	    pboxClipped = pboxClippedBase;
#ifdef PIXPRIV
	    if (ibm8514CacheTile(pPix)) {
		while (n--) {
		    ibm8514CImageFill(pPix->slot,
				      pboxClipped->x1, pboxClipped->y1,
				      pboxClipped->x2 - pboxClipped->x1,
				      pboxClipped->y2 - pboxClipped->y1,
				      xrot, yrot,
				      ibm8514alu[pGC->alu], pGC->planemask);
		    pboxClipped++;
		}
	    } else
#endif
	    {
		while (n--) {
		    ibm8514ImageFill(pboxClipped->x1, pboxClipped->y1,
				     pboxClipped->x2 - pboxClipped->x1,
				     pboxClipped->y2 - pboxClipped->y1,
				     pPix->devPrivate.ptr, pixWidth,
				     width, height, xrot, yrot,
				     ibm8514alu[pGC->alu], pGC->planemask);
		    pboxClipped++;
		}
	    }
	    break;
	  case FillStippled:
	    xrot = pDrawable->x + pGC->patOrg.x;
	    yrot = pDrawable->y + pGC->patOrg.y;

	    pPix = pGC->stipple;
	    width = pPix->drawable.width;
	    height = pPix->drawable.height;
	    pixWidth = PixmapBytePad(width, pPix->drawable.depth);

	    pboxClipped = pboxClippedBase;
#ifdef PIXPRIV
	    if (ibm8514CacheStipple(pPix)) {
		while (n--) {
		    ibm8514CImageStipple(pPix->slot,
					 pboxClipped->x1, pboxClipped->y1,
					 pboxClipped->x2 - pboxClipped->x1,
					 pboxClipped->y2 - pboxClipped->y1,
					 xrot, yrot, pGC->fgPixel,
					 ibm8514alu[pGC->alu], pGC->planemask);
		    pboxClipped++;
		}
	    } else
#endif
	    {
		while (n--) {
		    ibm8514ImageStipple(pboxClipped->x1, pboxClipped->y1,
					pboxClipped->x2 - pboxClipped->x1,
					pboxClipped->y2 - pboxClipped->y1,
					pPix->devPrivate.ptr, pixWidth,
					width, height, xrot, yrot,
					pGC->fgPixel, 0,
					ibm8514alu[pGC->alu],
					pGC->planemask, 0);
		    pboxClipped++;
		}
	    }
	    break;
	  case FillOpaqueStippled:
	    xrot = pDrawable->x + pGC->patOrg.x;
	    yrot = pDrawable->y + pGC->patOrg.y;

	    pPix = pGC->stipple;
	    width = pPix->drawable.width;
	    height = pPix->drawable.height;
	    pixWidth = PixmapBytePad(width, pPix->drawable.depth);

	    pboxClipped = pboxClippedBase;
#ifdef PIXPRIV
	    if (ibm8514CacheOpStipple(pPix)) {
		while (n--) {
		    ibm8514CImageOpStipple(pPix->slot,
					   pboxClipped->x1, pboxClipped->y1,
					   pboxClipped->x2 - pboxClipped->x1,
					   pboxClipped->y2 - pboxClipped->y1,
					   xrot, yrot,
					   pGC->fgPixel, pGC->bgPixel,
					   ibm8514alu[pGC->alu],
					   pGC->planemask);
		    pboxClipped++;
		}
	    } else
#endif
	    {
		while (n--) {
		    ibm8514ImageStipple(pboxClipped->x1, pboxClipped->y1,
					pboxClipped->x2 - pboxClipped->x1,
					pboxClipped->y2 - pboxClipped->y1,
					pPix->devPrivate.ptr, pixWidth,
					width, height, xrot, yrot,
					pGC->fgPixel, pGC->bgPixel,
					ibm8514alu[pGC->alu],
					pGC->planemask, 1);
		    pboxClipped++;
		}
	    }
	    break;
	}
    }
    if (pboxClippedBase != stackRects)
    	DEALLOCATE_LOCAL(pboxClippedBase);
}
