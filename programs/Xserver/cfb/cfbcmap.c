/* $XConsortium: cfbcmap.c,v 4.19 94/04/17 20:28:46 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/cfb/cfbcmap.c,v 3.3.2.5 1998/07/19 13:21:43 dawes Exp $ */
/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or X Consortium
not be used in advertising or publicity pertaining to 
distribution  of  the software  without specific prior 
written permission. Sun and X Consortium make no 
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


#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "resource.h"
#include "micmap.h"

#if defined(GLXEXT) && !defined(XFree86LOADER)
extern Bool GlxInitVisuals(
    VisualPtr *         visualp,
    DepthPtr *          depthp,
    int *               nvisualp,
    int *               ndepthp,
    int *               rootDepthp,
    VisualID *          defaultVisp,
    unsigned long       sizes,
    int                 bitsPerRGB
);
#endif

#ifdef	STATIC_COLOR

static ColormapPtr InstalledMaps[MAXSCREENS];

int
cfbListInstalledColormaps(pScreen, pmaps)
    ScreenPtr	pScreen;
    Colormap	*pmaps;
{
    /* By the time we are processing requests, we can guarantee that there
     * is always a colormap installed */
    *pmaps = InstalledMaps[pScreen->myNum]->mid;
    return (1);
}


void
cfbInstallColormap(pmap)
    ColormapPtr	pmap;
{
    int index = pmap->pScreen->myNum;
    ColormapPtr oldpmap = InstalledMaps[index];

    if(pmap != oldpmap)
    {
	/* Uninstall pInstalledMap. No hardware changes required, just
	 * notify all interested parties. */
	if(oldpmap != (ColormapPtr)None)
	    WalkTree(pmap->pScreen, TellLostMap, (char *)&oldpmap->mid);
	/* Install pmap */
	InstalledMaps[index] = pmap;
	WalkTree(pmap->pScreen, TellGainedMap, (char *)&pmap->mid);

    }
}

void
cfbUninstallColormap(pmap)
    ColormapPtr	pmap;
{
    int index = pmap->pScreen->myNum;
    ColormapPtr curpmap = InstalledMaps[index];

    if(pmap == curpmap)
    {
	if (pmap->mid != pmap->pScreen->defColormap)
	{
	    curpmap = (ColormapPtr) LookupIDByType(pmap->pScreen->defColormap,
						   RT_COLORMAP);
	    (*pmap->pScreen->InstallColormap)(curpmap);
	}
    }
}

#endif

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
    Bool ret;
    ret = miInitVisuals(visualp, depthp, nvisualp, ndepthp, rootDepthp,
			defaultVisp, sizes, bitsPerRGB, -1);
    if (!ret)
	return FALSE;

#if defined(GLXEXT) && !defined(XFree86LOADER)
    return GlxInitVisuals
	   (
               visualp, 
               depthp,
               nvisualp,
               ndepthp,
               rootDepthp,
               defaultVisp,
               sizes,
               bitsPerRGB
           );
#else
    return TRUE;
#endif
}
