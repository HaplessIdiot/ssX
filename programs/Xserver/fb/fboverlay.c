/*
 * $XFree86$
 *
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#include "fb.h"
#include "fboverlay.h"

int	fbOverlayGeneration;
int	fbOverlayScreenPrivateIndex;

/*
 * Replace this if you want something supporting
 * multiple overlays with the same depth
 */
Bool
fbOverlayCreateWindow(WindowPtr pWin)
{
    FbOverlayScrPrivPtr	pScrPriv = fbOverlayGetScrPriv(pWin->drawable.pScreen);
    int			i;
    
    if (pWin->drawable.class != InputOutput)
	return TRUE;
    for (i = 0; i < pScrPriv->nlayers; i++)
	 if (pWin->drawable.depth == pScrPriv->pLayer[i]->drawable.depth)
	 {
	    pWin->devPrivates[fbWinPrivateIndex].ptr = (pointer) pScrPriv->pLayer[i];
	    return TRUE;
	 }
    return FALSE;
}

Bool
fbOverlayCloseScreen (int iScreen, ScreenPtr pScreen)
{
    FbOverlayScrPrivPtr	pScrPriv = fbOverlayGetScrPriv(pScreen);
    int			i;

    for (i = 0; i < pScrPriv->nlayers; i++)
	(*pScreen->DestroyPixmap)(pScrPriv->pLayer[i]);
    return TRUE;
}

Bool
fbOverlayCreateScreenResources(ScreenPtr pScreen)
{
    int			i;
    FbOverlayScrInitPtr	pInit = (FbOverlayScrInitPtr) (pScreen)->devPrivates[fbOverlayScreenPrivateIndex].ptr;
    FbOverlayScrPrivPtr	pScrPriv;
    PixmapPtr		pPixmap;
    FbOverlayInitPtr	overlay;
    
    if (!miCreateScreenResources(pScreen))
	return FALSE;
    
    pScrPriv = xalloc (sizeof (FbOverlayScrPrivRec));
    if (!pScrPriv)
	return FALSE;

    for (i = 0; i < pInit->nlayers; i++)
    {
	overlay = &pInit->init[i];
	pPixmap = (*pScreen->CreatePixmap)(pScreen, 0, 0, overlay->depth);
	if (!pPixmap)
	    return FALSE;
	if (!(*pScreen->ModifyPixmapHeader)(pPixmap, pScreen->width,
					    pScreen->height, overlay->depth,
					    BitsPerPixel(overlay->depth),
					    PixmapBytePad(overlay->width, overlay->depth),
					    overlay->pbits))
	    return FALSE;
	pScrPriv->pLayer[i] = pPixmap;
    }
    pScrPriv->nlayers = pInit->nlayers;
    xfree (pInit);
    (pScreen)->devPrivates[fbOverlayScreenPrivateIndex].ptr = (pointer) pScrPriv;
    pScreen->devPrivate = pScrPriv->pLayer[0];
    return TRUE;
}

Bool
fbOverlaySetupScreen(ScreenPtr	pScreen,
		     pointer	pbits1,
		     pointer	pbits2,
		     int	xsize,
		     int	ysize,
		     int	dpix,
		     int	dpiy,
		     int	width1,
		     int	width2,
		     int	bpp1,
		     int	bpp2)
{
    return fbSetupScreen (pScreen,
			  pbits1,
			  xsize,
			  ysize,
			  dpix,
			  dpiy,
			  width1,
			  bpp1);
}

Bool
fbOverlayFinishScreenInit(ScreenPtr	pScreen,
			  pointer	pbits1,
			  pointer	pbits2,
			  int		xsize,
			  int		ysize,
			  int		dpix,
			  int		dpiy,
			  int		width1,
			  int		width2,
			  int		bpp1,
			  int		bpp2,
			  int		depth1,
			  int		depth2)
{
    VisualPtr	visuals;
    DepthPtr	depths;
    int		nvisuals;
    int		ndepths;
    VisualID	defaultVisual;
    FbOverlayScrInitPtr	pInit;

    if (fbOverlayGeneration != serverGeneration)
    {
	fbOverlayScreenPrivateIndex = AllocateScreenPrivateIndex ();
	fbOverlayGeneration = serverGeneration;
    }
    
    pInit = xalloc (sizeof (FbOverlayScrInitRec));
    if (!pInit)
	return FALSE;
    
    
    if (!fbInitVisuals (&visuals, &depths, &nvisuals, &ndepths, &depth1,
			&defaultVisual, ((unsigned long)1<<(bpp1-1)) |
			((unsigned long)1<<(bpp2-1)), 8))
	return FALSE;
    if (! miScreenInit(pScreen, 0, xsize, ysize, dpix, dpiy, 0,
			depth1, ndepths, depths,
			defaultVisual, nvisuals, visuals
#ifdef FB_OLD_SCREEN
		       , (miBSFuncPtr) 0
#endif
		       ))
	return FALSE;
    /* MI thinks there's no frame buffer */
#ifdef MITSHM
    ShmRegisterFbFuncs(pScreen);
#endif
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 2;
    
    pInit->nlayers = 2;
    pInit->init[0].pbits = pbits1;
    pInit->init[0].width = width1;
    pInit->init[0].depth = depth1;
    
    pInit->init[1].pbits = pbits2;
    pInit->init[1].width = width2;
    pInit->init[1].depth = depth2;

    pScreen->devPrivates[fbOverlayScreenPrivateIndex].ptr = (pointer) pInit;
    
    /* overwrite miCloseScreen with our own */
    pScreen->CloseScreen = fbOverlayCloseScreen;
    pScreen->CreateScreenResources = fbOverlayCreateScreenResources;
    pScreen->CreateWindow = fbOverlayCreateWindow;
    return TRUE;
}
