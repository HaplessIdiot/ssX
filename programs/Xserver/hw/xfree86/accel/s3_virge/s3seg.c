/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3_virge/s3seg.c,v 3.1tsi Exp $ */
/*

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL AND KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL DIGITAL OR KEVIN E. MARTIN BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

Modified for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)

*/

/*
 * Modified by Amancio Hasty and Jon Tombs
 *
 */
/* $XConsortium: s3seg.c /main/6 1996/01/11 12:26:46 kaleb $ */


#include "X.h"

#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "mistruct.h"
#include "miline.h"

#include "cfb.h"
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "cfbmskbits.h"
#include "misc.h"
#include "xf86.h"
#include "s3v.h"

void
s3Segment(pDrawable, pGC, nseg, pSeg)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int   nseg;
     register xSegment *pSeg;
{
   int   nboxInit;
   register int nbox;
   BoxPtr pboxInit;
   register BoxPtr pbox;

   unsigned int oc1;		/* outcode of point 1 */
   unsigned int oc2;		/* outcode of point 2 */

   int   xorg, yorg;		/* origin of window */

   int   adx;			/* abs values of dx and dy */
   int   ady;
   int   signdx;		/* sign of dx and dy */
   int   signdy;
   int   len;			/* length of segment */
   int   axis;			/* major axis */
   int   octant;
   unsigned int bias = miGetZeroLineBias(pDrawable->pScreen);

 /* a bunch of temporaries */
   int   tmp;
   register int y1, y2;
   register int x1, x2;
   RegionPtr cclip;
   cfbPrivGCPtr devPriv;

   if (!xf86VTSema)
   {
      if (xf86VTSema) WaitIdleEmpty();
      switch (s3InfoRec.bitsPerPixel) {
      case 8:
	 cfbSegmentSS(pDrawable, pGC, nseg, pSeg);
         break;
      case 16:
	 cfb16SegmentSS(pDrawable, pGC, nseg, pSeg);
         break;
      case 24:
	 cfb24SegmentSS(pDrawable, pGC, nseg, pSeg);
         break;
      case 32:
	 cfb32SegmentSS(pDrawable, pGC, nseg, pSeg);
	 break;
      }
      if (xf86VTSema) WaitIdleEmpty();
      return;
   }

   if (0) {
      int i;
      WaitIdleEmpty();
      for (i=0; i<nseg; i++) {
	 pSeg[i].y1 += 20;
	 pSeg[i].y2 += 20;
      }
      cfbSegmentSS(pDrawable, pGC, nseg, pSeg);
      for (i=0; i<nseg; i++) {
	 pSeg[i].y1 -= 20;
	 pSeg[i].y2 -= 20;
      }
      WaitIdleEmpty();
   } else if (0) {
      int tmp1 = pGC->fgPixel;
      int tmp2 = pGC->alu;
      pGC->fgPixel = 0x1a;
      pGC->alu = 3;
      WaitIdleEmpty();
      cfbSegmentSS(pDrawable, pGC, nseg, pSeg);
      WaitIdleEmpty();
      pGC->fgPixel = tmp1;
      pGC->alu = tmp2;
   }

   devPriv = (cfbPrivGC *) (pGC->devPrivates[cfbGCPrivateIndex].ptr);
   cclip = devPriv->pCompositeClip;
   pboxInit = REGION_RECTS(cclip);
   nboxInit = REGION_NUM_RECTS(cclip);

   BLOCK_CURSOR;
   ;SET_WRT_MASK(pGC->planemask);

#if 0
   /* Fix problem writing to the cursor storage area */
   WaitQueue(4);
   SETL_CLIP_L_R(0, pDrawable->pScreen->width-1);
   SETL_CLIP_T_B(0, pDrawable->pScreen->height-1);
#else
   WaitQueue(2);
#endif
   SETL_PAT_FG_CLR(pGC->fgPixel);
   SETL_CMD_SET(s3_gcmd | CMD_LINE | MIX_MONO_PATT | CMD_AUTOEXEC | s3alu_sp[pGC->alu]);

if(0)ErrorF("seg alu %2d %2x %2x  col %3d %8x %8x\n",pGC->alu,s3alu[pGC->alu]>>17,s3alu_sp[pGC->alu]>>17,pGC->fgPixel
	    ,s3_gcmd | CMD_LINE | CMD_AUTOEXEC | s3alu[pGC->alu]
	    ,s3_gcmd | CMD_LINE | CMD_AUTOEXEC | s3alu_sp[pGC->alu]
);

   xorg = pDrawable->x;
   yorg = pDrawable->y;

   while (nseg--) {
      nbox = nboxInit;
      pbox = pboxInit;

      x1 = pSeg->x1 + xorg;
      y1 = pSeg->y1 + yorg;
      x2 = pSeg->x2 + xorg;
      y2 = pSeg->y2 + yorg;

      pSeg++;

      if (x1 == x2) {

	 /*
	  * make the line go top to bottom of screen, keeping endpoint
	  * semantics
	  */
	 if (y1 > y2) {
	    register int tmp;

	    tmp = y2;
	    y2 = y1 + 1;
	    y1 = tmp + 1;
	    if (pGC->capStyle != CapNotLast)
	       y1--;
	 } else if (pGC->capStyle != CapNotLast)
	    y2++;
	 /* get to first band that might contain part of line */
	 while ((nbox) && (pbox->y2 <= y1)) {
	    pbox++;
	    nbox--;
	 }

	 if (nbox) {
	    /* stop when lower edge of box is beyond end of line */
	    while ((nbox) && (y2 >= pbox->y1)) {
	       if ((x1 >= pbox->x1) && (x1 < pbox->x2)) {
		  int   y1t, y2t;

		  /* this box has part of the line in it */
		  y1t = max(y1, pbox->y1);
		  y2t = min(y2, pbox->y2);
		  if (y1t != y2t) {
                     /* Since the ViRGE draws from bottom to top, I draw
                        from x2,y2 to x1,y1, where x2,y2 is skipped. */
		     WaitQueue(5);
		     SETL_LXEND0_END1(x1, x1);
		     SETL_LDX(0);  /* dX == 0 */
		     SETL_LXSTART(x1 << 20);
		     SETL_LYSTART(y2t - 1);
		     SETL_LYCNT(y2t - y1t);
		  }
	       }
	       nbox--;
	       pbox++;
	    }
	 }
      } else if (y1 == y2) {

	 /*
	  * force line from left to right, keeping endpoint semantics
	  */
	 if (x1 > x2) {
	    register int tmp;

	    tmp = x2;
	    x2 = x1 + 1;
	    x1 = tmp + 1;
	    if (pGC->capStyle != CapNotLast)
	       x1--;
	 } else if (pGC->capStyle != CapNotLast)
	    x2++;

	 /* find the correct band */
	 while ((nbox) && (pbox->y2 <= y1)) {
	    pbox++;
	    nbox--;
	 }

	 /* try to draw the line, if we haven't gone beyond it */
	 if ((nbox) && (pbox->y1 <= y1)) {
	    /* when we leave this band, we're done */
	    tmp = pbox->y1;
	    while ((nbox) && (pbox->y1 == tmp)) {
	       int   x1t, x2t;

	       if (pbox->x2 <= x1) {
		  /* skip boxes until one might contain start point */
		  nbox--;
		  pbox++;
		  continue;
	       }
	       /* stop if left of box is beyond right of line */
	       if (pbox->x1 >= x2) {
		  nbox = 0;
		  break;
	       }
	       x1t = max(x1, pbox->x1);
	       x2t = min(x2, pbox->x2);
	       if (x1t != x2t) {
                  /* Skip x2,y2 */
		  WaitQueue(5);
		  SETL_LXEND0_END1(x1t, x2t-1);
		  SETL_LDX(0);	/* dY == 0 */
		  SETL_LXSTART(x1t << 20);
		  SETL_LYSTART(y1);
		  SETL_LYCNT(1 | 0x80000000);
	       }
	       nbox--;
	       pbox++;
	    }
	 }
      } else {			/* sloped line */
	 int xdelta, xfixup, xdir;

	 adx = x2 - x1;
	 ady = y2 - y1;
	 CalcLineDeltas(x1, y1, x2, y2, adx, ady, signdx, signdy,
			1, 1, octant);

	 if (adx > ady) {
	    axis = X_AXIS;
	 } else {
	    axis = Y_AXIS;
	    SetYMajorOctant(octant);
	 }

	 xdelta = -((x2-x1) << 20) / (y2-y1);
	 if (axis == Y_AXIS)
	    if (xdelta >= 0)
	       xfixup = 0x80000;
	    else
	       xfixup = 0x7ffff;
	 else
	    if (xdelta > 0)
	       xfixup = xdelta >> 1;
	    else
	       xfixup = (xdelta >> 1) + ((1<<20) - 1);
	 if ((x1 < x2) ^ (y1 < y2))
	    xdir = 0x80000000;
	 else
	    xdir = 0;

if(0)ErrorF("S %4d,%-4d  %4d,%-4d  %4d,%-4d  %x %8x %8x %6g %6g\n"
       ,x1,y1, x2,y2, x2-x1, y2-y1
       ,xdir>>28
       ,xdelta ,xfixup
       ,xdelta/(double)(1<<20),xfixup/(double)(1<<20)
       );

	 /*
	  * we have bresenham parameters and two points. all we have to do now
	  * is clip and draw.
	  */

	 while (nbox--) {
	    oc1 = 0;
	    oc2 = 0;
	    OUTCODES(oc1, x1, y1, pbox);
	    OUTCODES(oc2, x2, y2, pbox);
	    if ((oc1 | oc2) == 0) {
	       len = ady;

	       /*
		* NOTE:  The ViRGE hardware routines for generating lines may
		* not match the software generated lines of mi, cfb, and mfb
		* because it's using simple DDA instead of bresenham.
		*/
	       ;SET_CURPT((short)x1, (short)y1);
	       ;SET_ERR_TERM((short)(e + fix));
	       ;SET_DESTSTP((short)e2, (short)e1);
	       ;SET_MAJ_AXIS_PCNT((short)len);
	       ;SET_CMD(cmd2);

#if 0
	       if (y1 > y2) {
		  if (pGC->capStyle != CapNotLast)
		     ErrorF("LXEND0_END1 %8x %d %d\n",x1<<16|x2,x1, x2);
		  else if (xdir)
		     ErrorF("LXEND0_END1 %8x\n",x1<<16|(x2-1),x1, x2-1);
		  else
		     ErrorF("LXEND0_END1 %8x\n",x1<<16|(x2+1),x1, x2+1);
		  ErrorF("LDX %8x %8f\n",xdelta,xdelta/(double)(1<<20));
		  ErrorF("LXSTART %8x %8f\n",(x1 << 20) + xfixup,((x1 << 20) + xfixup)/(double)(1<<20));
		  ErrorF("LYSTART %8x %d\n",y1,y1);
		  ErrorF("LYCNT %8x %d\n",(len+1) | xdir,len+1);
	       } else {
		  if (pGC->capStyle != CapNotLast)
		     ErrorF("LXEND0_END1 %8x\n",x2<<16|x1,x2, x1);
		  else if (xdir)
		     ErrorF("LXEND0_END1 %8x\n",(x2+1)<<16|x1,x2+1, x1);
		  else
		     ErrorF("LXEND0_END1 %8x\n",(x2-1)<<16|x1,x2-1, x1);
		  ErrorF("LDX %8x\n",xdelta);
		  ErrorF("LXSTART %8x %8f\n",(x2 << 20) + xfixup,((x2 << 20) + xfixup)/(double)(1<<20));
		  ErrorF("LYSTART %8x %d\n",y2,y2);
		  ErrorF("LYCNT %8x %d\n",(len+1) | xdir,len+1);
	       }
#endif
	       WaitQueue(5);
	       if (y1 > y2) {
		  if (pGC->capStyle != CapNotLast)
		     SETL_LXEND0_END1(x1, x2);
		  else if (xdir)
		     SETL_LXEND0_END1(x1, x2-1);
		  else
		     SETL_LXEND0_END1(x1, x2+1);
		  SETL_LDX(xdelta);
		  SETL_LXSTART((x1 << 20) + xfixup);
		  SETL_LYSTART(y1);
		  SETL_LYCNT((len+1) | xdir);
	       } else {
		  if (pGC->capStyle != CapNotLast)
		     SETL_LXEND0_END1(x2, x1);
		  else if (xdir)
		     SETL_LXEND0_END1(x2+1, x1);
		  else
		     SETL_LXEND0_END1(x2-1, x1);
		  SETL_LDX(xdelta);
		  SETL_LXSTART((x2 << 20) + xfixup);
		  SETL_LYSTART(y2);
		  SETL_LYCNT((len+1) | xdir);
	       }

	       break;
	    } else if (oc1 & oc2) {
	       pbox++;
	    } else {

	       /*
		* let the mi helper routine do our work; better than
		* duplicating code...
		*/
	       int   clip1=0, clip2=0; /* clippedness of the endpoints */

	       int new_x1 = x1, new_y1 = y1, new_x2 = x2, new_y2 = y2;

               if (miZeroClipLine(pbox->x1, pbox->y1,
				  pbox->x2-1, pbox->y2-1,
				  &new_x1, &new_y1,
				  &new_x2, &new_y2,
				  adx, ady,
				  &clip1, &clip2,
				  octant, bias,
				  oc1, oc2) == -1)
		  {
		     pbox++;
		     continue;
		  }
	       if (axis == X_AXIS)
		  len = abs(new_x2 - new_x1);
	       else
		  len = abs(new_y2 - new_y1);

	       if (clip2 != 0 || pGC->capStyle != CapNotLast)
		  len++;
	       if (len) {
		  int new_xstartx;
		  len = abs(new_y2 - new_y1);

		  ;SET_CURPT((short)new_x1, (short)new_y1);
		  ;SET_ERR_TERM((short)(err + fix));
		  ;SET_DESTSTP((short)e2, (short)e1);
		  ;SET_MAJ_AXIS_PCNT((short)len);
		  ;SET_CMD(cmd2);
		  if (y1 > y2) {
		     new_xstartx = (x1 << 20) + xfixup + (xdelta * (y1 - new_y1));
		     new_xstartx += (new_x1 - new_xstartx);
		  } else {
		     new_xstartx = (x2 << 20) + xfixup + (xdelta * (y2 - new_y2));
		     new_xstartx += (new_x2 - new_xstartx);
		  }					    
#if 0
		  if (y1 > y2) {
		     if (pGC->capStyle != CapNotLast)
			ErrorF("LXEND0_END1 %8x %d %d\n",new_x1<<16|new_x2,new_x1, new_x2);
		     else if (xdir)
			ErrorF("LXEND0_END1 %8x\n",new_x1<<16|(new_x2-1),new_x1, new_x2-1);
		     else
			ErrorF("LXEND0_END1 %8x\n",new_x1<<16|(new_x2+1),new_x1, new_x2+1);
		     ErrorF("LDX %8x %8f\n",xdelta,xdelta/(double)(1<<20));
		     ErrorF("LXSTART %8x %8f\n",(new_x1 << 20) + xfixup,((new_x1 << 20) + xfixup)/(double)(1<<20));
		     ErrorF("LYSTART %8x %d\n",new_y1,new_y1);
		     ErrorF("LYCNT %8x %d\n",(len+1) | xdir,len+1);
		  } else {
		     if (pGC->capStyle != CapNotLast)
			ErrorF("LXEND0_END1 %8x\n",new_x2<<16|new_x1,new_x2, new_x1);
		     else if (xdir)
			ErrorF("LXEND0_END1 %8x\n",(new_x2+1)<<16|new_x1,new_x2+1, new_x1);
		     else
			ErrorF("LXEND0_END1 %8x\n",(new_x2-1)<<16|new_x1,new_x2-1, new_x1);
		     ErrorF("LDX %8x\n",xdelta);
		     ErrorF("LXSTART %8x %8f\n",(new_x2 << 20) + xfixup,((new_x2 << 20) + xfixup)/(double)(1<<20));
		     ErrorF("LYSTART %8x %d\n",new_y2,new_y2);
		     ErrorF("LYCNT %8x %d\n",(len+1) | xdir,len+1);
		  }
#endif
		  WaitQueue(5);
		  if (y1 > y2) {
		     if (pGC->capStyle != CapNotLast)
			SETL_LXEND0_END1(new_x1, new_x2);
		     else if (xdir)
			SETL_LXEND0_END1(new_x1, new_x2-1);
		     else
			SETL_LXEND0_END1(new_x1, new_x2+1);
		     SETL_LDX(xdelta);
		     SETL_LXSTART((new_x1 << 20) + xfixup);
		     SETL_LYSTART(new_y1);
		     SETL_LYCNT((len+1) | xdir);
		  } else {
		     if (pGC->capStyle != CapNotLast)
			SETL_LXEND0_END1(new_x2, new_x1);
		     else if (xdir)
			SETL_LXEND0_END1(new_x2+1, new_x1);
		     else
			SETL_LXEND0_END1(new_x2-1, new_x1);
		     SETL_LDX(xdelta);
		     SETL_LXSTART((new_x2 << 20) + xfixup);
		     SETL_LYSTART(new_y2);
		     SETL_LYCNT((len+1) | xdir);
		  }
	       }
	       pbox++;
	    }
	 } /* while (nbox--) */
      }	/* sloped line */
   } /* while (nline--) */
   
#if 0
   WaitQueue(7);
   SETL_CLIP_L_R(0, s3ScissR);
   SETL_CLIP_T_B(0, s3ScissB);
#else
   WaitQueue(5);
#endif
   SETL_CMD_SET(CMD_NOP);

   /* avoid system hangs again :-( */
   SETB_PAT_FG_CLR(0);
   SETB_RDEST_XY(0,0);
   SETB_RWIDTH_HEIGHT(0,1);
   SETB_CMD_SET(s3_gcmd | CMD_BITBLT | MIX_MONO_PATT | MIX_MONO_SRC | ROP_DPo);

   UNBLOCK_CURSOR;
}
