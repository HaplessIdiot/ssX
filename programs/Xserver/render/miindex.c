/*
 * $XFree86: xc/programs/Xserver/render/miindex.c,v 1.6 2002/11/05 05:34:40 keithp Exp $
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

static unsigned short  CubeLevels[] = { 
    0x0000, 0x5555, 0xaaaa, 0xffff
};

#define NUM_CUBE_LEVELS	(sizeof (CubeLevels) / sizeof (CubeLevels[0]))

static unsigned short  GrayLevels[] = { 
    0x0000, 0x1555, 0x2aaa,
    0x4000, 0x5555, 0x6aaa,
    0x8000, 0x9555, 0xaaaa,
    0xbfff, 0xd555, 0xeaaa, 0xffff,
};
		    
#define NUM_GRAY_LEVELS	(sizeof (GrayLevels) / sizeof (GrayLevels[0]))

Bool
miBuildRenderColormap (ColormapPtr  pColormap,
		       Pixel	    *first,
		       Pixel	    *last)
{
    int		r, g, b;
    unsigned short  red, green, blue;
    Pixel	pix;
    int		policy = PictureCmapPolicy;
    int		num = pColormap->pVisual->ColormapEntries;
    int		needed;

    if (policy == PictureCmapPolicyDefault)
    {
	if (num >= 256 && (pColormap->pVisual->class|DynamicClass) == PseudoColor)
	    policy = PictureCmapPolicyColor;
	else if (num >= 64)
	    policy = PictureCmapPolicyGray;
	else
	    policy = PictureCmapPolicyMono;
    }
    /*
     * Make sure enough cells are free for the chosen policy
     */
    for (;;)
    {
	switch (policy) {
	case PictureCmapPolicyColor:
	    needed = 71;
	    break;
	case PictureCmapPolicyGray:
	    needed = 11;
	    break;
	case PictureCmapPolicyMono:
	default:
	    needed = 0;
	    break;
	}
	if (needed <= pColormap->freeRed)
	    break;
	policy--;
    } 
    
    /*
     * Initialize the search bounds for the mapping code
     */
    if (pColormap->pScreen->whitePixel > pColormap->pScreen->blackPixel)
    {
	*first = pColormap->pScreen->blackPixel;
	*last = pColormap->pScreen->whitePixel;
    }
    else
    {
	*first = pColormap->pScreen->whitePixel;
	*last = pColormap->pScreen->blackPixel;
    }
    
    switch (policy) {
    case PictureCmapPolicyColor:
	for (r = 0; r < NUM_CUBE_LEVELS; r++)
	    for (g = 0; g < NUM_CUBE_LEVELS; g++)
		for (b = 0; b < NUM_CUBE_LEVELS; b++)
		{
		    red = CubeLevels[r];
		    green = CubeLevels[g];
		    blue = CubeLevels[b];
		    if (AllocColor (pColormap, &red, &green, 
				    &blue, &pix, 0) != Success)
			return FALSE;
		    if (pix < *first)
			*first = pix;
		    if (pix > *last)
			*last = pix;
		}
	/* fall through ... */
    case PictureCmapPolicyGray:
	for (g = 0; g < NUM_GRAY_LEVELS; g++)
	{
	    red = green = blue = GrayLevels[g];
	    if (AllocColor (pColormap, &red, &green, &blue, &pix, 0) != Success)
		return FALSE;
	    if (pix < *first)
		*first = pix;
	    if (pix > *last)
		*last = pix;
	}
	/* fall through ... */
    case PictureCmapPolicyMono:
	break;
    }

    return TRUE;
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

	r = v & 0xff;
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
    Pixel	    first, last;
    Pixel	    pix[MI_MAX_INDEXED];
    xrgb	    rgb[MI_MAX_INDEXED];
    Pixel	    p, r, g, b;

    if (pFormat->pVisual->ColormapEntries > MI_MAX_INDEXED)
	return FALSE;
    pIndexed = xalloc (sizeof (miIndexedRec));
    if (!pIndexed)
	return FALSE;
    first = 0;
    last = pFormat->pVisual->ColormapEntries - 1;
    if (pFormat->pVisual->class & DynamicClass)
    {
	if (!miBuildRenderColormap (pFormat->pColormap, &first, &last))
	{
	    xfree (pIndexed);
	    return FALSE;
	}
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
	    pIndexed->ent[r] = FindBestGray (pIndexed, first, last-first+1, r);
	break;
    case PseudoColor:
	pIndexed->color = TRUE;
	p = 0;
	for (r = 0; r < 32; r++)
	    for (g = 0; g < 32; g++)
		for (b = 0; b < 32; b++)
		{
		    pIndexed->ent[p] = FindBestColor (pIndexed, first, last-first+1, r, g, b);
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

void
miUpdateIndexed (ScreenPtr	pScreen,
		 PictFormatPtr	pFormat,
		 int		ndef,
		 xColorItem	*pdef)
{
    miIndexedPtr pIndexed = pFormat->indexed;

    if (pIndexed)
    {
	while (ndef--)
	{
	    pIndexed->rgba[pdef->pixel] = (0xff000000 |
					   ((pdef->red   & 0xff00) << 8) |
					   ((pdef->green & 0xff00)     ) |
					   ((pdef->blue  & 0xff00) >> 8));
	    pdef++;
	}
    }
}

#endif /* _MIINDEX_H_ */
