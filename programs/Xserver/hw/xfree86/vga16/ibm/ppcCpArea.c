/* $XConsortium: ppcCpArea.c,v 1.3 94/10/12 21:06:18 kaleb Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga16/ibm/ppcCpArea.c,v 3.3 1995/01/28 17:06:02 dawes Exp $ */
/*
 * Copyright IBM Corporation 1987,1988,1989
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that 
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/

/* Header: /andrew/X11/r3src/r3plus/server/ddx/ibm/ppc/RCS/ppcCpArea.c,v 9.4 89/05/07 15:30:29 paul Exp */
/* Source: /andrew/X11/r3src/r3plus/server/ddx/ibm/ppc/RCS/ppcCpArea.c,v */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "X.h"
#include "servermd.h"
#include "misc.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mi.h"

#include "OScompiler.h"

#include "ppc.h"
extern int mfbGCPrivateIndex;

static RegionPtr
ppcCopyAreaFromPixmap( pSrcDrawable, pDstDrawable, pGC, srcx, srcy,
		       width, height, dstx, dsty )
register PixmapPtr pSrcDrawable ;
register DrawablePtr pDstDrawable ;
GC *pGC ;
int srcx, srcy ;
int width, height ;
register int dstx, dsty ;
{
	register RegionPtr prgnDst ;
	ScreenPtr pScreen ;
	RegionPtr prgnExposed ;
	int nbox ;
	/* temporaries for shuffling rectangles */
	xRectangle *origSource ;
	DDXPointRec *origDest ;
	ppcPrivGC * pPriv = (ppcPrivGC *) ( pGC->devPrivates[mfbGCPrivateIndex].ptr ) ;

	/* BY HERE, You know you are going Pixmap to window */
	if ( pPriv->fExpose ) {
		if ( !( origSource = (xRectangle *)
		    ALLOCATE_LOCAL( sizeof( xRectangle ) ) ) )
			return NULL ;
		origSource->x = srcx ;
		origSource->y = srcy ;
		origSource->width = width ;
		origSource->height = height ;
		if ( !( origDest = (DDXPointRec *)
		    ALLOCATE_LOCAL( sizeof( DDXPointRec ) ) ) ) {
			DEALLOCATE_LOCAL( origSource ) ;
			return NULL ;
		}
		origDest->x = dstx ;
		origDest->y = dsty ;
	}
	else {
		origSource = (xRectangle *) 0 ;
		origDest = (DDXPointRec *) 0 ;
	}

	/* clip the left and top edges of the source */
	if ( srcx < 0 ) {
		width += srcx ;
		dstx += srcx ;
		srcx = 0 ;
	}
	if ( srcy < 0 ) {
		height += srcy ;
		dsty += srcy ;
		srcy = 0 ;
	}

	dstx += pDstDrawable->x ;
	dsty += pDstDrawable->y ;

	pScreen = pDstDrawable->pScreen ;
	/* clip the source */
	{
		BoxRec dstBox ;

		dstBox.x1 = dstx ;
		dstBox.x2 = dstx + MIN( width, pSrcDrawable->drawable.width - srcx ) ;
		dstBox.y1 = dsty ;
		dstBox.y2 = dsty + MIN( height, pSrcDrawable->drawable.height - srcy ) ;

		prgnDst = (* pScreen->RegionCreate)( &dstBox, 1 ) ;

if ( ! pPriv->pCompositeClip )
	printf( "Fatal Error! no Composite Clip Region\n" ) ;

		/* clip the shape of to the destination composite clip */
		(* pScreen->Intersect)( prgnDst, prgnDst,
					pPriv->pCompositeClip ) ;
	}

	/* nbox != 0 destination region is visable */
	if ( nbox = REGION_NUM_RECTS(prgnDst) ) {
		{
		register BoxPtr pbox = REGION_RECTS(prgnDst);
		register char *data = pSrcDrawable->devPrivate.ptr ;
		register int stride = pSrcDrawable->devKind ;
		register int dx ;
		register int dy ;

		dx = srcx - dstx ;
		dy = srcy - dsty ;

		for ( ; nbox-- ; pbox++ )
			vgaDrawColorImage( (WindowPtr)pDstDrawable,
				 pbox->x1, pbox->y1,
				 pbox->x2 - pbox->x1,
				 pbox->y2 - pbox->y1,
				 (unsigned char *)data + pbox->x1 + dx
				  + ( ( pbox->y1 + dy ) * stride ),
				 stride,
				 pGC->alu, pGC->planemask ) ;

		}
		if ( origSource ) {
			prgnExposed = miHandleExposures(
					(DrawablePtr) pSrcDrawable,
					pDstDrawable, pGC,
			    		origSource->x, origSource->y,
			    		origSource->width, origSource->height,
			    		origDest->x, origDest->y,
					pGC->planemask ) ;
			DEALLOCATE_LOCAL( origSource ) ;
			DEALLOCATE_LOCAL( origDest ) ;
		}
		else
			prgnExposed = (RegionPtr) 0 ;
	}
	else /* nbox == 0 no visable destination region */
		prgnExposed = (RegionPtr) 0 ;

	(* pScreen->RegionDestroy)( prgnDst ) ;

	return prgnExposed ;
}

/*
 * This now uses the same algorithm a mfb and cfb.
 */

RegionPtr
vga16CopyArea(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty)
register DrawablePtr pSrcDrawable;
register DrawablePtr pDstDrawable;
register GC *pGC;
int srcx, srcy;
int width, height;
int dstx, dsty;
{
    RegionPtr prgnSrcClip;	/* may be a new region, or just a copy */
    Bool freeSrcClip = FALSE;

    RegionPtr prgnExposed;
    RegionRec rgnDst;
    DDXPointPtr pptSrc;
    register DDXPointPtr ppt;
    register BoxPtr pbox;
    int i;
    register int dx;
    register int dy;
    xRectangle origSource;
    DDXPointRec origDest;
    int numRects;
    BoxRec fastBox;
    int fastClip = 0;		/* for fast clipping with pixmap source */
    int fastExpose = 0;		/* for fast exposures with pixmap source */

    /*
     * Check for a few special cases.
     */
    if ( pDstDrawable->type != DRAWABLE_WINDOW )
	return miCopyArea( pSrcDrawable, pDstDrawable, pGC,
			   srcx, srcy, width, height, dstx, dsty ) ;
    /* BY HERE, You know you are going to a Window */
    if ( !( (WindowPtr) pDstDrawable )->realized )
	return NULL ;

    if ( pSrcDrawable->type != DRAWABLE_WINDOW )
	return ppcCopyAreaFromPixmap( (PixmapPtr) pSrcDrawable,
				      pDstDrawable,
				      pGC, srcx, srcy, width,
				      height, dstx, dsty ) ;

    /* Begin code from mfb/cfbCopyArea */

    origSource.x = srcx;
    origSource.y = srcy;
    origSource.width = width;
    origSource.height = height;
    origDest.x = dstx;
    origDest.y = dsty;

    if ((pSrcDrawable != pDstDrawable) &&
	pSrcDrawable->pScreen->SourceValidate)
    {
	(*pSrcDrawable->pScreen->SourceValidate) (pSrcDrawable, srcx, srcy, width, height);
    }

    srcx += pSrcDrawable->x;
    srcy += pSrcDrawable->y;

    /* clip the source */

    if (pSrcDrawable->type == DRAWABLE_PIXMAP)
    {
	if ((pSrcDrawable == pDstDrawable) &&
	    (pGC->clientClipType == CT_NONE))
	{
	    prgnSrcClip = ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip;
	}
	else
	{
	    fastClip = 1;
	}
    }
    else
    {
	if (pGC->subWindowMode == IncludeInferiors)
	{
	    if (!((WindowPtr) pSrcDrawable)->parent)
	    {
		/*
		 * special case bitblt from root window in
		 * IncludeInferiors mode; just like from a pixmap
		 */
		fastClip = 1;
	    }
	    else if ((pSrcDrawable == pDstDrawable) &&
		(pGC->clientClipType == CT_NONE))
	    {
		prgnSrcClip = ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip;
	    }
	    else
	    {
		prgnSrcClip = NotClippedByChildren((WindowPtr)pSrcDrawable);
		freeSrcClip = TRUE;
	    }
	}
	else
	{
	    prgnSrcClip = &((WindowPtr)pSrcDrawable)->clipList;
	}
    }

    fastBox.x1 = srcx;
    fastBox.y1 = srcy;
    fastBox.x2 = srcx + width;
    fastBox.y2 = srcy + height;

    /* Don't create a source region if we are doing a fast clip */
    if (fastClip)
    {
	fastExpose = 1;
	/*
	 * clip the source; if regions extend beyond the source size,
 	 * make sure exposure events get sent
	 */
	if (fastBox.x1 < pSrcDrawable->x)
	{
	    fastBox.x1 = pSrcDrawable->x;
	    fastExpose = 0;
	}
	if (fastBox.y1 < pSrcDrawable->y)
	{
	    fastBox.y1 = pSrcDrawable->y;
	    fastExpose = 0;
	}
	if (fastBox.x2 > pSrcDrawable->x + (int) pSrcDrawable->width)
	{
	    fastBox.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
	    fastExpose = 0;
	}
	if (fastBox.y2 > pSrcDrawable->y + (int) pSrcDrawable->height)
	{
	    fastBox.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;
	    fastExpose = 0;
	}
    }
    else
    {
	(*pGC->pScreen->RegionInit)(&rgnDst, &fastBox, 1);
	(*pGC->pScreen->Intersect)(&rgnDst, &rgnDst, prgnSrcClip);
    }

    dstx += pDstDrawable->x;
    dsty += pDstDrawable->y;

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	if (!((WindowPtr)pDstDrawable)->realized)
	{
	    if (!fastClip)
		(*pGC->pScreen->RegionUninit)(&rgnDst);
	    if (freeSrcClip)
		(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
	    return NULL;
	}
    }

    dx = srcx - dstx;
    dy = srcy - dsty;

    /* Translate and clip the dst to the destination composite clip */
    if (fastClip)
    {
	RegionPtr cclip;

        /* Translate the region directly */
        fastBox.x1 -= dx;
        fastBox.x2 -= dx;
        fastBox.y1 -= dy;
        fastBox.y2 -= dy;

	/* If the destination composite clip is one rectangle we can
	   do the clip directly.  Otherwise we have to create a full
	   blown region and call intersect */
	cclip = ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip;
        if (REGION_NUM_RECTS(cclip) == 1)
        {
	    BoxPtr pBox = REGION_RECTS(cclip);

	    if (fastBox.x1 < pBox->x1) fastBox.x1 = pBox->x1;
	    if (fastBox.x2 > pBox->x2) fastBox.x2 = pBox->x2;
	    if (fastBox.y1 < pBox->y1) fastBox.y1 = pBox->y1;
	    if (fastBox.y2 > pBox->y2) fastBox.y2 = pBox->y2;

	    /* Check to see if the region is empty */
	    if (fastBox.x1 >= fastBox.x2 || fastBox.y1 >= fastBox.y2)
		(*pGC->pScreen->RegionInit)(&rgnDst, NullBox, 0);
	    else
		(*pGC->pScreen->RegionInit)(&rgnDst, &fastBox, 1);
	}
        else
	{
	    /* We must turn off fastClip now, since we must create
	       a full blown region.  It is intersected with the
	       composite clip below. */
	    fastClip = 0;
	    (*pGC->pScreen->RegionInit)(&rgnDst, &fastBox,1);
	}
    }
    else
    {
        (*pGC->pScreen->TranslateRegion)(&rgnDst, -dx, -dy);
    }

    if (!fastClip)
    {
	(*pGC->pScreen->Intersect)(&rgnDst,
				   &rgnDst,
				 ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip);
    }

    /* Do bit blitting */
    numRects = REGION_NUM_RECTS(&rgnDst);
    if (numRects && width && height)
    {
	/*
	 * Don't both allocating some boxes, just call the blit routine
	 * directly.
	 */
	pbox = REGION_RECTS(&rgnDst);
	for (i = numRects; --i >= 0; pbox++, ppt++)
	{
	    vgaBitBlt( (WindowPtr)pDstDrawable, pGC->alu,
			pGC->planemask, pGC->planemask,
			pbox->x1 + dx,		/* x0 */
			pbox->y1 + dy,		/* y0 */
			pbox->x1,		/* x1 */
			pbox->y1,		/* y1 */
			pbox->x2 - pbox->x1,	/* w */
			pbox->y2 - pbox->y1 );	/* h */
	}
    }

    prgnExposed = NULL;
    if (((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->fExpose) 
    {
        /* Pixmap sources generate a NoExposed (we return NULL to do this) */
        if (!fastExpose)
	    prgnExposed =
		miHandleExposures(pSrcDrawable, pDstDrawable, pGC,
				  origSource.x, origSource.y,
				  (int)origSource.width,
				  (int)origSource.height,
				  origDest.x, origDest.y, (unsigned long)0);
    }
    (*pGC->pScreen->RegionUninit)(&rgnDst);
    if (freeSrcClip)
	(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
    return prgnExposed;
}
