/* $XConsortium: cir_blt.c,v 1.1 94/03/28 21:48:22 dpw Exp $ */
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
 * Modifications: Harm Hanemaayer <hhanemaa@cs.ruu.nl>
 *
 * Id: cir_im.c,v 0.7 1993/09/16 01:07:25 scooper Exp
 */



#include "misc.h"
#include "xf86.h"
#include "X.h"
#include "Xos.h"
#include "Xmd.h"
#include "Xproto.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "cfb.h"
#include "cfbmskbits.h"
#include "cfb8bit.h"

#include "mergerop.h"
#include "vgaBank.h"
#include "compiler.h"
#include "os.h"		/* For FatalError */

#include "cir_driver.h"

extern pointer vgaBase;


/*
 * This is the mid-level BitBlt function that calls the appropriate
 * low level function, depending on size, specific chipset, etc.
 *
 * We could also use the BitBLT engine for non-GXcopy blits; do they
 * ever happen?
 */


void
CirrusBitBlt (pdstBase, psrcBase, widthSrc, widthDst, x, y,
	      x1, y1, w, h, xdir, ydir, alu, planemask)
     pointer pdstBase, psrcBase;	/* start of src bitmap */
     int widthSrc, widthDst;
     int x, y, x1, y1, w, h;	/* Src x,y; Dst x1,y1; Dst (w)idth,(h)eight */
     int xdir, ydir;
     int alu;
     unsigned long planemask;
{
  unsigned int psrc, pdst;
  int i;

#if 0
  ErrorF("x = %d, y = %d, x1 = %d, y1 = %d, w = %d, h = %d, xdir = %d, ydir = %d\n",
      x, y, x1, y1, w, h, xdir, ydir);
#endif      

  if (alu == GXcopy && (planemask & 0xFF) == 0xFF)
    {

      if (!HAVEBITBLTENGINE()) {
          if (xdir == 1 && ydir == 1) {
              /* Special case for extended write mode bitblt (scroll up). */
              if (w >= 32 && (x & 7) == (x1 & 7)) {
                  CirrusLatchedBitBlt(x, y, x1, y1, w, h, widthDst);
                  return;
              }
#if 1
              /* Call Cirrus framebuffer memcpy routine for remaining */
              /* forward blits. */
              CirrusSimpleBitBlt(x, y, x1, y1, w, h, widthDst);
              return;
#endif              
          }

	  if (xdir == 1 && ydir == -1) {
              /* Special case for reversed extended write mode bitblt */
              /* (scroll down). */
              if (w >= 32 && (x & 7) == (x1 & 7)) {
                  CirrusLatchedBitBltReversed(x, y, x1, y1, w, h, -widthDst);
                  return;
              }
          }

          /* Let cfb do remaining (non-forward) blits. */
          vgaBitBlt(pdstBase, psrcBase, widthSrc, widthDst, x, y,
	      x1, y1, w, h, xdir, ydir, alu, planemask);
	  return;
      }

      /* We have a hardware BitBLT engine. */
      /* For small areas, use the Cirrus framebuffer memcpy routine. */
      if (w * h < 200) {
#if 1
          if (xdir == 1 && ydir == 1) {
              CirrusSimpleBitBlt(x, y, x1, y1, w, h, widthDst);
              return;
          }
#endif          
          vgaBitBlt(pdstBase, psrcBase, widthSrc, widthDst, x, y,
              x1, y1, w, h, xdir, ydir, alu, planemask);
          return;
      }

      /* Use the hardware blit. */
      /* The line-by-line blits can probably be largely avoided similar to */
      /* the paradise/wd driver. -- HH */

      if (widthSrc < 0)
          widthSrc = -widthSrc;
      if (widthDst < 0)
          widthDst = -widthDst;

      if (xdir == 1 && ydir == -1 && y != y1) {
          /* Tranform to xdir = -1 blit. */
          xdir = -1;
      }

      if (xdir == 1)		/* left to right */
	{
	  if (ydir == 1)	/* top to bottom */
	    {
	      psrc = (y * widthSrc) + x;
	      pdst = (y1 * widthDst) + x1;
	    }
	  else
	    /* bottom to top */
	    {
	      psrc = ((y + h - 1) * widthSrc) + x;
	      pdst = ((y1 + h - 1) * widthDst) + x1;
	    }
	}
      else
	/* right to left */
	{
	  if (ydir == 1)	/* top to bottom */
	    {
	      psrc = (y * widthSrc) + x + w - 1;
	      pdst = (y1 * widthDst) + x1 + w - 1;
	    }
	  else
	    /* bottom to top */
	    {
	      psrc = ((y + h - 1) * widthSrc) + x + w - 1;
	      pdst = ((y1 + h - 1) * widthDst) + x1 + w - 1;
	    }
	}

      /* I could probably do the line by line */
      /* blits a little faster by breaking the */
      /* blit regions into rectangles */
      /* and blitting those, making sure I don't */
      /* overwrite stuff. However, the */
      /* difference between the line by line */
      /* and block blits isn't noticable to */
      /* me, so I think I'll blow it off. */
      
      /* HH: Hmm, it is very noticable; moving up while editing is */
      /* significantly slower (this is now fixed). */
     
      if (xdir == 1)
	{
	  if (ydir == 1)
	    {			/* Nothing special, straight blit */
	      CirrusBLTBitBlt(pdst, psrc, widthDst, widthSrc, w, h, 1);
	    }
	  else
	    /* Line by line, going up. */
	    {
	      for (i = 0; i < h; i++)
		{
		  CirrusBLTBitBlt(pdst, psrc, widthDst, widthSrc, w, 1, 1);
		  psrc -= widthSrc;
		  pdst -= widthDst;
		}
	    }
	}
      else
	{

	  if (ydir == 1)	/* Line by line, going down and to the left */
	    {
	      for (i = 0; i < h; i++)
		{
		  CirrusBLTBitBlt(pdst, psrc, widthDst, widthSrc, w, 1, -1);
		  psrc += widthSrc;
		  pdst += widthDst;
		}
	    }
	  else
	    /* Another stock blit, albeit backwards */
	    {
	      CirrusBLTBitBlt(pdst, psrc, widthDst, widthSrc, w, h, -1);
	    }
	}
    }
  else
    /* Non GXcopy, or planemask != 0xff. */
    vgaBitBlt(pdstBase, psrcBase, widthSrc, widthDst, x, y,
        x1, y1, w, h, xdir, ydir, alu, planemask);
}

