/*
 * Copyright Loïc Grenié 1999
 */
/* $XFree86$ */

#include "apm.h"
#include "rushproc.h"
#include "xaalocal.h"

static __inline__ void __xf86UnlockPixmap(int scrnIndex, PixmapPtr pix);

int
xf86RushLockPixmap(int scrnIndex, PixmapPtr pix)
{
    APMDECL(xf86Screens[scrnIndex]);
    ApmPixmapPtr pApmPriv = APM_GET_PIXMAP_PRIVATE(pix);
    FBAreaPtr area = ((XAAPixmapPtr)XAA_GET_PIXMAP_PRIVATE(pix))->offscreenArea;

#if 0
    /*
     * This is just an attempt, because Daryll is tampering with MY registers.
     */
    if (pApm->noLinear) {
	WRXB(0xDB, (RDXB(0xDB) & 0xF4) |  0x0A);
	ApmWriteSeq(0x1B, 0x20);
	ApmWriteSeq(0x1C, 0x2F);
    }
    else {
	WRXB_IOP(0xDB, (RDXB_IOP(0xDB) & 0xF4) |  0x0A);
	wrinx(0x3C4, 0x1B, 0x20);
	wrinx(0x3C4, 0x1C, 0x2F);
    }
#endif

    if (area) {
	/*
	 * Easy : just make it un(re)movable...
	 */
	pApmPriv->MoveAreaCallback	= area->MoveAreaCallback;
	area->MoveAreaCallback		= NULL;
	pApmPriv->RemoveAreaCallback	= area->RemoveAreaCallback;
	area->RemoveAreaCallback	= NULL;
	return pApm->LinAddress +
		    (area->box.x1 + area->box.y1 * pApm->displayWidth) *
				pApm->bitsPerPixel / 8;
    }

    return 0;
}

static __inline__ void
__xf86UnlockPixmap(int scrnIndex, PixmapPtr pix)
{
    ApmPixmapPtr pApmPriv = APM_GET_PIXMAP_PRIVATE(pix);
    XAAPixmapPtr pXAAPriv = XAA_GET_PIXMAP_PRIVATE(pix);
    FBAreaPtr area = pXAAPriv->offscreenArea;

    if ((pXAAPriv->flags & OFFSCREEN) && !area->MoveAreaCallback && !area->RemoveAreaCallback) {
	area->MoveAreaCallback		= pApmPriv->MoveAreaCallback;
	area->RemoveAreaCallback	= pApmPriv->RemoveAreaCallback;
    }
}

void
xf86RushUnlockPixmap(int scrnIndex, PixmapPtr pix)
{
    __xf86UnlockPixmap(scrnIndex, pix);
}

void
xf86RushUnlockAllPixmaps()
{
    int		scrnIndex;

    for (scrnIndex = 0; scrnIndex < screenInfo.numScreens; scrnIndex++) {
	PixmapLinkPtr pLink = GET_XAAINFORECPTR_FROM_SCREEN(xf86Screens[scrnIndex]->pScreen)->OffscreenPixmaps;

	while(pLink) {
	    __xf86UnlockPixmap(scrnIndex, pLink->pPix);	
	    pLink = pLink->next;
	}    
    }
}
