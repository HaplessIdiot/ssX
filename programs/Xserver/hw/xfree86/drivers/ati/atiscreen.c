/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atiscreen.c,v 1.1 1999/07/06 11:38:37 dawes Exp $ */
/*
 * Copyright 1999 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
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

#include "ati.h"
#include "aticonsole.h"
#include "atidac.h"
#include "atiscreen.h"

#include "xf86cmap.h"

#include "xf1bpp.h"
#include "xf4bpp.h"
#undef PSZ
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"

#include "mibank.h"
#include "micmap.h"
#include "mipointer.h"

/*
 * ATIScreenInit --
 *
 * This function is called by DIX to initialize the screen.
 */
Bool
ATIScreenInit
(
    int       iScreen,
    ScreenPtr pScreen,
    int       argc,
    char      **argv
)
{
    ScrnInfoPtr  pScreenInfo = xf86Screens[iScreen];
    ATIPtr       pATI        = ATIPTR(pScreenInfo);
    int          VisualMask;

    /* Set video hardware state */
    if (!ATIEnterGraphics(pScreen, pScreenInfo, pATI))
        return FALSE;

    /* Re-initialize mi's visual list */
    miClearVisualTypes();

    if (pScreenInfo->depth > 8)
        VisualMask = TrueColorMask;
    else
        VisualMask = miGetDefaultVisualMask(pScreenInfo->depth);

    if (!miSetVisualTypes(pScreenInfo->depth, VisualMask,
                          pScreenInfo->rgbBits, pScreenInfo->defaultVisual))
        return FALSE;

    /* Initialize framebuffer layer */
    switch (pScreenInfo->bitsPerPixel)
    {
        case 1:
            pATI->Closeable = xf1bppScreenInit(pScreen, pATI->pMemory,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
            break;

        case 4:
            pATI->Closeable = xf4bppScreenInit(pScreen, pATI->pMemory,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
            break;

        case 8:
            pATI->Closeable = cfbScreenInit(pScreen, pATI->pMemory,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
            break;

        case 16:
            pATI->Closeable = cfb16ScreenInit(pScreen, pATI->pMemory,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
            break;

        case 24:
            pATI->Closeable = cfb24ScreenInit(pScreen, pATI->pMemory,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
            break;

        case 32:
            pATI->Closeable = cfb32ScreenInit(pScreen, pATI->pMemory,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
            break;

        default:
            return FALSE;
    }

    if (!pATI->Closeable)
        return FALSE;

    /* Fixup RGB ordering */
    if (pScreenInfo->depth > 8)
    {
        VisualPtr pVisual = pScreen->visuals + pScreen->numVisuals;

        while (--pVisual >= pScreen->visuals)
        {
            if ((pVisual->class | DynamicClass) != DirectColor)
                continue;

            pVisual->offsetRed = pScreenInfo->offset.red;
            pVisual->offsetGreen = pScreenInfo->offset.green;
            pVisual->offsetBlue = pScreenInfo->offset.blue;

            pVisual->redMask = pScreenInfo->mask.red;
            pVisual->greenMask = pScreenInfo->mask.green;
            pVisual->blueMask = pScreenInfo->mask.blue;
        }
    }

    xf86SetBlackWhitePixels(pScreen);

    /* Initialize banking */
    if ((pATI->ApertureSize < (pATI->VideoRAM * 1024)) &&
        !miInitializeBanking(pScreen,
                             pScreenInfo->virtualX, pScreenInfo->virtualY,
                             pScreenInfo->displayWidth, &pATI->BankInfo))
        return FALSE;

    /* Initialize backing store */
    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);

    /* Initialize software cursor */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* Create default colourmap */
    if (!miCreateDefColormap(pScreen))
        return FALSE;

    if (pScreenInfo->depth > 1)
        if (!xf86HandleColormaps(pScreen, (pScreenInfo->depth == 4) ? 16 : 256,
                                 pScreenInfo->rgbBits, ATILoadPalette, NULL,
                                 CMAP_PALETTED_TRUECOLOR |
                                 CMAP_LOAD_EVEN_IF_OFFSCREEN))
            return FALSE;

    /* Set pScreen->SaveScreen and wrap CloseScreen vector */
    pScreen->SaveScreen = ATISaveScreen;
    pATI->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = ATICloseScreen;

    if (serverGeneration == 1)
        xf86ShowUnusedOptions(pScreenInfo->scrnIndex, pScreenInfo->options);

    return TRUE;
}

/*
 * ATICloseScreen --
 *
 * This function is called by DIX to close the screen.
 */
Bool
ATICloseScreen
(
    int       iScreen,
    ScreenPtr pScreen
)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[iScreen];
    ATIPtr      pATI        = ATIPTR(pScreenInfo);
    Bool        Closed = TRUE;

    if ((pScreen->CloseScreen = pATI->CloseScreen))
    {
        pATI->CloseScreen = NULL;
        Closed = (*pScreen->CloseScreen)(iScreen, pScreen);
    }

    pATI->Closeable = FALSE;

    ATILeaveGraphics(pScreenInfo, pATI);

    return Closed;
}
