/* $XFree86$ */
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
#include "cfbmskbits.h"
#include "mibstore.h"
#include "p9000.h"

extern RegionPtr mfbPixmapToRegion();
extern Bool mfbAllocatePrivates();

extern int defaultColorVisualClass;

#define _BP 6 /**** VGA ****/
#define _RZ ((PSZ + 2) / 3)
#define _RS 0
#define _RM ((1 << _RZ) - 1)
#define _GZ ((PSZ - _RZ + 1) / 2)
#define _GS _RZ
#define _GM (((1 << _GZ) - 1) << _GS)
#define _BZ (PSZ - _RZ - _GZ)
#define _BS (_RZ + _GZ)
#define _BM (((1 << _BZ) - 1) << _BS)
#define _CE (1 << _RZ)

static VisualRec visuals[] = {
/* vid  class        bpRGB cmpE nplan rMask gMask bMask oRed oGreen oBlue */
#ifndef STATIC_COLOR
    0,  PseudoColor, _BP,  1<<PSZ,   PSZ,  0,   0,   0,   0,   0,   0,
    0,  DirectColor, _BP, _CE,       PSZ,  _RM, _GM, _BM, _RS, _GS, _BS,
    0,  GrayScale,   _BP,  1<<PSZ,   PSZ,  0,   0,   0,   0,   0,   0,
    0,  StaticGray,  _BP,  1<<PSZ,   PSZ,  0,   0,   0,   0,   0,   0,
#endif
    0,  StaticColor, _BP,  1<<PSZ,   PSZ,  _RM, _GM, _BM, _RS, _GS, _BS,
    0,  TrueColor,   _BP, _CE,       PSZ,  _RM, _GM, _BM, _RS, _GS, _BS
};

#ifdef OMIT_ME

#define	NUMVISUALS	((sizeof visuals)/(sizeof visuals[0]))
static  VisualID VIDs[NUMVISUALS];
static DepthRec depths[] = {
/* depth	numVid		vids */
    1,		0,		NULL,
    8,		NUMVISUALS,	VIDs
};
static unsigned long cfbGeneration = 0;
#endif /* OMIT_ME */

#define NUMDEPTHS	((sizeof depths)/(sizeof depths[0]))

int cfbWindowPrivateIndex;
int cfbGCPrivateIndex;

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
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
    cfbScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width);
    return(1);
}

