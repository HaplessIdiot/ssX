/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaNonTEText.c,v 1.4 1998/09/13 05:23:55 dawes Exp $ */

/********************************************************************

   In this file we have GC level replacements for PolyText8/16,
   ImageText8/16, ImageGlyphBlt and PolyGlyphBlt for NonTE (proportional) 
   fonts. The idea is that everything in this file is device independent.
   The mentioned GCOps are merely wrappers for the 
   PolyGlyphBltNonTEColorExpansion and ImageGlyphBltNonTEColorExpansion
   functions which calculate the boxes containing arbitrarily clipped 
   text and passes them to the NonTEGlyphRenderer which will usually 
   be a lower level XAA function which renders these clipped glyphs using
   the basic color expansion functions exported by the chipset driver.
   The NonTEGlyphRenderer itself may optionally be driver supplied to
   facilitate work-arounds/optimizations at a higher level than usual.

   Written by Mark Vojkovich (mvojkovi@ucsd.edu)

********************************************************************/

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "font.h"
#include "scrnintstr.h"
#include "dixfontstr.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"
#include "gcstruct.h"
#include "pixmapstr.h"


static void ImageGlyphBltNonTEColorExpansion(ScrnInfoPtr pScrn,
				int xInit, int yInit, FontPtr font,
				int fg, int bg, int rop, unsigned planemask,
				RegionPtr cclip, int nglyph,
				unsigned char* gBase, CharInfoPtr *ppci);
static int PolyGlyphBltNonTEColorExpansion(ScrnInfoPtr pScrn,
				int xInit, int yInit, FontPtr font,
				int fg, int rop, unsigned planemask,
				RegionPtr cclip, int nglyph,
				unsigned char* gBase, CharInfoPtr *ppci);

/********************************************************************

   GC level replacements for PolyText8/16 and ImageText8/16
   for NonTE fonts when using color expansion.

********************************************************************/


int
XAAPolyText8NonTEColorExpansion(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x, 
    int 	y,
    int 	count,
    char	*chars )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    unsigned long n;
    int width = 0;

    (*pGC->font->get_glyphs)(pGC->font, (unsigned long)count, 
		(unsigned char *)chars, Linear8Bit, &n, infoRec->CharInfo);

    if(n) {
	width = PolyGlyphBltNonTEColorExpansion( infoRec->pScrn, 
		x + pDraw->x, y + pDraw->y, pGC->font, 
		pGC->fgPixel, pGC->alu, pGC->planemask, 
		pGC->pCompositeClip, n, FONTGLYPHS(pGC->font),
 		infoRec->CharInfo);
    }

    return (x + width);
}


int
XAAPolyText16NonTEColorExpansion(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x, 
    int		y,
    int		count,
    unsigned short *chars )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    unsigned long n;
    int width = 0;

    (*pGC->font->get_glyphs)(
		pGC->font, (unsigned long)count, (unsigned char *)chars,
		(FONTLASTROW(pGC->font) == 0) ? Linear16Bit : TwoD16Bit,
		&n, infoRec->CharInfo);

    if(n) {
	width = PolyGlyphBltNonTEColorExpansion( infoRec->pScrn, 
		x + pDraw->x, y + pDraw->y, pGC->font, 
		pGC->fgPixel, pGC->alu, pGC->planemask, 
		pGC->pCompositeClip, n, FONTGLYPHS(pGC->font),
		infoRec->CharInfo);
    }

    return (x + width);
}


void
XAAImageText8NonTEColorExpansion(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x, 
    int		y,
    int		count,
    char	*chars )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    unsigned long n;

    (*pGC->font->get_glyphs)(pGC->font, (unsigned long)count, 
		(unsigned char *)chars, Linear8Bit, &n, infoRec->CharInfo);

    if(n) ImageGlyphBltNonTEColorExpansion(
	infoRec->pScrn, x + pDraw->x, y + pDraw->y,
	pGC->font, pGC->fgPixel, pGC->bgPixel, GXcopy, pGC->planemask, 
	pGC->pCompositeClip, n, FONTGLYPHS(pGC->font), infoRec->CharInfo);
}


void
XAAImageText16NonTEColorExpansion(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x, 
    int		y,
    int		count,
    unsigned short *chars )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    unsigned long n;

    (*pGC->font->get_glyphs)(
		pGC->font, (unsigned long)count, (unsigned char *)chars,
		(FONTLASTROW(pGC->font) == 0) ? Linear16Bit : TwoD16Bit,
		&n, infoRec->CharInfo);

    if(n) ImageGlyphBltNonTEColorExpansion(
	infoRec->pScrn, x + pDraw->x, y + pDraw->y,
	pGC->font, pGC->fgPixel, pGC->bgPixel, GXcopy, pGC->planemask, 
	pGC->pCompositeClip, n, FONTGLYPHS(pGC->font), infoRec->CharInfo);
}



/********************************************************************

   GC level replacements for ImageGlyphBlt and PolyGlyphBlt for
   NonTE fonts when using color expansion.

********************************************************************/


void
XAAImageGlyphBltNonTEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int xInit, int yInit,
    unsigned int nglyph,
    CharInfoPtr *ppci,      /* array of character info */
    pointer pglyphBase	       /* start of array of glyphs */
)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

    ImageGlyphBltNonTEColorExpansion(
	infoRec->pScrn, xInit + pDraw->x, yInit + pDraw->y,
	pGC->font, pGC->fgPixel, pGC->bgPixel, GXcopy, pGC->planemask, 
	pGC->pCompositeClip, nglyph, (unsigned char*)pglyphBase, ppci);
}

void
XAAPolyGlyphBltNonTEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int xInit, int yInit,
    unsigned int nglyph,
    CharInfoPtr *ppci,      /* array of character info */
    pointer pglyphBase	       /* start of array of glyphs */
)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

    PolyGlyphBltNonTEColorExpansion(
	infoRec->pScrn, xInit + pDraw->x, yInit + pDraw->y,
	pGC->font, pGC->fgPixel, pGC->alu, pGC->planemask, 
	pGC->pCompositeClip, nglyph, (unsigned char*)pglyphBase, ppci);
}




/********************************************************************

   ImageGlyphBltNonTEColorExpansion -
   PolyGlyphBltNonTEColorExpansion -

   These guys compute the clipped pieces of text and send it to
   the lower-level function which will handle acceleration of 
   arbitrarily clipped text.
  
********************************************************************/



static int
CollectCharacterInfo(
    NonTEGlyphPtr glyphs,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    FontPtr pfont
){
   int i, w = 0;
   
   for(i = 0; i < nglyph; i++, ppci++, glyphs++) {
	glyphs->bits = (unsigned char*)((*ppci)->bits);
	glyphs->start = w + (*ppci)->metrics.leftSideBearing;
	glyphs->end = w + (*ppci)->metrics.rightSideBearing;
	glyphs->yoff = (*ppci)->metrics.ascent;
	glyphs->height = glyphs->yoff + (*ppci)->metrics.descent;
	glyphs->srcwidth = ((glyphs->end - glyphs->start + 31) >> 5) << 2;
	w += (*ppci)->metrics.characterWidth;
   }
   return w;
}


static void
ImageGlyphBltNonTEColorExpansion(
   ScrnInfoPtr pScrn,
   int xInit, int yInit,
   FontPtr font,
   int fg, int bg, int rop,
   unsigned planemask,
   RegionPtr cclip,
   int nglyph,
   unsigned char* gBase,
   CharInfoPtr *ppci 
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int skippix, skipglyph, width, n, i;
    int LeftText, RightText, TopText, BottomText;
    int LeftBox, RightBox, TopBox, BottomBox;
    int Left, Right, Top, Bottom;
    int LeftEdge, RightEdge, ytop, ybot;
    int nbox = REGION_NUM_RECTS(cclip);
    BoxPtr pbox = REGION_RECTS(cclip);

    width = CollectCharacterInfo(infoRec->GlyphInfo, nglyph, ppci, font);

    /* compute an approximate but covering bounding box */
    LeftText = xInit + infoRec->GlyphInfo[0].start;
    RightText = xInit + infoRec->GlyphInfo[nglyph - 1].end;
    TopText = yInit - FONTMAXBOUNDS(font,ascent);
    BottomText = yInit + FONTMAXBOUNDS(font,descent);

    /* find our backing rectangle dimensions */
    LeftBox = xInit;
    RightBox = LeftBox + width;
    TopBox = yInit - FONTASCENT(font);
    BottomBox = yInit + FONTDESCENT(font);

    Left = min(LeftText, LeftBox);
    Right = max(RightText, RightBox);
    Top = min(TopText, TopBox);
    Bottom = max(BottomText, BottomBox);
 
    /* get into the first band that may contain part of our string */
    while(nbox && (Top >= pbox->y2)) {
	pbox++; nbox--;
    }

    /* stop when the lower edge of the box is beyond our string */
    while(nbox && (Bottom >= pbox->y1)) {

	/* handle backing rect first */
	LeftEdge = max(LeftBox, pbox->x1);
	RightEdge = min(RightBox, pbox->x2);
	if(RightEdge > LeftEdge) {	    
	    ytop = max(TopBox, pbox->y1);
	    ybot = min(BottomBox, pbox->y2);

	    if(ybot > ytop) {
		(*infoRec->SetupForSolidFill)(pScrn, bg, rop, planemask);
		(*infoRec->SubsequentSolidFillRect)(pScrn, 
			LeftEdge, ytop, RightEdge - LeftEdge, ybot - ytop);
	    }
	}

	/* Now do the text */
	LeftEdge = max(LeftText, pbox->x1);
	RightEdge = min(RightText, pbox->x2);

	if(RightEdge > LeftEdge) { /* we're possibly drawing something */
	    ytop = max(TopText, pbox->y1);
	    ybot = min(BottomText, pbox->y2);
	    if(ybot > ytop) {
		skippix = LeftEdge - xInit;
		skipglyph = 0;
		while(skippix >= infoRec->GlyphInfo[skipglyph].end)
		   skipglyph++;

		skippix = RightEdge - xInit;
		n = 0; i = skipglyph;
		while((i < nglyph) && (skippix > infoRec->GlyphInfo[i].start)) {
		    i++; n++;
		}

		if(n) (*infoRec->NonTEGlyphRenderer)(pScrn,
			xInit, yInit, n, infoRec->GlyphInfo + skipglyph, 
			pbox, fg, rop, planemask); 
	    }
	}

	nbox--; pbox++;
    }
}


static int
PolyGlyphBltNonTEColorExpansion(
   ScrnInfoPtr pScrn,
   int xInit, int yInit,
   FontPtr font,
   int fg, int rop,
   unsigned planemask,
   RegionPtr cclip,
   int nglyph,
   unsigned char* gBase,
   CharInfoPtr *ppci 
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int skippix, skipglyph, width, n, i;
    int Left, Right, Top, Bottom;
    int LeftEdge, RightEdge;
    int nbox = REGION_NUM_RECTS(cclip);
    BoxPtr pbox = REGION_RECTS(cclip);

    width = CollectCharacterInfo(infoRec->GlyphInfo, nglyph, ppci, font);

    /* compute an approximate but covering bounding box */
    Left = xInit + infoRec->GlyphInfo[0].start;
    Right = xInit + infoRec->GlyphInfo[nglyph - 1].end;
    Top = yInit - FONTMAXBOUNDS(font,ascent);
    Bottom = yInit + FONTMAXBOUNDS(font,descent);

    /* get into the first band that may contain part of our string */
    while(nbox && (Top >= pbox->y2)) {
	pbox++; nbox--;
    }

    /* stop when the lower edge of the box is beyond our string */
    while(nbox && (Bottom >= pbox->y1)) {
	LeftEdge = max(Left, pbox->x1);
	RightEdge = min(Right, pbox->x2);

	if(RightEdge > LeftEdge) { /* we're possibly drawing something */

	    skippix = LeftEdge - xInit;
	    skipglyph = 0;
	    while(skippix >= infoRec->GlyphInfo[skipglyph].end)
		skipglyph++;

	    skippix = RightEdge - xInit;
	    n = 0; i = skipglyph;
	    while((i < nglyph) && (skippix > infoRec->GlyphInfo[i].start)) {
		i++; n++;
	    }

	    if(n) (*infoRec->NonTEGlyphRenderer)(pScrn,
			xInit, yInit, n, infoRec->GlyphInfo + skipglyph, 
			pbox, fg, rop, planemask); 
	}

	nbox--; pbox++;
    }
    return width;
}


/* It is possible that the none of the glyphs passed to the 
   NonTEGlyphRenderer will be drawn.  This function being called
   indicates that part of the text string's bounding box is visible
   but not necessarily that any of the characters are visible */

void XAANonTEGlyphRenderer(
   ScrnInfoPtr pScrn,
   int x, int y, int n,
   NonTEGlyphPtr glyphs,
   BoxPtr pbox,
   int fg, int rop,
   unsigned int planemask
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int x1, x2, y1, y2, i, w, h, skipleft, skiptop;
    unsigned char *src;

    for(i = 0; i < n; i++, glyphs++) {
	x1 = x + glyphs->start;
	x2 = x + glyphs->end;
	y1 = y - glyphs->yoff;
	y2 = y1 + glyphs->height;

	if(y1 < pbox->y1) {
	    skiptop = pbox->y1 - y1;
	    y1 = pbox->y1;
	} else skiptop = 0;
	if(y2 > pbox->y2) y2 = pbox->y2;
	h = y2 - y1;
	if(h <= 0) continue;

	if(x1 < pbox->x1) {
	    skipleft = pbox->x1 - x1;
	    x1 = pbox->x1;
	} else skipleft = 0;
	if(x2 > pbox->x2) x2 = pbox->x2;

	w = x2 - x1;

	if(w > 0) {
	    src = glyphs->bits + (skiptop * glyphs->srcwidth);

	    if(skipleft) {
		src += (skipleft >> 5) << 2;
		skipleft &= 31;
	    }

	    (*infoRec->WriteBitmap)(pScrn, x1, y1, w, h, src,
			glyphs->srcwidth, skipleft, fg, -1, rop, planemask);
	}
    }  

}
