/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xf86wrapper.c,v 3.11 1998/01/24 16:58:57 hohndel Exp $ */


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
#include "xf86gcmap.h"
#include "xf86xaa.h"
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


/* prototypes for static functions */
void static xf86FillPolygonWrapper(DrawablePtr, GCPtr, int, int, int, DDXPointPtr);
void static xf86PolyArcWrapper(DrawablePtr, GCPtr, int, xArc *);
void static xf86PolyLinesWrapper(DrawablePtr, GCPtr, int, int, DDXPointPtr);
void static xf86PolySegmentWrapper(DrawablePtr, GCPtr, int, register xSegment *);
void static xf86ImageGlyphBltWrapper(DrawablePtr, GC *, int, int, unsigned int, CharInfoPtr *, unsigned char *);
void static xf86PolyGlyphBltWrapper(DrawablePtr, GC *, int, int, unsigned int, CharInfoPtr *, unsigned char *);
void static xf86FillSpansWrapper(DrawablePtr, GC *, int, DDXPointPtr, int *, int);
void static xf86PolyFillArcWrapper(DrawablePtr, GCPtr, int, xArc *);
void static xf86PolyFillRectWrapper(DrawablePtr, GCPtr, int, xRectangle *);
RegionPtr static xf86CopyAreaWrapper(register DrawablePtr, register DrawablePtr, GC *, int, int, int, int, int, int);
void static xf86PutImageWrapper(DrawablePtr, GCPtr, int, int, int, int, int, int, int, char *);
void static xf86PolyPointWrapper(DrawablePtr, GCPtr, int, int, xPoint *);
void static xf86SetSpansWrapper(DrawablePtr, GCPtr, char *, register DDXPointPtr, int *, int, int);
void static xf86PushPixelsWrapper(GCPtr, PixmapPtr, DrawablePtr, int, int, int, int);
void static xf86SaveAreasWrapper(PixmapPtr, RegionPtr, int, int, WindowPtr);
void static xf86RestoreAreasWrapper(PixmapPtr, RegionPtr, int, int, WindowPtr);

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
     /* It's possible that these don't even need a SYNC_CHECK.
	I haven't checked.  If they don't, they don't need a wrapper
	either */   
  
     if(devPriv->rop == GXcopy)
	cfbFillPoly1RectCopy(pDrawable, pGC, shape, mode, count, ptsIn);
     else
	cfbFillPoly1RectGeneral(pDrawable, pGC, shape, mode, count, ptsIn);
}


void static
xf86PolyArcWrapper(pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    /* Only used when PIXEL_ADDR is defined */
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

    SYNC_CHECK;   

    switch (devPriv->rop) {
	case GXxor:
	    cfbZeroPolyArcSS8Xor(pDraw, pGC, narcs, parcs);
	    break;
	case GXcopy:
	    cfbZeroPolyArcSS8Copy(pDraw, pGC, narcs, parcs);
	    break;
	default:
	    cfbZeroPolyArcSS8General(pDraw, pGC, narcs, parcs);
	    break;
    }
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
#ifdef NO_ONE_RECT
	cfb8LineSS1Rect(pDrawable, pGC, mode, npt, pptInit);
#else
	cfbLineSS(pDrawable, pGC, mode, npt, pptInit);
#endif
     } else 
	cfbLineSD(pDrawable, pGC, mode, npt, pptInit);
}


void static
xf86PolySegmentWrapper(pDrawable, pGC, nseg, pSeg)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		nseg;
    register xSegment	*pSeg;
{
    SYNC_CHECK;     

    if (pGC->lineStyle == LineSolid) {
#ifdef NO_ONE_RECT
	cfbSegmentSS1Rect(pDrawable, pGC, nseg, pSeg);
#else
	cfbSegmentSS(pDrawable, pGC, nseg, pSeg);
#endif
    } else
	cfbSegmentSD(pDrawable, pGC, nseg, pSeg);

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

    if (pGC->fillStyle == FillSolid && pGC->alu == GXcopy &&
	(pGC->planemask & PMSK) == PMSK)
       cfbImageGlyphBlt8(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase); 
    else
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

    /* cfbPolyGlyphBlt8 only supports GXcopy */
    if (pGC->fillStyle == FillSolid && pGC->alu == GXcopy)
       cfbPolyGlyphBlt8(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase); 
    else
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
    void (*FillSpansFunc) (
#if NeedNestedPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
#endif
	);
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

    switch (pGC->fillStyle) {
    case FillSolid:
	switch (devPriv->rop) {
	case GXcopy:
	    FillSpansFunc = cfbSolidSpansCopy;
	    break;
	case GXxor:
	    FillSpansFunc = cfbSolidSpansXor;
	    break;
	default:
	    FillSpansFunc = cfbSolidSpansGeneral;
	    break;
	}
	break;
    case FillTiled:
	if (pGC->pRotatedPixmap) {
	    if (pGC->alu == GXcopy && (pGC->planemask & PMSK) == PMSK)
		FillSpansFunc = cfbTile32FSCopy;
	    else
		FillSpansFunc = cfbTile32FSGeneral;
	} else
	    FillSpansFunc = cfbUnnaturalTileFS;
	break;
    case FillStippled:
#ifdef FOUR_BIT_CODE
	if (pGC->pRotatedPixmap)
	    FillSpansFunc = cfb8Stipple32FS;
	else
#endif
	    FillSpansFunc = cfbUnnaturalStippleFS;
	break;
    case FillOpaqueStippled:
#ifdef FOUR_BIT_CODE
	if (pGC->pRotatedPixmap)
	    FillSpansFunc = cfb8OpaqueStipple32FS;
	else
#endif
	    FillSpansFunc = cfbUnnaturalStippleFS;
	break;
    default:
	FatalError("xf86ValidateGC: illegal fillStyle\n");
    }

    SYNC_CHECK;
  
   (*FillSpansFunc)(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
   
}


void  static
xf86PolyFillArcWrapper(pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
     cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);
     SYNC_CHECK;     

     if(devPriv->rop == GXcopy) 
	 cfbPolyFillArcSolidCopy(pDraw, pGC, narcs, parcs);
     else
	 cfbPolyFillArcSolidGeneral(pDraw, pGC, narcs, parcs);
}

void  static
xf86PolyFillRectWrapper(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		nrectFill; 
    xRectangle	*prectInit;  
{
     SYNC_CHECK;
        
     cfbPolyFillRect(pDrawable, pGC, nrectFill, prectInit);
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

    if(xf86VTSema && ((pSrcDrawable->type == DRAWABLE_WINDOW) ||
	(pDstDrawable->type == DRAWABLE_WINDOW))) {
    	SYNC_CHECK;     
    }

    return cfbCopyArea(pSrcDrawable, pDstDrawable,
		       pGC, srcx, srcy, width, height, dstx, dsty);
}

void static
xf86PutImageWrapper(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		depth, x, y, w, h;
    int		leftPad;
    int		format;
    char 	*pImage;
{
    SYNC_CHECK;     

    if (pGC->alu == GXcopy)
	cfbPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage);
    else
	miPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage);
}


void static
xf86PolyPointWrapper(pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr pDrawable;
    GCPtr pGC;
    int mode;
    int npt;
    xPoint *pptInit;
{
    /* currently unused */
}


void static
xf86SetSpansWrapper(pDrawable, pGC, pcharsrc, ppt, pwidth, nspans, fSorted)
    DrawablePtr		pDrawable;
    GCPtr		pGC;
    char		*pcharsrc;
    register DDXPointPtr ppt;
    int			*pwidth;
    int			nspans;
    int			fSorted;
{
    SYNC_CHECK;

    cfbSetSpans(pDrawable, pGC, pcharsrc, ppt, pwidth, nspans, fSorted);
}

void static
xf86PushPixelsWrapper (pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr	pGC;
    PixmapPtr	pBitmap;
    DrawablePtr	pDrawable;
    int		dx, dy, xOrg, yOrg;
{
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

    SYNC_CHECK;

#ifdef FOUR_BIT_CODE
    if (pGC->fillStyle == FillSolid && devPriv->rop == GXcopy)
	cfbPushPixels8(pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg);
    else
#endif
    	mfbPushPixels(pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg);
}


/* These are the wrappers for the backing store functions. */

void static
xf86SaveAreasWrapper(pPixmap, prgnSave, xorg, yorg, pWin)
    PixmapPtr	  	pPixmap;  	/* Backing pixmap */
    RegionPtr	  	prgnSave; 	/* Region to save (pixmap-relative) */
    int	    	  	xorg;	    	/* X origin of region */
    int	    	  	yorg;	    	/* Y origin of region */
    WindowPtr		pWin;
{
    SYNC_CHECK;

    cfbSaveAreas(pPixmap, prgnSave, xorg, yorg, pWin);
}

void static
xf86RestoreAreasWrapper(pPixmap, prgnRestore, xorg, yorg, pWin)
    PixmapPtr	  	pPixmap;  	/* Backing pixmap */
    RegionPtr	  	prgnRestore; 	/* Region to restore (screen-relative)*/
    int	    	  	xorg;	    	/* X origin of window */
    int	    	  	yorg;	    	/* Y origin of window */
    WindowPtr		pWin;
{
    SYNC_CHECK;

    cfbRestoreAreas(pPixmap, prgnRestore, xorg, yorg, pWin);
}

void XF86NAME(xf86InitWrappers)(void)
{
    if(!xf86GCInfoRec.FillPolygonWrapper)
	xf86GCInfoRec.FillPolygonWrapper = xf86FillPolygonWrapper;

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

    if(!xf86GCInfoRec.PolyPointWrapper)
	xf86GCInfoRec.PolyPointWrapper = xf86PolyPointWrapper;

    if(!xf86GCInfoRec.PutImageWrapper)
	xf86GCInfoRec.PutImageWrapper = xf86PutImageWrapper;

    if(!xf86GCInfoRec.SetSpansWrapper)
	xf86GCInfoRec.SetSpansWrapper = xf86SetSpansWrapper;

    if(!xf86GCInfoRec.PushPixelsWrapper)
	xf86GCInfoRec.PushPixelsWrapper = xf86PushPixelsWrapper;

    if(!xf86GCInfoRec.SaveAreasWrapper)
	xf86GCInfoRec.SaveAreasWrapper = xf86SaveAreasWrapper;

    if(!xf86GCInfoRec.RestoreAreasWrapper)
	xf86GCInfoRec.RestoreAreasWrapper = xf86RestoreAreasWrapper;

}

