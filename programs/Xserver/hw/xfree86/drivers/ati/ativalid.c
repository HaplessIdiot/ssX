/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/ativalid.c,v 1.5 1999/09/25 14:37:23 dawes Exp $ */
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

#include "atiadapter.h"
#include "atichip.h"
#include "aticrtc.h"
#include "atistruct.h"
#include "ativalid.h"
#include "xf86.h"

/*
 * ATIValidMode --
 *
 * This checks for hardware-related limits on mode timings.  This assumes
 * xf86CheckMode has already done some basic consistency checks.
 */
int
ATIValidMode
(
    int iScreen,
    DisplayModePtr pMode,
    Bool Verbose,
    int flags
)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[iScreen];
    ATIPtr      pATI        = ATIPTR(pScreenInfo);
    Bool        InterlacedSeen;
    int         VDisplay, VTotal, HBlankWidth;
    int         HAdjust, VAdjust, VScan;

    if (flags & MODECHECK_FINAL)
    {
        if (pATI->MaximumInterlacedPitch)
        {
            /*
             * Ensure no interlaced modes have a scanline pitch larger than the
             * limit.
             */
            if (pMode->Flags & V_INTERLACE)
                InterlacedSeen = TRUE;
            else
                InterlacedSeen = pATI->InterlacedSeen;

            if (InterlacedSeen &&
                (pScreenInfo->displayWidth > pATI->MaximumInterlacedPitch))
                return MODE_INTERLACE_WIDTH;

            pATI->InterlacedSeen = InterlacedSeen;
        }

        return MODE_OK;
    }

    if (pMode->VScan <= 1)
        VScan = 1;
    else
        VScan = pMode->VScan;

    if (pMode->Flags & V_DBLSCAN)
        VScan <<= 1;

    if (!pATI->OptionCRT && (pATI->LCDPanelID >= 0))
    {
        if ((pMode->HDisplay > pATI->LCDHorizontal) ||
            (pMode->VDisplay > pATI->LCDVertical))
            return MODE_PANEL;

        /* Adjust effective timings for monitor checks */
        pMode->SynthClock = pATI->LCDClock;

        HAdjust = pATI->LCDHorizontal - pMode->HDisplay;
        pMode->CrtcHDisplay = pATI->LCDHorizontal;
        pMode->CrtcHBlankStart = pATI->LCDHorizontal;
        pMode->CrtcHSyncStart += HAdjust;
        pMode->CrtcHSyncEnd += HAdjust;
        pMode->CrtcHBlankEnd += HAdjust;
        pMode->CrtcHTotal += HAdjust;

        /*
         * Perhaps this should be changed to not ignore the doublescanning or
         * multiscanning specified by the user.
         */
        VScan = pATI->LCDVertical / pMode->VDisplay;
        switch (pATI->NewHW.crtc)
        {
            case ATI_CRTC_VGA:
                if (VScan > 64)
                    VScan = 64;
                break;

            case ATI_CRTC_MACH64:
                if (VScan > 2)
                    VScan = 2;
                break;

            default:
                break;
        }
        VAdjust = pATI->LCDVertical - (pMode->VDisplay * VScan);
        pMode->CrtcVDisplay = pATI->LCDVertical;
        pMode->CrtcVBlankStart = pATI->LCDVertical;
        pMode->CrtcVSyncStart = (pMode->VSyncStart * VScan) + VAdjust;
        pMode->CrtcVSyncEnd = (pMode->VSyncEnd * VScan) + VAdjust;
        pMode->CrtcVBlankEnd = (pMode->VTotal * VScan) + VAdjust;
        pMode->CrtcVTotal = pMode->CrtcVBlankEnd;
    }

    HBlankWidth = (pMode->HTotal >> 3) - (pMode->HDisplay >> 3);
    if (!HBlankWidth)
        return MODE_HBLANK_NARROW;

    switch (pATI->NewHW.crtc)
    {
        case ATI_CRTC_VGA:
            /* Prevent overscans */
            if (HBlankWidth > 63)
                return MODE_HBLANK_WIDE;

            if (pMode->HDisplay > 2048)
                return MODE_BAD_HVALUE;

            if (VScan > 64)
                return MODE_BAD_VSCAN;

            VDisplay = pMode->VDisplay * VScan;
            VTotal = pMode->VTotal * VScan;

            if ((pMode->Flags & V_INTERLACE) && (pATI->Chip < ATI_CHIP_264CT))
            {
                VDisplay >>= 1;
                VTotal >>= 1;
            }

            if ((VDisplay > 2048) || (VTotal > 2050))
                return MODE_BAD_VVALUE;

            if (pATI->Adapter != ATI_ADAPTER_VGA)
                break;

            if ((VDisplay > 1024) || (VTotal > 1025))
                return MODE_BAD_VVALUE;

            break;

        case ATI_CRTC_MACH64:
            if (VScan > 2)
                return MODE_NO_VSCAN;

            break;

        default:
            break;
    }

    return MODE_OK;
}
