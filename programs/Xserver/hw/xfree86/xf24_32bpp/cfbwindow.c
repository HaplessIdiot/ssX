/* $XFree86$ */

#include "X.h"
#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb24.h"
#include "cfb32.h"
#include "cfb24_32.h"
#include "mi.h"


Bool
cfb24_32CreateWindow(WindowPtr pWin)
{
    cfbPrivWin *pPrivWin = cfbGetWindowPrivate(pWin);

    pPrivWin->fastBackground = FALSE;
    pPrivWin->fastBorder = FALSE;

    pWin->drawable.bitsPerPixel = 24;
    return TRUE;
}


Bool
cfb24_32DestroyWindow(WindowPtr pWin)
{
    return TRUE;
}

Bool
cfb24_32PositionWindow(
    WindowPtr pWin,
    int x, int y
){
    return TRUE;
}


Bool
cfb24_32ChangeWindowAttributes(
    WindowPtr pWin,
    unsigned long mask
){ 
    cfb24_32PixmapPtr pixPriv;
    PixmapPtr pPix;

    /* The dix layer may have incremented a refcnt.  We sync them here */

    if((mask & CWBackPixmap) && (pWin->backgroundState == BackgroundPixmap)) {
	pPix = pWin->background.pixmap;
	pixPriv = CFB24_32_GET_PIXMAP_PRIVATE(pPix);

	if(pixPriv->pix && (pPix->refcnt != pixPriv->pix->refcnt))
	    pixPriv->pix->refcnt = pPix->refcnt;

	if(pPix->drawable.bitsPerPixel != 24)
	    pWin->background.pixmap = cfb24_32RefreshPixmap(pPix);
    }	

    if((mask & CWBorderPixmap) && !pWin->borderIsPixel) {
	pPix = pWin->border.pixmap;
	pixPriv = CFB24_32_GET_PIXMAP_PRIVATE(pPix);

	if(pixPriv->pix && (pPix->refcnt != pixPriv->pix->refcnt))
	    pixPriv->pix->refcnt = pPix->refcnt;

	if(pPix->drawable.bitsPerPixel != 24)
	    pWin->border.pixmap = cfb24_32RefreshPixmap(pPix);
    }

    return TRUE;
}

