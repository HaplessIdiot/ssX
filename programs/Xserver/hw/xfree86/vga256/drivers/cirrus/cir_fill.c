/* $XConsortium: cir_fill.c,v 1.1 94/03/28 21:49:00 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cir_fill.c,v 3.0 1994/04/29 14:10:09 dawes Exp $ */
/*
 *
 * Copyright 1993 by Bill Reynolds, Santa Fe, New Mexico
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Bill Reynolds not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Bill Reynolds makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * BILL REYNOLDS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL BILL REYNOLDS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Bill Reynolds, bill@goshawk.lanl.gov
 *
 * Reworked by: Simon P. Cooper, <scooper@vizlab.rutgers.edu>
 * Modified by: Harm Hanemaayer, <hhanemaa@cs.ruu.nl>
 *
 * Id: cir_fill.c,v 0.7 1993/09/16 01:07:25 scooper Exp
 */

/*
 * This file contains mid-level solid fill functions that call different
 * low-level functions depending on size, card configuration etc.
 */

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
#include "cfbrrop.h"
#include "mergerop.h"
#include "vgaBank.h"
#include "xf86.h"
#include "vga.h"	/* For vgaBase. */

#include "compiler.h"

#include "cir_driver.h"

void CirrusFillBoxSolid();


void
CirrusFillRectSolidCopy (pDrawable, pGC, nBox, pBox)
    DrawablePtr	    pDrawable;
    GCPtr	    pGC;
    int		    nBox;
    BoxPtr	    pBox;
{
  unsigned long rrop_xor,rrop_and;
  RROP_FETCH_GC(pGC);

  CirrusFillBoxSolid (pDrawable, nBox, pBox, rrop_xor, 0, pGC->alu);
}


/* This is what CirrusPolyFillRect uses for non-GXcopy fills. */

void
CirrusFillRectSolidGeneral (pDrawable, pGC, nBox, pBox)
    DrawablePtr	    pDrawable;
    GCPtr	    pGC;
    int		    nBox;
    BoxPtr	    pBox;
{
  unsigned long rrop_xor,rrop_and;
  RROP_FETCH_GC(pGC);

  if ((pGC->planemask & 0xff) == 0xff)
      CirrusFillBoxSolid(pDrawable, nBox, pBox, pGC->fgPixel, pGC->bgPixel,
          pGC->alu);
  else
      cfbFillRectSolidGeneral(pDrawable, pGC, nBox, pBox);
}


/* General mid-level solid fill. Makes a choice of low-level routines. */

void
CirrusFillBoxSolid (pDrawable, nBox, pBox, pixel1, pixel2, alu)
    DrawablePtr	    pDrawable;
    int		    nBox;
    BoxPtr	    pBox;
    unsigned long   pixel1;
    unsigned long   pixel2;
    int	            alu;
{
  unsigned char   *pdstBase;
  Bool            flag;
  int		  widthDst;
  int             h;
  int		  w;

  if (pDrawable->type == DRAWABLE_WINDOW)
    {
      pdstBase = (unsigned char *)
	(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
      widthDst = (int)
	(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind);
    }
  else
    {
      pdstBase = (unsigned char *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
      widthDst = (int)(((PixmapPtr)pDrawable)->devKind);
    }

  flag = CHECKSCREEN(pdstBase);	/* On screen? */

  if (alu != GXcopy && !HAVEBITBLTENGINE())
      /* We can't handle non-GXcopy fills without the blit engine. */
      flag = 0;

  if (flag)
    {

      for (; nBox; nBox--, pBox++)
	{
	  unsigned int dstAddr;
	      
	  h = pBox->y2 - pBox->y1;
	  w = pBox->x2 - pBox->x1;
	  
          dstAddr = pBox->y1 * widthDst + pBox->x1;

          if (alu == GXcopy) {
              unsigned long bits;

              /* For small widths, we use the specific function for small */
              /* widths (which is not really accelerated). */          
              if (w < 32) {
                  bits = 0xffffffff;
       	          Cirrus32bitFillSmall(pBox->x1, pBox->y1, w, h,
       	              &bits, 1, 0, 0, pixel1, pixel1, widthDst);
       	          continue;
       	      }
	
	      /* We use the color expansion function for solid fills in the
	       * following cases:
	       * - If the chip has no bitblt engine (i.e. 5420/2/4).
	       * - If we have a bitblt engine, but the card is local bus
	       *   and the width is big enough, with different cut-off
	       *   points for the 5426, 5428, and 5429.
	       *   For 5434 (speculative), the bitblt engine is used.
	       */
	      if (!HAVEBITBLTENGINE() ||
	          (cirrusBusType == CIRRUS_FASTBUS &&
	          ((cirrusChip == CLGD5426 && w >= 200) ||
	           (cirrusChip == CLGD5428 && w >= 250) ||
		   (cirrusChip == CLGD5429 && w >= 250)))) {
                  bits = 0xffffffff;
                  CirrusColorExpand32bitFill(pBox->x1, pBox->y1, w, h,
        	      &bits, 1, 0, 0, pixel1, pixel1, widthDst);
        	  continue;
              }

              /* Avoid the blitter setup overhead for remaining small fills. */
              if (w * h < 100) {
                  bits = 0xffffffff;
       	          Cirrus32bitFillSmall(pBox->x1, pBox->y1, w, h,
       	              &bits, 1, 0, 0, pixel1, pixel1, widthDst);
       	          continue;
       	      }

              /* Use the blitter. */
              CirrusBLTColorExpand8x8PatternFill(dstAddr, pixel1, pixel2, w,
                  h, widthDst, CROP_SRC, 0xffffffff, 0xffffffff);

          }
          else {	/* alu != GXcopy */
              /* This is a joke. The alu is always GXcopy. */
              /* When this is changed, we'll be able use the blitter for */
              /* almost all operations (notably Invert will help the */
              /* xstone benchmark, for what it's worth). */
             
              /* OK, it should be fixed now. It is used. And we support all */
              /* rops. */
              /* [Note: Invert basically doubled the blit xstones...] */

	      CirrusBLTColorExpand8x8PatternFill(dstAddr, pixel1, pixel2, w,
	          h, widthDst, cirrus_rop[alu], 0xffffffff, 0xffffffff);
          }
	}
    }
  else vgacfbFillBoxSolid (pDrawable, nBox, pBox, pixel1, pixel2, alu);

}
