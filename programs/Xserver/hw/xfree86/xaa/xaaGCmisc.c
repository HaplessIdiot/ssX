/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaGCmisc.c,v 1.3 1998/07/31 10:41:30 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"
#include "migc.h"
#include "mi.h"
#include "gcstruct.h"
#include "pixmapstr.h"

void
XAAValidateCopyArea(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw )
{
   XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

   if(infoRec->CopyArea &&
	CHECK_PLANEMASK(pGC,infoRec->CopyAreaFlags) &&
	CHECK_ROP(pGC,infoRec->CopyAreaFlags)
	)
	pGC->ops->CopyArea = infoRec->CopyArea;
   else
	pGC->ops->CopyArea = XAAFallbackOps.CopyArea;
}

void
XAAValidatePutImage(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw )
{
   XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

   if(infoRec->PutImage &&
	CHECK_PLANEMASK(pGC,infoRec->PutImageFlags) &&
	CHECK_ROP(pGC,infoRec->PutImageFlags) &&
	CHECK_COLORS(pGC,infoRec->PutImageFlags)
	)
	pGC->ops->PutImage = infoRec->PutImage;
   else
	pGC->ops->PutImage = XAAFallbackOps.PutImage;
}

void
XAAValidateCopyPlane(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw )
{
   XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

   if(infoRec->CopyPlane &&
	CHECK_PLANEMASK(pGC,infoRec->CopyPlaneFlags) &&
	CHECK_ROP(pGC,infoRec->CopyPlaneFlags) &&
	CHECK_COLORS(pGC,infoRec->CopyPlaneFlags)
	)
	pGC->ops->CopyPlane = infoRec->CopyPlane;
   else
	pGC->ops->CopyPlane = XAAFallbackOps.CopyPlane;
}

void
XAAValidatePushPixels(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw )
{
   XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

   if(infoRec->PushPixelsSolid &&
	(pGC->fillStyle == FillSolid) &&
	CHECK_PLANEMASK(pGC,infoRec->PushPixelsFlags) &&
	CHECK_ROP(pGC,infoRec->PushPixelsFlags) &&
	CHECK_FG(pGC,infoRec->PushPixelsFlags) &&
	(!(infoRec->PushPixelsFlags & TRANSPARENCY_GXCOPY_ONLY) ||
	  (pGC->alu == GXcopy))
	)
	pGC->ops->PushPixels = infoRec->PushPixelsSolid;
   else
	pGC->ops->PushPixels = XAAFallbackOps.PushPixels;

}


/* We make the assumption that the FillSpans, PolyFillRect, FillPolygon
   and PolyFillArc functions are linked in a way that they all have 
   the same rop/color/planemask restrictions. If the driver provides 
   a GC level replacement for these, it will need to supply a new 
   Validate functions if it breaks this assumption */


void
XAAValidateFillSpans(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw )
{
   int type = -1;
   XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

   if(pGC->fillStyle != FillTiled) changes &= ~GCTile;
   if((pGC->fillStyle == FillTiled) || (pGC->fillStyle == FillSolid)) 
	changes &= ~GCStipple;
   if(!changes) return;
   

   pGC->ops->FillSpans = XAAFallbackOps.FillSpans;
   pGC->ops->PolyFillRect = XAAFallbackOps.PolyFillRect;
   pGC->ops->FillPolygon = XAAFallbackOps.FillPolygon;
   pGC->ops->PolyFillArc = XAAFallbackOps.PolyFillArc;

   switch(pGC->fillStyle){
   case FillSolid:
	if(infoRec->FillSpansSolid &&
		CHECK_PLANEMASK(pGC,infoRec->FillSpansSolidFlags) &&
		CHECK_ROP(pGC,infoRec->FillSpansSolidFlags) &&
		CHECK_FG(pGC,infoRec->FillSpansSolidFlags)
		) {
	     pGC->ops->FillSpans = infoRec->FillSpansSolid;
	     pGC->ops->PolyFillRect = infoRec->PolyFillRectSolid;
	     pGC->ops->FillPolygon = infoRec->FillPolygonSolid;
	     pGC->ops->PolyFillArc = infoRec->PolyFillArcSolid;
	} else return;
	break;
   case FillStippled:
        type = (*infoRec->StippledFillChooser)(pGC);
	break;
   case FillOpaqueStippled:
        type = (*infoRec->OpaqueStippledFillChooser)(pGC);
	break;
   case FillTiled:
        type = (*infoRec->TiledFillChooser)(pGC);
	break;
   default: return;
   }

   switch(type) {
   case -1: break;  /* solid rects */
   case 0: return;  /* using fallbacks */
   case DO_COLOR_8x8:
	pGC->ops->FillSpans = infoRec->FillSpansColor8x8Pattern;
	pGC->ops->PolyFillRect = infoRec->PolyFillRectColor8x8Pattern;
	break;
   case DO_MONO_8x8:
	pGC->ops->FillSpans = infoRec->FillSpansMono8x8Pattern;
	pGC->ops->PolyFillRect = infoRec->PolyFillRectMono8x8Pattern;
	pGC->ops->FillPolygon = infoRec->FillPolygonMono8x8Pattern;
	break;
   case DO_CACHE_BLT:
	pGC->ops->FillSpans = infoRec->FillSpansCacheBlt;
	pGC->ops->PolyFillRect = infoRec->PolyFillRectCacheBlt;
	pGC->ops->FillPolygon = infoRec->FillPolygonCacheBlt;
	break;
   case DO_COLOR_EXPAND:
	pGC->ops->FillSpans = infoRec->FillSpansColorExpand;
	pGC->ops->PolyFillRect = infoRec->PolyFillRectColorExpand;
	break;
   case DO_CACHE_EXPAND:
	pGC->ops->FillSpans = infoRec->FillSpansCacheExpand;
	pGC->ops->PolyFillRect = infoRec->PolyFillRectCacheExpand;
	pGC->ops->FillPolygon = infoRec->FillPolygonCacheExpand;
	break;
   default: return;
   }

   if(pGC->ops->FillPolygon == XAAFallbackOps.FillPolygon)
	pGC->ops->FillPolygon = miFillPolygon;
   if(pGC->ops->PolyFillArc == XAAFallbackOps.PolyFillArc)
	pGC->ops->PolyFillArc = miPolyFillArc;

}


/* We make the assumption that these Text8/16 and GlyphBlt functions
   are linked in a way that they all have the same rop/color/planemask
   restrictions. If the driver provides a GC level replacement for
   these, it will need to supply a new Validate functions if it breaks
   this assumption */

void
XAAValidatePolyGlyphBlt(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw )
{
   XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

   pGC->ops->PolyText8 = XAAFallbackOps.PolyText8;
   pGC->ops->PolyText16 = XAAFallbackOps.PolyText16;
   pGC->ops->PolyGlyphBlt = XAAFallbackOps.PolyGlyphBlt;


   if(pGC->fillStyle != FillSolid) return;

   /* no fonts wider than 32 pixels */
   if((FONTMAXBOUNDS(pGC->font, rightSideBearing) - 
	FONTMINBOUNDS(pGC->font, leftSideBearing) > 32))
	return;

   /* no funny business */
   if((FONTMINBOUNDS(pGC->font, characterWidth) <= 0) ||
	((FONTASCENT(pGC->font) + FONTDESCENT(pGC->font)) <= 0))
	return;

   /* Do I need to check that this font is 4 bytes wide?  Or is
	that guaranteed? */

   /* Check for TE Fonts */
   if(!TERMINALFONT(pGC->font)) {
	if(infoRec->PolyGlyphBltNonTE &&
	    CHECK_PLANEMASK(pGC,infoRec->PolyGlyphBltNonTEFlags) &&
	    CHECK_ROP(pGC,infoRec->PolyGlyphBltNonTEFlags) &&
	    CHECK_FG(pGC,infoRec->PolyGlyphBltNonTEFlags) &&
	    (!(infoRec->PolyGlyphBltNonTEFlags & TRANSPARENCY_GXCOPY_ONLY) ||
	  	(pGC->alu == GXcopy))
	) {
	    pGC->ops->PolyText8 = infoRec->PolyText8NonTE; 
	    pGC->ops->PolyText16 = infoRec->PolyText16NonTE;
	    pGC->ops->PolyGlyphBlt = infoRec->PolyGlyphBltNonTE; 
	}
   } else {
	if(infoRec->PolyGlyphBltTE &&
	    CHECK_PLANEMASK(pGC,infoRec->PolyGlyphBltTEFlags) &&
	    CHECK_ROP(pGC,infoRec->PolyGlyphBltTEFlags) &&
	    CHECK_FG(pGC,infoRec->PolyGlyphBltTEFlags) &&
	    (!(infoRec->PolyGlyphBltTEFlags & TRANSPARENCY_GXCOPY_ONLY) ||
	  	(pGC->alu == GXcopy))
	) {
	    pGC->ops->PolyText8 = infoRec->PolyText8TE;
	    pGC->ops->PolyText16 = infoRec->PolyText16TE;
	    pGC->ops->PolyGlyphBlt = infoRec->PolyGlyphBltTE;
	}
   }
}

void
XAAValidateImageGlyphBlt(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw )
{
   XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

   pGC->ops->ImageText8 = XAAFallbackOps.ImageText8;
   pGC->ops->ImageText16 = XAAFallbackOps.ImageText16;
   pGC->ops->ImageGlyphBlt = XAAFallbackOps.ImageGlyphBlt;

   /* no fonts wider than 32 pixels */
   if((FONTMAXBOUNDS(pGC->font, rightSideBearing) - 
	FONTMINBOUNDS(pGC->font, leftSideBearing) > 32))
	return;

   /* no funny business */
   if((FONTMINBOUNDS(pGC->font, characterWidth) <= 0) ||
	((FONTASCENT(pGC->font) + FONTDESCENT(pGC->font)) <= 0))
	return;

   /* Do I need to check that this font is 4 bytes wide?  Or is
	that guaranteed? */


   /* Check for TE Fonts */
   if(!TERMINALFONT(pGC->font)) {

	if(infoRec->ImageGlyphBltNonTE &&
		CHECK_PLANEMASK(pGC,infoRec->ImageGlyphBltNonTEFlags)
	) {
	    if ((!(infoRec->ImageGlyphBltMask & GCBackground) && 
		  CHECK_FG(pGC,infoRec->ImageGlyphBltNonTEFlags)) ||
		  CHECK_COLORS(pGC,infoRec->ImageGlyphBltNonTEFlags)) {
		pGC->ops->ImageText8 = infoRec->ImageText8NonTE;
		pGC->ops->ImageText16 = infoRec->ImageText16NonTE;
		pGC->ops->ImageGlyphBlt = infoRec->ImageGlyphBltNonTE;
	    }
	}
   } else {
	if(infoRec->ImageGlyphBltTE &&
		CHECK_PLANEMASK(pGC,infoRec->ImageGlyphBltTEFlags)
	) {
	    if ((!(infoRec->ImageGlyphBltMask & GCBackground) && 
		  CHECK_FG(pGC,infoRec->ImageGlyphBltTEFlags)) ||
		  CHECK_COLORS(pGC,infoRec->ImageGlyphBltTEFlags)) {
		  
		pGC->ops->ImageText8 = infoRec->ImageText8TE;
		pGC->ops->ImageText16 = infoRec->ImageText16TE;
		pGC->ops->ImageGlyphBlt = infoRec->ImageGlyphBltTE;
	    }
	}
   }
}


void
XAAValidatePolylines(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw )
{
   XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

   pGC->ops->PolySegment = XAAFallbackOps.PolySegment;
   pGC->ops->Polylines = XAAFallbackOps.Polylines;
   pGC->ops->PolyRectangle = XAAFallbackOps.PolyRectangle;
   pGC->ops->PolyArc = XAAFallbackOps.PolyArc;


   if((pGC->ops->FillSpans != XAAFallbackOps.FillSpans) &&
	(pGC->lineWidth > 0)){
	
	pGC->ops->PolyArc = miPolyArc;	
	pGC->ops->PolySegment = miPolySegment;	
	pGC->ops->PolyRectangle = miPolyRectangle;
	if(pGC->lineStyle == LineSolid)
	    pGC->ops->Polylines = miWideLine;
	else
	    pGC->ops->Polylines = miWideDash;
   }

   if(infoRec->PolyRectangleThinSolid &&
	(pGC->lineWidth == 0) &&
	(pGC->fillStyle == FillSolid) &&
	(pGC->lineStyle == LineSolid) &&
	CHECK_PLANEMASK(pGC,infoRec->PolyRectangleThinSolidFlags) &&
	CHECK_ROP(pGC,infoRec->PolyRectangleThinSolidFlags) &&
	CHECK_FG(pGC,infoRec->PolyRectangleThinSolidFlags)) {

	pGC->ops->PolyRectangle = infoRec->PolyRectangleThinSolid;
   }

   if(infoRec->PolylinesWideSolid &&
	(pGC->lineWidth > 0) &&
	(pGC->fillStyle == FillSolid) &&
	(pGC->lineStyle == LineSolid) &&
	CHECK_PLANEMASK(pGC,infoRec->PolylinesWideSolidFlags) &&
	CHECK_ROP(pGC,infoRec->PolylinesWideSolidFlags) &&
	CHECK_FG(pGC,infoRec->PolylinesWideSolidFlags)) {

	pGC->ops->Polylines = infoRec->PolylinesWideSolid;
   } 
}
