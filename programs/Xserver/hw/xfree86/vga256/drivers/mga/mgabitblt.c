/*
 * MGA Millennium (MGA2064W) with Ti3026 RAMDAC driver v.1.1
 *
 * The driver is written without any chip documentation. All extended ports
 * and registers come from tracing the VESA-ROM functions.
 * The BitBlt Engine comes from tracing the windows BitBlt function.
 *
 * Author:	Radoslaw Kapitan, Tarnow, Poland
 *			kapitan@student.uci.agh.edu.pl
 *		original source
 *
 * Now that MATROX has released documentation to the public, enhancing
 * this driver has become much easier. Nevertheless, this work continues
 * to be based on Radoslaw's original source
 *
 * Contributors:
 *		Andrew Vanderstock, Melbourne, Australia
 *			vanderaj@mail2.svhm.org.au
 *		additions, corrections, cleanups
 *
 *		Dirk Hohndel
 *			hohndel@XFree86.Org
 *		integrated into XFree86-3.1.2Gg
 *		fixed some problems with PCI probing and mapping
 */
 
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/mga/mgabitblt.c,v 3.0 1996/09/26 14:00:34 dawes Exp $ */

#include "vga256.h"
#include "mgareg.h"
#include "mga.h"

static void (*stdDoBitbltCopy)();
int MGAScrnWidth;

extern void cfb16DoBitbltCopy(); 
extern void cfb32DoBitbltCopy();
extern void cfb16DoBitbltGeneral();
extern void cfb32DoBitbltGeneral();
extern void cfb16DoBitbltXor();
extern void cfb32DoBitbltXor();
extern void cfb16DoBitbltOr();
extern void cfb32DoBitbltOr();
extern RegionPtr cfb16BitBlt();
extern RegionPtr cfb32BitBlt();

void 
MGABlitterInit(bpp, width)
int bpp, width;
{
    int shift;
    
    if (bpp == 8)
    {
        stdDoBitbltCopy = vga256DoBitbltCopy;
        shift = 0;
    }
    if (bpp == 16)
    {
        stdDoBitbltCopy = cfb16DoBitbltCopy;
        shift = 1;
    }
    if (bpp == 32)
    {
        stdDoBitbltCopy = cfb32DoBitbltCopy;
        shift = 2;
    }
    
    MGAScrnWidth = width;

    OUTREG(MGAREG_FCOL, 0x00000000);
    OUTREG(MGAREG_SHIFT, 0x00000000);
    OUTREG(MGAREG_OPMODE, 0x01000000);                                                        
    
    OUTREG(MGAREG_MACCESS, shift);
    OUTREG(MGAREG_YDSTORG, 0x00000000);
    OUTREG(MGAREG_PLNWT, 0xFFFFFFFF);
    OUTREG(MGAREG_PITCH, MGAScrnWidth);
    	        
}


static void
MGABitBlt(widthSrc, xsrc, ysrc, xdst, ydst, w, h, xdir, ydir)
int widthSrc, xsrc, ysrc, xdst, ydst, w, h, xdir, ydir;
{
    long srcStart, srcStop, regSGN;
    
    if(ydir == 1)    /* top to bottom */
    {
        regSGN = 0;
    }
    else             /* bottom to top */
    {
        ysrc += h - 1;
        ydst += h - 1;
        regSGN = 4;
    }
    
    if(xdir == 1)    /* left to right */
    {
        srcStart = ysrc * MGAScrnWidth + xsrc;
        srcStop  = srcStart + w - 1;
    }
    else             /* right to left */
    { 
        srcStop  = ysrc * MGAScrnWidth + xsrc;
        srcStart = srcStop + w - 1;
        regSGN |= 1;
    }
       
    if(!MGAWaitForBlitter())
        ErrorF("MGA: BitBlt Engine timeout\n");

    OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);  /* (maxX << 16) | minX */
    OUTREG(MGAREG_YTOP, 0x00000000);  /* minPixelPointer */
    OUTREG(MGAREG_YBOT, 0x007FFFFF);  /* maxPixelPointer */
    OUTREG(MGAREG_DWGCTL, 0x040C4008);
    OUTREG(MGAREG_FXBNDRY, ((xdst + w - 1) << 16) | xdst);
    OUTREG(MGAREG_AR5, widthSrc);
    OUTREG(MGAREG_YDSTLEN, (ydst << 16) | h);
    OUTREG(MGAREG_SGN, regSGN);
    OUTREG(MGAREG_AR3, srcStart);
    OUTREG(MGAREG_AR0 + MGAREG_EXEC, srcStop);	/* go for it */
}

/*
 * DoBitbltCopy
 *
 * Author: Keith Packard
 * Modifications: Radoslaw Kapitan
 */ 
void
MGADoBitbltCopy(pSrc, pDst, alu, prgnDst, pptSrc, planemask)
DrawablePtr	pSrc, pDst;
int		alu;
RegionPtr	prgnDst;
DDXPointPtr	pptSrc;
unsigned long   planemask;
{
  int widthSrc, widthDst;	/* add to get to same position in next line */

  BoxPtr pbox;
  int nbox;
  
  BoxPtr pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
                                  /* temporaries for shuffling rectangles */
  DDXPointPtr pptTmp, pptNew1, pptNew2;
                                  /* shuffling boxes entails shuffling the
                                     source points too */

  int xdir;			/* 1 = left right, -1 = right left/ */
  int ydir;			/* 1 = top down, -1 = bottom up */
  int careful;

  if(!(pSrc->type == DRAWABLE_WINDOW && pDst->type == DRAWABLE_WINDOW))
  {
      (*stdDoBitbltCopy)(pSrc, pDst, alu, prgnDst, pptSrc, planemask);
      return;
  }
  
  widthSrc = widthDst = MGAScrnWidth;

  /* XXX we have to err on the side of safety when both are windows,
   * because we don't know if IncludeInferiors is being used.
   */
  careful = ((pSrc == pDst) ||
	     ((pSrc->type == DRAWABLE_WINDOW) &&
	      (pDst->type == DRAWABLE_WINDOW)));
  
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
      widthSrc = -widthSrc;
      widthDst = -widthDst;
      
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
  
  while(nbox--) {
    MGABitBlt(widthSrc,
	         pptSrc->x, pptSrc->y, pbox->x1, pbox->y1,
	         pbox->x2 - pbox->x1, pbox->y2 - pbox->y1,
	         xdir, ydir);
    pbox++;
    pptSrc++;
  }
  
  /* free up stuff */
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
  
  if(!MGAWaitForBlitter())
      ErrorF("MGA: BitBlt Engine timeout\n");
}


RegionPtr
MGA16CopyArea(pSrcDrawable, pDstDrawable, pGC,
                 srcx, srcy, width, height, dstx, dsty)
DrawablePtr pSrcDrawable, pDstDrawable;
GC *pGC;
int srcx, srcy, width, height, dstx, dsty;
{
    void (*doBitBlt)();
    
    doBitBlt = MGADoBitbltCopy;
    if (pGC->alu != GXcopy || (pGC->planemask & 0xFFFF) != 0xFFFF)
    {
	doBitBlt = cfb16DoBitbltGeneral;
	if ((pGC->planemask & 0xFFFF) == 0xFFFF)
	{
	    switch (pGC->alu) {
	    case GXxor:
		doBitBlt = cfb16DoBitbltXor;
		break;
	    case GXor:
		doBitBlt = cfb16DoBitbltOr;
		break;
	    }
	}
    }
    return cfb16BitBlt(pSrcDrawable, pDstDrawable, pGC, 
                       srcx, srcy, width, height, dstx, dsty, doBitBlt, 0L);
}


RegionPtr
MGA32CopyArea(pSrcDrawable, pDstDrawable, pGC,
                 srcx, srcy, width, height, dstx, dsty)
DrawablePtr pSrcDrawable, pDstDrawable;
GC *pGC;
int srcx, srcy, width, height, dstx, dsty;
{
    void (*doBitBlt)();
    
    doBitBlt = MGADoBitbltCopy;
    if (pGC->alu != GXcopy || (pGC->planemask & 0xFFFFFFFF) != 0xFFFFFFFF)
    {
	doBitBlt = cfb32DoBitbltGeneral;
	if ((pGC->planemask & 0xFFFFFFFF) == 0xFFFFFFFF)
	{
	    switch (pGC->alu) {
	    case GXxor:
		doBitBlt = cfb32DoBitbltXor;
		break;
	    case GXor:
		doBitBlt = cfb32DoBitbltOr;
		break;
	    }
	}
    }
    return cfb32BitBlt(pSrcDrawable, pDstDrawable, pGC, 
                       srcx, srcy, width, height, dstx, dsty, doBitBlt, 0L);
}
