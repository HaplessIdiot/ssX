/*
 * $XFree86: xc/programs/Xserver/render/mitrap.c,v 1.1 2002/05/13 05:25:33 keithp Exp $
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mi.h"
#include "picturestr.h"
#include "mipict.h"

PicturePtr
miCreateAlphaPicture (ScreenPtr pScreen, PictFormatPtr pPictFormat,
		      CARD16 width, CARD16 height)
{
    PixmapPtr	    pPixmap;
    PicturePtr	    pPicture;
    GCPtr	    pGC;
    int		    error;
    xRectangle	    rect;

    if (!pPictFormat)
    {
	pPictFormat = PictureMatchFormat (pScreen, 8, PICT_a8);
	if (!pPictFormat)
	    return 0;
    }

    pPixmap = (*pScreen->CreatePixmap) (pScreen, width, height, 
					pPictFormat->depth);
    if (!pPixmap)
	return 0;
    pGC = GetScratchGC (pPixmap->drawable.depth, pScreen);
    if (!pGC)
    {
	(*pScreen->DestroyPixmap) (pPixmap);
	return 0;
    }
    ValidateGC (&pPixmap->drawable, pGC);
    rect.x = 0;
    rect.y = 0;
    rect.width = width;
    rect.height = height;
    (*pGC->ops->PolyFillRect)(&pPixmap->drawable, pGC, 1, &rect);
	
    pPicture = CreatePicture (0, &pPixmap->drawable, pPictFormat,
			      0, 0, serverClient, &error);
    (*pScreen->DestroyPixmap) (pPixmap);
    return pPicture;
}

void
miTrapezoidBounds (int ntrap, xTrapezoid *traps, BoxPtr box)
{
    box->y1 = FixedToInt (traps->top);
    box->y2 = FixedToInt (FixedCeil (traps->bottom));
    box->x1 = FixedToInt (min (traps->left.p1.x, traps->left.p2.x));
    box->x2 = FixedToInt (FixedCeil (max (traps->right.p1.x, traps->right.p2.x)));
    ntrap--;
    traps++;
    while (ntrap--)
    {
	INT16 y1 = FixedToInt (traps->top);
	INT16 y2 = FixedToInt (FixedCeil (traps->bottom));
	INT16 x1 = FixedToInt (min (traps->left.p1.x, traps->left.p2.x));
	INT16 x2 = FixedToInt (FixedCeil (max (traps->right.p1.x, traps->right.p2.x)));

	traps++;
	if (y1 < box->y1)
	    box->y1 = y1;
	if (y2 > box->y2)
	    box->y2 = y2;
	if (x1 < box->x1)
	    box->x1 = x1;
	if (x2 > box->x2)
	    box->x2 = x2;
    }
    /* XXX bug in fbtrap.c */
    box->x1--;
    box->y1--;
    box->x2++;
    box->y2++;
}

void
miTrapezoids (CARD8	    op,
	      PicturePtr    pSrc,
	      PicturePtr    pDst,
	      PictFormatPtr maskFormat,
	      INT16	    xSrc,
	      INT16	    ySrc,
	      int	    ntrap,
	      xTrapezoid    *traps)
{
    ScreenPtr		pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr    ps = GetPictureScreen(pScreen);
    PicturePtr		pPicture = 0;
    BoxRec		bounds;
    
    if (maskFormat)
    {
	miTrapezoidBounds (ntrap, traps, &bounds);
	pPicture = miCreateAlphaPicture (pScreen, maskFormat,
					 bounds.x2 - bounds.x1,
					 bounds.y2 - bounds.y1);
	if (!pPicture)
	    return;
    }
    while (ntrap--)
    {
	if (!maskFormat)
	{
	    miTrapezoidBounds (1, traps, &bounds);
	    pPicture = miCreateAlphaPicture (pScreen, maskFormat,
					     bounds.x2 - bounds.x1,
					     bounds.y2 - bounds.y1);
	    if (!pPicture)
		break;
	}
	(*ps->RasterizeTrapezoid) (pPicture, traps, 
				   -bounds.x1, -bounds.y1);
	if (!maskFormat)
	{
	    CompositePicture (op, pSrc, pPicture, pDst,
			      xSrc, ySrc, 0, 0, bounds.x1, bounds.y1,
			      bounds.x2 - bounds.x1,
			      bounds.y2 - bounds.y1);
	    FreePicture (pPicture, 0);
	}
	traps++;
    }
    /* XXX adjust xSrc and ySrc */
    if (maskFormat)
    {
	CompositePicture (op, pSrc, pPicture, pDst,
			  xSrc, ySrc, 0, 0, bounds.x1, bounds.y1,
			  bounds.x2 - bounds.x1,
			  bounds.y2 - bounds.y1);
	FreePicture (pPicture, 0);
    }
}
