/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/aticonsole.c,v 1.6 2000/01/21 01:12:14 dawes Exp $ */
/*
 * Copyright 1997 through 1999 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "aticonsole.h"
#include "aticrtc.h"
#include "atilock.h"
#include "atimach64.h"
#include "atistruct.h"
#include "ativga.h"
#include "atividmem.h"
#include "xf86.h"

/*
 * ATISaveScreen --
 *
 * DIX calls this function to blank the screen.
 */
Bool
ATISaveScreen
(
    ScreenPtr pScreen,
    int       Mode
)
{
    ScrnInfoPtr pScreenInfo;
    ATIPtr      pATI;
    Bool	On;

    On = xf86IsBlank(Mode);

    if (On)
        SetTimeSinceLastInputEvent();

    if (!pScreen)
        return TRUE;

    pScreenInfo = xf86Screens[pScreen->myNum];
    if (!pScreenInfo->vtSema)
        return TRUE;

    pATI = ATIPTR(pScreenInfo);
    switch (pATI->NewHW.crtc)
    {
        case ATI_CRTC_VGA:
            ATIVGASaveScreen(pATI, On);
            break;

        case ATI_CRTC_MACH64:
            ATIMach64SaveScreen(pATI, On);
            break;

        default:
            break;
    }

    return TRUE;
}

/*
 * ATIEnterGraphics --
 *
 * This function sets the hardware to a graphics video state.
 */
Bool
ATIEnterGraphics
(
    ScreenPtr   pScreen,
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    /* Map apertures */
    if (!ATIMapApertures(pScreenInfo, pATI))
        return FALSE;

    /* Unlock device */
    ATIUnlock(pATI);

    /* Calculate hardware data */
    if (pScreen &&
        !ATICRTCCalculate(pScreenInfo, pATI, &pATI->NewHW,
                          pScreenInfo->currentMode))
        return FALSE;

    pScreenInfo->vtSema = TRUE;

    /* Save current state */
    ATICRTCSave(pScreenInfo, pATI, &pATI->OldHW);

    /* Set graphics state */
    ATICRTCSet(pScreenInfo, pATI, &pATI->NewHW);

    /* Possibly blank the screen */
    if (pScreen)
       (void)ATISaveScreen(pScreen, SCREEN_SAVER_ON);

    /* Position the screen */
    (*pScreenInfo->AdjustFrame)(pScreenInfo->scrnIndex,
        pScreenInfo->frameX0, pScreenInfo->frameY0, 0);

    SetTimeSinceLastInputEvent();

    return TRUE;
}

/*
 * ATILeaveGraphics --
 *
 * This function restores the hardware to its previous state.
 */
void
ATILeaveGraphics
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    if (pScreenInfo->vtSema)
    {
        /* If not exiting, save graphics video state */
        if (!xf86ServerIsExiting())
            ATICRTCSave(pScreenInfo, pATI, &pATI->NewHW);

        /* Restore mode in effect on server entry */
        ATICRTCSet(pScreenInfo, pATI, &pATI->OldHW);

        pScreenInfo->vtSema = FALSE;
    }

    /* Lock device */
    ATILock(pATI);

    /* Unmap apertures */
    ATIUnmapApertures(pScreenInfo, pATI);

    SetTimeSinceLastInputEvent();
}

/*
 * ATISwitchMode --
 *
 * This function switches to another graphics video state.
 */
Bool
ATISwitchMode
(
    int            iScreen,
    DisplayModePtr pMode,
    int            flags
)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[iScreen];
    ATIPtr      pATI        = ATIPTR(pScreenInfo);

    /* Calculate new hardware data */
    if (!ATICRTCCalculate(pScreenInfo, pATI, &pATI->NewHW, pMode))
        return FALSE;

    /* Set new hardware state */
    if (pScreenInfo->vtSema)
        ATICRTCSet(pScreenInfo, pATI, &pATI->NewHW);

    SetTimeSinceLastInputEvent();

    return TRUE;
}

/*
 * ATIEnterVT --
 *
 * This function sets the server's virtual console to a graphics video state.
 */
Bool
ATIEnterVT
(
    int iScreen,
    int flags
)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[iScreen];
    ScreenPtr   pScreen     = pScreenInfo->pScreen;
    ATIPtr      pATI        = ATIPTR(pScreenInfo);
    PixmapPtr	pPixmap	    = (*pScreen->GetScreenPixmap) (pScreen);
    Bool	ret;
    Bool	enabled;

    if (!ATIEnterGraphics(NULL, pScreenInfo, pATI))
        return FALSE;

    /* If used, modify banking interface */
    if (!miModifyBanking(pScreen, &pATI->BankInfo))
        return FALSE;

    enabled = pPixmap->devPrivate.ptr != NULL;
    if (!enabled)
	pPixmap->devPrivate = pScreenInfo->pixmapPrivate;
    
    /* Tell framebuffer about remapped aperture */
    ret = (*pScreen->ModifyPixmapHeader)(pPixmap, 
					 -1, -1, -1, -1, -1, pATI->pMemory);

    if (!enabled)
    {
	pScreenInfo->pixmapPrivate = pPixmap->devPrivate;
	pPixmap->devPrivate.ptr = NULL;
    }
    return ret;
}

/*
 * ATILeaveVT --
 *
 * This function restores the server's virtual console to its state on server
 * entry.
 */
void
ATILeaveVT
(
    int iScreen,
    int flags
)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[iScreen];

    ATILeaveGraphics(pScreenInfo, ATIPTR(pScreenInfo));
}

/*
 * ATIFreeScreen --
 *
 * This function frees all driver data related to a screen.
 */
void
ATIFreeScreen
(
    int iScreen,
    int flags
)
{
    ScreenPtr   pScreen     = screenInfo.screens[iScreen];
    ScrnInfoPtr pScreenInfo = xf86Screens[iScreen];
    ATIPtr      pATI        = ATIPTR(pScreenInfo);

    if (pATI->Closeable)
        (void)(*pScreen->CloseScreen)(iScreen, pScreen);

    ATILeaveGraphics(pScreenInfo, pATI);

    xfree(pATI->OldHW.frame_buffer);
    xfree(pATI->NewHW.frame_buffer);

    xfree(pATI->pShadow);

    xfree(pATI);
    pScreenInfo->driverPrivate = NULL;
}
