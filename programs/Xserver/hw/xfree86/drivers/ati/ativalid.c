/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/ativalid.c,v 1.2tsi Exp $ */
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
    ATIPtr pATI = ATIPTR(xf86Screens[iScreen]);
    int    VDisplay, VTotal;
    int    HBlankWidth = (pMode->HTotal >> 3) - (pMode->HDisplay >> 3);

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

            VDisplay = pMode->VDisplay;
            VTotal = pMode->VTotal;

            if (pMode->VScan > 1)
            {
                if (pMode->VScan > 32)
                    return MODE_BAD_VSCAN;

                VDisplay *= pMode->VScan;
                VTotal *= pMode->VScan;
            }

            if (pMode->Flags & V_DBLSCAN)
            {
                VDisplay <<= 1;
                VTotal <<= 1;
            }

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
            if (pMode->VScan > 1)
                return MODE_NO_VSCAN;

            break;

        default:
            break;
    }

    return MODE_OK;
}
