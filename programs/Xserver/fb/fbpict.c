/*
 * $XFree86$
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

#define cvt8888to0565(s)    ((((s) >> 3) & 0x001f) | \
			     (((s) >> 5) & 0x07e0) | \
			     (((s) >> 8) & 0xf800))
#define cvt0565to8888(s)    (((((s) << 3) & 0xf8) | (((s) >> 2) & 0x7)) | \
			     ((((s) << 5) & 0xfc00) | (((s) >> 1) & 0x300)) | \
			     ((((s) << 8) & 0xf80000) | (((s) << 3) & 0x70000)))

#if IMAGE_BYTE_ORDER == MSBFirst
#define Fetch24(a)  ((unsigned long) (a) & 1 ? \
		     ((*(a) << 16) | *((CARD16 *) ((a)+1))) : \
		     ((*((CARD16 *) (a)) << 8) | *((a)+2)))
#define Store24(a,v) ((unsigned long) (a) & 1 ? \
		      ((*(a) = (CARD8) ((v) >> 16)), \
		       (*((CARD16 *) ((a)+1)) = (CARD16) (v))) : \
		      ((*((CARD16 *) (a)) = (CARD16) ((v) >> 8)), \
		       (*((a)+2) = (CARD8) (v))))
#else
#define Fetch24(a)  ((unsigned long) (a) & 1 ? \
		     ((*(a)) | (*((CARD16 *) ((a)+1)) << 8)) : \
		     ((*((CARD16 *) (a))) | (*((a)+2) << 16)))
#define Store24(a,v) ((unsigned long) (a) & 1 ? \
		      ((*(a) = (CARD8) (v)), \
		       (*((CARD16 *) ((a)+1)) = (CARD16) ((v) >> 8))) : \
		      ((*((CARD16 *) (a)) = (CARD16) (v)),\
		       (*((a)+2) = (CARD8) ((v) >> 16))))
#endif
		      
static CARD32
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

static CARD32
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

/*
 * Naming convention:
 *
 *  opSRCxMASKxDST
 */

static void
fbCompositeSolidMask_nx8x8888 (CARD8      op,
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
    switch (srcBpp) {
    case 32:
	src = *(CARD32 *) srcBits;
	break;
    case 24:
	src = Fetch24 ((CARD8 *) srcBits);
	break;
    case 16:
	src = *(CARD16 *) srcBits;
	src = cvt0565to8888(src);
	break;
    }
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
    maskLine += maskStride * yMask + xMask;
    
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

static void
fbCompositeSolidMask_nx8x0888 (CARD8      op,
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
    CARD8	*dstLine, *dst;
    CARD32	d;
    CARD8	*maskLine, *mask, m;
    FbBits	*dstBits, *maskBits, *srcBits;
    FbStride	dstStride, maskStride, srcStride;
    int		dstBpp, maskBpp, srcBpp;
    CARD16	w;

    fbGetDrawable(pSrc->pDrawable, srcBits, srcStride, srcBpp);
    switch (srcBpp) {
    case 32:
	src = *(CARD32 *) srcBits;
	break;
    case 24:
	src = Fetch24 ((CARD8 *) srcBits);
	break;
    case 16:
	src = *(CARD16 *) srcBits;
	src = cvt0565to8888(src);
	break;
    }
    /* manage missing src alpha */
    if (pSrc->pFormat->direct.alphaMask == 0)
	src |= 0xff000000;
    srca = src >> 24;
    if (srca == 0)
	return;
    
    fbGetDrawable(pDst->pDrawable, dstBits, dstStride, dstBpp);
    dstLine = (CARD8 *) dstBits;
    dstStride = dstStride * sizeof (FbBits) / sizeof (CARD8);
    dstLine += dstStride * yDst + xDst * 3;
    
    fbGetDrawable(pMask->pDrawable, maskBits, maskStride, maskBpp);
    maskLine = (CARD8 *) maskBits;
    maskStride = maskStride * sizeof (FbBits) / sizeof (CARD8);
    maskLine += maskStride * yMask + xMask;
    
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
		    d = src;
		else
		{
		    d = Fetch24(dst);
		    d = fbOver (src, d);
		}
		Store24(dst,d);
	    }
	    else if (m)
	    {
		d = fbOver (fbIn(src,m), Fetch24(dst));
		Store24(dst,d);
	    }
	    dst += 3;
	}
    }
}

static void
fbCompositeSolidMask_nx8x0565 (CARD8      op,
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
    CARD16	*dstLine, *dst;
    CARD32	d;
    CARD8	*maskLine, *mask, m;
    FbBits	*dstBits, *maskBits, *srcBits;
    FbStride	dstStride, maskStride, srcStride;
    int		dstBpp, maskBpp, srcBpp;
    CARD16	w;

    fbGetDrawable(pSrc->pDrawable, srcBits, srcStride, srcBpp);
    switch (srcBpp) {
    case 32:
	src = *(CARD32 *) srcBits;
	break;
    case 24:
	src = Fetch24 ((CARD8 *) srcBits);
	break;
    case 16:
	src = *(CARD16 *) srcBits;
	src = cvt0565to8888(src);
	break;
    }
    /* manage missing src alpha */
    if (pSrc->pFormat->direct.alphaMask == 0)
	src |= 0xff000000;
    srca = src >> 24;
    if (srca == 0)
	return;
    
    fbGetDrawable(pDst->pDrawable, dstBits, dstStride, dstBpp);
    dstLine = (CARD16 *) dstBits;
    dstStride = dstStride * sizeof (FbBits) / sizeof (CARD16);
    dstLine += dstStride * yDst + xDst;
    
    fbGetDrawable(pMask->pDrawable, maskBits, maskStride, maskBpp);
    maskLine = (CARD8 *) maskBits;
    maskStride = maskStride * sizeof (FbBits) / sizeof (CARD8);
    maskLine += maskStride * yMask + xMask;
    
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
		    d = src;
		else
		{
		    d = *dst;
		    d = fbOver (src, cvt0565to8888(d));
		}
		*dst = cvt8888to0565(d);
	    }
	    else if (m)
	    {
		d = *dst;
		d = fbOver (fbIn(src,m), cvt0565to8888(d));
		*dst = cvt8888to0565(d);
	    }
	    dst++;
	}
    }
}

static void
fbCompositeSrc_8888x8888 (CARD8      op,
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
    CARD32	*dstLine, *dst, dstMask;
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

static void
fbCompositeSrc_8888x0888 (CARD8      op,
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
    CARD8	*dstLine, *dst;
    CARD32	d;
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

    fbGetDrawable(pDst->pDrawable, dstBits, dstStride, dstBpp);
    dstLine = (CARD8 *) dstBits;
    dstStride = dstStride * sizeof (FbBits) / sizeof (CARD8);
    dstLine += dstStride * yDst + xDst * 3;

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
	    if (a)
	    {
		if (a == 0xff)
		    d = s;
		else
		    d = fbOver (s, Fetch24(dst));
		Store24(dst,d);
	    }
	    dst += 3;
	}
    }
}

static void
fbCompositeSrc_8888x0565 (CARD8      op,
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
    CARD16	*dstLine, *dst;
    CARD32	d;
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

    fbGetDrawable(pDst->pDrawable, dstBits, dstStride, dstBpp);
    dstLine = (CARD16 *) dstBits;
    dstStride = dstStride * sizeof (FbBits) / sizeof (CARD16);
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
	    if (a)
	    {
		if (a == 0xff)
		    d = s;
		else
		{
		    d = *dst;
		    d = fbOver (s, cvt0565to8888(d));
		}
		*dst = cvt8888to0565(d);
	    }
	    dst++;
	}
    }
}

static void
fbCompositeSrc_0565x0565 (CARD8      op,
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
    CARD16	*dstLine, *dst;
    CARD16	*srcLine, *src;
    FbBits	*dstBits, *srcBits;
    FbStride	dstStride, srcStride;
    int		dstBpp, srcBpp;
    CARD16	w;
    
    fbGetDrawable(pSrc->pDrawable, srcBits, srcStride, srcBpp);
    srcLine = (CARD16 *) srcBits;
    srcStride = srcStride * sizeof (FbBits) / sizeof (CARD16);
    srcLine += srcStride * ySrc + xSrc;

    fbGetDrawable(pDst->pDrawable, dstBits, dstStride, dstBpp);
    dstLine = (CARD16 *) dstBits;
    dstStride = dstStride * sizeof (FbBits) / sizeof (CARD16);
    dstLine += dstStride * yDst + xDst;

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;

	while (w--)
	    *dst++ = *src++;
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
		    switch (pSrc->pDrawable->bitsPerPixel) {
		    case 32:
		    case 24:
		    case 16:
			switch (pMask->pDrawable->bitsPerPixel) {
			case 8:
			    switch (pDst->pDrawable->bitsPerPixel) {
			    case 16:
				func = fbCompositeSolidMask_nx8x0565;
				break;
			    case 24:
				func = fbCompositeSolidMask_nx8x0888;
				break;
			    case 32:
				func = fbCompositeSolidMask_nx8x8888;
				break;
			    }
			    break;
			}
			break;
		    }
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
	    switch (pSrc->pDrawable->bitsPerPixel) {
	    case 32:
		switch (pDst->pDrawable->bitsPerPixel) {
		case 32:
		    func = fbCompositeSrc_8888x8888;
		    break;
		case 24:
		    func = fbCompositeSrc_8888x0888;
		    break;
		case 16:
		    func = fbCompositeSrc_8888x0565;
		    break;
		}
		break;
	    case 16:
		switch (pDst->pDrawable->bitsPerPixel) {
		case 16:
		    func = fbCompositeSrc_0565x0565;
		    break;
		}
		break;
	    }
	}
    }
    if (func)
    {
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
    REGION_UNINIT (pDst->pDrawable->pScreen, &region);
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
