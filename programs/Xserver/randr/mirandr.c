/*
 * $XFree86: xc/programs/Xserver/render/mipict.c,v 1.4 2000/12/05 03:13:32 keithp Exp $
 *
 * Copyright © 2001 Compaq Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Compaq not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Compaq makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * COMPAQ DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Jim Gettys, Compaq Computer Corporation
 */

#include "scrnintstr.h"
#include "mi.h"
#include "randr.h"
#include "randrstr.h"
#include <stdio.h>

/*
 * This function assumes that only a single depth can be
 * displayed at a time, but that all visuals of that depth
 * can be displayed simultaneously.  It further assumes that
 * only a single size is available.  Hardware providing
 * additional capabilties should use different code.
 */

Bool
miRRGetInfo (ScreenPtr pScreen)
{
    int	i;
    for (i = 0; i < pScreen->numDepths; i++)
    {
	if (pScreen->allowedDepths[i].numVids)
	{
	    RRVisualSetPtr  pVisualSet;

	    pVisualSet = RRCreateVisualSet (pScreen);
	    if (pVisualSet)
	    {
		RRSetOfVisualSetPtr pSetOfVisualSet;
		RRSizeInfoPtr	    pSize;

		if (!RRAddDepthToVisualSet (pScreen,
					    pVisualSet,
					    &pScreen->allowedDepths[i]))
		{
		    RRDestroyVisualSet (pScreen, pVisualSet);
		    return FALSE;
		}
		pVisualSet = RRRegisterVisualSet (pScreen, pVisualSet);
		if (!pVisualSet)
		    return FALSE;

		pSetOfVisualSet = RRCreateSetOfVisualSet (pScreen);
		
		if (!RRAddVisualSetToSetOfVisualSet (pScreen,
						     pSetOfVisualSet,
						     pVisualSet))
		{
		    RRDestroySetOfVisualSet (pScreen, pSetOfVisualSet);
		    /* pVisualSet left until screen closed */
		    return FALSE;
		}

		pSize = RRRegisterSize (pScreen,
					pScreen->width,
					pScreen->height,
					pScreen->mmWidth,
					pScreen->mmHeight,
					pSetOfVisualSet);
		if (!pSize)
		    return FALSE;
	    }
	}
    }
}

/*
 * Any hardware that can actually change anything will need something
 * different here
 */
Bool
miRRSetConfig (ScreenPtr	pScreen,
	       int		rotation,
	       int		swap,
	       RRSizeInfoPtr	size)
{
    return TRUE;
}


Bool
miRandRInit (ScreenPtr pScreen)
{
    rrScrPrivPtr    rp;
    
    fprintf(stderr, "got to miRandRInit\n");
    if (!RRScreenInit (pScreen))
	return FALSE;
    fprintf(stderr, "got back successfully from RRScreenInit\n");
    rp = rrGetScrPriv(pScreen);
    rp->rrGetInfo = miRRGetScreenInfo;
    rp->rrSetConfig = miRRSetScreenConfig;
    return TRUE;
}
