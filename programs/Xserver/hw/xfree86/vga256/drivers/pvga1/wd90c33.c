/* $XFree86$ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 * /proj/X11/mit/server/ddx/at386/vga/RCS/vgaBitBlt.c,v 1.5 91/02/10 16:44:40 root Exp
 */

/* 90C31 accel code: Mike Tierney <floyd@eng.umd.edu> */
/* 90C33 accel code: Bill Conn <conn@bnr.ca> */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"cfb.h"
#include	"cfbmskbits.h"
#include	"cfb8bit.h"
#include        "cfbrrop.h"
#include	"fastblt.h"
#include	"mergerop.h"
#include        "vgaBank.h"

#include "compiler.h"
#include "wd90c33.h"


#if 0
void printERA()
{
  int i;
  outw(INDEX_CONTROL_PORT,1);
  for(i=0;i<=0xc;i++){
    ErrorF("%04x ",inw(REGISTER_ACCESS_PORT));
  }
  ErrorF("\n");
  outw(INDEX_CONTROL_PORT,3);
  for(i=0;i<=0xb;i++){
    ErrorF("%04x ",inw(REGISTER_ACCESS_PORT));
  }
  ErrorF("\n");
}
#endif

void
C33BitBlt(pdstBase, psrcBase, widthSrc, widthDst, x, y,
	    x1, y1, w, h, xdir, ydir, alu, planemask)
     unsigned char *psrcBase, *pdstBase;  /* start of src and dst bitmaps */
     int    widthSrc, widthDst;
     int    x, y, x1, y1, w, h;
     int    xdir, ydir;
     int    alu;
     unsigned long  planemask;

{
  int psrcx, psrcy, pdstx, pdsty;
  int blit_direct;

/* ErrorF("C33BitBlt %d %d\n",x,y);   */
  if (xdir == 1 )    /* left to right */
   {
      psrcx = x;
      pdstx = x1;
      blit_direct = C33_BLT_DIRECT_X_POS;
   }
   else                           /* right to left */ 
   {
      psrcx = x+w;
      pdstx = x1+w;
      blit_direct = C33_BLT_DIRECT_X_NEG;
   }

   if (ydir == 1)    /* top to bottom */
   {
      psrcy = y;
      pdsty = y1;
      blit_direct = C33_BLT_DIRECT_Y_POS;
   }
   else                           /* bottom to top */
   {
      psrcy = y+h;
      pdsty = y1+h;
      blit_direct = C33_BLT_DIRECT_Y_NEG;
   }

   C33_SELECT_REG_BANK3;
   C33_WAIT_COMMAND_BUFFER(3);
   SET_C33_BANK3_MAP_BASE_ADDR(0);
   SET_C33_BANK3_ROW_PITCH(widthDst);
   SET_C33_BANK3_MASK_BYTE_0((planemask & 0xFF));

   C33_SELECT_REG_BANK1;
   C33_WAIT_COMMAND_BUFFER(6);
   SET_C33_BANK1_SRC_X(psrcx);
   SET_C33_BANK1_SRC_Y(psrcy);
   SET_C33_BANK1_DEST_X(pdstx);
   SET_C33_BANK1_DEST_Y(pdsty);
   SET_C33_BANK1_DIM_X(w);
   SET_C33_BANK1_DIM_Y(h);

   C33_WAIT_COMMAND_BUFFER(7);
   SET_C33_BANK1_RAS_OP((alu << 8));
   SET_C33_BANK1_CLIP_LEFT(0);
   SET_C33_BANK1_CLIP_RIGHT(0xFFF);
   SET_C33_BANK1_CLIP_TOP(0);
   SET_C33_BANK1_CLIP_BOTTOM(0xFFF);
   SET_C33_BANK1_CNTRL2(C33_8_BITS_PER_PIXEL|C33_RESERVED_BIT5|C33_HOST_BLT_THRU_MEM);
   SET_C33_BANK1_CNTRL1(C33_DRAW_MODE_BITBLT|blit_direct|C33_SRC_FMT_COLOR);
   C33_WAIT_COMMAND_BUFFER(C33_MAX_LOCATIONS_AVAILABLE);
  /* must wait, since memory writes can mess up as well */
   C33_WAIT_DRAWING_ENGINE; 
}

void
C33cfbDoBitbltCopy(pSrc, pDst, alu, prgnDst, pptSrc, planemask)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned long   planemask;
{
    unsigned long *psrcBase, *pdstBase;	
				/* start of src and dst bitmaps */
    int widthSrc, widthDst;	/* add to get to same position in next line */

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
    int blit_dir;

    MROP_DECLARE_REG()

    int careful;
    int srcaddr, dstaddr;


    MROP_INITIALIZE(alu,planemask);

    cfbGetByteWidthAndPointer (pSrc, widthSrc, (unsigned char *)psrcBase)

    cfbGetByteWidthAndPointer (pDst, widthDst, (unsigned char *)pdstBase)

    BANK_FLAG_BOTH(psrcBase,pdstBase)

    /* XXX we have to err on the side of safety when both are windows,
     * because we don't know if IncludeInferiors is being used.
     */
    careful = ((pSrc == pDst) ||
	       ((pSrc->type == DRAWABLE_WINDOW) &&
		(pDst->type == DRAWABLE_WINDOW)));

    if (!CHECKSCREEN(psrcBase) || !CHECKSCREEN(pdstBase))
    {
       pvga1_stdcfbDoBitbltCopy (pSrc, pDst, alu, prgnDst, pptSrc, planemask);
       return;
    }
    

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

    if (careful && (pptSrc->x < pbox->x1) && (pptSrc->y <= pbox->y1))
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


    while(nbox--)
    {
      int psrcx, psrcy, pdstx, pdsty;
      int blit_direct;
      
      w = pbox->x2 - pbox->x1;
      h = pbox->y2 - pbox->y1;

      if (xdir == -1)    /* dragging left to right */
	{
	  psrcx = pptSrc->x+w-1;
	  pdstx = pbox->x1+w-1;
	  blit_direct = C33_BLT_DIRECT_X_NEG;
	}
      else                           /* right to left */ 
	{
	  psrcx = pptSrc->x;
	  pdstx = pbox->x1;
	  blit_direct = C33_BLT_DIRECT_X_POS;
	}
      
      if (ydir == -1)   /* dragging top to bottom */
	{
	  psrcy = pptSrc->y+h-1;
	  pdsty = pbox->y1+h-1;
	  blit_direct |= C33_BLT_DIRECT_Y_NEG;
	}
      else                           /* bottom to top */
	{
	  psrcy = pptSrc->y;
	  pdsty = pbox->y1;
	  blit_direct |= C33_BLT_DIRECT_Y_POS;
	}

       /** handle the blit **/

/* ErrorF("pbox->x1=%4d,pbox->x2=%4d,pptSrc->x=%4d,xdir=%2d\n", */
/* pbox->x1,pbox->x2,pptSrc->x,xdir); */
/* ErrorF("pbox->y1=%4d,pbox->y2=%4d,pptSrc->y=%4d,ydir=%2d\n", */
/* pbox->y1,pbox->y2,pptSrc->y,ydir); */
/* ErrorF("C33DoBitBltCopy srcx=%4d srcy=%4d dstx=%4d dsty=%4d w=%4d h=%4d dir=%4x\n", */
/* psrcx,psrcy,pdstx,pdsty,w,h,blit_direct);   */

/*ErrorF(" widthDst=%d planemask=%x\n",widthDst,planemask);*/

	C33_SELECT_REG_BANK3;
	C33_WAIT_COMMAND_BUFFER(3);
	SET_C33_BANK3_MAP_BASE_ADDR(0);
	SET_C33_BANK3_ROW_PITCH(widthDst);
	SET_C33_BANK3_MASK_BYTE_0((planemask & 0xFF));
	
	C33_SELECT_REG_BANK1;
	C33_WAIT_COMMAND_BUFFER(6);
	SET_C33_BANK1_SRC_X(psrcx);
	SET_C33_BANK1_SRC_Y(psrcy);
	SET_C33_BANK1_DEST_X(pdstx);
	SET_C33_BANK1_DEST_Y(pdsty);
	SET_C33_BANK1_DIM_X(w);
	SET_C33_BANK1_DIM_Y(h);
	
	C33_WAIT_COMMAND_BUFFER(7);
	SET_C33_BANK1_RAS_OP((GXcopy << 8));
	SET_C33_BANK1_CLIP_LEFT(0);
	SET_C33_BANK1_CLIP_RIGHT(0xFFF);
	SET_C33_BANK1_CLIP_TOP(0);
	SET_C33_BANK1_CLIP_BOTTOM(0xFFF);
	SET_C33_BANK1_CNTRL2(C33_8_BITS_PER_PIXEL|C33_RESERVED_BIT5|C33_HOST_BLT_THRU_MEM);
	SET_C33_BANK1_CNTRL1(C33_DRAW_MODE_BITBLT|blit_direct|C33_SRC_FMT_COLOR);
	C33_WAIT_COMMAND_BUFFER(C33_MAX_LOCATIONS_AVAILABLE);

        pbox++;
        pptSrc++;
    }

    /* must wait, since memory writes can mess up as well */
    C33_WAIT_DRAWING_ENGINE; 

/* printERA();*/

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


void
C33cfbFillBoxSolid (pDrawable, nBox, pBox, pixel1, pixel2, alu)
    DrawablePtr	    pDrawable;
    int		    nBox;
    BoxPtr	    pBox;
    unsigned long   pixel1;
    unsigned long   pixel2;
    int	            alu;
{
    unsigned char   *pdstBase;
    unsigned long   pdst;
    unsigned long   widthDst;
    unsigned long   h;
    unsigned long   fill1;
    unsigned long   w;

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

    if (!CHECKSCREEN(pdstBase))
    {
        pvga1_stdcfbFillBoxSolid (pDrawable, nBox, pBox, pixel1, pixel2, alu);
        return;
    }

    fill1 = PFILL(pixel1);

    for (; nBox; nBox--, pBox++)
    {
      h = pBox->y2 - pBox->y1;
      w = pBox->x2 - pBox->x1;
      C33_SELECT_REG_BANK3;
      C33_WAIT_COMMAND_BUFFER(5);
      SET_C33_BANK3_MAP_BASE_ADDR(0);
      SET_C33_BANK3_ROW_PITCH(widthDst);
      SET_C33_BANK3_MASK_BYTE_0((0xFF));
      SET_C33_BANK3_FOREGROUND_COLOR_BYTE_0((fill1 & 0xFF));
      SET_C33_BANK3_BACKGROUND_COLOR_BYTE_0(0);
      C33_SELECT_REG_BANK1;
      C33_WAIT_COMMAND_BUFFER(4);
      SET_C33_BANK1_DEST_X(pBox->x1);
      SET_C33_BANK1_DEST_Y(pBox->y1);
      SET_C33_BANK1_DIM_X(w);
      SET_C33_BANK1_DIM_Y(h);
      
      C33_WAIT_COMMAND_BUFFER(7);
      SET_C33_BANK1_RAS_OP((alu << 8));
      SET_C33_BANK1_CLIP_LEFT(0);
      SET_C33_BANK1_CLIP_RIGHT(0xFFF);
      SET_C33_BANK1_CLIP_TOP(0);
      SET_C33_BANK1_CLIP_BOTTOM(0xFFF);
      SET_C33_BANK1_CNTRL2(C33_8_BITS_PER_PIXEL|
			   C33_RESERVED_BIT5|
			   C33_HOST_BLT_THRU_MEM);
      SET_C33_BANK1_CNTRL1(C33_DRAW_MODE_BITBLT|
			   C33_SRC_FMT_FIXED_COLOR);
    }
    /* must wait, since memory writes can mess up as well */
    C33_WAIT_COMMAND_BUFFER(C33_MAX_LOCATIONS_AVAILABLE);
    C33_WAIT_DRAWING_ENGINE; 
}


void
C33cfbFillRectSolidCopy (pDrawable, pGC, nBox, pBox)
    DrawablePtr	    pDrawable;
    GCPtr	    pGC;
    int		    nBox;
    BoxPtr	    pBox;
{
    RROP_DECLARE

    RROP_FETCH_GC(pGC);

    C33cfbFillBoxSolid (pDrawable, nBox, pBox, rrop_xor, 0, GXcopy);
}

