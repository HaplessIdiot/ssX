/*
 * $XFree86: $
 *
 * Copyright © 2001 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _MIINDEX_H_
#define _MIINDEX_H_

#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "mi.h"
#include "picturestr.h"
#include "mipict.h"
#include "colormapst.h"

void
miBuildRenderColormap (ColormapPtr  pColormap,
		       int	    first,
		       int	    num)
{
    int		cube, ramp;
    int		r, g, b;
    xColorItem	defs[MI_MAX_INDEXED];
    int		n;
    
    cube = 0;
    if ((pColormap->pVisual->class | DynamicClass) == PseudoColor)
    {
	for (cube = 0; cube * cube * cube < num; cube++)
	    ;
	cube--;
    }
    if (cube == 1)
	cube = 0;
    n = 0;
    ramp = num - (cube * cube * cube);
    for (r = 0; r < cube; r++)
	for (g = 0; g < cube; g++)
	    for (b = 0; b < cube; b++)
	    {
		defs[n].pixel = first + n;
		defs[n].flags = DoRed|DoGreen|DoBlue;
		defs[n].red = r * 65535 / (cube - 1);
		defs[n].green = g * 65535 / (cube - 1);
		defs[n].blue = b * 65535 / (cube - 1);
		n++;
	    }
    for (g = 0; g < ramp; g++)
    {
	defs[n].pixel = first + n;
	defs[n].flags = DoRed|DoGreen|DoBlue;
	defs[n].red = 
	defs[n].green = 
	defs[n].blue = g * 65535 / (ramp - 1);
	n++;
    }
    StoreColors (pColormap, num, defs);
}

/* 0 <= red, green, blue < 32 */
static Pixel
FindBestColor (miIndexedPtr pIndexed, int first, int num,
	       int red, int green, int blue)
{
    Pixel   best = first;
    int	    bestDist = 1 << 30;
    int	    dist;
    int	    dr, dg, db;
    while (num--)
    {
	CARD32	v = pIndexed->rgba[first];

	dr = ((v >> 19) & 0x1f);
	dg = ((v >> 11) & 0x1f);
	db = ((v >> 3) & 0x1f);
	dr = dr - red;
	dg = dg - green;
	db = db - blue;
	dist = dr * dr + dg * dg + db * db;
	if (dist < bestDist)
	{
	    bestDist = dist;
	    best = first;
	}
	first++;
    }
    return best;
}

/* 0 <= gray < 32768 */
static Pixel
FindBestGray (miIndexedPtr pIndexed, int first, int num, int gray)
{
    Pixel   best = first;
    int	    bestDist = 1 << 30;
    int	    dist;
    int	    dr;
    int	    r;
    
    while (num--)
    {
	CARD32	v = pIndexed->rgba[first];

	r = v >> 16;
	r = r | (r << 8);
	dr = gray - (r >> 1);
	dist = dr * dr;
	if (dist < bestDist)
	{
	    bestDist = dist;
	    best = first;
	}
	first++;
    }
    return best;
}

Bool
miInitIndexed (ScreenPtr	pScreen,
	       PictFormatPtr	pFormat)
{
    miIndexedPtr    pIndexed;
    int		    first, num;
    Pixel	    pix[MI_MAX_INDEXED];
    xrgb	    rgb[MI_MAX_INDEXED];
    Pixel	    p, r, g, b;

    if (pFormat->pVisual->ColormapEntries > MI_MAX_INDEXED)
	return FALSE;
    pIndexed = xalloc (sizeof (miIndexedRec));
    if (!pIndexed)
	return FALSE;
    first = 0;
    num = pFormat->pVisual->ColormapEntries;
    if (pFormat->pVisual->class & DynamicClass)
    {
	if (pFormat->pVisual->vid == pScreen->rootVisual)
	{
	    num = num * 2 / 3;
	    pix[0] = num;
	    if (AllocColorCells (0, pFormat->pColormap, num, 0, TRUE,
				 pix, 0) != Success)
	    {
		xfree (pIndexed);
		return FALSE;
	    }
	    first = pix[0];
	}
	miBuildRenderColormap (pFormat->pColormap, first, num);
    }
    /*
     * Build mapping from pixel value to ARGB
     */
    for (p = 0; p < pFormat->pVisual->ColormapEntries; p++)
	pix[p] = p;
    QueryColors (pFormat->pColormap, pFormat->pVisual->ColormapEntries,
		 pix, rgb);
    for (p = 0; p < pFormat->pVisual->ColormapEntries; p++)
	pIndexed->rgba[p] = (0xff000000 |
			     ((rgb[p].red   & 0xff00) << 8) |
			     ((rgb[p].green & 0xff00)     ) |
			     ((rgb[p].blue  & 0xff00) >> 8));
    /*
     * Build mapping from RGB to pixel value.  This could probably be
     * done a bit quicker...
     */
    switch (pFormat->pVisual->class | DynamicClass) {
    case GrayScale:
	pIndexed->color = FALSE;
	for (r = 0; r < 32768; r++)
	    pIndexed->ent[r] = FindBestGray (pIndexed, first, num, r);
	break;
    case PseudoColor:
	pIndexed->color = TRUE;
	p = 0;
	for (r = 0; r < 32; r++)
	    for (g = 0; g < 32; g++)
		for (b = 0; b < 32; b++)
		{
		    pIndexed->ent[p] = FindBestColor (pIndexed, first, num, r, g, b);
		    p++;
		}
	break;
    }
    pFormat->indexed = pIndexed;
    return TRUE;
}

void
miCloseIndexed (ScreenPtr	pScreen,
		PictFormatPtr	pFormat)
{
    if (pFormat->indexed)
    {
	xfree (pFormat->indexed);
	pFormat->indexed = 0;
    }
}

#endif /* _MIINDEX_H_ */
