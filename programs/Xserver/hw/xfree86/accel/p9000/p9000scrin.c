/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/p9000/p9000scrin.c,v 3.0 1994/05/29 02:05:43 dawes Exp $ */
/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

Modified for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)

ERIK NYGREN, KEVIN E. MARTIN AND RICKARD E. FAITH DISCLAIM ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL ERIK
NYGREN, KEVIN E. MARTIN OR RICKARD E. FAITH BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)

Modified for the P9000 by Erik Nygren (nygren@mit.edu)

********************************************************/

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"
#include "cfb.h"
#include "mi.h"
#include "mistruct.h"
#include "dix.h"
#include "p9000.h"
#include "p9000reg.h"
#include "vgaBank.h"

#ifdef P9000_BANKED
extern void p9000VGABankInit();
#endif /* P9000_BANKED */

extern pointer vgaBase;
extern RegionPtr mfbPixmapToRegion();
extern Bool mfbAllocatePrivates();

Bool
p9000ScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width)
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
    int width;			/* pixel width of frame buffer */
{
#ifdef DEBUG
    ErrorF("Doing cfbScreenInit: base %lx, size %d,%d dpi %d,%d width %d\n",
	   pbits, xsize, ysize, dpix, dpiy, width);
#endif

#ifdef P9000_BANKED
    if (p9000BankSwitching) p9000VGABankInit();
#endif /* P9000_BANKED */
    /* Use VGABASE instead of passed video region if banked */
    cfbScreenInit(pScreen, 
                  (p9000BankSwitching ? ((pointer)vgaBase) : pbits),
                  xsize, ysize, dpix, dpiy, width);
    return(1);
}

