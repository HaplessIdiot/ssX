/* $XFree86$ */


#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb32.h"
#include "cfb8_32.h"
#include "mistruct.h"
#include "regionstr.h"
#include "cfbmskbits.h"


/* We don't bother with cfb's fastBackground/Border so we don't
   need to use the Window privates */

extern WindowPtr *WindowTable;


Bool
cfb8_32CreateWindow(WindowPtr pWin)
{
    pWin->drawable.bitsPerPixel = 32;
    return TRUE;
}


Bool
cfb8_32DestroyWindow(WindowPtr pWin)
{
    return TRUE;
}

Bool
cfb8_32PositionWindow(
    WindowPtr pWin,
    int x, int y
){
    return TRUE;
}



void
cfb8_32CopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DDXPointPtr ppt, pptSrc;
    RegionRec rgnDst8, rgnDst32;
    BoxPtr pbox;
    int i, nbox, dx, dy;
    WindowPtr pRoot = WindowTable[pScreen->myNum];

    REGION_INIT(pScreen, &rgnDst8, NullBox, 0);
    REGION_INIT(pScreen, &rgnDst32, NullBox, 0);

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pScreen, prgnSrc, -dx, -dy);
    REGION_INTERSECT(pScreen, &rgnDst8, &pWin->borderClip, prgnSrc);

    nbox = REGION_NUM_RECTS(&rgnDst8);
    if(nbox &&
	(pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec)))) {

	pbox = REGION_RECTS(&rgnDst8);
	for (i = nbox, ppt = pptSrc; i--; ppt++, pbox++) {
	    ppt->x = pbox->x1 + dx;
	    ppt->y = pbox->y1 + dy;
	}

	cfbDoBitblt8To8GXcopy((DrawablePtr)pRoot, (DrawablePtr)pRoot,
				GXcopy, &rgnDst8, pptSrc, ~0, 0);
	DEALLOCATE_LOCAL(pptSrc);
    }

    cfb8_32SegregateChildren(pWin, &rgnDst32);
    if(REGION_NOTEMPTY(pScreen, &rgnDst32)) {
	REGION_INTERSECT(pScreen, &rgnDst32, &rgnDst32, prgnSrc);
	nbox = REGION_NUM_RECTS(&rgnDst32);
	if(nbox &&
	  (pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec)))){

	    pbox = REGION_RECTS(&rgnDst32);
	    for (i = nbox, ppt = pptSrc; i--; ppt++, pbox++) {
		ppt->x = pbox->x1 + dx;
		ppt->y = pbox->y1 + dy;
	    }

	    cfbDoBitblt24To24GXcopy((DrawablePtr)pRoot, (DrawablePtr)pRoot,
				GXcopy, &rgnDst32, pptSrc, ~0, 0);
	    DEALLOCATE_LOCAL(pptSrc);
	}
    }

    REGION_UNINIT(pScreen, &rgnDst8);
    REGION_UNINIT(pScreen, &rgnDst32);
}

Bool
cfb8_32ChangeWindowAttributes(
    WindowPtr pWin,
    unsigned long mask
){
    return TRUE;
}


void
cfb8_32SegregateChildren(
    WindowPtr pWin, 
    RegionPtr pReg32
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    WindowPtr pChild;

    for(pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
	if(pChild->drawable.depth == 24)
	    REGION_UNION(pScreen, pReg32, pReg32, &pChild->borderClip);

	if(pChild->firstChild)
	    cfb8_32SegregateChildren(pChild, pReg32);
    }
}

void
cfb8_32WindowExposures(
   WindowPtr pWin,
   RegionPtr pReg,
   RegionPtr pOtherReg
){

    if(REGION_NUM_RECTS(pReg) && (pWin->drawable.depth == 24)) {
	cfb8_32ScreenPtr pScreenPriv = 
		CFB8_32_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);

	cfb8_32FillBoxSolid8((DrawablePtr)pWin, REGION_NUM_RECTS(pReg),
		REGION_RECTS(pReg), pScreenPriv->key);
    }

    miWindowExposures(pWin, pReg, pOtherReg);
}




