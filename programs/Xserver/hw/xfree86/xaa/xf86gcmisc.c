/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xf86gcmisc.c,v 3.17 1997/08/26 10:01:45 hohndel Exp $ */

/*
 * Copyright 1996  The XFree86 Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * HARM HANEMAAYER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Written by Harm Hanemaayer (H.Hanemaayer@inter.nl.net).
 */

/*
 * These functions are compiled seperately for:
 * cfb8
 * cfb16
 * cfb32
 * vga256 (banked 8bpp)
 *
 * They are called from xf86ValidateGC.
 *
 * Contents:
 *     xf86GCNewFillPolygon(GCPtr, new_cfb_line);
 *     xf86GCNewRectangle(GCPtr, new_cfb_line);
 *     xf86GCNewLine(GCPtr, DrawablePtr, new_cfb_line);
 *     xf86GCNewText(GCPtr, cfb_new_text);
 *     xf86GCNewFillSpans(GCPtr, cfb_new_fillspans);
 *     xf86GCNewFillArea(GCPtr, cfb_new_fillarea);
 *     xf86GCNewCopyArea(GCPtr);
 */

#include "gcstruct.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "mi.h"
#include "mispans.h"

#ifdef VGA256
/*
 * VGA256 is a special case. We don't use cfb for non-accelerated
 * functions, but instead use their vga256 equivalents.
 */
#include "vga256.h"
#include "vga256map.h"
#else
#include "cfb.h"
#include "cfbmskbits.h"
#endif

#include "xf86.h"
#include "xf86xaa.h"
#include "xf86gcmap.h"
#include "xf86maploc.h"
#include "xf86local.h"


#if PSZ == 8
# define useTEGlyphBlt  cfbTEGlyphBlt8
#else
# ifdef WriteBitGroup
#  define useTEGlyphBlt	cfbImageGlyphBlt8
# else
#  define useTEGlyphBlt	cfbImageGlyphBlt8 /* cfbTEGlyphBlt */ /* FIXME!!! [kmg] */
# endif
#endif

#ifdef WriteBitGroup
# define useImageGlyphBlt	cfbImageGlyphBlt8
# define usePolyGlyphBlt	cfbPolyGlyphBlt8
#else
# define useImageGlyphBlt	miImageGlyphBlt
# define usePolyGlyphBlt	miPolyGlyphBlt
#endif

/*
 *  	Every function that writes to the framebuffer directly
 *	will need a wrapper.  If it merely calls another pGC->ops
 *	function, it doesn't need one since the pGC->ops function
 *	that it calls will either be accelerated or already have
 * 	a wrapper.  Most mi routines and many of the cfb don't need
 *	wrappers.
 */ 

void xf86GCNewFillPolygon(pGC)
    GCPtr pGC;
{
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

#ifdef NO_ONE_RECT
    if(pGC->fillStyle == FillSolid) {
#else
    if (devPriv->oneRect && pGC->fillStyle == FillSolid) {
	if (xf86GCInfoRec.FillPolygonSolid &&
            CHECKPLANEMASK(xf86GCInfoRec.FillPolygonSolidFlags) &&
            CHECKROP(xf86GCInfoRec.FillPolygonSolidFlags) &&
            CHECKRGBEQUAL(xf86GCInfoRec.FillPolygonSolidFlags))
            pGC->ops->FillPolygon = xf86GCInfoRec.FillPolygonSolid;
	else
#endif
            pGC->ops->FillPolygon = xf86GCInfoRec.FillPolygonWrapper;
    } else
    	pGC->ops->FillPolygon = miFillPolygon;
}


void xf86GCNewRectangle(pGC)
    GCPtr pGC;
{
    if (xf86GCInfoRec.PolyRectangleSolidZeroWidth &&
        pGC->lineStyle == LineSolid &&
        pGC->fillStyle == FillSolid &&
        pGC->lineWidth == 0 &&
        CHECKPLANEMASK(xf86GCInfoRec.PolyRectangleSolidZeroWidthFlags) &&
        CHECKROP(xf86GCInfoRec.PolyRectangleSolidZeroWidthFlags) &&
        CHECKRGBEQUAL(xf86GCInfoRec.PolyRectangleSolidZeroWidthFlags))
        pGC->ops->PolyRectangle = xf86GCInfoRec.PolyRectangleSolidZeroWidth;
    else
    	pGC->ops->PolyRectangle = miPolyRectangle;
}

void xf86GCNewLine(pGC, pDrawable)
    GCPtr pGC;
    DrawablePtr pDrawable;
{
    void (*PolyLineFunc)();
    void (*PolySegmentFunc)();
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

    /* ARCS */
    if(pGC->lineWidth == 0) {
#ifdef PIXEL_ADDR
    	if ((pGC->lineStyle == LineSolid) && (pGC->fillStyle == FillSolid))
	    pGC->ops->PolyArc = xf86GCInfoRec.PolyArcWrapper;
	else
#endif
	   pGC->ops->PolyArc = miZeroPolyArc;
    } else
	pGC->ops->PolyArc = miPolyArc;
 

    /* LINES */
    if((pGC->lineWidth == 0) && (pGC->fillStyle == FillSolid)) { 
            PolySegmentFunc = xf86GCInfoRec.PolySegmentWrapper;
            PolyLineFunc = xf86GCInfoRec.PolyLinesWrapper;
    } else {
            PolySegmentFunc = miPolySegment;

	    if(pGC->lineStyle == LineSolid) {
		if(pGC->lineWidth == 0)
             	    PolyLineFunc = miZeroLine;
		else
           	    PolyLineFunc = miWideLine;
	    } else
            	PolyLineFunc = miWideDash;
    }

    if((pGC->fillStyle == FillSolid)  && (pGC->lineWidth == 0)) {
      if(pGC->lineStyle == LineSolid) {
  	if(xf86GCInfoRec.PolyLineSolidZeroWidth &&
#if !defined(NO_ONE_RECT)
	  (!(xf86GCInfoRec.PolyLineSolidZeroWidthFlags & ONE_RECT_CLIPPING)
	  || devPriv->oneRect) &&
#endif
          CHECKPLANEMASK(xf86GCInfoRec.PolyLineSolidZeroWidthFlags) &&
          CHECKROP(xf86GCInfoRec.PolyLineSolidZeroWidthFlags) &&
          CHECKRGBEQUAL(xf86GCInfoRec.PolyLineSolidZeroWidthFlags))
              PolyLineFunc = xf86GCInfoRec.PolyLineSolidZeroWidth;


        if (xf86GCInfoRec.PolySegmentSolidZeroWidth &&
#if !defined(NO_ONE_RECT)
	  (!(xf86GCInfoRec.PolyLineSolidZeroWidthFlags & ONE_RECT_CLIPPING)
	  || devPriv->oneRect) &&
#endif
          CHECKPLANEMASK(xf86GCInfoRec.PolySegmentSolidZeroWidthFlags) &&
          CHECKROP(xf86GCInfoRec.PolySegmentSolidZeroWidthFlags) &&
          CHECKRGBEQUAL(xf86GCInfoRec.PolySegmentSolidZeroWidthFlags) &&
          (pGC->capStyle != CapNotLast || 
          !(xf86GCInfoRec.PolySegmentSolidZeroWidthFlags & NO_CAP_NOT_LAST)))
        	PolySegmentFunc = xf86GCInfoRec.PolySegmentSolidZeroWidth;

     } else {
 	if(xf86GCInfoRec.PolyLineDashedZeroWidth &&
#if !defined(NO_ONE_RECT)
	   (!(xf86GCInfoRec.PolyLineDashedZeroWidthFlags & ONE_RECT_CLIPPING)
	   || devPriv->oneRect) &&
#endif
           CHECKPLANEMASK(xf86GCInfoRec.PolyLineDashedZeroWidthFlags) &&
           CHECKROP(xf86GCInfoRec.PolyLineDashedZeroWidthFlags) &&
           CHECKRGBEQUAL(xf86GCInfoRec.PolyLineDashedZeroWidthFlags))
               PolyLineFunc = xf86GCInfoRec.PolyLineDashedZeroWidth;


        if (xf86GCInfoRec.PolySegmentDashedZeroWidth &&
#if !defined(NO_ONE_RECT)
	  (!(xf86GCInfoRec.PolySegmentDashedZeroWidthFlags & ONE_RECT_CLIPPING)
	  || devPriv->oneRect) &&
#endif
          CHECKPLANEMASK(xf86GCInfoRec.PolySegmentDashedZeroWidthFlags) &&
          CHECKROP(xf86GCInfoRec.PolySegmentDashedZeroWidthFlags) &&
          CHECKRGBEQUAL(xf86GCInfoRec.PolySegmentDashedZeroWidthFlags) &&
          (pGC->capStyle != CapNotLast || 
          !(xf86GCInfoRec.PolySegmentDashedZeroWidthFlags & NO_CAP_NOT_LAST)))
        	PolySegmentFunc = xf86GCInfoRec.PolySegmentDashedZeroWidth;


      }
    }
#if !defined(NO_ONE_RECT) 
    else if((pGC->fillStyle == FillSolid) && (pGC->lineStyle == LineSolid)
	&& miSpansEasyRop(pGC->alu)) {
	if (xf86AccelInfoRec.SubsequentFillRectSolid && devPriv->oneRect &&
            CHECKPLANEMASK(xf86GCInfoRec.PolyFillRectSolidFlags) &&
            CHECKROP(xf86GCInfoRec.PolyFillRectSolidFlags) &&
            CHECKRGBEQUAL(xf86GCInfoRec.PolyFillRectSolidFlags))
            PolyLineFunc = xf86WideLineSolid1Rect;

    }
#endif

    pGC->ops->Polylines = PolyLineFunc;
    pGC->ops->PolySegment = PolySegmentFunc;
}

void
xf86GCNewText(pGC)
    GCPtr pGC;
{
    void (*PolyGlyphBltFunc) () = miPolyGlyphBlt;
    void (*ImageGlyphBltFunc) () = miImageGlyphBlt;
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

    /* This is the MatchCommon logic. */
 
    if(!(FONTMAXBOUNDS(pGC->font, rightSideBearing) -
	FONTMINBOUNDS(pGC->font, leftSideBearing) > 32 ||
	FONTMINBOUNDS(pGC->font, characterWidth) < 0)) {
	if (TERMINALFONT(pGC->font)
#ifdef FOUR_BIT_CODE
	    && FONTMAXBOUNDS(pGC->font,characterWidth) >= PGSZB
#endif
	) {
#ifdef WriteBitGroup
	   PolyGlyphBltFunc = xf86GCInfoRec.PolyGlyphBltWrapper;
 	   ImageGlyphBltFunc = cfbImageGlyphBlt8;
#else
 	   ImageGlyphBltFunc = xf86GCInfoRec.ImageGlyphBltWrapper;
#endif
	} else {
#ifdef WriteBitGroup
	   PolyGlyphBltFunc = xf86GCInfoRec.PolyGlyphBltWrapper;
	   ImageGlyphBltFunc = cfbImageGlyphBlt8;
#endif
	}
    }

    /* Replace with accelerated functions if possible. */
    if (!TERMINALFONT(pGC->font)) {
	if (xf86GCInfoRec.PolyGlyphBltNonTE
	    && pGC->fillStyle == FillSolid
	    && CHECKPLANEMASK(xf86GCInfoRec.PolyGlyphBltNonTEFlags)
            && CHECKROP(xf86GCInfoRec.PolyGlyphBltNonTEFlags)
            && CHECKSOURCEROP()
            && CHECKRGBEQUAL(xf86GCInfoRec.PolyGlyphBltNonTEFlags))
	    PolyGlyphBltFunc = xf86GCInfoRec.PolyGlyphBltNonTE;
	if (xf86GCInfoRec.ImageGlyphBltNonTE
	    && pGC->fillStyle == FillSolid
	    && CHECKPLANEMASK(xf86GCInfoRec.ImageGlyphBltNonTEFlags)
            && CHECKROP(xf86GCInfoRec.ImageGlyphBltNonTEFlags)
            && CHECKSOURCEROP()
            && CHECKRGBEQUAL(xf86GCInfoRec.ImageGlyphBltNonTEFlags)
	    && CHECKRGBEQUALBG(xf86GCInfoRec.PolyFillRectSolidFlags))
	    ImageGlyphBltFunc = xf86GCInfoRec.ImageGlyphBltNonTE;
    } else {	/* TERMINALFONT(pGC->font) */
	if (xf86GCInfoRec.PolyGlyphBltTE
	    && pGC->fillStyle == FillSolid
	    && CHECKPLANEMASK(xf86GCInfoRec.PolyGlyphBltTEFlags)
	    && CHECKROP(xf86GCInfoRec.PolyGlyphBltTEFlags)
            && CHECKSOURCEROP()
            && CHECKRGBEQUAL(xf86GCInfoRec.PolyGlyphBltTEFlags))
	    PolyGlyphBltFunc = xf86GCInfoRec.PolyGlyphBltTE;
	if (xf86GCInfoRec.ImageGlyphBltTE
	    && CHECKPLANEMASK(xf86GCInfoRec.ImageGlyphBltTEFlags)
	    && CHECKROP(xf86GCInfoRec.ImageGlyphBltTEFlags)
            && CHECKSOURCEROP()
	    && CHECKRGBEQUALBOTH(xf86GCInfoRec.ImageGlyphBltTEFlags))
	    ImageGlyphBltFunc = xf86GCInfoRec.ImageGlyphBltTE;
    }

    pGC->ops->ImageGlyphBlt = ImageGlyphBltFunc;
    pGC->ops->PolyGlyphBlt = PolyGlyphBltFunc;
}


void
xf86GCNewFillSpans(pGC)
    GCPtr pGC;
{
    void (*FillSpansFunc) ();


    /* Non-accelerated defaults. */
    FillSpansFunc = xf86GCInfoRec.FillSpansWrapper;

    /* Replace with accelerated functions if possible. */
    switch (pGC->fillStyle) {
    case FillSolid:
        if (!xf86GCInfoRec.FillSpansSolid) 
	    break;
	if (!CHECKPLANEMASK(xf86GCInfoRec.FillSpansSolidFlags))
	    break;
	if (!CHECKROP(xf86GCInfoRec.FillSpansSolidFlags))
	    break;
	if (!CHECKRGBEQUAL(xf86GCInfoRec.FillSpansSolidFlags))
	    break;
	    FillSpansFunc = xf86GCInfoRec.FillSpansSolid;
	break;
    case FillTiled:
        if (!xf86GCInfoRec.FillSpansTiled)
            break;
	if (!CHECKPLANEMASK(xf86GCInfoRec.FillSpansTiledFlags))
	    break;
	if (!CHECKROP(xf86GCInfoRec.FillSpansTiledFlags))
	    break;
	FillSpansFunc = xf86GCInfoRec.FillSpansTiled;
	break;
    case FillStippled:
        if (!xf86GCInfoRec.FillSpansStippled)
            break;
	if (!CHECKPLANEMASK(xf86GCInfoRec.FillSpansStippledFlags))
	    break;
	if (!CHECKROP(xf86GCInfoRec.FillSpansStippledFlags))
	    break;
        if (!CHECKSOURCEROP())
	    break;
	FillSpansFunc = xf86GCInfoRec.FillSpansStippled;
	break;
    case FillOpaqueStippled:
        if (!xf86GCInfoRec.FillSpansOpaqueStippled)
            break;
	if (!CHECKPLANEMASK(xf86GCInfoRec.FillSpansOpaqueStippledFlags))
	    break;
	if (!CHECKROP(xf86GCInfoRec.FillSpansOpaqueStippledFlags))
	    break;
        if (!CHECKSOURCEROP())
	    break;
	FillSpansFunc = xf86GCInfoRec.FillSpansOpaqueStippled;
	break;
    }
    pGC->ops->FillSpans = FillSpansFunc;
}


void
xf86GCNewFillArea(pGC)
    GCPtr pGC;
{
    void (*PolyFillRectFunc) ();
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

#ifdef FOUR_BIT_CODE
    PolyFillRectFunc = xf86GCInfoRec.PolyFillRectWrapper;
#else
    if ((pGC->fillStyle == FillSolid) || (pGC->fillStyle == FillTiled)) 
    	PolyFillRectFunc = xf86GCInfoRec.PolyFillRectWrapper;
    else 
    	PolyFillRectFunc = miPolyFillRect;
#endif


    /* Now check if we can replace the operation. */
    switch (pGC->fillStyle) {
    case FillSolid:
        if (!xf86GCInfoRec.PolyFillRectSolid)
            break;
	if (!CHECKPLANEMASK(xf86GCInfoRec.PolyFillRectSolidFlags))
	    break;
        if (!CHECKROP(xf86GCInfoRec.PolyFillRectSolidFlags))
	    break;
	if (!CHECKRGBEQUAL(xf86GCInfoRec.PolyFillRectSolidFlags))
	    break;
	/* There is a replacement for PolyFillRect, with this rop. */
	PolyFillRectFunc = xf86GCInfoRec.PolyFillRectSolid;
	break;
    case FillTiled:
    	if (!xf86GCInfoRec.PolyFillRectTiled)
    	    break;
	if (!CHECKPLANEMASK(xf86GCInfoRec.PolyFillRectTiledFlags))
	    break;
	if (!CHECKROP(xf86GCInfoRec.PolyFillRectTiledFlags))
	    break;
        if (!CHECKSOURCEROP())
	    break;
	PolyFillRectFunc = xf86GCInfoRec.PolyFillRectTiled;
	break;
    case FillStippled:
    	if (!xf86GCInfoRec.PolyFillRectStippled)
    	    break;
	if (!CHECKPLANEMASK(xf86GCInfoRec.PolyFillRectStippledFlags))
	    break;
	if (!CHECKROP(xf86GCInfoRec.PolyFillRectStippledFlags))
	    break;
        if (!CHECKSOURCEROP())
	    break;
	PolyFillRectFunc = xf86GCInfoRec.PolyFillRectStippled;
	break;
    case FillOpaqueStippled:
    	if (!xf86GCInfoRec.PolyFillRectOpaqueStippled)
    	    break;
	if (!CHECKPLANEMASK(xf86GCInfoRec.PolyFillRectOpaqueStippledFlags))
	    break;
	if (!CHECKROP(xf86GCInfoRec.PolyFillRectOpaqueStippledFlags))
            break;
        if (!CHECKSOURCEROP())
	    break;
	PolyFillRectFunc = xf86GCInfoRec.PolyFillRectOpaqueStippled;
	break;
    }
    pGC->ops->PolyFillRect = PolyFillRectFunc;


    if (pGC->fillStyle == FillSolid) {
        if (xf86GCInfoRec.PolyFillArcSolid &&
            CHECKPLANEMASK(xf86GCInfoRec.PolyFillArcSolidFlags) &&
            CHECKROP(xf86GCInfoRec.PolyFillArcSolidFlags) &&
            CHECKRGBEQUAL(xf86GCInfoRec.PolyFillArcSolidFlags))
	    pGC->ops->PolyFillArc = xf86GCInfoRec.PolyFillArcSolid;
	else 
    	    pGC->ops->PolyFillArc = xf86GCInfoRec.PolyFillArcWrapper;
    } else 
    	pGC->ops->PolyFillArc = miPolyFillArc;
}


void
xf86GCNewCopyArea(pGC)
    GCPtr pGC;
{
    if (xf86GCInfoRec.CopyArea &&
	CHECKPLANEMASK(xf86GCInfoRec.CopyAreaFlags) &&
	CHECKROP(xf86GCInfoRec.CopyAreaFlags) &&
	CHECKSOURCEROP()
	)
	pGC->ops->CopyArea = xf86GCInfoRec.CopyArea;
    else
    	pGC->ops->CopyArea = xf86GCInfoRec.CopyAreaWrapper;
}


void xf86ImageGlyphBltFallBack(pDrawable, pGC, xInit, yInit, nglyph, ppci,
pglyphBase)
    DrawablePtr pDrawable;
    GCPtr pGC;
    int xInit, yInit;
    int nglyph;
    CharInfoPtr *ppci;		       /* array of character info */
    unsigned char *pglyphBase;	       /* start of array of glyphs */
{
    void (*ImageGlyphBltFunc) ();
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);


    /* Simulate the MatchCommon logic. */
    if (pGC->fillStyle == FillSolid && devPriv->rop == GXcopy
    && 	FONTMAXBOUNDS(pGC->font,rightSideBearing) -
        FONTMINBOUNDS(pGC->font,leftSideBearing) <= 32 &&
	FONTMINBOUNDS(pGC->font,characterWidth) >= 0) {

	if (TERMINALFONT(pGC->font)
#ifdef FOUR_BIT_CODE
	    && FONTMAXBOUNDS(pGC->font,characterWidth) >= PGSZB
#endif
	) 
	    ImageGlyphBltFunc = useTEGlyphBlt;
	else
	    ImageGlyphBltFunc = useImageGlyphBlt;
    } else 
    if (FONTMAXBOUNDS(pGC->font, rightSideBearing) -
	FONTMINBOUNDS(pGC->font, leftSideBearing) > 32 ||
	FONTMINBOUNDS(pGC->font, characterWidth) < 0) {
	ImageGlyphBltFunc = miImageGlyphBlt;
    } else {
	/* special case ImageGlyphBlt for terminal emulator fonts */
#if !defined(WriteBitGroup) || PSZ == 8
	if (TERMINALFONT(pGC->font) &&
	    (pGC->planemask & PMSK) == PMSK
#ifdef FOUR_BIT_CODE
	    && FONTMAXBOUNDS(pGC->font, characterWidth) >= PGSZB
#endif
	    ) {
	    ImageGlyphBltFunc = useTEGlyphBlt;
	} else
#endif
	{
#ifdef WriteBitGroup
	    if (devPriv->rop == GXcopy &&
		pGC->fillStyle == FillSolid &&
		(pGC->planemask & PMSK) == PMSK)
		ImageGlyphBltFunc = cfbImageGlyphBlt8;
	    else
#endif
		ImageGlyphBltFunc = miImageGlyphBlt;
	}
    }

    /* Not all of these functions require a sync so it may be good	
	to go through this more thoroughly */
    SYNC_CHECK;

    (*ImageGlyphBltFunc)(pDrawable, pGC, xInit, yInit, nglyph, ppci,
        pglyphBase);
}

void xf86PolyGlyphBltFallBack(pDrawable, pGC, xInit, yInit, nglyph, ppci,
pglyphBase)
    DrawablePtr pDrawable;
    GCPtr pGC;
    int xInit, yInit;
    int nglyph;
    CharInfoPtr *ppci;		       /* array of character info */
    unsigned char *pglyphBase;	       /* start of array of glyphs */
{
    void (*PolyGlyphBltFunc) ();
    cfbPrivGCPtr devPriv;

    devPriv = cfbGetGCPrivate(pGC);


    /* Simulate the MatchCommon logic. */
    if (pGC->fillStyle == FillSolid && devPriv->rop == GXcopy
    && 	FONTMAXBOUNDS(pGC->font,rightSideBearing) -
        FONTMINBOUNDS(pGC->font,leftSideBearing) <= 32 &&
	FONTMINBOUNDS(pGC->font,characterWidth) >= 0
#ifdef FOUR_BIT_CODE
	&& FONTMAXBOUNDS(pGC->font,characterWidth) >= PGSZB
#endif
	) 
        PolyGlyphBltFunc = usePolyGlyphBlt;
    else if (FONTMAXBOUNDS(pGC->font, rightSideBearing) -
	FONTMINBOUNDS(pGC->font, leftSideBearing) > 32 ||
	FONTMINBOUNDS(pGC->font, characterWidth) < 0) {
	PolyGlyphBltFunc = miPolyGlyphBlt;
    } else {
#ifdef WriteBitGroup
	if (pGC->fillStyle == FillSolid) {
	    if (devPriv->rop == GXcopy) 
		PolyGlyphBltFunc = cfbPolyGlyphBlt8;
	    else
#ifdef FOUR_BIT_CODE
		PolyGlyphBltFunc = cfbPolyGlyphRop8;
#else
		PolyGlyphBltFunc = miPolyGlyphBlt;
#endif
	} else
#endif
	    PolyGlyphBltFunc = miPolyGlyphBlt;
    }

    /* Not all of these functions require a sync so it may be good	
	to go through this more thoroughly */
    SYNC_CHECK;

    (*PolyGlyphBltFunc)(pDrawable, pGC, xInit, yInit, nglyph, ppci,
        pglyphBase);
}

void xf86FillSpansFallBack(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GC		*pGC;
    int		nInit;		/* number of spans to fill */
    DDXPointPtr pptInit;	/* pointer to list of start points */
    int *pwidthInit;		/* pointer to list of n widths */
    int fSorted;
{
    void (*FillSpansFunc) ();
    cfbPrivGCPtr devPriv;


    devPriv = cfbGetGCPrivate(pGC);
    if (pGC->fillStyle == FillSolid && devPriv->rop == GXcopy)
         /* Simulate the MatchCommon logic. */
         FillSpansFunc = cfbSolidSpansCopy;
    else {
        switch (pGC->fillStyle) {
        case FillSolid:
	    switch (devPriv->rop) {
	    case GXcopy:
	        FillSpansFunc = cfbSolidSpansCopy;
	        break;
	    case GXxor:
	        FillSpansFunc = cfbSolidSpansXor;
	        break;
	    default:
	        FillSpansFunc = cfbSolidSpansGeneral;
	        break;
	    }
	    break;
        case FillTiled:
	    if (devPriv->pRotatedPixmap) {
	        if (pGC->alu == GXcopy && (pGC->planemask & PMSK) == PMSK)
		    FillSpansFunc = cfbTile32FSCopy;
	        else
		    FillSpansFunc = cfbTile32FSGeneral;
	    } else
	        FillSpansFunc = cfbUnnaturalTileFS;
	    break;
        case FillStippled:
#ifdef FOUR_BIT_CODE
	    if (devPriv->pRotatedPixmap)
	        FillSpansFunc = cfb8Stipple32FS;
	    else
#endif
	        FillSpansFunc = cfbUnnaturalStippleFS;
	    break;
        case FillOpaqueStippled:
#ifdef FOUR_BIT_CODE
	    if (devPriv->pRotatedPixmap)
	        FillSpansFunc = cfb8OpaqueStipple32FS;
	    else 
#endif
	        FillSpansFunc = cfbUnnaturalStippleFS;
	    break;
        }
    }
    
    SYNC_CHECK;

    (*FillSpansFunc)(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
}

void xf86FillRectTileFallBack(pDrawable, pGC, nBoxInit, pBoxInit)
    DrawablePtr pDrawable;
    register GCPtr pGC;
    int		nBoxInit;	/* number of rectangles to fill */
    BoxPtr	pBoxInit;  	/* Pointer to first rectangle to fill */
{
    void (*FillRectTileFunc) ();
    cfbPrivGCPtr devPriv;

    devPriv = cfbGetGCPrivate(pGC);
    if (!devPriv->pRotatedPixmap)
        FillRectTileFunc = cfbFillRectTileOdd;
    else {
	if (pGC->alu == GXcopy && (pGC->planemask & PMSK) == PMSK)
	    FillRectTileFunc = cfbFillRectTile32Copy;
	else
	    FillRectTileFunc =
	        cfbFillRectTile32General;
    }

    SYNC_CHECK;

    (*FillRectTileFunc)(pDrawable, pGC, nBoxInit, pBoxInit);
}

void xf86FillRectStippledFallBack(pDrawable, pGC, nBoxInit, pBoxInit)
    DrawablePtr pDrawable;
    register GCPtr pGC;
    int		nBoxInit;	/* number of rectangles to fill */
    BoxPtr	pBoxInit;  	/* Pointer to first rectangle to fill */
{
#if PSZ == 8
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

    SYNC_CHECK;

    if (!devPriv->pRotatedPixmap)
	cfb8FillRectStippledUnnatural(pDrawable, pGC, nBoxInit, pBoxInit);
    else
	cfb8FillRectTransparentStippled32(pDrawable, pGC, nBoxInit, pBoxInit);

#else
    xf86miFillRectStippledFallBack(pDrawable, pGC, nBoxInit, pBoxInit);
#endif
}

void xf86FillRectOpaqueStippledFallBack(pDrawable, pGC, nBoxInit, pBoxInit)
    DrawablePtr pDrawable;
    register GCPtr pGC;
    int		nBoxInit;	/* number of rectangles to fill */
    BoxPtr	pBoxInit;  	/* Pointer to first rectangle to fill */
{
#if PSZ == 8
    cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

    SYNC_CHECK;

    if (!devPriv->pRotatedPixmap)
	cfb8FillRectStippledUnnatural(pDrawable, pGC, nBoxInit, pBoxInit);
    else
	cfb8FillRectOpaqueStippled32(pDrawable, pGC, nBoxInit, pBoxInit);

#else
    xf86miFillRectStippledFallBack(pDrawable, pGC, nBoxInit, pBoxInit);
#endif
}
