/* $XConsortium: ppcRslvC.c,v 1.2 94/04/17 20:31:56 dpw Exp $ */
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

/*
 * Copyright IBM Corporation 1987,1988,1989
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that 
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/

/* Header: /andrew/X11/R3src/tape/server/ddx/ibm/ppc/RCS/ppcRslvC.c,v 9.1 88/10/24 04:01:18 paul Exp */
/* Source: /andrew/X11/R3src/tape/server/ddx/ibm/ppc/RCS/ppcRslvC.c,v */

/* Generic Color Resolution Scheme
 * P. Shupak 12/31/87
 */

#include "X.h"
#include "screenint.h"
#include "scrnintstr.h"
#include "colormapst.h"	/* GJA */
#include "OScompiler.h"


void
ppcResolveColor( pRed, pGreen, pBlue, pVisual )
register unsigned short * const pRed ;
register unsigned short * const pGreen ;
register unsigned short * const pBlue ;
register VisualPtr const pVisual ;
{ 
    /* GJA -- took this from cfb */
    int shift = 16 - pVisual->bitsPerRGBValue;
    unsigned lim = (1 << pVisual->bitsPerRGBValue) - 1;

    if ((pVisual->class == PseudoColor) || (pVisual->class == DirectColor))
    {
	/* rescale to rgb bits */
	*pRed = ((*pRed >> shift) * 65535) / lim;
	*pGreen = ((*pGreen >> shift) * 65535) / lim;
	*pBlue = ((*pBlue >> shift) * 65535) / lim;
    }
    else if (pVisual->class == GrayScale)
    {
	/* rescale to gray then rgb bits */
	*pRed = (30L * *pRed + 59L * *pGreen + 11L * *pBlue) / 100;
	*pBlue = *pGreen = *pRed = ((*pRed >> shift) * 65535) / lim;
    }
    else if (pVisual->class == StaticGray)
    {
	unsigned limg = pVisual->ColormapEntries - 1;
	/* rescale to gray then [0..limg] then [0..65535] then rgb bits */
	*pRed = (30L * *pRed + 59L * *pGreen + 11L * *pBlue) / 100;
	*pRed = ((((*pRed * (limg + 1))) >> 16) * 65535) / limg;
	*pBlue = *pGreen = *pRed = ((*pRed >> shift) * 65535) / lim;
    }
    else {
	Error( "can't resolve color for this visual" ) ;
    }
}
	

/* GJA --
 * Took the code below from cfb. It's for staticgray visuals.
 */
Bool
ppcInitializeColormap(pmap)
    register ColormapPtr	pmap;
{
    register unsigned i;
    register VisualPtr pVisual;
    unsigned lim, maxent, shift;

    pVisual = pmap->pVisual;
    lim = (1 << pVisual->bitsPerRGBValue) - 1;
    shift = 16 - pVisual->bitsPerRGBValue;
    maxent = pVisual->ColormapEntries - 1;

    if (pVisual->class == StaticGray)
    {
	for(i = 0; i <= maxent; i++)
	{
	    /* rescale to [0..65535] then rgb bits */
	    pmap->red[i].co.local.red = ((((i * 65535) / maxent) >> shift)
					 * 65535) / lim;
	    pmap->red[i].co.local.green = pmap->red[i].co.local.red;
	    pmap->red[i].co.local.blue = pmap->red[i].co.local.red;
	}
    }
    return TRUE;
}


Bool
vga16CreateDefColormap(pScreen)
    ScreenPtr pScreen;
{
    unsigned short	zero = 0, ones = 0xFFFF;
    VisualPtr	pVisual;
    ColormapPtr	cmap;
    
    for (pVisual = pScreen->visuals;
	 pVisual->vid != pScreen->rootVisual;
	 pVisual++)
	;

    if (CreateColormap(pScreen->defColormap, pScreen, pVisual, &cmap,
		       (pVisual->class & DynamicClass) ? AllocNone : AllocAll,
		       0)
	!= Success)
	return FALSE;
    if ((AllocColor(cmap, &ones, &ones, &ones, &(pScreen->whitePixel), 0) !=
       	   Success) ||
    	(AllocColor(cmap, &zero, &zero, &zero, &(pScreen->blackPixel), 0) !=
       	   Success))
    	return FALSE;
    (*pScreen->InstallColormap)(cmap);
    return TRUE;
}
