/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3_virge/s3seg.c,v 3.4 1996/10/08 12:21:14 dawes Exp $ */
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

   if (!xf86VTSema || ((pGC->planemask & s3BppPMask) != s3BppPMask))
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

   if (0)ErrorF("%d %x %d %d %d\n",pGC->alu, pGC->fgPixel, 
	  pGC->alu == GXxor , (pGC->fgPixel & 0xf) == 0xf,
	  pGC->alu == GXxor && (pGC->fgPixel & 0xf) == 0xf);
   if (0 && pGC->alu == 1) {
      int i;
      int tmp1 = pGC->fgPixel;
ErrorF("SS %x %x %d",pGC->fgPixel,pGC->bgPixel, nseg);
      pGC->alu = 3;
      WaitIdleEmpty();
      for (i=0; i<nseg; i++) {
	 pSeg[i].x1 += 0;
	 pSeg[i].x2 += 0;
	 pSeg[i].y1 += 10;
	 pSeg[i].y2 += 10;
      }
      cfbSegmentSS(pDrawable, pGC, nseg, pSeg);
      for (i=0; i<nseg; i++) {
	 pSeg[i].x1 -= 0;
	 pSeg[i].x2 -= 0;
	 pSeg[i].y1 -= 10;
	 pSeg[i].y2 -= 10;
      }
      WaitIdleEmpty();
ErrorF("SS %x %x %d\n",pGC->fgPixel, pGC->bgPixel, nseg);
      pGC->fgPixel = tmp1;
      pGC->alu = 3;
   } else if (0 && pGC->alu == GXxor && (pGC->fgPixel & 0xf) == 0xf) {
      int i;
      int tmp1 = pGC->fgPixel;
      int tmp2 = pGC->alu;
      /* pGC->fgPixel &= ~0x03; */
      /* pGC->alu = 3; */
      WaitIdleEmpty();
      cfbSegmentSS(pDrawable, pGC, nseg, pSeg);
      WaitIdleEmpty();
      /* pGC->fgPixel = 0x03; */
      /* pGC->alu = tmp2; */
ErrorF("%d %x %d %d %d\n",pGC->alu, pGC->fgPixel, 
	  pGC->alu == GXxor , (pGC->fgPixel & 0xf) == 0xf,
	  pGC->alu == GXxor && (pGC->fgPixel & 0xf) == 0xf);
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
	       xfixup = 0x7ffff;
	    else
	       xfixup = 0x80000;
	 else
	    if (xdelta > 0)
	       xfixup = (xdelta-1) >> 1;
	    else {
	       xdelta--;
	       xfixup = ((xdelta+1) >> 1) + ((1<<20) - 1);
	    }
	 if ((x1 < x2) ^ (y1 < y2))
	    xdir = 0x80000000;
	 else
	    xdir = 0;

if(0)ErrorF("S %4d,%-4d  %4d,%-4d  %4d,%-4d  %x %8x %8x %16.12f %16.12f\n"
       ,x1,y1, x2,y2, x2-x1, y2-y1
       ,(xdir>>28) & 0xf
       ,xdelta ,xfixup
       ,xdelta/(double)(1<<20),xfixup/(double)(1<<20)
       );

	 /*
	  * we have bresenham parameters and two points. all we have to do now
	  * is clip and draw.
	  */
	 
	 while (nbox--) {
	    int n,xss,xs,xe,ys,ye;
	    
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

	       if (y1 > y2) {
		  ys = y1;
		  xs = x1;
		  xss = (x1 << 20) + xfixup;
		  if (pGC->capStyle != CapNotLast)
		     xe = x2;
		  else if (xdir)
		     xe = x2 - 1;
		  else
		     xe = x2 + 1;
	       } else {
		  ys = y2;
		  xe = x1;
		  xss = (x2 << 20) + xfixup;
		  if (pGC->capStyle != CapNotLast)
		     xs = x2;
		  else if (xdir)
		     x2 = x2 + 1;
		  else
		     xs = x2 - 1;
	       }

	       /* split long lines 
		* 
		* ViRGE vector generator uses DDA instead of bresenham and
		* for long sloped lines (> 500 scanlines) rounding error can result
		* in wrong pixels been drawn.  longer lines are splited in 
		* multple parts and starting points fraction is set so that
		* parts will join exactly (mostly experimental values)
		*/

#define LEN 500
	       if (len <= LEN) { /* use old code to avoid FP stuff for short lines */
		  if (0 && adx == 999 && ady == 999) {
		     ErrorF("LXEND0_END1 %8x\n", xs<<16|xe, xs,xe);
		     ErrorF("LDX %8x\n", xdelta);
		     ErrorF("LXSTART %8x %8f\n", xss, xss/(double)(1<<20));
		     ErrorF("LYSTART %8x %d\n",ys,ys);
		     ErrorF("LYCNT %8x %d\n",(len+1) | xdir,len+1);
		  }
		  WaitQueue(5);
		  SETL_LXEND0_END1(xs, xe);
		  SETL_LDX(xdelta);
		  SETL_LXSTART(xss);
		  SETL_LYSTART(ys);
		  SETL_LYCNT((len+1) | xdir);
	       }
	       else {
		  double xd2 = -(double)(x2-x1) / (double)(y2-y1);

		  for (n=0; len>0; n++, len -= LEN) {
#define NN (0*n)
#if 0
		     ErrorF("LXEND0_END1 %8x\n",
			    (NN+xs + (int)(n * LEN * xd2))<<16 | ( NN+xs + (int)((n+1) * LEN * xd2)-1),
			    (NN+xs + (int)(n * LEN * xd2)), ( NN+xs + (int)((n+1) * LEN * xd2)-1));
		     ErrorF("LDX %8x %8f\n",xdelta,xdelta/(double)(1<<20));
		     ErrorF("LXSTART %8x %8f\n",((NN)<<20) + xss + (int)((n<<20) * (LEN * xd2)),(((NN)<<20) + xss + (int)((n<<20) * (LEN * xd2))) / (double)(1<<20));
		     ErrorF("LYSTART %8x %d\n",ys - n * LEN,ys - n * LEN);
		     if (len > LEN)
			ErrorF("LYCNT %8x %d\n",(LEN+1) | xdir,LEN+1);
		     else
			ErrorF("LYCNT %8x %d\n",(len+1) | xdir,len+1);
#endif
		     WaitQueue(5);
		     SETL_LXEND0_END1((((NN)<<20) + xss + (int)((n<<20) * (LEN * xd2))) >> 20,
				      len <= LEN ? xe : 
				      ((((NN)<<20) + xss + (int)(((n+1)<<20) * (LEN * xd2))) >> 20) 
				      + (xdir ? -1 : 1));
		     SETL_LDX(xdelta);
		     SETL_LXSTART(((NN)<<20) + xss + (int)((n<<20) * (LEN * xd2)));
		     SETL_LYSTART(ys - n * LEN);
		     if (len > LEN)
			SETL_LYCNT((LEN+1) | xdir);
                     else
			SETL_LYCNT((len+1) | xdir);
		  }
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
		  int xofs,yofs;
		  double xd2;

		  len = abs(new_y2 - new_y1);

		  ;SET_CURPT((short)new_x1, (short)new_y1);
		  ;SET_ERR_TERM((short)(err + fix));
		  ;SET_DESTSTP((short)e2, (short)e1);
		  ;SET_MAJ_AXIS_PCNT((short)len);
		  ;SET_CMD(cmd2);
		  
		  if (y1 > y2) {
		     xss = (new_x1 << 20) + xfixup;
		     if (pGC->capStyle != CapNotLast)
			xe = new_x2;
		     else if (xdir)
			xe = new_x2 - 1;
		     else
			xe = new_x2 + 1;
		     ys = new_y1;
		     xs = new_x1;
		     xofs = x1 - new_x1;
		     yofs = y1 - new_y1;
		  } else {
		     ys = new_y2;
		     xe = new_x1;
		     xss = (new_x2 << 20) + xfixup;
		     if (pGC->capStyle != CapNotLast)
			xs = new_x2;
		     else if (xdir)
			x2 = new_x2 + 1;
		     else
			xs = new_x2 - 1;
		     xofs = x2 - new_x2;
		     yofs = y2 - new_y2;
		  }		  

		  if (len <= LEN) { /* use old code to avoid FP stuff for short lines */
		     if (0 && adx == 999 && ady == 999) {
			ErrorF("LXEND0_END1 %8x\n", xs<<16|xe, xs,xe);
			ErrorF("LDX %8x\n", xdelta);
			ErrorF("LXSTART %8x %8f\n", xss, xss/(double)(1<<20));
			ErrorF("LYSTART %8x %d\n",ys,ys);
			ErrorF("LYCNT %8x %d\n",(len+1) | xdir,len+1);
		     }
		     WaitQueue(5);
		     SETL_LXEND0_END1(xs, xe);
		     SETL_LDX(xdelta);
		     if (yofs == 0) 
			SETL_LXSTART(xss);
		     else {
			double xd2 = -(double)(x2-x1) / (double)(y2-y1);
			if (y1 > y2)
			   xss = (x1 << 20) + xfixup;
			else 
			   xss = (x2 << 20) + xfixup;
			SETL_LXSTART(xss + (int)((yofs<<20) * xd2));
		     }
		     SETL_LYSTART(ys);
		     SETL_LYCNT((len+1) | xdir);
		  }
		  else {
		     double xd2 = -(double)(x2-x1) / (double)(y2-y1);

		     if (y1 > y2)
			xss = (x1 << 20) + xfixup;
		     else 
			xss = (x2 << 20) + xfixup;

		     for (n=0; len+(new_y1 ==  new_y2) > 0; n++, len -= LEN) {
#if 0
			ErrorF("LXEND0_END1 %8x\n",
			       (NN+xs + (int)((n * LEN + yofs) * xd2))<<16 | ( NN+xs + (int)(((n+1) * LEN + yofs) * xd2)-1),
			       (NN+xs + (int)((n * LEN + yofs) * xd2)), ( NN+xs + (int)(((n+1) * LEN + yofs) * xd2)-1));
			ErrorF("LDX %8x %8f\n",xdelta,xdelta/(double)(1<<20));
			ErrorF("LXSTART %8x %8f\n",((NN)<<20) + xss + (int)(((n * LEN + yofs)<<20) * xd2), (((NN)<<20) + xss + (int)(((n * LEN + yofs)<<20) * xd2)) / (double)(1<<20));
			ErrorF("LYSTART %8x %d\n",ys - n * LEN,ys - n * LEN);
			if (len > LEN)
			   ErrorF("LYCNT %8x %d\n",(LEN+1) | xdir,LEN+1);
			else
			   ErrorF("LYCNT %8x %d\n",(len+1) | xdir,len+1);
#endif
			WaitQueue(5);
			SETL_LXEND0_END1(n==0 ? xs : 
					 (((NN)<<20) + xss + (int)(((n * LEN + yofs)<<20) * xd2)) >> 20,
					 len <= LEN ? xe : 
					 ((((NN)<<20) + xss + (int)((((n+1) * LEN + yofs)<<20) * xd2)) >> 20)
					 + (xdir ? -1 : 1));
			SETL_LDX(xdelta);
			SETL_LXSTART(((NN)<<20) + xss + (int)(((n * LEN + yofs)<<20) * xd2));
			SETL_LYSTART(ys - n * LEN);
			if (len > LEN)
			   SETL_LYCNT((LEN+1) | xdir);
			else
			   SETL_LYCNT((len+1) | xdir);
		     }
		  }
		  
#if 0
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
#endif

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
