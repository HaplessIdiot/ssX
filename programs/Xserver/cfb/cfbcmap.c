/* $TOG: cfbcmap.c /main/25 1998/02/09 14:05:03 kaleb $ */
/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or The Open Group
not be used in advertising or publicity pertaining to 
distribution  of  the software  without specific prior 
written permission. Sun and The Open Group make no 
representations about the suitability of this software for 
any purpose. It is provided "as is" without any express or 
implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/
/* $XFree86: xc/programs/Xserver/cfb/cfbcmap.c,v 3.10 1999/03/01 02:15:09 dawes Exp $ */


#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "resource.h"
#include "micmap.h"

int
cfbListInstalledColormaps(pScreen, pmaps)
    ScreenPtr	pScreen;
    Colormap	*pmaps;
{
    return miListInstalledColormaps(pScreen, pmaps);
}

void
cfbInstallColormap(pmap)
    ColormapPtr	pmap;
{
    miInstallColormap(pmap);
}

void
cfbUninstallColormap(pmap)
    ColormapPtr	pmap;
{
    miUninstallColormap(pmap);
}

void
cfbResolveColor(pred, pgreen, pblue, pVisual)
    unsigned short	*pred, *pgreen, *pblue;
    register VisualPtr	pVisual;
{
    miResolveColor(pred, pgreen, pblue, pVisual);
}

Bool
cfbInitializeColormap(pmap)
    register ColormapPtr	pmap;
{
    return miInitializeColormap(pmap);
}

int
cfbExpandDirectColors (pmap, ndef, indefs, outdefs)
    ColormapPtr	pmap;
    int		ndef;
    xColorItem	*indefs, *outdefs;
{
    return miExpandDirectColors(pmap, ndef, indefs, outdefs);
}

Bool
cfbCreateDefColormap(pScreen)
    ScreenPtr pScreen;
{
    return miCreateDefColormap(pScreen);
}

void
cfbClearVisualTypes()
{
    miClearVisualTypes();
}

Bool
cfbSetVisualTypes (depth, visuals, bitsPerRGB)
    int	    depth;
    int	    visuals;
    int     bitsPerRGB;
{
    return miSetVisualTypes(depth, visuals, bitsPerRGB, -1);
}

/*
 * Given a list of formats for a screen, create a list
 * of visuals and depths for the screen which coorespond to
 * the set which can be used with this version of cfb.
 */

Bool
cfbInitVisuals (visualp, depthp, nvisualp, ndepthp, rootDepthp, defaultVisp, sizes, bitsPerRGB)
    VisualPtr	*visualp;
    DepthPtr	*depthp;
    int		*nvisualp, *ndepthp;
    int		*rootDepthp;
    VisualID	*defaultVisp;
    unsigned long   sizes;
    int		bitsPerRGB;
{
    return miInitVisuals(visualp, depthp, nvisualp, ndepthp, rootDepthp,
			 defaultVisp, sizes, bitsPerRGB, -1);
}
