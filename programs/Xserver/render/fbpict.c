/*
 * $Id: fbpict.c,v 1.1 2000/08/25 23:40:56 keithp Exp $
 *
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#include "fb.h"
#include "picturestr.h"
#include "mipict.h"

typedef void	(*CompositeFunc) (CARD8      op,
				  PicturePtr pSrc,
				  PicturePtr pMask,
				  PicturePtr pDst,
				  INT16      xSrc,
				  INT16      ySrc,
				  INT16      xMask,
				  INT16      yMask,
				  INT16      xDst,
				  INT16      yDst,
				  CARD16     width,
				  CARD16     height);

#define INT_MULT(a,b,t) ( (t) = (a) * (b) + 0x80, ( ( ( (t)>>8 ) + (t) )>>8 ) )
#define GET8(v,i)   ((CARD16) (CARD8) ((v) >> i))
#define OVER(x,y,i,a,t) ((t) = INT_MULT(GET8(y,i),(a),(t)) + GET8(x,i),\
			 (CARD32) ((CARD8) ((t) | (0 - ((t) >> 8)))) << (i))
#define IN(x,i,a,t) ((CARD32) INT_MULT(GET8(x,i),(a),(t)) << (i))

CARD32
fbOver (CARD32 x, CARD32 y)
{
    CARD16  a = ~x >> 24;
    CARD16  t;
    CARD32  m,n,o,p;

    m = OVER(x,y,0,a,t);
    n = OVER(x,y,8,a,t);
    o = OVER(x,y,16,a,t);
    p = OVER(x,y,24,a,t);
    return m|n|o|p;
}

CARD32
fbIn (CARD32 x, CARD8 y)
{
    CARD16  a = y;
    CARD16  t;
    CARD32  m,n,o,p;

    m = IN(x,0,a,t);
    n = IN(x,8,a,t);
    o = IN(x,16,a,t);
    p = IN(x,24,a,t);
    return m|n|o|p;
}

void
fbCompositeSolidMask (CARD8      op,
		      PicturePtr pSrc,
		      PicturePtr pMask,
		      PicturePtr pDst,
		      INT16      xSrc,
		      INT16      ySrc,
		      INT16      xMask,
		      INT16      yMask,
		      INT16      xDst,
		      INT16      yDst,
		      CARD16     width,
		      CARD16     height)
{
    CARD32	src, srca;
    CARD32	*dstLine, *dst, d, dstMask;
    CARD8	*maskLine, *mask, m;
    FbBits	*dstBits, *maskBits, *srcBits;
    FbStride	dstStride, maskStride, srcStride;
    int		dstBpp, maskBpp, srcBpp;
    CARD16	w;

    fbGetDrawable(pSrc->pDrawable, srcBits, srcStride, srcBpp);
    src = *(CARD32 *) srcBits;
    /* manage missing src alpha */
    if (pSrc->pFormat->direct.alphaMask == 0)
	src |= 0xff000000;
    dstMask = FbFullMask (pDst->pDrawable->depth);
    srca = src >> 24;
    if (srca == 0)
	return;
    
    fbGetDrawable(pDst->pDrawable, dstBits, dstStride, dstBpp);
    dstLine = (CARD32 *) dstBits;
    dstStride = dstStride * sizeof (FbBits) / sizeof (CARD32);
    dstLine += dstStride * yDst + xDst;
    
    fbGetDrawable(pMask->pDrawable, maskBits, maskStride, maskBpp);
    maskLine = (CARD8 *) maskBits;
    maskStride = maskStride * sizeof (FbBits) / sizeof (CARD8);
    maskLine += maskStride * yMask + yMask;
    
    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	mask = maskLine;
	maskLine += maskStride;
	w = width;

	while (w--)
	{
	    m = *mask++;
	    if (m == 0xff)
	    {
		if (srca == 0xff)
		    *dst = src & dstMask;
		else
		    *dst = fbOver (src, *dst) & dstMask;
	    }
	    else if (m)
	    {
		d = fbIn (src, m);
		*dst = fbOver (d, *dst) & dstMask;
	    }
	    dst++;
	}
    }
}

void
fbCompositeSrc (CARD8      op,
		PicturePtr pSrc,
		PicturePtr pMask,
		PicturePtr pDst,
		INT16      xSrc,
		INT16      ySrc,
		INT16      xMask,
		INT16      yMask,
		INT16      xDst,
		INT16      yDst,
		CARD16     width,
		CARD16     height)
{
    CARD32	*dstLine, *dst, d, dstMask;
    CARD32	*srcLine, *src, s;
    CARD8	a;
    FbBits	*dstBits, *srcBits;
    FbStride	dstStride, srcStride;
    int		dstBpp, srcBpp;
    CARD16	w;
    
    fbGetDrawable(pSrc->pDrawable, srcBits, srcStride, srcBpp);
    srcLine = (CARD32 *) srcBits;
    srcStride = srcStride * sizeof (FbBits) / sizeof (CARD32);
    srcLine += srcStride * ySrc + xSrc;

    dstMask = FbFullMask (pDst->pDrawable->depth);
    fbGetDrawable(pDst->pDrawable, dstBits, dstStride, dstBpp);
    dstLine = (CARD32 *) dstBits;
    dstStride = dstStride * sizeof (FbBits) / sizeof (CARD32);
    dstLine += dstStride * yDst + xDst;

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;

	while (w--)
	{
	    s = *src++;
	    a = s >> 24;
	    if (a == 0xff)
		*dst = s & dstMask;
	    else if (a)
		*dst = fbOver (s, *dst) & dstMask;
	    dst++;
	}
    }
}

void
fbComposite (CARD8      op,
	     PicturePtr pSrc,
	     PicturePtr pMask,
	     PicturePtr pDst,
	     INT16      xSrc,
	     INT16      ySrc,
	     INT16      xMask,
	     INT16      yMask,
	     INT16      xDst,
	     INT16      yDst,
	     CARD16     width,
	     CARD16     height)
{
    RegionRec	    region;
    int		    n;
    BoxPtr	    pbox;
    CompositeFunc   func;
    
    xDst += pDst->pDrawable->x;
    yDst += pDst->pDrawable->y;
    xSrc += pSrc->pDrawable->x;
    ySrc += pSrc->pDrawable->y;
    if (pMask)
    {
	xMask += pMask->pDrawable->x;
	yMask += pMask->pDrawable->y;
    }
    
    if (!miComputeCompositeRegion (&region,
				   pSrc,
				   pMask,
				   pDst,
				   xSrc,
				   ySrc,
				   xMask,
				   yMask,
				   xDst,
				   yDst,
				   width,
				   height))
	return;
				   
    func = 0;
    if (pMask)
    {
	if (pMask->repeat)
	{
	    ;
	}
	else
	{
	    if (pSrc->repeat)
	    {
		if (pSrc->pDrawable->width == 1 &&
		    pSrc->pDrawable->height == 1)
		{
		    func = fbCompositeSolidMask;
		}
		else
		{
		    ;
		}
	    }
	    else
	    {
		;
	    }
	}
    }
    else
    {
	if (pSrc->repeat)
	{
	    if (pSrc->pDrawable->width == 1 &&
		pSrc->pDrawable->height == 1)
	    {
		;
	    }
	    else
	    {
		;
	    }
	}
	else
	{
	    func = fbCompositeSrc;
	}
    }
    if (!func)
	return;
    n = REGION_NUM_RECTS (&region);
    pbox = REGION_RECTS (&region);
    while (n--)
    {
	(*func) (op, pSrc, pMask, pDst,
		 pbox->x1 - xDst + xSrc,
		 pbox->y1 - yDst + ySrc,
		 pbox->x1 - xDst + xMask,
		 pbox->y1 - yDst + yMask,
		 pbox->x1,
		 pbox->y1,
		 pbox->x2 - pbox->x1,
		 pbox->y2 - pbox->y1);
	pbox++;
    }
}

Bool
fbPictureInit (ScreenPtr pScreen, PictFormatPtr formats, int nformats)
{
    PictureScreenPtr    ps;

    if (!miPictureInit (pScreen, formats, nformats))
	return FALSE;
    ps = GetPictureScreen(pScreen);
    ps->Composite = fbComposite;
    ps->Glyphs = miGlyphs;
    return TRUE;
}
