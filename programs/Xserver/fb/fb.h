/*
 * $Id$
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
/* $XFree86: xc/programs/Xserver/fb/fb.h,v 1.4 2000/01/20 01:40:14 tsi Exp $ */

#ifndef _FB_H_
#define _FB_H_

#include "X.h"
#include "scrnintstr.h"
#include "pixmap.h"
#include "pixmapstr.h"
#include "region.h"
#include "gcstruct.h"
#include "colormap.h"
#include "miscstruct.h"
#include "servermd.h"
#include "windowstr.h"
#include "mi.h"
#include "migc.h"
#include "mibstore.h"

/*
 * This single define controls the basic size of data manipulated
 * by this software; it must be log2(sizeof (FbBits) * 8)
 */

#ifndef FB_SHIFT
#define FB_SHIFT    5
#endif
    
#define FB_UNIT	    (1 << FB_SHIFT)
#define FB_HALFUNIT (1 << (FB_SHIFT-1))
#define FB_MASK	    (FB_UNIT - 1)
#define FB_ALLONES  ((FbBits) -1)
    
#if GLYPHPADBYTES != 4
#error "GLYPHPADBYTES must be 4"
#endif
#if GETLEFTBITS_ALIGNMENT != 1
#error "GETLEFTBITS_ALIGNMENT must be 1"
#endif
/* whether to bother to include 24bpp support */
#ifndef FBNO24BIT
#define FB_24BIT
#endif

#define FB_STIP_SHIFT	LOG2_BITMAP_PAD
#define FB_STIP_UNIT	(1 << FB_STIP_SHIFT)
#define FB_STIP_MASK	(FB_STIP_UNIT - 1)
#define FB_STIP_ALLONES	((FbStip) -1)
    
#define FB_STIP_ODDSTRIDE(s)	(((s) & (FB_MASK >> FB_STIP_SHIFT)) != 0)
#define FB_STIP_ODDPTR(p)	((((int) (p)) & (FB_MASK >> 3)) != 0)
    
#define FbStipStrideToBitsStride(s) (((s) >> (FB_SHIFT - FB_STIP_SHIFT)))
#define FbBitsStrideToStipStride(s) (((s) << (FB_SHIFT - FB_STIP_SHIFT)))
    
#define FbFullMask(n)   ((n) == FB_UNIT ? FB_ALLONES : ((((FbBits) 1) << n) - 1))
    
#if FB_SHIFT == 6
#ifdef WIN32
typedef unsigned __int64    FbBits;
#else
typedef unsigned long long  FbBits;
#endif
#endif
#if FB_SHIFT == 5
typedef unsigned long	    FbBits;
#endif
typedef unsigned long	    FbStip;
typedef int		    FbStride;


#ifdef FB_DEBUG
extern void fbValidateDrawable(DrawablePtr d);
extern void fbInitializeDrawable(DrawablePtr d);
extern void fbSetBits (FbStip *bits, int stride, FbStip data);
#define FB_HEAD_BITS   (FbStip) (0xbaadf00d)
#define FB_TAIL_BITS   (FbStip) (0xbaddf0ad)
#else
#define fbValidateDrawable(d)
#define fdInitializeDrawable(d)
#endif

#include "fbrop.h"

#if BITMAP_BIT_ORDER == LSBFirst
#define FbScrLeft(x,n)	((x) >> (n))
#define FbScrRight(x,n)	((x) << (n))
/* #define FbLeftBits(x,n)	((x) & ((((FbBits) 1) << (n)) - 1)) */
#define FbLeftStipBits(x,n) ((x) & ((((FbStip) 1) << (n)) - 1))
#define FbStipMoveLsb(x,s,n)	(FbStipRight (x,(s)-(n)))
#else
#define FbScrLeft(x,n)	((x) << (n))
#define FbScrRight(x,n)	((x) >> (n))
/* #define FbLeftBits(x,n)	((x) >> (FB_UNIT - (n))) */
#define FbLeftStipBits(x,n) ((x) >> (FB_STIP_UNIT - (n)))
#define FbStipMoveLsb(x,s,n)	(x)
#endif

#define GetHighWord(x) (((int) (x)) >> 16)

#if IMAGE_BYTE_ORDER == MSBFirst
#define intToCoord(i,x,y)   (((x) = GetHighWord(i)), ((y) = (int) ((short) (i))))
#define coordToInt(x,y)	(((x) << 16) | (y))
#define intToX(i)	(GetHighWord(i))
#define intToY(i)	((int) ((short) i))
#else
#define intToCoord(i,x,y)   (((x) = (int) ((short) (i))), ((y) = GetHighWord(i)))
#define coordToInt(x,y)	(((y) << 16) | (x))
#define intToX(i)	((int) ((short) (i)))
#define intToY(i)	(GetHighWord(i))
#endif

#define FbStipLeft(x,n)	FbScrLeft(x,n)
#define FbStipRight(x,n) FbScrRight(x,n)

#define FbRotLeft(x,n)	FbScrLeft(x,n) | (n ? FbScrRight(x,FB_UNIT-n) : 0)
#define FbRotRight(x,n)	FbScrRight(x,n) | (n ? FbScrLeft(x,FB_UNIT-n) : 0)

#define FbRotStipLeft(x,n)  FbStipLeft(x,n) | (n ? FbStipRight(x,FB_STIP_UNIT-n) : 0)
#define FbRotStipRight(x,n)  FbStipRight(x,n) | (n ? FbStipLeft(x,FB_STIP_UNIT-n) : 0)

#define FbLeftMask(x)	    ( ((x) & FB_MASK) ? \
			     FbScrRight(FB_ALLONES,(x) & FB_MASK) : 0)
#define FbRightMask(x)	    ( ((FB_UNIT - (x)) & FB_MASK) ? \
			     FbScrLeft(FB_ALLONES,(FB_UNIT - (x)) & FB_MASK) : 0)

#define FbLeftStipMask(x)   ( ((x) & FB_STIP_MASK) ? \
			     FbStipRight(FB_STIP_ALLONES,(x) & FB_STIP_MASK) : 0)
#define FbRightStipMask(x)  ( ((FB_STIP_UNIT - (x)) & FB_STIP_MASK) ? \
			     FbScrLeft(FB_STIP_ALLONES,(FB_STIP_UNIT - (x)) & FB_STIP_MASK) : 0)

#define FbBitsMask(x,w)	(FbScrRight(FB_ALLONES,(x) & FB_MASK) & \
			 FbScrLeft(FB_ALLONES,(FB_UNIT - ((x) + (w))) & FB_MASK))

#define FbStipMask(x,w)	(FbStipRight(FB_STIP_ALLONES,(x) & FB_STIP_MASK) & \
			 FbStipLeft(FB_STIP_ALLONES,(FB_STIP_UNIT - ((x)+(w))) & FB_STIP_MASK))


#define FbMaskBits(x,w,l,n,r) { \
    n = (w); \
    r = FbRightMask((x)+n); \
    l = FbLeftMask(x); \
    if (l) { \
	n -= FB_UNIT - ((x) & FB_MASK); \
	if (n < 0) { \
	    n = 0; \
	    l &= r; \
	    r = 0; \
	} \
    } \
    n >>= FB_SHIFT; \
}

#define FbMaskStip(x,w,l,n,r) { \
    n = (w); \
    r = FbRightStipMask((x)+n); \
    l = FbLeftStipMask(x); \
    if (l) { \
	n -= FB_STIP_UNIT - ((x) & FB_STIP_MASK); \
	if (n < 0) { \
	    n = 0; \
	    l &= r; \
	    r = 0; \
	} \
    } \
    n >>= FB_STIP_SHIFT; \
}

/* Rotate a filled pixel value to the specified alignement */
#define FbRot24(p,b)	    (FbScrRight(p,b) | FbScrLeft(p,24-(b)))
#define FbRot24Stip(p,b)    (FbStipRight(p,b) | FbStipLeft(p,24-(b)))

/* step a filled pixel value to the next/previous FB_UNIT alignment */
#define FbNext24Pix(p)	(FbRot24(p,(24-FB_UNIT%24)))
#define FbPrev24Pix(p)	(FbRot24(p,FB_UNIT%24))
#define FbNext24Stip(p)	(FbRot24(p,(24-FB_STIP_UNIT%24)))
#define FbPrev24Stip(p)	(FbRot24(p,FB_STIP_UNIT%24))

/* step a rotation value to the next/previous rotation value */
#if FB_UNIT == 64
#define FbNext24Rot(r)        ((r) == 16 ? 0 : (r) + 8)
#define FbPrev24Rot(r)        ((r) == 0 ? 16 : (r) - 8)
#endif
#if FB_UNIT == 32
#define FbNext24Rot(r)        ((r) == 0 ? 16 : (r) - 8)
#define FbPrev24Rot(r)        ((r) == 16 ? 0 : (r) + 8)
#endif

#define FbNext24RotStip(r)        ((r) == 0 ? 16 : (r) - 8)
#define FbPrev24RotStip(r)        ((r) == 16 ? 0 : (r) + 8)

/* Whether 24-bit specific code is needed for this filled pixel value */
#define FbCheck24Pix(p)	((p) == FbNext24Pix(p))

extern int	fbGCPrivateIndex;
extern const GCOps	fbGCOps;
extern const GCFuncs	fbGCFuncs;

#ifdef TEKX11
#define FB_OLD_GC
#define FB_OLD_SCREEN
#endif

/* private field of GC */
typedef struct {
#ifdef FB_OLD_GC
    unsigned char       pad1;
    unsigned char       pad2;
    unsigned char       pad3;
    unsigned		fExpose:1;
    unsigned		freeCompClip:1;
    PixmapPtr		pRotatedPixmap;
    RegionPtr		pCompositeClip;
#endif    
    FbBits		and, xor;	/* reduced rop values */
    FbBits		bgand, bgxor;	/* for stipples */
    FbBits		fg, bg, pm;	/* expanded and filled */
    unsigned int	dashLength;	/* total of all dash elements */
    Bool		oneRect;	/* clip list is single rectangle */
} FbGCPrivRec, *FbGCPrivPtr;

#define fbGetGCPrivate(pGC)	((FbGCPrivPtr)\
	(pGC)->devPrivates[fbGCPrivateIndex].ptr)

#ifdef FB_OLD_GC
#define fbGetCompositeClip(pGC) (fbGetGCPrivate(pGC)->pCompositeClip)
#define fbGetExpose(pGC)	(fbGetGCPrivate(pGC)->fExpose)
#define fbGetFreeCompClip(pGC)	(fbGetGCPrivate(pGC)->freeCompClip)
#define fbGetRotatedPixmap(pGC)	(fbGetGCPrivate(pGC)->pRotatedPixmap)
#else
#define fbGetCompositeClip(pGC) ((pGC)->pCompositeClip)
#define fbGetExpose(pGC)	((pGC)->fExpose)
#define fbGetFreeCompClip(pGC)	((pGC)->freeCompClip)
#define fbGetRotatedPixmap(pGC)	((pGC)->pRotatedPixmap)
#endif

#define fbGetScreenPixmap(s)	((PixmapPtr) (s)->devPrivate)
#define fbGetWindowPixmap(d)	fbGetScreenPixmap((d)->pScreen)

#define fbGetDrawable(pDrawable, pointer, stride, bpp) { \
    PixmapPtr   _pPix; \
    if ((pDrawable)->type != DRAWABLE_PIXMAP) \
	_pPix = fbGetWindowPixmap(pDrawable); \
    else \
	_pPix = (PixmapPtr) (pDrawable); \
    (pointer) = (FbBits *) _pPix->devPrivate.ptr; \
    (stride) = ((int) _pPix->devKind) / sizeof (FbBits); \
    (bpp) = _pPix->drawable.bitsPerPixel; \
}

#define fbGetStipDrawable(pDrawable, pointer, stride, bpp) { \
    PixmapPtr   _pPix; \
    if ((pDrawable)->type != DRAWABLE_PIXMAP) \
	_pPix = fbGetWindowPixmap(pDrawable); \
    else \
	_pPix = (PixmapPtr) (pDrawable); \
    (pointer) = (FbStip *) _pPix->devPrivate.ptr; \
    (stride) = ((int) _pPix->devKind) / sizeof (FbStip); \
    (bpp) = _pPix->drawable.bitsPerPixel; \
}

/*
 * XFree86 empties the root BorderClip when the VT is inactive,
 * here's a macro which uses that to disable GetImage and GetSpans
 */

#define fbWindowEnabled(pWin) \
    REGION_NOTEMPTY((pWin)->drawable.pScreen, \
		    &WindowTable[(pWin)->drawable.pScreen->myNum]->borderClip)

#define fbDrawableEnabled(pDrawable) \
    ((pDrawable)->type == DRAWABLE_PIXMAP ? \
     TRUE : fbWindowEnabled((WindowPtr) pDrawable))

#ifdef FB_OLD_SCREEN
#define BitsPerPixel(d) (\
    ((1 << PixmapWidthPaddingInfo[d].padBytesLog2) * 8 / \
    (PixmapWidthPaddingInfo[d].padRoundUp+1)))
#endif

#define FbPowerOfTwo(w)	    (((w) & ((w) - 1)) == 0)
/*
 * Accelerated tiles are power of 2 width <= FB_UNIT
 */
#define FbEvenTile(w)	    ((w) <= FB_UNIT && FbPowerOfTwo(w))
/*
 * Accelerated stipples are power of 2 width and <= FB_UNIT/dstBpp
 * with dstBpp a power of 2 as well
 */
#define FbEvenStip(w,bpp)   ((w) * (bpp) <= FB_UNIT && FbPowerOfTwo(w) && FbPowerOfTwo(bpp))

/*
 * fballpriv.c
 */
Bool
fbAllocatePrivates(ScreenPtr pScreen, int *pGCIndex);
    
/*
 * fbarc.c
 */

void
fbPolyArc (DrawablePtr	pDrawable,
	   GCPtr	pGC,
	   int		narcs,
	   xArc		*parcs);

/*
 * fbbits.c
 */

void	
fbBresSolid8(DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    dashOffset,
	     int	    signdx,
	     int	    signdy,
	     int	    axis,
	     int	    x1,
	     int	    y1,
	     int	    e,
	     int	    e1,
	     int	    e3,
	     int	    len);

void	
fbBresDash8 (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    dashOffset,
	     int	    signdx,
	     int	    signdy,
	     int	    axis,
	     int	    x1,
	     int	    y1,
	     int	    e,
	     int	    e1,
	     int	    e3,
	     int	    len);

void	
fbDots8 (FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstBpp,
	 BoxPtr	    pBox,
	 xPoint	    *pts,
	 int	    npt,
	 int	    xoff,
	 int	    yoff,
	 FbBits	    and,
	 FbBits	    xor);

void	
fbArc8 (FbBits	    *dst,
	FbStride    dstStride,
	int	    dstBpp,
	xArc	    *arc,
	int	    dx,
	int	    dy,
	FbBits	    and,
	FbBits	    xor);

void
fbGlyph8 (FbBits    *dstLine,
	  FbStride  dstStride,
	  int	    dstBpp,
	  FbStip    *stipple,
	  FbBits    fg,
	  int	    height,
	  int	    shift);

void
fbPolyline8 (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    mode,
	     int	    npt,
	     DDXPointPtr    ptsOrig);

void
fbPolySegment8 (DrawablePtr pDrawable,
		GCPtr	    pGC,
		int	    nseg,
		xSegment    *pseg);

void	
fbBresSolid16(DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    dashOffset,
	      int	    signdx,
	      int	    signdy,
	      int	    axis,
	      int	    x1,
	      int	    y1,
	      int	    e,
	      int	    e1,
	      int	    e3,
	      int	    len);

void	
fbBresDash16(DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    dashOffset,
	     int	    signdx,
	     int	    signdy,
	     int	    axis,
	     int	    x1,
	     int	    y1,
	     int	    e,
	     int	    e1,
	     int	    e3,
	     int	    len);

void	
fbDots16(FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstBpp,
	 BoxPtr	    pBox,
	 xPoint	    *pts,
	 int	    npt,
	 int	    xoff,
	 int	    yoff,
	 FbBits	    and,
	 FbBits	    xor);

void	
fbArc16(FbBits	    *dst,
	FbStride    dstStride,
	int	    dstBpp,
	xArc	    *arc,
	int	    dx,
	int	    dy,
	FbBits	    and,
	FbBits	    xor);

void
fbGlyph16(FbBits    *dstLine,
	  FbStride  dstStride,
	  int	    dstBpp,
	  FbStip    *stipple,
	  FbBits    fg,
	  int	    height,
	  int	    shift);

void
fbPolyline16 (DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    mode,
	      int	    npt,
	      DDXPointPtr   ptsOrig);

void
fbPolySegment16 (DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		nseg,
		 xSegment	*pseg);

void	
fbBresSolid32(DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    dashOffset,
	      int	    signdx,
	      int	    signdy,
	      int	    axis,
	      int	    x1,
	      int	    y1,
	      int	    e,
	      int	    e1,
	      int	    e3,
	      int	    len);

void	
fbBresDash32(DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    dashOffset,
	     int	    signdx,
	     int	    signdy,
	     int	    axis,
	     int	    x1,
	     int	    y1,
	     int	    e,
	     int	    e1,
	     int	    e3,
	     int	    len);

void	
fbDots32(FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstBpp,
	 BoxPtr	    pBox,
	 xPoint	    *pts,
	 int	    npt,
	 int	    xoff,
	 int	    yoff,
	 FbBits	    and,
	 FbBits	    xor);

void	
fbArc32(FbBits	    *dst,
	FbStride    dstStride,
	int	    dstBpp,
	xArc	    *arc,
	int	    dx,
	int	    dy,
	FbBits	    and,
	FbBits	    xor);

void
fbGlyph32(FbBits    *dstLine,
	  FbStride  dstStride,
	  int	    dstBpp,
	  FbStip    *stipple,
	  FbBits    fg,
	  int	    height,
	  int	    shift);
void
fbPolyline32 (DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    mode,
	      int	    npt,
	      DDXPointPtr   ptsOrig);

void
fbPolySegment32 (DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		nseg,
		 xSegment	*pseg);

/*
 * fbblt.c
 */
void
fbBlt (FbBits   *src, 
       FbStride	srcStride,
       int	srcX,
       
       FbBits   *dst,
       FbStride dstStride,
       int	dstX,
       
       int	width, 
       int	height,
       
       int	alu,
       FbBits	pm,
       int	bpp,
       
       Bool	reverse,
       Bool	upsidedown);

void
fbBlt24 (FbBits	    *srcLine,
	 FbStride   srcStride,
	 int	    srcX,

	 FbBits	    *dstLine,
	 FbStride   dstStride,
	 int	    dstX,

	 int	    width, 
	 int	    height,

	 int	    alu,
	 FbBits	    pm,

	 Bool	    reverse,
	 Bool	    upsidedown);
    
void
fbBltStip (FbStip   *src,
	   FbStride srcStride,	    /* in FbStip units, not FbBits units */
	   int	    srcX,
	   
	   FbStip   *dst,
	   FbStride dstStride,	    /* in FbStip units, not FbBits units */
	   int	    dstX,

	   int	    width, 
	   int	    height,

	   int	    alu,
	   FbBits   pm,
	   int	    bpp);
    
/*
 * fbbltone.c
 */
void
fbBltOne (FbStip   *src,
	  FbStride srcStride,
	  int	   srcX,
	  FbBits   *dst,
	  FbStride dstStride,
	  int	   dstX,
	  int	   dstBpp,

	  int	   width,
	  int	   height,

	  FbBits   fgand,
	  FbBits   fbxor,
	  FbBits   bgand,
	  FbBits   bgxor);
 
#ifdef FB_24BIT
void
fbBltOne24 (FbStip    *src,
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
	  FbBits    bgxor);
#endif

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
	    Pixel	    planeMask);

/*
 * fbbstore.c
 */
void
fbSaveAreas(PixmapPtr	pPixmap,
	    RegionPtr	prgnSave,
	    int		xorg,
	    int		yorg,
	    WindowPtr	pWin);

void
fbRestoreAreas(PixmapPtr    pPixmap,
	       RegionPtr    prgnRestore,
	       int	    xorg,
	       int	    yorg,
	       WindowPtr    pWin);

/*
 * fbcmap.c
 */
int
fbListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps);

void
fbInstallColormap(ColormapPtr pmap);

void
fbUninstallColormap(ColormapPtr pmap);

void
fbResolveColor(unsigned short	*pred, 
	       unsigned short	*pgreen, 
	       unsigned short	*pblue,
	       VisualPtr	pVisual);

Bool
fbInitializeColormap(ColormapPtr pmap);

int
fbExpandDirectColors (ColormapPtr   pmap, 
		      int	    ndef,
		      xColorItem    *indefs,
		      xColorItem    *outdefs);

Bool
fbCreateDefColormap(ScreenPtr pScreen);

Bool
fbSetVisualTypes (int depth, int visuals, int bitsPerRGB);

Bool
fbInitVisuals (VisualPtr    *visualp, 
	       DepthPtr	    *depthp,
	       int	    *nvisualp,
	       int	    *ndepthp,
	       int	    *rootDepthp,
	       VisualID	    *defaultVisp,
	       unsigned long	sizes,
	       int	    bitsPerRGB);

/*
 * fbcopy.c
 */

typedef void	(*fbCopyProc) (DrawablePtr  pSrcDrawable,
			       DrawablePtr  pDstDrawable,
			       GCPtr	    pGC,
			       BoxPtr	    pDstBox,
			       int	    nbox,
			       int	    dx,
			       int	    dy,
			       Bool	    reverse,
			       Bool	    upsidedown,
			       Pixel	    bitplane,
			       void	    *closure);

void
fbCopyNtoN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure);

void
fbCopy1toN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure);

void
fbCopyNto1 (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure);

void
fbCopyRegion (DrawablePtr   pSrcDrawable,
	      DrawablePtr   pDstDrawable,
	      GCPtr	    pGC,
	      RegionPtr	    pDstRegion,
	      int	    dx,
	      int	    dy,
	      fbCopyProc    copyProc,
	      Pixel	    bitPlane,
	      void	    *closure);

RegionPtr
fbDoCopy (DrawablePtr	pSrcDrawable,
	  DrawablePtr	pDstDrawable,
	  GCPtr		pGC,
	  int		xIn, 
	  int		yIn,
	  int		widthSrc, 
	  int		heightSrc,
	  int		xOut, 
	  int		yOut,
	  fbCopyProc	copyProc,
	  Pixel		bitplane,
	  void		*closure);
	  
RegionPtr
fbCopyArea (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    int		xIn, 
	    int		yIn,
	    int		widthSrc, 
	    int		heightSrc,
	    int		xOut, 
	    int		yOut);

RegionPtr
fbCopyPlane (DrawablePtr    pSrcDrawable,
	     DrawablePtr    pDstDrawable,
	     GCPtr	    pGC,
	     int	    xIn, 
	     int	    yIn,
	     int	    widthSrc, 
	     int	    heightSrc,
	     int	    xOut, 
	     int	    yOut,
	     unsigned long  bitplane);

/*
 * fbfill.c
 */
void
fbFill (DrawablePtr pDrawable,
	GCPtr	    pGC,
	int	    x,
	int	    y,
	int	    width,
	int	    height);

void
fbSolidBoxClipped (DrawablePtr	pDrawable,
		   RegionPtr	pClip,
		   int		x1,
		   int		y1,
		   int		x2,
		   int		y2,
		   FbBits	and,
		   FbBits	xor);

/*
 * fbfillrect.c
 */
void
fbPolyFillRect(DrawablePtr  pDrawable, 
	       GCPtr	    pGC, 
	       int	    nrectInit,
	       xRectangle   *prectInit);

#define fbPolyFillArc miPolyFillArc

#define fbFillPolygon miFillPolygon

/*
 * fbfillsp.c
 */
void
fbFillSpans (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    nInit,
	     DDXPointPtr    pptInit,
	     int	    *pwidthInit,
	     int	    fSorted);


/*
 * fbgc.c
 */

Bool
fbCreateGC(GCPtr pGC);

void
fbPadPixmap (PixmapPtr pPixmap);
    
void
fbValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable);

/*
 * fbgetsp.c
 */
void
fbGetSpans(DrawablePtr	pDrawable, 
	   int		wMax, 
	   DDXPointPtr	ppt, 
	   int		*pwidth, 
	   int		nspans, 
	   char		*pchardstStart);

/*
 * fbglyph.c
 */
void
fbPolyGlyphBlt (DrawablePtr	pDrawable,
		GCPtr		pGC,
		int		x, 
		int		y,
		unsigned int	nglyph,
		CharInfoPtr	*ppci,
		pointer		pglyphBase);

void
fbImageGlyphBlt (DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		x,
		 int		y,
		 unsigned int	nglyph,
		 CharInfoPtr	*ppci,
		 pointer	pglyphBase);

void
fbGlyph24 (FbBits   *dstBits,
	   FbStride dstStride,
	   int	    dstBpp,
	   FbStip   *stipple,
	   FbBits   fg,
	   int	    x,
	   int	    height);

/*
 * fbimage.c
 */

void
fbPutImage (DrawablePtr	pDrawable,
	    GCPtr	pGC,
	    int		depth,
	    int		x,
	    int		y,
	    int		w,
	    int		h,
	    int		leftPad,
	    int		format,
	    char	*pImage);

void
fbPutZImage (DrawablePtr	pDrawable,
	     RegionPtr		pClip,
	     int		alu,
	     FbBits		pm,
	     int		x,
	     int		y,
	     int		width,
	     int		height,
	     FbStip		*src,
	     FbStride		srcStride);

void
fbPutXYImage (DrawablePtr	pDrawable,
	      RegionPtr		pClip,
	      FbBits		fg,
	      FbBits		bg,
	      FbBits		pm,
	      int		alu,
	      Bool		opaque,
	      
	      int		x,
	      int		y,
	      int		width,
	      int		height,

	      FbStip		*src,
	      FbStride		srcStride,
	      int		srcX);

void
fbGetImage (DrawablePtr	    pDrawable,
	    int		    x,
	    int		    y,
	    int		    w,
	    int		    h,
	    unsigned int    format,
	    unsigned long   planeMask,
	    char	    *d);
/*
 * fbline.c
 */

void
fbPolyLine (DrawablePtr	pDrawable,
	    GCPtr	pGC,
	    int		mode,
	    int		npt,
	    DDXPointPtr	ppt);

void
fbFixCoordModePrevious (int npt,
			DDXPointPtr ppt);

void
fbPolySegment (DrawablePtr  pDrawable,
	       GCPtr	    pGC,
	       int	    nseg,
	       xSegment	    *pseg);

#define fbPolyRectangle	miPolyRectangle

/*
 * fbpixmap.c
 */

PixmapPtr
fbCreatePixmap (ScreenPtr pScreen, int width, int height, int depth);

Bool
fbDestroyPixmap (PixmapPtr pPixmap);

RegionPtr
fbPixmapToRegion(PixmapPtr pPix);

/*
 * fbpoint.c
 */

void
fbPolyPoint (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    mode,
	     int	    npt,
	     xPoint	    *pptInit);

/*
 * fbpush.c
 */
void
fbPushPattern (DrawablePtr  pDrawable,
	       GCPtr	    pGC,
	       
	       FbStip	    *src,
	       FbStride	    srcStride,
	       int	    srcX,

	       int	    x,
	       int	    y,

	       int	    width,
	       int	    height);

void
fbPushFill (DrawablePtr	pDrawable,
	    GCPtr	pGC,

	    FbStip	*src,
	    FbStride	srcStride,
	    int		srcX,
	    
	    int		x,
	    int		y,
	    int		width,
	    int		height);

void
fbPush1toN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure);

void
fbPushPixels (GCPtr	    pGC,
	      PixmapPtr	    pBitmap,
	      DrawablePtr   pDrawable,
	      int	    dx,
	      int	    dy,
	      int	    xOrg,
	      int	    yOrg);


/*
 * fbscreen.c
 */

Bool
fbCloseScreen (int index, ScreenPtr pScreen);

Bool
fbRealizeFont(ScreenPtr pScreen, FontPtr pFont);

Bool
fbUnrealizeFont(ScreenPtr pScreen, FontPtr pFont);

Bool
fbSetupScreen(ScreenPtr	pScreen, 
	      pointer	pbits,		/* pointer to screen bitmap */
	      int	xsize, 		/* in pixels */
	      int	ysize,
	      int	dpix,		/* dots per inch */
	      int	dpiy,
	      int	width,		/* pixel width of frame buffer */
	      int	bpp);		/* bits per pixel of frame buffer */

Bool
fbFinishScreenInit(ScreenPtr	pScreen,
		   pointer	pbits,
		   int		xsize,
		   int		ysize,
		   int		dpix,
		   int		dpiy,
		   int		width,
		   int		bpp);

Bool
fbScreenInit(ScreenPtr	pScreen,
	     pointer	pbits,
	     int	xsize,
	     int	ysize,
	     int	dpix,
	     int	dpiy,
	     int	width,
	     int	bpp);

void
fbInitializeBackingStore (ScreenPtr pScreen);
    
/*
 * fbseg.c
 */
typedef void	FbBres (DrawablePtr	pDrawable,
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
			int		len);

FbBres fbBresSolid, fbBresDash, fbBresFill, fbBresFillDash;
/*
 * fbsetsp.c
 */

void
fbSetSpans (DrawablePtr	    pDrawable,
	    GCPtr	    pGC,
	    char	    *src,
	    DDXPointPtr	    ppt,
	    int		    *pwidth,
	    int		    nspans,
	    int		    fSorted);

FbBres *
fbSelectBres (DrawablePtr   pDrawable,
	      GCPtr	    pGC);

void
fbBres (DrawablePtr	pDrawable,
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
	int		len);

void
fbSegment (DrawablePtr	pDrawable,
	   GCPtr	pGC,
	   int		x1,
	   int		y1,
	   int		x2,
	   int		y2,
	   Bool		drawLast,
	   int		*dashOffset);


/*
 * fbsolid.c
 */

void
fbSolid (FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstX,
	 int	    bpp,

	 int	    width,
	 int	    height,

	 FbBits	    and,
	 FbBits	    xor);

#ifdef FB_24BIT
void
fbSolid24 (FbBits   *dst,
	   FbStride dstStride,
	   int	    dstX,

	   int	    width,
	   int	    height,

	   FbBits   and,
	   FbBits   xor);
#endif

/*
 * fbstipple.c
 */
void
fbEvenStipple (FbBits   *dst,
	       FbStride dstStride,
	       int	dstX,
	       int	dstBpp,

	       int	width,
	       int	height,

	       FbStip   *stip,
	       int	stipHeight,

	       FbBits   fgand,
	       FbBits   fgxor,
	       FbBits   bgand,
	       FbBits   bgxor,

	       int	xRot,
	       int	yRot);

void
fbOddStipple (FbBits	*dst,
	      FbStride	dstStride,
	      int	dstX,
	      int	dstBpp,

	      int	width,
	      int	height,

	      FbStip	*stip,
	      FbStride	stipStride,
	      int	stipWidth,
	      int	stipHeight,

	      FbBits	fgand,
	      FbBits	fgxor,
	      FbBits	bgand,
	      FbBits	bgxor,

	      int	xRot,
	      int	yRot);

void
fbStipple (FbBits   *dst,
	   FbStride dstStride,
	   int	    dstX,
	   int	    dstBpp,

	   int	    width,
	   int	    height,

	   FbStip   *stip,
	   FbStride stipStride,
	   int	    stipWidth,
	   int	    stipHeight,

	   FbBits   fgand,
	   FbBits   fgxor,
	   FbBits   bgand,
	   FbBits   bgxor,

	   int	    xRot,
	   int	    yRot);

/*
 * fbtile.c
 */

void
fbEvenTile (FbBits	*dst,
	    FbStride	dstStride,
	    int		dstX,

	    int		width,
	    int		height,

	    FbBits	*tile,
	    int		tileHeight,

	    int		alu,
	    FbBits	pm,
	    int		xRot,
	    int		yRot);

void
fbOddTile (FbBits	*dst,
	   FbStride	dstStride,
	   int		dstX,

	   int		width,
	   int		height,

	   FbBits	*tile,
	   FbStride	tileStride,
	   int		tileWidth,
	   int		tileHeight,

	   int		alu,
	   FbBits	pm,
	   int		bpp,
	   
	   int		xRot,
	   int		yRot);

void
fbTile (FbBits	    *dst,
	FbStride    dstStride,
	int	    dstX,

	int	    width,
	int	    height,

	FbBits	    *tile,
	FbStride    tileStride,
	int	    tileWidth,
	int	    tileHeight,
	
	int	    alu,
	FbBits	    pm,
	int	    bpp,
	
	int	    xRot,
	int	    yRot);

/*
 * fbutil.c
 */
FbBits
fbReplicatePixel (Pixel p, int bpp);

/*
 * fbwindow.c
 */

Bool
fbCreateWindow(WindowPtr pWin);

Bool
fbDestroyWindow(WindowPtr pWin);

Bool
fbMapWindow(WindowPtr pWindow);

Bool
fbPositionWindow(WindowPtr pWin, int x, int y);

Bool 
fbUnmapWindow(WindowPtr pWindow);
    
void
fbCopyWindowProc (DrawablePtr	pSrcDrawable,
		  DrawablePtr	pDstDrawable,
		  GCPtr		pGC,
		  BoxPtr	pbox,
		  int		nbox,
		  int		dx,
		  int		dy,
		  Bool		reverse,
		  Bool		upsidedown,
		  Pixel		bitplane,
		  void		*closure);

void 
fbCopyWindow(WindowPtr	    pWin, 
	     DDXPointRec    ptOldOrg, 
	     RegionPtr	    prgnSrc);

Bool
fbChangeWindowAttributes(WindowPtr pWin, unsigned long mask);

void
fbFillRegionSolid (DrawablePtr	pDrawable,
		   RegionPtr	pRegion,
		   FbBits	and,
		   FbBits	xor);

void
fbFillRegionTiled (DrawablePtr	pDrawable,
		   RegionPtr	pRegion,
		   PixmapPtr	pTile);

void
fbPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what);


#endif /* _FB_H_ */
