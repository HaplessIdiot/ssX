#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "xf86str.h"
#include "mi.h"
#include "miline.h"
#include "xaa.h"
#include "xaalocal.h"
#include "xaawrap.h"
#include "xf86fbman.h"
#include "servermd.h"

void
XAAMoveOutOffscreenPixmaps(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);
    PixmapLinkPtr pLink = infoRec->OffscreenPixmaps;
    XAAPixmapPtr pPriv;
    
    while(pLink) {
	pPriv = XAA_GET_PIXMAP_PRIVATE(pLink->pPix);
	pLink->area = pPriv->offscreenArea;
	XAAMoveOutOffscreenPixmap(pLink->pPix);	
	pLink = pLink->next;
    }    
}



void
XAAMoveInOffscreenPixmaps(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);
    PixmapLinkPtr pLink = infoRec->OffscreenPixmaps;
    PixmapPtr pPix, pScreenPix;
    XAAPixmapPtr pPriv;
    FBAreaPtr area;

    pScreenPix = (*pScreen->GetScreenPixmap)(pScreen);

    while(pLink) {
	pPix = pLink->pPix;
    	pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
	area = pPriv->offscreenArea = pLink->area;

	xfree(pPix->devPrivate.ptr);

	pPix->drawable.x = area->box.x1;
	pPix->drawable.y = area->box.y1;
	pPix->devKind = pScreenPix->devKind;
	pPix->devPrivate.ptr = pScreenPix->devPrivate.ptr;
	pPix->drawable.serialNumber = NEXT_SERIAL_NUMBER;

	pLink = pLink->next;
    }    
}


void
XAARemoveAreaCallback(FBAreaPtr area)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(area->pScreen);
    PixmapPtr pPix = (PixmapPtr)area->devPrivate.ptr;
    XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);

    XAAMoveOutOffscreenPixmap(pPix);

    pPriv->flags &= ~OFFSCREEN;	

    DELIST_OFFSCREEN_PIXMAP(pPix);
}

void
XAAMoveOutOffscreenPixmap(PixmapPtr pPix) 
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
    int width, height, devKind, bitsPerPixel;
    PixmapPtr tmpPix;
    unsigned char *data;
    GCPtr pGC;

    width = pPix->drawable.width;
    height = pPix->drawable.height;
    bitsPerPixel = pPix->drawable.bitsPerPixel;

    devKind = (((width * bitsPerPixel) + 31) >> 5) << 2; /* which macro ?*/
    if(!(data = (unsigned char*)xalloc(devKind * height)))
	FatalError("Out of memory\n");

    tmpPix = GetScratchPixmapHeader(pScreen, width, height, 
		pPix->drawable.depth, bitsPerPixel, devKind, data);
    if(!tmpPix) {
	xfree(data);
	FatalError("Out of memory\n");
    }

    pGC = GetScratchGC(pPix->drawable.depth, pScreen);
    ValidateGC((DrawablePtr)tmpPix, pGC);

    (*pGC->ops->CopyArea)((DrawablePtr)pPix, (DrawablePtr)tmpPix,
		pGC, 0, 0, width, height, 0, 0);	

    FreeScratchGC(pGC);
    FreeScratchPixmapHeader(tmpPix);

    pPix->drawable.x = 0;
    pPix->drawable.y = 0;
    pPix->devKind = devKind;
    pPix->devPrivate.ptr = data;
    pPix->drawable.serialNumber = NEXT_SERIAL_NUMBER;

    pPriv->offscreenArea = NULL;
}
