#include "gcstruct.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "mi.h"

#ifdef VGA256
/*
 * VGA256 is a special case. We don't use cfb for non-accelerated
 * functions, but instead use their vga256 equivalents.
 */
#include "vga256.h"
#include "vga256map.h"
#else
#include "cfb.h"
#include "cfbmskbits.h"
#endif

#include "xf86.h"
#include "xf86xaa.h"
#include "xf86gcmap.h"
#include "xf86maploc.h"
#include "xf86local.h"

#if PSZ == 8
# define useTEGlyphBlt  cfbTEGlyphBlt8
#else
# ifdef WriteBitGroup
#  define useTEGlyphBlt	cfbImageGlyphBlt8
# else
#  define useTEGlyphBlt	cfbTEGlyphBlt
# endif
#endif

#ifdef WriteBitGroup
# define useImageGlyphBlt	cfbImageGlyphBlt8
# define usePolyGlyphBlt	cfbPolyGlyphBlt8
#else
# define useImageGlyphBlt	miImageGlyphBlt
# define usePolyGlyphBlt	miPolyGlyphBlt
#endif


void static
xf86FillPolygonWrapper(pDrawable, pGC, shape, mode, count, ptsIn)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		shape;
    int		mode;
    int		count;
    DDXPointPtr	ptsIn;
{
     cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);
     
     SYNC_CHECK;     

     if (devPriv->oneRect && pGC->fillStyle == FillSolid) {
	if (devPriv->rop == GXcopy)
            cfbFillPoly1RectCopy(pDrawable, pGC, shape, mode, count, ptsIn);
	else
	    cfbFillPoly1RectGeneral(pDrawable, pGC, shape, mode, count, ptsIn);
     } else
	miFillPolygon(pDrawable, pGC, shape, mode, count, ptsIn);

}


void static
xf86PolyRectangleWrapper(pDrawable, pGC, nRectsInit, pRectsInit)
    DrawablePtr  pDrawable;	
    GCPtr        pGC;    	
    int	         nRectsInit; 	
    xRectangle  *pRectsInit;
{
     SYNC_CHECK;     

     miPolyRectangle(pDrawable, pGC, nRectsInit, pRectsInit);
}

void static
xf86PolyArcWrapper(pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    SYNC_CHECK;     
   
    if (pGC->lineWidth == 0) 
    	miZeroPolyArc(pDraw, pGC, narcs, parcs);
    else
    	miPolyArc(pDraw, pGC, narcs, parcs);
}

void static
xf86PolyLinesWrapper(pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		mode;		
    int		npt;		
    DDXPointPtr pptInit;
{
    SYNC_CHECK;     

     if (pGC->lineStyle == LineSolid) {
	    if(pGC->lineWidth == 0) {
		if (pGC->fillStyle == FillSolid)
		   cfbLineSS(pDrawable, pGC, mode, npt, pptInit);
 		else
		   miZeroLine(pDrawable, pGC, mode, npt, pptInit);
	    }
	    else
		miWideLine(pDrawable, pGC, mode, npt, pptInit);
     } else {
	    if ((pGC->lineWidth == 0) && (pGC->fillStyle == FillSolid))
		cfbLineSD(pDrawable, pGC, mode, npt, pptInit);
	    else
		miWideDash(pDrawable, pGC, mode, npt, pptInit);
     }
}


void static
xf86PolySegmentWrapper(pDrawable, pGC, nseg, pSeg)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		nseg;
    register xSegment	*pSeg;
{
    SYNC_CHECK;     

     if((pGC->lineWidth == 0) && (pGC->fillStyle == FillSolid)) {
     	if (pGC->lineStyle == LineSolid) 
		cfbSegmentSS(pDrawable, pGC, nseg, pSeg);
	else
		cfbSegmentSD(pDrawable, pGC, nseg, pSeg);
     } else 
       	miPolySegment(pDrawable, pGC, nseg, pSeg);

}

void static
xf86ImageGlyphBltWrapper(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		
    unsigned char *pglyphBase;	
{
    SYNC_CHECK;     

     miImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
}

void static
xf86PolyGlyphBltWrapper(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		
    unsigned char *pglyphBase;
{
    SYNC_CHECK;     

     miPolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
}

void static
xf86FillSpansWrapper(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GC		*pGC;
    int		nInit;	
    DDXPointPtr pptInit;	
    int *pwidthInit;		
    int fSorted;
{
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

    SYNC_CHECK;     

    switch (pGC->fillStyle) {
    case FillSolid:
	switch (devPriv->rop) {
	case GXcopy:
	    cfbSolidSpansCopy(pDrawable, pGC, nInit, pptInit, 
						pwidthInit, fSorted);
	    break;
	case GXxor:
	    cfbSolidSpansXor(pDrawable, pGC, nInit, pptInit, 
						pwidthInit, fSorted);
	    break;
	default:
	    cfbSolidSpansGeneral(pDrawable, pGC, nInit, pptInit, 
						pwidthInit, fSorted);
	    break;
	}
	break;
    case FillTiled:
	if (devPriv->pRotatedPixmap) {
	    if (pGC->alu == GXcopy && (pGC->planemask & PMSK) == PMSK)
		cfbTile32FSCopy(pDrawable, pGC, nInit, pptInit, 
						pwidthInit, fSorted);
	    else
		cfbTile32FSGeneral(pDrawable, pGC, nInit, pptInit, 	
						pwidthInit, fSorted);
	} else
	    cfbUnnaturalTileFS(pDrawable, pGC, nInit, pptInit, 
						pwidthInit, fSorted);
	break;
    case FillStippled:
	    cfbUnnaturalStippleFS(pDrawable, pGC, nInit, pptInit, 
						pwidthInit, fSorted);
	break;
    case FillOpaqueStippled:
	    cfbUnnaturalStippleFS(pDrawable, pGC, nInit, pptInit, 
						pwidthInit, fSorted);
	break;
    }
}


void  static
xf86PolyFillArcWrapper(pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    SYNC_CHECK;     

     if (pGC->fillStyle == FillSolid) {
    	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	    if(devPriv->rop == GXcopy) 
	        cfbPolyFillArcSolidCopy(pDraw, pGC, narcs, parcs);
	    else
	        cfbPolyFillArcSolidGeneral(pDraw, pGC, narcs, parcs);
      } else 
	 	miPolyFillArc(pDraw, pGC, narcs, parcs);
	     
}

void  static
xf86PolyFillRectWrapper(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		nrectFill; 
    xRectangle	*prectInit;  
{
    SYNC_CHECK;     
        
     if ((pGC->fillStyle == FillSolid) || (pGC->fillStyle == FillTiled)) {
        cfbPolyFillRect(pDrawable, pGC, nrectFill, prectInit);
     } else 
       	miPolyFillRect(pDrawable, pGC, nrectFill, prectInit);

}


RegionPtr  static
xf86CopyAreaWrapper(pSrcDrawable, pDstDrawable,
            pGC, srcx, srcy, width, height, dstx, dsty)
    register DrawablePtr pSrcDrawable;
    register DrawablePtr pDstDrawable;
    GC *pGC;
    int srcx, srcy;
    int width, height;
    int dstx, dsty;
{
    SYNC_CHECK;     

     cfbCopyArea(pSrcDrawable, pDstDrawable,
            pGC, srcx, srcy, width, height, dstx, dsty);
}




void XF86NAME(xf86InitWrappers)()
{
    if(!xf86GCInfoRec.FillPolygonWrapper)
	xf86GCInfoRec.FillPolygonWrapper = xf86FillPolygonWrapper;

    if(!xf86GCInfoRec.PolyRectangleWrapper)
	xf86GCInfoRec.PolyRectangleWrapper = xf86PolyRectangleWrapper;

    if(!xf86GCInfoRec.PolyArcWrapper)
	xf86GCInfoRec.PolyArcWrapper = xf86PolyArcWrapper;

    if(!xf86GCInfoRec.PolyLinesWrapper)
	xf86GCInfoRec.PolyLinesWrapper = xf86PolyLinesWrapper;

    if(!xf86GCInfoRec.PolySegmentWrapper)
	xf86GCInfoRec.PolySegmentWrapper = xf86PolySegmentWrapper;

    if(!xf86GCInfoRec.ImageGlyphBltWrapper)
	xf86GCInfoRec.ImageGlyphBltWrapper = xf86ImageGlyphBltWrapper;

    if(!xf86GCInfoRec.PolyGlyphBltWrapper)
	xf86GCInfoRec.PolyGlyphBltWrapper = xf86PolyGlyphBltWrapper;

    if(!xf86GCInfoRec.FillSpansWrapper)
	xf86GCInfoRec.FillSpansWrapper = xf86FillSpansWrapper;

    if(!xf86GCInfoRec.PolyFillRectWrapper)
	xf86GCInfoRec.PolyFillRectWrapper = xf86PolyFillRectWrapper;

    if(!xf86GCInfoRec.PolyFillArcWrapper)
	xf86GCInfoRec.PolyFillArcWrapper = xf86PolyFillArcWrapper;

    if(!xf86GCInfoRec.CopyAreaWrapper)
	xf86GCInfoRec.CopyAreaWrapper = xf86CopyAreaWrapper;

}



