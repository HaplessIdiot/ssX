/*
 * $Id: fbbltone.c,v 1.2 2000/01/21 01:11:57 dawes Exp $
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
/* $XFree86: xc/programs/Xserver/fb/fbbltone.c,v 1.1 1999/11/19 13:53:42 hohndel Exp $ */

#include "fb.h"

/*
 *  Example: srcX = 13 dstX = 8	(FB unit 32 dstBpp 8)
 *
 *	**** **** **** **** **** **** **** ****
 *			^
 *	********  ********  ********  ********
 *		  ^
 *  leftShift = 12
 *  rightShift = 20
 *
 *  Example: srcX = 0 dstX = 8 (FB unit 32 dstBpp 8)
 *
 *	**** **** **** **** **** **** **** ****
 *	^		
 *	********  ********  ********  ********
 *		  ^
 *
 *  leftShift = 24
 *  rightShift = 8
 */

#define LoadBits {\
    if (leftShift) { \
	bitsRight = *src++; \
	bits = (FbStipLeft (bitsLeft, leftShift) | \
		FbStipRight(bitsRight, rightShift)); \
	bitsLeft = bitsRight; \
    } else \
	bits = *src++; \
}
    
void
fbBltOne (FbStip    *src,
	  FbStride  srcStride,	    /* FbStip units per scanline */
	  int	    srcX,	    /* bit position of source */
	  FbBits    *dst,
	  FbStride  dstStride,	    /* FbBits units per scanline */
	  int	    dstX,	    /* bit position of dest */
	  int	    dstBpp,	    /* bits per destination unit */

	  int	    width,	    /* width in bits of destination */
	  int	    height,	    /* height in scanlines */

	  FbBits    fgand,	    /* rrop values */
	  FbBits    fgxor,
	  FbBits    bgand,
	  FbBits    bgxor)
{
    const FbBits    *fbBits;
    int		    pixelsPerDst;		/* dst pixels per FbBits */
    int		    unitsPerSrc;		/* src patterns per FbStip */
    int		    leftShift, rightShift;	/* align source with dest */
    FbBits	    startmask, endmask;		/* dest scanline masks */
    FbStip	    bits, bitsLeft, bitsRight;	/* source bits */
    FbStip	    left;
    FbBits	    mask;
    int		    nDst;			/* dest longwords (w.o. end) */
    int		    w;
    int		    n, nmiddle;
    int		    nthistime;
    int		    dstS;			/* stipple-relative dst X coordinate */
    Bool	    copy;			/* accelerate dest-invariant */
    Bool	    transparent;		/* accelerate 0 nop */
    int		    srcinc;			/* source units consumed */
    int		    dstinc;			/* dest units produced */
    Bool	    endNeedsLoad;		/* need load for endmask */

#ifdef FB_24BIT
    if (dstBpp == 24)
    {
	fbBltOne24 (src, srcStride, srcX,
		    dst, dstStride, dstX, dstBpp,
		    width, height,
		    fgand, fgxor, bgand, bgxor);
	return;
    }
#endif
    
    /*
     * Number of destination units in FbBits == number of stipple pixels
     * used each time
     */
    pixelsPerDst = FB_UNIT / dstBpp;

    /*
     * Number of source stipple patterns in FbStip 
     */
    unitsPerSrc = FB_STIP_UNIT / pixelsPerDst;
    
    copy = FALSE;
    transparent = FALSE;
    if (bgand == 0 && fgand == 0)
	copy = TRUE;
    else if (bgand == FB_ALLONES && bgxor == 0)
	transparent = TRUE;

    /*
     * Adjust source and dest to nearest FbBits boundary
     */
    src += srcX >> FB_STIP_SHIFT;
    dst += dstX >> FB_SHIFT;
    srcX &= FB_STIP_MASK;
    dstX &= FB_MASK;

    FbMaskBits(dstX, width, startmask, nmiddle, endmask);

    /*
     * Compute effective dest alignment requirement for
     * source -- must align source to dest unit boundary
     */
    dstS = dstX / dstBpp;
    /*
     * Compute shift constants for effective alignement
     */
    if (srcX >= dstS)
    {
	leftShift = srcX - dstS;
	rightShift = FB_STIP_UNIT - leftShift;
    }
    else
    {
	rightShift = dstS - srcX;
	leftShift = FB_STIP_UNIT - rightShift;
    }
    /*
     * Get pointer to stipple mask array for this depth
     */
    fbBits = 0;	/* unused */
    if (pixelsPerDst <= 8)
	fbBits = fbStippleTable[pixelsPerDst];
    
    /*
     * Compute total number of destination words written, but 
     * don't count endmask 
     */
    nDst = nmiddle;
    if (startmask)
	nDst++;
    
    dstStride -= nDst;

    /*
     * Compute total number of source words consumed
     */
    
    srcinc = (nDst + unitsPerSrc - 1) / unitsPerSrc;
    
    if (srcX > dstS)
	srcinc++;
    if (endmask)
    {
	endNeedsLoad = nDst % unitsPerSrc == 0;
	if (endNeedsLoad)
	    srcinc++;
    }

    srcStride -= srcinc;
    
    /*
     * Copy rectangle
     */
    while (height--)
    {
	w = nDst;	    /* total units across scanline */
	n = unitsPerSrc;    /* units avail in single stipple */
	if (n > w)
	    n = w;
	
	bitsLeft = 0;
	if (srcX > dstS)
	    bitsLeft = *src++;
	if (n)
	{
	    /*
	     * Load first set of stipple bits
	     */
	    LoadBits;

	    /*
	     * Consume stipple bits for startmask
	     */
	    if (startmask)
	    {
#if FB_UNIT > 32
		if (pixelsPerDst == 16)
		    mask = FbStipple16Bits(FbLeftStipBits(bits,16));
		else
#endif
		    mask = fbBits[FbLeftStipBits(bits,pixelsPerDst)];
		if (!transparent || (mask & startmask))
		{
		    *dst = FbStippleRRopMask (*dst, mask, 
					      fgand, fgxor, bgand, bgxor, 
					      startmask);
		}
		bits = FbStipLeft (bits, pixelsPerDst);
		dst++;
		n--;
		w--;
	    }
	    /*
	     * Consume stipple bits across scanline
	     */
	    for (;;)
	    {
		w -= n;
#if FB_UNIT > 32
		if (pixelsPerDst == 16)
		{
		    if (copy)
		    {
			while (n--)
			{
			    mask = FbStipple16Bits(FbLeftStipBits(bits,16));
			    *dst = FbOpaqueStipple (mask, fgxor, bgxor);
			    dst++;
			    bits = FbStipLeft(bits, 16);
			}
		    }
		    else
		    {
			while (n--)
			{
			    left = FbLeftStipBits(bits,16);
			    if (left || !transparent)
			    {
				mask = FbStipple16Bits(left);
				*dst = FbStippleRRop (*dst, mask,
						      fgand, fgxor, bgand, bgxor);
			    }
			    dst++;
			    bits = FbStipLeft(bits, 16);
			}
		    }
		}
		else
#endif
		{
		    if (copy)
		    {
			while (n--)
			{
			    mask = fbBits[FbLeftStipBits(bits,pixelsPerDst)];
			    *dst = FbOpaqueStipple (mask, fgxor, bgxor);
			    dst++;
			    bits = FbStipLeft(bits, pixelsPerDst);
			}
		    }
		    else
		    {
			while (n--)
			{
			    left = FbLeftStipBits(bits,pixelsPerDst);
			    if (left || !transparent)
			    {
				mask = fbBits[left];
				*dst = FbStippleRRop (*dst, mask,
						      fgand, fgxor, bgand, bgxor);
			    }
			    dst++;
			    bits = FbStipLeft(bits, pixelsPerDst);
			}
		    }
		}
		if (!w)
		    break;
		/*
		 * Load another set and reset number of available units
		 */
		LoadBits;
		n = unitsPerSrc;
		if (n > w)
		    n = w;
	    }
	}
	/*
	 * Consume stipple bits for endmask
	 */
	if (endmask)
	{
	    if (endNeedsLoad)
	    {
		LoadBits;
	    }
#if FB_UNIT > 32
	    if (pixelsPerDst == 16)
		mask = FbStipple16Bits(FbLeftStipBits(bits,16));
	    else
#endif
		mask = fbBits[FbLeftStipBits(bits,pixelsPerDst)];
	    if (!transparent || (mask & endmask))
	    {
		*dst = FbStippleRRopMask (*dst, mask, 
					  fgand, fgxor, bgand, bgxor, 
					  endmask);
	    }
	    dst;
	}
	dst += dstStride;
	src += srcStride;
    }
}

#ifdef FB_24BIT

/*
 * Crufty macros to initialize the mask array, most of this
 * is to avoid compile-time warnings about shift overflow
 */

#define Mask24Pos(x,r) ((x)*24-((r) ? 24 - (r) : 0))
#define Mask24Neg(x,r)	(Mask24Pos(x,r) < 0 ? -Mask24Pos(x,r) : 0)
#define Mask24Check(x,r)    (Mask24Pos(x,r) < 0 ? 0 : \
			     Mask24Pos(x,r) >= FB_UNIT ? 0 : Mask24Pos(x,r))
#define Mask24(x,r) (Mask24Pos(x,r) < FB_UNIT ? \
		     (Mask24Pos(x,r) < 0 ? \
		      FbScrLeft (FbBitsMask(0,24),Mask24Neg(x,r)) : \
		      FbScrRight (FbBitsMask(0,24),Mask24Check(x,r))) \
		     : 0)

#define SelMask24(b,n,r)	((((b) >> n) & 1) * Mask24(n,r))

/*
 * Untested for MSBFirst or FB_UNIT == 32
 */

#if FB_UNIT == 64
#define C4_24(b,r) \
    (SelMask24(b,0,r) | \
     SelMask24(b,1,r) | \
     SelMask24(b,2,r) | \
     SelMask24(b,3,r))

#define FbStip24New(rot)    (2 + (rot != 0))
#define FbStip24Len	    4

const FbBits	fbStipple24Bits[3][1 << FbStip24Len] = {
    /* rotate 0 */
    {
	C4_24( 0, 0), C4_24( 1, 0), C4_24( 2, 0), C4_24( 3, 0),
	C4_24( 4, 0), C4_24( 5, 0), C4_24( 6, 0), C4_24( 7, 0),
	C4_24( 8, 0), C4_24( 9, 0), C4_24(10, 0), C4_24(11, 0),
	C4_24(12, 0), C4_24(13, 0), C4_24(14, 0), C4_24(15, 0),
    },
    /* rotate 8 */
    {
	C4_24( 0, 8), C4_24( 1, 8), C4_24( 2, 8), C4_24( 3, 8),
	C4_24( 4, 8), C4_24( 5, 8), C4_24( 6, 8), C4_24( 7, 8),
	C4_24( 8, 8), C4_24( 9, 8), C4_24(10, 8), C4_24(11, 8),
	C4_24(12, 8), C4_24(13, 8), C4_24(14, 8), C4_24(15, 8),
    },
    /* rotate 16 */
    {
	C4_24( 0,16), C4_24( 1,16), C4_24( 2,16), C4_24( 3,16),
	C4_24( 4,16), C4_24( 5,16), C4_24( 6,16), C4_24( 7,16),
	C4_24( 8,16), C4_24( 9,16), C4_24(10,16), C4_24(11,16),
	C4_24(12,16), C4_24(13,16), C4_24(14,16), C4_24(15,16),
    }
};

#endif

#if FB_UNIT == 32
#define C2_24(b,r)  \
    (SelMask24(b,0,r) | \
     SelMask24(b,1,r))

#define FbStip24Len	    2
#define FbStip24New(rot)    (1 + (rot == 8))

const FbBits	fbStipple24Bits[3][1 << FbStip24Len] = {
    /* rotate 0 */
    {
	C2_24( 0, 0), C2_24 ( 1, 0), C2_24 ( 2, 0), C2_24 ( 3, 0),
    },
    /* rotate 8 */
    {
	C2_24( 0, 8), C2_24 ( 1, 8), C2_24 ( 2, 8), C2_24 ( 3, 8),
    },
    /* rotate 16 */
    {
	C2_24( 0,16), C2_24 ( 1,16), C2_24 ( 2,16), C2_24 ( 3,16),
    }
};
#endif

#define fbFirstStipBits(len,stip) {\
    int	__len = (len); \
    if (len <= remain) { \
	stip = FbLeftStipBits(bits, len); \
    } else { \
	stip = FbLeftStipBits(bits, remain); \
	bits = *src++; \
	__len = (len) - remain; \
	stip |= FbStipRight(FbLeftStipBits(bits, __len), remain); \
	remain = FB_STIP_UNIT; \
    } \
    bits = FbStipLeft (bits, __len); \
    remain -= __len; \
}

#define fbInitStipBits(offset,len,stip) {\
    bits = FbStipLeft (*src++,offset); \
    remain = FB_STIP_UNIT - offset; \
    fbFirstStipBits(len,stip); \
    stip = FbStipRight (stip, FbStip24Len - len); \
}
    
#define fbNextStipBits(rot,stip) {\
    int	    __new = FbStip24New(rot); \
    FbStip  __right; \
    fbFirstStipBits(__new, __right); \
    stip = (FbStipLeft (stip, __new) | \
	    FbStipRight (__right, FbStip24Len - __new)); \
    rot = FbNext24Rot (rot); \
}

/*
 * Use deep mask tables that incorporate rotation, pull
 * a variable number of bits out of the stipple and
 * reuse the right bits as needed for the next write
 *
 * Yes, this is probably too much code, but most 24-bpp screens
 * have no acceleration so this code is used for stipples, copyplane
 * and text
 */
void
fbBltOne24 (FbStip	*srcLine,
	    FbStride	srcStride,  /* FbStip units per scanline */
	    int		srcX,	    /* bit position of source */
	    FbBits	*dst,
	    FbStride	dstStride,  /* FbBits units per scanline */
	    int		dstX,	    /* bit position of dest */
	    int		dstBpp,	    /* bits per destination unit */

	    int		width,	    /* width in bits of destination */
	    int		height,	    /* height in scanlines */

	    FbBits	fgand,	    /* rrop values */
	    FbBits	fgxor,
	    FbBits	bgand,
	    FbBits	bgxor)
{
    FbStip	*src;
    FbBits	leftMask, rightMask, mask;
    int		nlMiddle, nl;
    FbStip	stip, bits, right;
    int		remain;
    int		dstS;
    int		offset, firstlen;
    int		rot0, rot;
    int		nDst;
    
    srcLine += srcX >> FB_STIP_SHIFT;
    dst += dstX >> FB_SHIFT;
    srcX &= FB_STIP_MASK;
    dstX &= FB_MASK;
    rot0 = dstX % 24;
    
    FbMaskBits (dstX, width, leftMask, nlMiddle, rightMask);
    
    dstS = (dstX + 23) / 24;
    firstlen = FbStip24Len - dstS;
    
    nDst = nlMiddle;
    if (leftMask)
	nDst++;
    dstStride -= nDst;
    
    /* opaque copy */
    if (bgand == 0 && fgand == 0)
    {
	while (height--)
	{
	    rot = rot0;
	    src = srcLine;
	    srcLine += srcStride;
	    fbInitStipBits (srcX,firstlen, stip);
	    if (leftMask)
	    {
		mask = fbStipple24Bits[rot >> 3][stip];
		*dst = *dst & ~leftMask | FbOpaqueStipple (mask,
							   FbRot24(fgxor, rot),
							   FbRot24(bgxor, rot)) & leftMask;
		dst++;
		fbNextStipBits(rot,stip);
	    }
	    nl = nlMiddle;
	    while (nl--)
	    {
		mask = fbStipple24Bits[rot>>3][stip];
		*dst = FbOpaqueStipple (mask, 
					FbRot24(fgxor, rot),
					FbRot24(bgxor, rot));
		dst++;
		fbNextStipBits(rot,stip);
	    }
	    if (rightMask)
	    {
		mask = fbStipple24Bits[rot >> 3][stip];
		*dst = *dst & ~rightMask | FbOpaqueStipple (mask,
							    FbRot24(fgxor, rot),
							    FbRot24(bgxor, rot)) & rightMask;
	    }
	    dst += dstStride;
	    src += srcStride;
	}
    }
    /* transparent copy */
    else if (bgand == FB_ALLONES && bgxor == 0 && fgand == 0)
    {
	while (height--)
	{
	    rot = rot0;
	    src = srcLine;
	    srcLine += srcStride;
	    fbInitStipBits (srcX, firstlen, stip);
	    if (leftMask)
	    {
		if (stip)
		{
		    mask = fbStipple24Bits[rot >> 3][stip] & leftMask;
		    *dst = *dst & ~mask | FbRot24(fgxor, rot) & mask;
		}
		dst++;
		fbNextStipBits (rot, stip);
	    }
	    nl = nlMiddle;
	    while (nl--)
	    {
		if (stip)
		{
		    mask = fbStipple24Bits[rot>>3][stip];
		    *dst = *dst & ~mask | FbRot24(fgxor,rot) & mask;
		}
		dst++;
		fbNextStipBits (rot, stip);
	    }
	    if (rightMask)
	    {
		if (stip)
		{
		    mask = fbStipple24Bits[rot >> 3][stip] & rightMask;
		    *dst = *dst & ~mask | FbRot24(fgxor, rot) & mask;
		}
	    }
	    dst += dstStride;
	}
    }
    else
    {
	while (height--)
	{
	    rot = rot0;
	    src = srcLine;
	    srcLine += srcStride;
	    fbInitStipBits (srcX, firstlen, stip);
	    if (leftMask)
	    {
		mask = fbStipple24Bits[rot >> 3][stip];
		*dst = FbStippleRRopMask (*dst, mask,
					  FbRot24(fgand, rot),
					  FbRot24(fgxor, rot),
					  FbRot24(bgand, rot),
					  FbRot24(bgxor, rot),
					  leftMask);
		dst++;
		fbNextStipBits(rot,stip);
	    }
	    nl = nlMiddle;
	    while (nl--)
	    {
		mask = fbStipple24Bits[rot >> 3][stip];
		*dst = FbStippleRRop (*dst, mask,
				      FbRot24(fgand, rot),
				      FbRot24(fgxor, rot),
				      FbRot24(bgand, rot),
				      FbRot24(bgxor, rot));
		dst++;
		fbNextStipBits(rot,stip);
	    }
	    if (rightMask)
	    {
		mask = fbStipple24Bits[rot >> 3][stip];
		*dst = FbStippleRRopMask (*dst, mask,
					  FbRot24(fgand, rot),
					  FbRot24(fgxor, rot),
					  FbRot24(bgand, rot),
					  FbRot24(bgxor, rot),
					  rightMask);
	    }
	    dst += dstStride;
	}
    }
}
#endif

/*
 * Not very efficient, but simple -- copy a single plane
 * from an N bit image to a 1 bit image
 */
 
void
fbBltPlane (FbBits	    *src,
	    FbStride	    srcStride,
	    int		    srcX,
	    int		    srcBpp,

	    FbStip	    *dst,
	    FbStride	    dstStride,
	    int		    dstX,
	    
	    int		    width,
	    int		    height,
	    
	    FbStip	    fgand,
	    FbStip	    fgxor,
	    FbStip	    bgand,
	    FbStip	    bgxor,
	    Pixel	    planeMask)
{
    FbBits	*s;
    FbBits	pm;
    FbBits	srcMask;
    FbBits	srcMaskFirst;
    FbBits	srcMask0;
    FbBits	srcBits;
    
    FbStip	pixel;
    FbStip	dstBits;
    FbStip	*d;
    FbStip	dstMask;
    FbStip	dstMaskFirst;
    FbStip	dstUnion;
    int		w;
    int		wt;
    int		dst_words;
    int		rot0;

    if (!width)
	return;
    
    src += srcX >> FB_SHIFT;
    srcX &= FB_MASK;

    dst += dstX >> FB_STIP_SHIFT;
    dstX &= FB_STIP_MASK;
    
    w = width / srcBpp;

    pm = fbReplicatePixel (planeMask, srcBpp);
#ifdef FB_24BIT
    if (srcBpp == 24)
    {
	int w = 24;

	rot0 = srcX % 24;
	if (srcX + w > FB_UNIT)
	    w = FB_UNIT - srcX;
	srcMaskFirst = FbRot24(pm,rot0) & FbBitsMask(srcX,w);
    }
    else
#endif
    {
	rot0 = 0;
	srcMaskFirst = pm & FbBitsMask(srcX, srcBpp);
	srcMask0 = pm & FbBitsMask(0, srcBpp);
    }
    
    dstMaskFirst = FbStipMask(dstX,1); 
    while (height--)
    {
	d = dst;
	dst += dstStride;
	s = src;
	src += srcStride;
	
	srcMask = srcMaskFirst;
#ifdef FB_24BIT
	if (srcBpp == 24)
	    srcMask0 = FbRot24(pm,rot0) & FbBitsMask(0, srcBpp);
#endif
    	srcBits = *s++;

	dstMask = dstMaskFirst;
	dstUnion = 0;
	dstBits = 0;
	
	wt = w;
	
	while (wt--)
	{
	    if (!srcMask)
	    {
		srcBits = *s++;
#ifdef FB_24BIT
		if (srcBpp == 24)
		    srcMask0 = FbNext24Pix(srcMask0) & FbBitsMask(0,24);
#endif
		srcMask = srcMask0;
	    }
	    if (!dstMask)
	    {
		*d = FbStippleRRopMask(*d, dstBits,
				       fgand, fgxor, bgand, bgxor,
				       dstUnion);
		d++;
		dstMask = FbStipMask(0,1);
		dstUnion = 0;
		dstBits = 0;
	    }
	    if (srcBits & srcMask)
		dstBits |= dstMask;
	    dstUnion |= dstMask;
	    srcMask = FbScrRight(srcMask,srcBpp);
	    dstMask = FbStipRight(dstMask,1);
	}
	if (dstUnion)
	    *d = FbStippleRRopMask(*d,dstBits,
				   fgand, fgxor, bgand, bgxor,
				   dstUnion);
    }
}

