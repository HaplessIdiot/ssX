/*
 * $Id: fbbits.h,v 1.1 1999/11/19 13:53:41 hohndel Exp $
 *
 * Copyright © 1998 Keith Packard
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
/* $XFree86: $ */

/*
 * This file defines functions for drawing some primitives using
 * underlying datatypes instead of masks
 */

/*
 * Define the following before including this file:
 *
 *  BRESSOLID	name of function for drawing a solid segment
 *  BRESDASH	name of function for drawing a dashed segment
 *  DOTS	name of function for drawing dots
 *  ARC		name of function for drawing a solid arc
 *  BITS	type of underlying unit
 */

void
BRESSOLID (DrawablePtr	pDrawable,
	   GCPtr	pGC,
	   int		dashOffset,
	   int		signdx,
	   int		signdy,
	   int		axis,
	   int		x1,
	   int		y1,
	   int		e,
	   int		e1,
	   int		e3,
	   int		len)
{
    FbBits	*dst;
    FbStride	dstStride;
    int		dstBpp;
    FbGCPrivPtr	pPriv = fbGetGCPrivate (pGC);
    BITS	*bits;
    FbStride	bitsStride;
    FbStride	majorStep, minorStep;
    BITS	xor = (BITS) pPriv->xor;
    
    fbGetDrawable (pDrawable, dst, dstStride, dstBpp);
    bits = ((BITS *) (dst + (y1 * dstStride))) + x1;
    bitsStride = dstStride * (sizeof (FbBits) / sizeof (BITS));
    if (signdy < 0)
	bitsStride = -bitsStride;
    if (axis == X_AXIS)
    {
	majorStep = signdx;
	minorStep = bitsStride;
    }
    else
    {
	majorStep = bitsStride;
	minorStep = signdx;
    }
    while (len--)
    {
	*bits = xor;
	bits += majorStep;
	e += e1;
	if (e >= 0)
	{
	    bits += minorStep;
	    e += e3;
	}
    }
}

void
BRESDASH (DrawablePtr	pDrawable,
	  GCPtr		pGC,
	  int		dashOffset,
	  int		signdx,
	  int		signdy,
	  int		axis,
	  int		x1,
	  int		y1,
	  int		e,
	  int		e1,
	  int		e3,
	  int		len)
{
    FbBits	*dst;
    FbStride	dstStride;
    int		dstBpp;
    FbGCPrivPtr	pPriv = fbGetGCPrivate (pGC);
    BITS	*bits;
    FbStride	bitsStride;
    FbStride	majorStep, minorStep;
    BITS	xor = (BITS) pPriv->xor;
    BITS	bgxor = (BITS) pPriv->bgxor;
    unsigned char   *dash, *lastDash;
    int		dashlen;
    Bool	even;
    Bool	doOdd;
    
    fbGetDrawable (pDrawable, dst, dstStride, dstBpp);
    doOdd = pGC->lineStyle == LineDoubleDash;
    even = TRUE;
    dash = pGC->dash;
    lastDash = dash + pGC->numInDashList;
    dashOffset %= pPriv->dashLength;
    while (dashOffset >= *dash)
    {
	dashOffset -= *dash++;
	if (dash == lastDash)
	    dash = pGC->dash;
	even = !even;
    }
    dashlen = *dash - dashOffset;
    bits = ((BITS *) (dst + (y1 * dstStride))) + x1;
    bitsStride = dstStride * (sizeof (FbBits) / sizeof (BITS));
    if (signdy < 0)
	bitsStride = -bitsStride;
    if (axis == X_AXIS)
    {
	majorStep = signdx;
	minorStep = bitsStride;
    }
    else
    {
	majorStep = bitsStride;
	minorStep = signdx;
    }
    while (len--)
    {
	if (even)
	    *bits = xor;
	else if (doOdd)
	    *bits = bgxor;
	bits += majorStep;
	e += e1;
	if (e >= 0)
	{
	    bits += minorStep;
	    e += e3;
	}
	if (!--dashlen)
	{
	    if (++dash == lastDash)
		dash = pGC->dash;
	    dashlen = *dash;
	    even = !even;
	}
    }
}

void
DOTS (FbBits	    *dst,
      FbStride	    dstStride,
      int	    dstBpp,
      BoxPtr	    pBox,
      xPoint	    *pts,
      int	    npt,
      FbBits	    and,
      FbBits	    xor)
{
    BITS	*bits = (BITS *) dst;
    BITS	fg = (BITS) xor;
    FbStride	bitsStride = dstStride * (sizeof (FbBits) / sizeof (BITS));
    int		x1, y1, x2, y2;
    int		x, y;

    x1 = pBox->x1;
    y1 = pBox->y1;
    x2 = pBox->x2;
    y2 = pBox->y2;
    while (npt--)
    {
	x = pts->x;
	y = pts->y;
	pts++;
	if (x1 <= x && x < x2 && y1 <= y && y < y2)
	    *(bits + y * bitsStride + x) = fg;
    }
}

void
ARC (FbBits	*dst,
     FbStride	dstStride,
     int	dstBpp,
     xArc	*arc,
     int	drawX,
     int	drawY,
     FbBits	and,
     FbBits	xor)
{
    BITS	    *bits;
    FbStride	    bitsStride;
    miZeroArcRec    info;
    Bool	    do360;
    int		    x;
    BITS	    *addrp;
    BITS	    *yorgp, *yorgop;
    BITS	    andBits, xorBits;
    int		    yoffset, dyoffset;
    int		    y, a, b, d, mask;
    int		    k1, k3, dx, dy;

    bits = (BITS *) dst;
    bitsStride = dstStride * (sizeof (FbBits) / sizeof (BITS));
    andBits = (BITS) and;
    xorBits = (BITS) xor;
    do360 = miZeroArcSetup(arc, &info, TRUE);
    yorgp = bits + ((info.yorg + drawY) * bitsStride);
    yorgop = bits + ((info.yorgo + drawY) * bitsStride);
    info.xorg += drawX;
    info.xorgo += drawX;
    MIARCSETUP();
    yoffset = y ? bitsStride : 0;
    dyoffset = 0;
    mask = info.initialMask;
    
#define COPY(d) *(d) = xorBits;
#define RROP(d)	*(d) = FbDoRRop (*(d), andBits, xorBits)
    if (!(arc->width & 1))
    {
	if (mask & 2)
	    RROP(yorgp + info.xorgo);
	if (mask & 8)
	    RROP(yorgop + info.xorgo);
    }
    if (!info.end.x || !info.end.y)
    {
	mask = info.end.mask;
	info.end = info.altend;
    }
    if (do360 && (arc->width == arc->height) && !(arc->width & 1))
    {
	int xoffset = bitsStride;
	BITS *yorghb = yorgp + (info.h * bitsStride) + info.xorg;
	BITS *yorgohb = yorghb - info.h;

	yorgp += info.xorg;
	yorgop += info.xorg;
	yorghb += info.h;
	while (1)
	{
	    if (andBits == 0)
	    {
		COPY(yorgp + yoffset + x);
		COPY(yorgp + yoffset - x);
		COPY(yorgop - yoffset - x);
		COPY(yorgop - yoffset + x);
	    }
	    else
	    {
		RROP(yorgp + yoffset + x);
		RROP(yorgp + yoffset - x);
		RROP(yorgop - yoffset - x);
		RROP(yorgop - yoffset + x);
	    }
	    if (a < 0)
		break;
	    if (andBits == 0)
	    {
		COPY(yorghb - xoffset - y);
		COPY(yorgohb - xoffset + y);
		COPY(yorgohb + xoffset + y);
		COPY(yorghb + xoffset - y);
	    }
	    else
	    {
		RROP(yorghb - xoffset - y);
		RROP(yorgohb - xoffset + y);
		RROP(yorgohb + xoffset + y);
		RROP(yorghb + xoffset - y);
	    }
	    xoffset += bitsStride;
	    MIARCCIRCLESTEP(yoffset += bitsStride;);
	}
	yorgp -= info.xorg;
	yorgop -= info.xorg;
	x = info.w;
	yoffset = info.h * bitsStride;
    }
    else if (do360)
    {
	while (y < info.h || x < info.w)
	{
	    MIARCOCTANTSHIFT(dyoffset = bitsStride;);
	    if (andBits == 0)
	    {
		COPY(yorgp + yoffset + info.xorg + x);
		COPY(yorgp + yoffset + info.xorgo - x);
		COPY(yorgop - yoffset + info.xorgo - x);
		COPY(yorgop - yoffset + info.xorg + x);
	    }
	    else
	    {
		RROP(yorgp + yoffset + info.xorg + x);
		RROP(yorgp + yoffset + info.xorgo - x);
		RROP(yorgop - yoffset + info.xorgo - x);
		RROP(yorgop - yoffset + info.xorg + x);
	    }
	    MIARCSTEP(yoffset += dyoffset;, yoffset += bitsStride;);
	}
    }
    else
    {
	while (y < info.h || x < info.w)
	{
	    MIARCOCTANTSHIFT(dyoffset = bitsStride;);
	    if ((x == info.start.x) || (y == info.start.y))
	    {
		mask = info.start.mask;
		info.start = info.altstart;
	    }
	    if (andBits == 0)
	    {
		if (mask & 1)
		    COPY(yorgp + yoffset + info.xorg + x);
		if (mask & 2)
		    COPY(yorgp + yoffset + info.xorgo - x);
		if (mask & 4)
		    COPY(yorgop - yoffset + info.xorgo - x);
		if (mask & 8)
		    COPY(yorgop - yoffset + info.xorg + x);
	    }
	    else
	    {
		if (mask & 1)
		    RROP(yorgp + yoffset + info.xorg + x);
		if (mask & 2)
		    RROP(yorgp + yoffset + info.xorgo - x);
		if (mask & 4)
		    RROP(yorgop - yoffset + info.xorgo - x);
		if (mask & 8)
		    RROP(yorgop - yoffset + info.xorg + x);
	    }
	    if ((x == info.end.x) || (y == info.end.y))
	    {
		mask = info.end.mask;
		info.end = info.altend;
	    }
	    MIARCSTEP(yoffset += dyoffset;, yoffset += bitsStride;);
	}
    }
    if ((x == info.start.x) || (y == info.start.y))
	mask = info.start.mask;
    if (mask & 1)
	RROP(yorgp + yoffset + info.xorg + x);
    if (mask & 4)
	RROP(yorgop - yoffset + info.xorgo - x);
    if (arc->height & 1)
    {
	if (mask & 2)
	    RROP(yorgp + yoffset + info.xorgo - x);
	if (mask & 8)
	    RROP(yorgop - yoffset + info.xorg + x);
    }
}

#define WRITE1(d,n,fg)	    d[n] = (BITS) fg

#ifdef BITS2
# define WRITE2(d,n,fg)	    *((BITS2 *) &(d[n])) = (BITS2) fg
#else
# define WRITE2(d,n,fg)	    WRITE1(d,n+1,WRITE1(d,n,fg))
#endif

#ifdef BITS4
# define WRITE4(d,n,fg)	    *((BITS4 *) &(d[n])) = (BITS4) fg
#else
# define WRITE4(d,n,fg)	    WRITE2(d,n+2,WRITE2(d,n,fg))
#endif

void
GLYPH (FbBits	*dstBits,
       FbStride	dstStride,
       int	dstBpp,
       FbStip	*stipple,
       FbBits	fg,
       int	x,
       int	height)
{
    int	    lshift;
    FbStip  bits;
    BITS    *dstLine;
    BITS    *dst;
    int	    n;
    int	    shift;

    dstLine = (BITS *) dstBits;
    dstLine += x & ~3;
    dstStride *= (sizeof (FbBits) / sizeof (BITS));
    shift = x & 3;
    lshift = 4 - shift;
    while (height--)
    {
	bits = *stipple++;
	dst = (BITS *) dstLine;
	n = lshift;
	while (bits)
	{
	    switch (FbStipRight (FbLeftStipBits (bits, n), 4-n)) {
	    case 0:
		break;
	    case 1:
		WRITE1(dst,0,fg);
		break;
	    case 2:
		WRITE1(dst,1,fg);
		break;
	    case 3:
		WRITE2(dst,0,fg);
		break;
	    case 4:
		WRITE1(dst,2,fg);
		break;
	    case 5:
		WRITE1(dst,0,fg);
		WRITE1(dst,2,fg);
		break;
	    case 6:
		WRITE1(dst,1,fg);
		WRITE1(dst,2,fg);
		break;
	    case 7:
		WRITE2(dst,0,fg);
		WRITE1(dst,2,fg);
		break;
	    case 8:
		WRITE1(dst,3,fg);
		break;
	    case 9:
		WRITE1(dst,0,fg);
		WRITE1(dst,3,fg);
		break;
	    case 10:
		WRITE1(dst,1,fg);
		WRITE1(dst,3,fg);
		break;
	    case 11:
		WRITE2(dst,0,fg);
		WRITE1(dst,3,fg);
		break;
	    case 12:
		WRITE2(dst,2,fg);
		break;
	    case 13:
		WRITE1(dst,0,fg);
		WRITE2(dst,2,fg);
		break;
	    case 14:
		WRITE1(dst,1,fg);
		WRITE2(dst,2,fg);
		break;
	    case 15:
		WRITE4(dst,0,fg);
		break;
	    }
	    bits = FbStipLeft (bits, n);
	    n = 4;
	    dst += 4;
	}
	dstLine += dstStride;
    }
}

#undef WRITE1
#undef WRITE2
#undef WRITE4
