/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaOverlay.c,v 1.1 1998/10/05 13:23:18 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "xf86_8plus24.h"


static void 
XAAOverlayFillSolidRects(
   WindowPtr pWin,
   unsigned long fg,
   unsigned long planemask,
   RegionPtr pReg
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_DRAWABLE((DrawablePtr)pWin);

    (*infoRec->FillSolidRects)(infoRec->pScrn, fg, GXcopy, planemask,
		REGION_NUM_RECTS(pReg), REGION_RECTS(pReg));
}


static void 
XAAOverlayFillTiledRects(
   WindowPtr pWin,
   PixmapPtr pPix,
   int xorg, int yorg,
   unsigned long planemask,
   RegionPtr pReg
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_DRAWABLE((DrawablePtr)pWin);
    XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
    int nBox = REGION_NUM_RECTS(pReg);
    BoxPtr pBox = REGION_RECTS(pReg);


    if(!(pPriv->flags & REDUCIBILITY_CHECKED) &&
	(infoRec->CanDoMono8x8 || infoRec->CanDoColor8x8)) {
	XAACheckTileReducibility(pPix, infoRec->CanDoMono8x8);
    }

    if(pPriv->flags & REDUCIBLE_TO_8x8) {
	    if((pPriv->flags & REDUCIBLE_TO_2_COLOR) &&
		infoRec->CanDoMono8x8 && infoRec->FillMono8x8PatternRects &&
		!(infoRec->FillMono8x8PatternRectsFlags & TRANSPARENCY_ONLY) && 
		(!(infoRec->FillMono8x8PatternRectsFlags & RGB_EQUAL) || 
		(CHECK_RGB_EQUAL(pPriv->fg) && CHECK_RGB_EQUAL(pPriv->bg)))) {

	    	(*infoRec->FillMono8x8PatternRects)(infoRec->pScrn,
			pPriv->fg, pPriv->bg, GXcopy, planemask, nBox, pBox,
			pPriv->pattern0, pPriv->pattern1, xorg, yorg);
		return;
	    }
	    if(infoRec->CanDoColor8x8 && infoRec->FillColor8x8PatternRects) {
		XAACacheInfoPtr pCache = (*infoRec->CacheColor8x8Pattern)(
					infoRec->pScrn, pPix, -1, -1);

		(*infoRec->FillColor8x8PatternRects) (infoRec->pScrn, 
			GXcopy, planemask, nBox, pBox, xorg, yorg, pCache);
		return;
	    }        
    }
    if(infoRec->UsingPixmapCache && infoRec->FillCacheBltRects && 
	(pPix->drawable.height <= infoRec->MaxCacheableTileHeight) &&
	(pPix->drawable.width <= infoRec->MaxCacheableTileWidth)) {

	XAACacheInfoPtr pCache = (*infoRec->CacheTile)(infoRec->pScrn, pPix);
	(*infoRec->FillCacheBltRects)(infoRec->pScrn, GXcopy, planemask,
		nBox, pBox, xorg, yorg, pCache);
	return;
    }

    if(infoRec->PolyFillRectImageWrite) {
	(*infoRec->FillImageWriteRects) (infoRec->pScrn, GXcopy, 
		planemask, nBox, pBox, xorg, yorg, pPix);
	return;
    }

    /* need to bail out here */
    (*diOverlayFBfuncs.FillTiledRects)(pWin, pPix, xorg, yorg, planemask, pReg);
}


static void 
XAAOverlayCopyAreas(
   WindowPtr pWin,
   unsigned long planemask,
   DDXPointPtr ppnts,
   RegionPtr pReg
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_DRAWABLE((DrawablePtr)pWin);

    infoRec->ScratchGC.planemask = planemask;
    infoRec->ScratchGC.alu = GXcopy;

    XAADoBitBlt((DrawablePtr)pWin, (DrawablePtr)pWin,
        		&(infoRec->ScratchGC), pReg, ppnts);

}


OverlayFBfuncs XAAOverlayFBfuncs = {
   XAAOverlayFillSolidRects,
   XAAOverlayFillTiledRects,
   XAAOverlayCopyAreas
};
