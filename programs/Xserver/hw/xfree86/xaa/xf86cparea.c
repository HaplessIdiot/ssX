/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xf86cparea.c,v 3.5 1997/10/13 17:16:51 hohndel Exp $ */

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
 * cfb copy area
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

Author: Keith Packard

*/

/*
 * Contents:
 * xf86CopyArea()
 *     High-level hook.
 * xf86DoBitblt()
 *     Dispatches to low-level function for set of BitBLTs.
 * xf86ScreenToScreenBitBlt()
 *     Dispatches to low-level function for single BitBLTs.
 * 
 * Written by Harm Hanemaayer (H.Hanemaayer@inter.nl.net).
 */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
/* PSZ doesn't matter. */
#define PSZ 8
#include	"cfb.h"

#include	"xf86.h"
#include	"xf86xaa.h"
#include	"xf86local.h"
#include 	"xf86Priv.h"

RegionPtr
xf86CopyArea(pSrcDrawable, pDstDrawable,
            pGC, srcx, srcy, width, height, dstx, dsty)
    register DrawablePtr pSrcDrawable;
    register DrawablePtr pDstDrawable;
    GC *pGC;
    int srcx, srcy;
    int width, height;
    int dstx, dsty;
{
    if(pDstDrawable->type == DRAWABLE_WINDOW) {
	if(pSrcDrawable->type == DRAWABLE_WINDOW)
            return (*xf86GCInfoRec.cfbBitBltDispatch)(
            	pSrcDrawable, pDstDrawable,
            	pGC, srcx, srcy, width, height, dstx, dsty,
            	xf86DoBitBlt, 0L);
	else if(xf86AccelInfoRec.DoImageWrite &&
	  	CHECKPLANEMASK(xf86AccelInfoRec.ImageWriteFlags) &&
              	CHECKROP(xf86AccelInfoRec.ImageWriteFlags) &&
              	CHECKRGBEQUAL(xf86AccelInfoRec.ImageWriteFlags) &&
	      	!((xf86AccelInfoRec.ImageWriteFlags & NO_GXCOPY) && 
	       	 (pGC->alu == GXcopy))) 
            return (*xf86GCInfoRec.cfbBitBltDispatch)(
            	pSrcDrawable, pDstDrawable,
            	pGC, srcx, srcy, width, height, dstx, dsty,
            	xf86AccelInfoRec.DoImageWrite, 0L);
    }

    SYNC_CHECK;

    return (*xf86GCInfoRec.CopyAreaFallBack)(pSrcDrawable, pDstDrawable, pGC,
   	    srcx, srcy, width, height, dstx, dsty);
}

void
xf86DoBitBlt(pSrc, pDst, alu, prgnDst, pptSrc, planemask, bitPlane)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned int    planemask;
    int		    bitPlane;
{
    BoxPtr pbox;
    int nbox;

    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
				/* temporaries for shuffling rectangles */
    DDXPointPtr pptTmp, pptNew1, pptNew2;
				/* shuffling boxes entails shuffling the
				   source points too */
    int w, h;
    int xdir;			/* 1 = left right, -1 = right left/ */
    int ydir;			/* 1 = top down, -1 = bottom up */
    int careful;

    /* XXX we have to err on the side of safety when both are windows,
     * because we don't know if IncludeInferiors is being used.
     */
    careful = 1;

    pbox = REGION_RECTS(prgnDst);
    nbox = REGION_NUM_RECTS(prgnDst);

    pboxNew1 = NULL;
    pptNew1 = NULL;
    pboxNew2 = NULL;
    pptNew2 = NULL;
    if (careful && (pptSrc->y < pbox->y1))
    {
        /* walk source botttom to top */
	ydir = -1;

	if (nbox > 1)
	{
	    /* keep ordering in each band, reverse order of bands */
	    pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    if(!pboxNew1)
		return;
	    pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pptNew1)
	    {
	        DEALLOCATE_LOCAL(pboxNew1);
	        return;
	    }
	    pboxBase = pboxNext = pbox+nbox-1;
	    while (pboxBase >= pbox)
	    {
	        while ((pboxNext >= pbox) &&
		       (pboxBase->y1 == pboxNext->y1))
		    pboxNext--;
	        pboxTmp = pboxNext+1;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp <= pboxBase)
	        {
		    *pboxNew1++ = *pboxTmp++;
		    *pptNew1++ = *pptTmp++;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew1 -= nbox;
	    pbox = pboxNew1;
	    pptNew1 -= nbox;
	    pptSrc = pptNew1;
        }
    }
    else
    {
	/* walk source top to bottom */
	ydir = 1;
    }

    if (careful && (pptSrc->x < pbox->x1))
    {
	/* walk source right to left */
        xdir = -1;

	if (nbox > 1)
	{
	    /* reverse order of rects in each band */
	    pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pboxNew2 || !pptNew2)
	    {
		if (pptNew2) DEALLOCATE_LOCAL(pptNew2);
		if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
		if (pboxNew1)
		{
		    DEALLOCATE_LOCAL(pptNew1);
		    DEALLOCATE_LOCAL(pboxNew1);
		}
	        return;
	    }
	    pboxBase = pboxNext = pbox;
	    while (pboxBase < pbox+nbox)
	    {
	        while ((pboxNext < pbox+nbox) &&
		       (pboxNext->y1 == pboxBase->y1))
		    pboxNext++;
	        pboxTmp = pboxNext;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp != pboxBase)
	        {
		    *pboxNew2++ = *--pboxTmp;
		    *pptNew2++ = *--pptTmp;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew2 -= nbox;
	    pbox = pboxNew2;
	    pptNew2 -= nbox;
	    pptSrc = pptNew2;
	}
    }
    else
    {
	/* walk source left to right */
        xdir = 1;
    }

    xf86AccelInfoRec.ScreenToScreenBitBlt(nbox, pptSrc, pbox, xdir, ydir, 
        alu, planemask);

    if (pboxNew2)
    {
	DEALLOCATE_LOCAL(pptNew2);
	DEALLOCATE_LOCAL(pboxNew2);
    }
    if (pboxNew1)
    {
	DEALLOCATE_LOCAL(pptNew1);
	DEALLOCATE_LOCAL(pboxNew1);
    }
}


/*
 * This functions performs the BitBlts, calling the low-level BitBLT
 * routines that talk to the hardware.
 */

void xf86ScreenToScreenBitBlt(nbox, pptSrc, pbox, xdir, ydir, alu, planemask)
    int nbox;
    DDXPointPtr pptSrc;
    BoxPtr pbox;
    int xdir, ydir;
    int alu;
    unsigned planemask;
{
    int dirsetup;
    if ((!(xf86AccelInfoRec.Flags & ONLY_TWO_BITBLT_DIRECTIONS)
    || (xdir == ydir)) &&
    (!(xf86AccelInfoRec.Flags & ONLY_LEFT_TO_RIGHT_BITBLT)
    || (xdir == 1))) {
        xf86AccelInfoRec.SetupForScreenToScreenCopy(
            xdir, ydir, alu, planemask, -1);
        for (; nbox; pbox++, pptSrc++, nbox--)
            xf86AccelInfoRec.SubsequentScreenToScreenCopy(pptSrc->x, pptSrc->y,
                pbox->x1, pbox->y1, pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
        if (xf86AccelInfoRec.Flags & BACKGROUND_OPERATIONS)
            SET_SYNC_FLAG;
        return;
    }

   if (xf86AccelInfoRec.Flags & ONLY_LEFT_TO_RIGHT_BITBLT) {
        /*
         * This is the case of a chip that only supports xdir = 1,
         * with ydir = 1 or ydir = -1, but we have xdir = -1.
         * This is adapted from cir_blitLG.c.
         */
        xf86AccelInfoRec.SetupForScreenToScreenCopy(
            1, ydir, alu, planemask, -1);
        for (; nbox; pbox++, pptSrc++, nbox--)
            if (pptSrc->y != pbox->y1 || pptSrc->x >= pbox->x1)
                /* No problem. Do a xdir = 1 blit instead. */
                xf86AccelInfoRec.SubsequentScreenToScreenCopy(
                    pptSrc->x, pptSrc->y, pbox->x1, pbox->y1,
                    pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
            else 
            {
                /*
                 * This is the difficult case. Needs striping into
                 * non-overlapping horizontal chunks.
                 */
                int stripeWidth, w, fullStripes, extra, i;
                stripeWidth = 16;
                w = pbox->x2 - pbox->x1;
                if (pbox->x1 - pptSrc->x < stripeWidth)
                    stripeWidth = pbox->x1 - pptSrc->x;
                fullStripes = w / stripeWidth;
                extra = w % stripeWidth;

                /* First, take care of the little bit on the far right */
                if (extra)
                    xf86AccelInfoRec.SubsequentScreenToScreenCopy(
                        pptSrc->x + fullStripes * stripeWidth, pptSrc->y,
                        pbox->x1 + fullStripes * stripeWidth, pbox->y1,
                        extra, pbox->y2 - pbox->y1);

                /* Now, take care of the rest of the blit */
                for (i = fullStripes - 1; i >= 0; i--)
                    xf86AccelInfoRec.SubsequentScreenToScreenCopy(
                        pptSrc->x + i * stripeWidth, pptSrc->y,
                        pbox->x1 + i * stripeWidth, pbox->y1,
                        stripeWidth, pbox->y2 - pbox->y1);
            }
        if (xf86AccelInfoRec.Flags & BACKGROUND_OPERATIONS)
            SET_SYNC_FLAG;
        return;
    }

    /*
     * Now the case of a chip that only supports xdir = ydir = 1 or
     * xdir = ydir = -1, but we have xdir != ydir.
     */
    dirsetup = 0;	/* No direction set up yet. */
    for (; nbox; pbox++, pptSrc++, nbox--) {
        if (xdir == 1 && pptSrc->y != pbox->y1) {
            /* Do a xdir = ydir = -1 blit instead. */
            if (dirsetup != -1) {
                xf86AccelInfoRec.SetupForScreenToScreenCopy(
                    -1, -1, alu, planemask, -1);
                dirsetup = -1;
            }
            xf86AccelInfoRec.SubsequentScreenToScreenCopy(pptSrc->x, pptSrc->y,
                pbox->x1, pbox->y1, pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
        }
        else if (xdir == -1 && pptSrc->y != pbox->y1) {
            /* Do a xdir = ydir = 1 blit instead. */
            if (dirsetup != 1) {
                xf86AccelInfoRec.SetupForScreenToScreenCopy(
                    1, 1, alu, planemask, -1);
                dirsetup = 1;
            }
            xf86AccelInfoRec.SubsequentScreenToScreenCopy(pptSrc->x, pptSrc->y,
                pbox->x1, pbox->y1, pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
        }
        else
            if (xdir == 1) {
                /*
                 * xdir = 1, ydir = -1.
                 * Perform line-by-line xdir = ydir = 1 blits, going up.
                 */
                int i;
                if (dirsetup != 1) {
                    xf86AccelInfoRec.SetupForScreenToScreenCopy(
                        1, 1, alu, planemask, -1);
                    dirsetup = 1;
                }
                for (i = pbox->y2 - pbox->y1 - 1; i >= 0; i--)
                    xf86AccelInfoRec.SubsequentScreenToScreenCopy(
                        pptSrc->x, pptSrc->y + i, pbox->x1, pbox->y1 + i,
                        pbox->x2 - pbox->x1, 1);
            }
            else {
                /*
                 * xdir = -1, ydir = 1.
                 * Perform line-by-line xdir = ydir = -1 blits, going down.
                 */
                int i;
                if (dirsetup != -1) {
                    xf86AccelInfoRec.SetupForScreenToScreenCopy(
                        -1, -1, alu, planemask, -1);
                    dirsetup = -1;
                }
                for (i = 0; i < pbox->y2 - pbox->y1; i++)
                    xf86AccelInfoRec.SubsequentScreenToScreenCopy(
                        pptSrc->x, pptSrc->y + i, pbox->x1, pbox->y1 + i,
                        pbox->x2 - pbox->x1, 1);
            }
    } /* next box */
    if (xf86AccelInfoRec.Flags & BACKGROUND_OPERATIONS)
        SET_SYNC_FLAG;
}



static void MoveDWORDS_FixedBase(dest, src, dwords)
   register CARD32* dest;
   register CARD32* src;
   register int dwords;
{
     while(dwords & ~0x03) {
	 *dest = *src;
	 *dest = *(src + 1);
	 *dest = *(src + 2);
	 *dest = *(src + 3);	
	 dwords -= 4;
	 src += 4;
     }
     switch(dwords) {
	case 0:	return;
	case 1: *dest = *src;
		return;
	case 2: *dest = *src;
		*dest = *(src + 1);
		return;
	case 3: *dest = *src;
		*dest = *(src + 1);
 		*dest = *(src + 2);
		return;
    }
}

static void MoveDWORDS(dest, src, dwords)
   register CARD32* dest;
   register CARD32* src;
   register int dwords;
{
     while(dwords & ~0x03) {
	*dest = *src;
	*(dest + 1) = *(src + 1);
	*(dest + 2) = *(src + 2);
	*(dest + 3) = *(src + 3);
	src += 4;
	dest += 4;
	dwords -= 4;
     }	
     switch(dwords) {
	case 0:	return;
	case 1: *dest = *src;
		return;
	case 2: *dest = *src;
		*(dest + 1) = *(src + 1);
		return;
	case 3: *dest = *src;
		*(dest + 1) = *(src + 1);
 		*(dest + 2) = *(src + 2);
		return;
    }
}

/* 
     xf86DoImageWrite transfers 8, 16, 24 and 32 bpp image data
  to the image transfer window with dword scanline padding.
*/
	

void
xf86DoImageWrite(pSrc, pDst, alu, prgnDst, pptSrc, planemask, bitPlane)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned int    planemask;
    int		    bitPlane;
{
    int srcwidth, skipleft, dwords;
    int x,w,h;
    unsigned char* psrcBase;			/* start of image */
    register unsigned char* srcPntr;		/* index into the image */
    int Bpp = xf86bpp >> 3; 
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);

    cfbGetByteWidthAndPointer(pSrc, srcwidth, psrcBase);

    xf86AccelInfoRec.SetupForImageWrite(alu, planemask, -1);

    for(; nbox; pbox++, pptSrc++, nbox--) {
	x = pbox->x1;
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

        srcPntr = psrcBase + (pptSrc->y * srcwidth) + (pptSrc->x * Bpp);

	if((skipleft = (int)srcPntr & 0x03)) {
	    if(!(xf86AccelInfoRec.ImageWriteFlags & LEFT_EDGE_CLIPPING)) {
		skipleft = 0;
		goto BAD_ALIGNMENT;
	    }

	    if(Bpp == 3)
		skipleft = 4 - skipleft;
	    else
	    	skipleft /= Bpp;

	    if((x < skipleft) && !(xf86AccelInfoRec.ImageWriteFlags &
				 LEFT_EDGE_CLIPPING_NEGATIVE_X)) {
		skipleft = 0;
		goto BAD_ALIGNMENT;
	    }
	    x -= skipleft;	     
	    w += skipleft;
	
	    if(Bpp == 3)
	    	srcPntr = (unsigned char*)(srcPntr - (3*skipleft));  
	    else   
	    	srcPntr = (unsigned char*)((int)srcPntr & ~0x03);     
	}
	
BAD_ALIGNMENT:

	switch(Bpp) {
	   case 1:	dwords = (w + 3) >> 2;
			break;
	   case 2:	dwords = (w + 1) >> 1;
			break;
	   case 3:	dwords = ((w + 1) * 3) >> 2;
			break;
	   default:	dwords = w;
			break;
	}

	xf86AccelInfoRec.SubsequentImageWrite(x,pbox->y1,w,h,skipleft);

	if(dwords <= xf86AccelInfoRec.ImageWriteRange) {
           if(srcwidth == (dwords << 2)) {
	   	int decrement = xf86AccelInfoRec.ImageWriteRange/dwords;

	   	while(h > decrement) {
	    	    MoveDWORDS(xf86AccelInfoRec.ImageWriteBase,
	 		(CARD32*)srcPntr, dwords * decrement);
	   	    srcPntr += (srcwidth * decrement);
		    h -= decrement;
	   	}
	   	if(h) {
	     	    MoveDWORDS(xf86AccelInfoRec.ImageWriteBase,
	 		(CARD32*)srcPntr, dwords * h);
	   	}
	    } else {
    	    	while(h--) {
	    	    MoveDWORDS(xf86AccelInfoRec.ImageWriteBase,
	 		(CARD32*)srcPntr, dwords);
	   	    srcPntr += srcwidth;
     	    	}
	    }
	} else {
    	    while(h--) {
	   	MoveDWORDS_FixedBase(xf86AccelInfoRec.ImageWriteBase, 
			(CARD32*)srcPntr, dwords);
	   	srcPntr += srcwidth;
	    }
	}

        if((xf86AccelInfoRec.ImageWriteFlags & CPU_TRANSFER_PAD_QWORD) &&
		((dwords * h) & 0x01))
	    *(CARD32*)xf86AccelInfoRec.ImageWriteBase = 0;

    }

    
    if(!(xf86AccelInfoRec.Flags & NO_SYNC_AFTER_CPU_COLOR_EXPAND))
    	xf86AccelInfoRec.Sync();
}

void
xf86ScanlineDoImageWrite(pSrc, pDst, alu, prgnDst, pptSrc, planemask, bitPlane)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned int    planemask;
    int		    bitPlane;
{
    int srcwidth, skipleft, dwords;
    int x,y,w,h;
    unsigned char* psrcBase;			/* start of image */
    register unsigned char* srcPntr;		/* index into the image */
    int Bpp = xf86bpp >> 3;
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);
    Bool PlusOne;
    int FirstLine = xf86AccelInfoRec.ImageWriteOffset;
    int SecondLine = (xf86AccelInfoRec.ImageWriteRange / 2) + FirstLine;
    CARD32 *FirstLinePntr = 
		(CARD32 *)(xf86AccelInfoRec.FramebufferBase + FirstLine);
    CARD32 *SecondLinePntr = 
		(CARD32*)(xf86AccelInfoRec.FramebufferBase + SecondLine);

    cfbGetByteWidthAndPointer(pSrc, srcwidth, psrcBase);

    /* setup for a left-to-right/top-to-bottom ScreenToScreenCopy */
    xf86AccelInfoRec.SetupForScreenToScreenCopy(1, 1, alu, planemask, -1);

    for(; nbox; pbox++, pptSrc++, nbox--) {
	x = pbox->x1;
	y = pbox->y1;
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	PlusOne = (h & 0x01);
	h >>= 1;	/* h is now linepairs */

        srcPntr = psrcBase + (pptSrc->y * srcwidth) + (pptSrc->x * Bpp);

	if((skipleft = (int)srcPntr & 0x03)) {
	    if(!(xf86AccelInfoRec.ImageWriteFlags & LEFT_EDGE_CLIPPING)) {
		skipleft = 0;
		goto BAD_ALIGNMENT_SCANLINE;
	    }
	    if(Bpp == 3)
		skipleft = 4 - skipleft;
	    else
		skipleft /= Bpp;

	    if ((x < skipleft) && !(xf86AccelInfoRec.ImageWriteFlags &
				  LEFT_EDGE_CLIPPING_NEGATIVE_X)) {
		skipleft = 0;
		goto BAD_ALIGNMENT_SCANLINE;
	    }
	    x -= skipleft;
	    w += skipleft;

	    if (Bpp == 3)
	    	srcPntr = (unsigned char*)(srcPntr - (3*skipleft));  
	    else
	    	srcPntr = (unsigned char*)((int)srcPntr & ~0x03);     
	}

BAD_ALIGNMENT_SCANLINE:
	
	switch(Bpp) {
	   case 1:	dwords = (w + 3) >> 2;
			break;
	   case 2:	dwords = (w + 1) >> 1;
			break;
	   case 3:	dwords = ((w + 1) * 3) >> 2;
			break;
	   default:	dwords = w;
			break;
	}

	if (dwords > (xf86AccelInfoRec.ImageWriteRange / 2)) {
		ErrorF("ImageWriteRange too small - truncating, errors.\n");
		dwords = xf86AccelInfoRec.ImageWriteRange / 2;
	}

	while(h--){
	   MoveDWORDS(FirstLinePntr,(CARD32*)srcPntr,dwords);
           xf86AccelInfoRec.SubsequentScanlineScreenToScreenCopy(
					FirstLine, skipleft, x, y++, w);
	   if (!(xf86AccelInfoRec.Flags & COP_FRAMEBUFFER_CONCURRENCY))
		xf86AccelInfoRec.Sync();
	   srcPntr += srcwidth;
	   MoveDWORDS(SecondLinePntr,(CARD32*)srcPntr,dwords);
           xf86AccelInfoRec.SubsequentScanlineScreenToScreenCopy(
					SecondLine, skipleft, x, y++, w);
	   if (!(xf86AccelInfoRec.Flags & COP_FRAMEBUFFER_CONCURRENCY))
		xf86AccelInfoRec.Sync();
	   srcPntr += srcwidth;
	}
	if(PlusOne) {
	   MoveDWORDS(FirstLinePntr,(CARD32*)srcPntr,dwords);
           xf86AccelInfoRec.SubsequentScanlineScreenToScreenCopy(
					FirstLine, skipleft, x, y, w);
	   if (!(xf86AccelInfoRec.Flags & COP_FRAMEBUFFER_CONCURRENCY))
		xf86AccelInfoRec.Sync();
	}
    }

    if (xf86AccelInfoRec.Flags & BACKGROUND_OPERATIONS)
    	SET_SYNC_FLAG;
}

extern WindowPtr *WindowTable;

void xf86ImageWrite(x, y, w, h, src, srcwidth, rop, planemask)
    int x;
    int y;
    int w;
    int h;
    void *src;
    int srcwidth;
    int rop;
    unsigned planemask;
{
    ScreenPtr pScreen;
    PixmapPtr pix;
    WindowPtr rootWin;
    BoxRec box;
    DDXPointRec ptSrc;
    RegionRec rgnDst;
    pScreen = screenInfo.screens[xf86ScreenIndex];
    rootWin = WindowTable[pScreen->myNum];
    pix = GetScratchPixmapHeader(pScreen, w, h, rootWin->drawable.depth,
        rootWin->drawable.bitsPerPixel, srcwidth, src);
    ptSrc.x = 0;
    ptSrc.y = 0;
    box.x1 = x;
    box.y1 = y;
    box.x2 = x + w;
    box.y2 = y + h;
    REGION_INIT(pScreen, &rgnDst, &box, 1);


/* These checks will need to be more robust if xf86pcache.c ever wants
   to send something other than GXCopy w/out planemask to xf86ImageWrite 
	(MArk) */ 

    if(xf86AccelInfoRec.DoImageWrite &&
	!(xf86AccelInfoRec.ImageWriteFlags & NO_GXCOPY)) {
           (*xf86AccelInfoRec.DoImageWrite)((DrawablePtr)pix,
	 	(DrawablePtr)rootWin, rop, &rgnDst, &ptSrc, planemask, 0L);
    } else {
    	SYNC_CHECK;
    	(*xf86AccelInfoRec.ImageWriteFallBack)((DrawablePtr)pix,
	 	(DrawablePtr)rootWin, rop, &rgnDst, &ptSrc, planemask);
    }
    REGION_UNINIT(pScreen, &rgnDst);
    FreeScratchPixmapHeader(pix);
}










