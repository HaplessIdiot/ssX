/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_rush.c,v 1.1 1999/07/10 12:17:29 dawes Exp $ */
/*
 * Copyright Loïc Grenié 1999
 */

#include "apm.h"
#include "xaalocal.h"

static __inline__ void	__xf86UnlockPixmap(ApmPtr, PixmapLinkPtr pLink);
static int		xf86RushLockPixmap(int scrnIndex, PixmapPtr pix);
static void		xf86RushUnlockPixmap(int scrnIndex, PixmapPtr pix);
static void		xf86RushUnlockAllPixmaps(void);

int
xf86RushLockPixmap(int scrnIndex, PixmapPtr pix)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    APMDECL(pScrn);
    ApmPixmapPtr pApmPriv = APM_GET_PIXMAP_PRIVATE(pix);
    FBAreaPtr area = ((XAAPixmapPtr)XAA_GET_PIXMAP_PRIVATE(pix))->offscreenArea;
    int	p2, width = pApm->bytesPerScanline;

    pApm->apmLock = TRUE;
    if (area) {
	RegionRec	NewReg;

	/*
	 * 1) Make it unmovable so that XAA won't know we're playing
	 *    with the cache.
	 * 2) Play musical chairs if needed.
	 */
	pApmPriv->MoveAreaCallback	= area->MoveAreaCallback;
	area->MoveAreaCallback		= NULL;
	pApmPriv->RemoveAreaCallback	= area->RemoveAreaCallback;
	area->RemoveAreaCallback	= NULL;
	pApmPriv->devPriv		= area->devPrivate.ptr;
	if (((pix->drawable.x + pApm->displayWidth * pix->drawable.y) * pApm->bitsPerPixel) & 32767) {
	    int		p1;

	    /*
	     * Not aligned on a 4KB boundary, need to move it around.
	     */
	    xf86FreeOffscreenArea(area);
	    p2 = 1;
	    while (!(p2 & width))
		p2 *= 2;
	    p1 = 4096 / p2 - 1;
	    switch(pApm->bitsPerPixel) {
	    case 16:
		p2 /= 2;
		break;
	    case 32:
		p2 /= 4;
		break;
	    }
	    ((XAAPixmapPtr)XAA_GET_PIXMAP_PRIVATE(pix))->offscreenArea = area =
			    xf86AllocateOffscreenArea(pScrn->pScreen,
				pix->drawable.width, pix->drawable.height + p1,
				p2, NULL, NULL, pApmPriv->devPriv);
	    if (area) {
		int	devKind = pApm->bytesPerScanline, off = devKind * p1;
		int	h, goal = (-area->box.x1 * (pApm->bitsPerPixel >> 3) - area->box.y1 * devKind) & 4095;

		pix->drawable.x = area->box.x1;
		for (h = p1; h >= 0; h--, off -= devKind)
		    if ((off & 4095) == goal)
			break;
		pix->drawable.y = area->box.y1 + h;
	    }
	    else {
		/*
		 * Failed, return the old one
		 */
		switch(pApm->bitsPerPixel) {
		    case 24:
		    case  8:	p2 = 4; break;
		    case 16:	p2 = 2; break;
		    case 32:	p2 = 1; break;
		    default:	p2 = 0; break;
		}
		((XAAPixmapPtr)XAA_GET_PIXMAP_PRIVATE(pix))->offscreenArea = area =
				xf86AllocateOffscreenArea(pScrn->pScreen,
				    pix->drawable.width, pix->drawable.height,
				    p2,
				    pApmPriv->MoveAreaCallback,
				    pApmPriv->RemoveAreaCallback,
				    pApmPriv->devPriv);
		/* The allocate can not fail: we just removed the old one. */
		pix->drawable.x = area->box.x1;
		pix->drawable.y = area->box.y1;
		return 0;
	    }
	}
	REGION_INIT(pApm->pScreen, &NewReg, &area->box, 1)
	REGION_UNION(pApm->pScreen, &pApm->apmLockedRegion, &pApm->apmLockedRegion, &NewReg);
	REGION_UNINIT(pApm->pScreen, &NewReg);
	return pApm->LinAddress +
		    ((pix->drawable.x + pApm->displayWidth * pix->drawable.y) * pApm->bitsPerPixel) / 8;
    }

    return 0;
}

static __inline__ void
__xf86UnlockPixmap(ApmPtr pApm, PixmapLinkPtr pLink)
{
    PixmapPtr pix = pLink->pPix;
    ApmPixmapPtr pApmPriv = APM_GET_PIXMAP_PRIVATE(pix);
    XAAPixmapPtr pXAAPriv = XAA_GET_PIXMAP_PRIVATE(pix);
    FBAreaPtr area = pXAAPriv->offscreenArea;

    if (!area)
	area = pLink->area;
    if ((pXAAPriv->flags & OFFSCREEN) && !area->MoveAreaCallback && !area->RemoveAreaCallback) {
	RegionRec	NewReg;

	area->MoveAreaCallback		= pApmPriv->MoveAreaCallback;
	area->RemoveAreaCallback	= pApmPriv->RemoveAreaCallback;
	area->devPrivate.ptr		= pApmPriv->devPriv;
	REGION_INIT(pApm->pScreen, &NewReg, &area->box, 1)
	REGION_SUBTRACT(pApm->pScreen, &pApm->apmLockedRegion, &pApm->apmLockedRegion, &NewReg);
	REGION_UNINIT(pApm->pScreen, &NewReg);
    }
}

void
xf86RushUnlockPixmap(int scrnIndex, PixmapPtr pix)
{
    APMDECL(xf86Screens[scrnIndex]);
    PixmapLinkPtr pLink = GET_XAAINFORECPTR_FROM_SCREEN(xf86Screens[scrnIndex]->pScreen)->OffscreenPixmaps;

    if (pApm->apmLock) {
	/*
	 * This is just an attempt, because Daryll is tampering with MY
	 * registers.
	 */
	if (!pApm->noLinear) {
	    WRXB(0xDB, (RDXB(0xDB) & 0xF4) |  0x0A);
	    ApmWriteSeq(0x1B, 0x20);
	    ApmWriteSeq(0x1C, 0x2F);
	}
	else {
	    WRXB_IOP(0xDB, (RDXB_IOP(0xDB) & 0xF4) |  0x0A);
	    wrinx(0x3C4, 0x1B, 0x20);
	    wrinx(0x3C4, 0x1C, 0x2F);
	}
	pApm->apmLock = FALSE;
    }
    while (pLink && pLink->pPix != pix)
	pLink = pLink->next;
    __xf86UnlockPixmap(pApm, pLink);
}

void
xf86RushUnlockAllPixmaps()
{
    int		scrnIndex;

    for (scrnIndex = 0; scrnIndex < screenInfo.numScreens; scrnIndex++) {
	PixmapLinkPtr pLink = GET_XAAINFORECPTR_FROM_SCREEN(xf86Screens[scrnIndex]->pScreen)->OffscreenPixmaps;

	while(pLink) {
	    __xf86UnlockPixmap(APMPTR(xf86Screens[scrnIndex]), pLink);	
	    pLink = pLink->next;
	}    
    }
}

#ifdef XF86RUSH_EXT
/*

Copyright (c) 1998 Daryll Strauss

*/

#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#define _XF86RUSH_SERVER_
#include "xf86rushstr.h"

static unsigned char RushReqCode = 0;
static int RushErrorBase;

static DISPATCH_PROC(ProcXF86RushDispatch);
static DISPATCH_PROC(ProcXF86RushQueryVersion);
static DISPATCH_PROC(ProcXF86RushLockPixmap);
static DISPATCH_PROC(ProcXF86RushUnlockPixmap);
static DISPATCH_PROC(ProcXF86RushUnlockAllPixmaps);

static DISPATCH_PROC(SProcXF86RushDispatch);

static void XF86RushResetProc(ExtensionEntry* extEntry);

void
XFree86RushExtensionInit()
{
    ExtensionEntry* extEntry;

    if ((extEntry = AddExtension(XF86RUSHNAME,
				XF86RushNumberEvents,
				XF86RushNumberErrors,
				ProcXF86RushDispatch,
				SProcXF86RushDispatch,
				XF86RushResetProc,
				StandardMinorOpcode))) {
	RushReqCode = (unsigned char)extEntry->base;
	RushErrorBase = extEntry->errorBase;
    }
}

/*ARGSUSED*/
static void
XF86RushResetProc (extEntry)
    ExtensionEntry* extEntry;
{
}

static int
ProcXF86RushQueryVersion(client)
    register ClientPtr client;
{
    xXF86RushQueryVersionReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xXF86RushQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = XF86RUSH_MAJOR_VERSION;
    rep.minorVersion = XF86RUSH_MINOR_VERSION;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    }
    WriteToClient(client, sizeof(xXF86RushQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86RushLockPixmap(client)
     register ClientPtr client;
{
  REQUEST(xXF86RushLockPixmapReq);
  xXF86RushLockPixmapReply rep;
  PixmapPtr pix;

  if (stuff->screen > screenInfo.numScreens)
    return BadValue;

  REQUEST_SIZE_MATCH(xXF86RushLockPixmapReq);
  rep.type = X_Reply;
  rep.length = 0;
  rep.sequenceNumber = client->sequence;
  pix = (PixmapPtr)SecurityLookupIDByType(client,
					  stuff->pixmap, RT_PIXMAP, 
					  SecurityReadAccess);
  rep.addr = xf86RushLockPixmap(stuff->screen, pix);

  WriteToClient(client, SIZEOF(xXF86RushLockPixmapReply), (char*)&rep);
  return client->noClientException;
}

static int
ProcXF86RushUnlockPixmap(client)
     register ClientPtr client;
{
  REQUEST(xXF86RushUnlockPixmapReq);
  PixmapPtr pix;

  if (stuff->screen > screenInfo.numScreens)
    return BadValue;

  REQUEST_SIZE_MATCH(xXF86RushUnlockPixmapReq);
  pix = (PixmapPtr)SecurityLookupIDByType(client,
					  stuff->pixmap, RT_PIXMAP, 
					  SecurityReadAccess);
  xf86RushUnlockPixmap(stuff->screen, pix);
  return client->noClientException;
}

static int
ProcXF86RushUnlockAllPixmaps(client)
     register ClientPtr client;
{

  REQUEST_SIZE_MATCH(xXF86RushUnlockAllPixmapsReq);
  xf86RushUnlockAllPixmaps();
  return client->noClientException;
}

int
ProcXF86RushDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);

    if (!LocalClient(client))
	return RushErrorBase + XF86RushClientNotLocal;

    switch (stuff->data)
    {
    case X_XF86RushQueryVersion:
	return ProcXF86RushQueryVersion(client);
    case X_XF86RushLockPixmap:
        return ProcXF86RushLockPixmap(client);
    case X_XF86RushUnlockPixmap:
        return ProcXF86RushUnlockPixmap(client);
    case X_XF86RushUnlockAllPixmaps:
        return ProcXF86RushUnlockAllPixmaps(client);
    default:
	return BadRequest;
    }
}

int
SProcXF86RushDispatch (client)
    register ClientPtr	client;
{
    return RushErrorBase + XF86RushClientNotLocal;
}
#endif /* XF86RUSH_EXT */
