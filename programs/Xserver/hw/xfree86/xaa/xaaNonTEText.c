/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaNonTEText.c,v 1.3 1998/08/13 14:46:12 dawes Exp $ */

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

   v1.0 - Mark Vojkovich (mvojkovi@ucsd.edu)

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
static void PolyGlyphBltNonTEColorExpansion(ScrnInfoPtr pScrn,
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
    unsigned long n, i;
    int w = 0;

    CharInfoPtr charinfo[255];	/* encoding only has 1 byte for count */

    GetGlyphs(pGC->font, (unsigned long)count, (unsigned char *)chars,
	      Linear8Bit, &n, charinfo);

    for (i=0; i < n; i++) w += charinfo[i]->metrics.characterWidth;

    if(n) PolyGlyphBltNonTEColorExpansion(
	infoRec->pScrn, x + pDraw->x, y + pDraw->y,
	pGC->font, pGC->fgPixel, pGC->alu, pGC->planemask, 
	pGC->pCompositeClip, n, FONTGLYPHS(pGC->font), charinfo);

    return (x + w);
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
    unsigned long n, i;
    int w = 0;

    CharInfoPtr charinfo[255];	/* encoding only has 1 byte for count */

    GetGlyphs(pGC->font, (unsigned long)count, (unsigned char *)chars,
	      (FONTLASTROW(pGC->font) == 0) ? Linear16Bit : TwoD16Bit,
	      &n, charinfo);

    for (i=0; i < n; i++) w += charinfo[i]->metrics.characterWidth;

    if(n) PolyGlyphBltNonTEColorExpansion(
	infoRec->pScrn, x + pDraw->x, y + pDraw->y,
	pGC->font, pGC->fgPixel, pGC->alu, pGC->planemask, 
	pGC->pCompositeClip, n, FONTGLYPHS(pGC->font), charinfo);

    return (x + w);
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
    CharInfoPtr charinfo[255];	/* encoding only has 1 byte for count */

    GetGlyphs(pGC->font, (unsigned long)count, (unsigned char *)chars,
	      Linear8Bit, &n, charinfo);

    if(n) ImageGlyphBltNonTEColorExpansion(
	infoRec->pScrn, x + pDraw->x, y + pDraw->y,
	pGC->font, pGC->fgPixel, pGC->bgPixel, GXcopy, pGC->planemask, 
	pGC->pCompositeClip, n, FONTGLYPHS(pGC->font), charinfo);
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
    FontPtr font = pGC->font;
    CharInfoPtr charinfo[255];	/* encoding only has 1 byte for count */

    GetGlyphs(font, (unsigned long)count, (unsigned char *)chars,
	      (FONTLASTROW(pGC->font) == 0) ? Linear16Bit : TwoD16Bit,
	      &n, charinfo);

    if(n) ImageGlyphBltNonTEColorExpansion(
	infoRec->pScrn, x + pDraw->x, y + pDraw->y,
	pGC->font, pGC->fgPixel, pGC->bgPixel, GXcopy, pGC->planemask, 
	pGC->pCompositeClip, n, FONTGLYPHS(pGC->font), charinfo);
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

#define MAX_GLYPHS	256

/* this should be OK until we multi-thread */
static NonTEGlyphInfo Glyphs[MAX_GLYPHS];
static int GlyphWidths[MAX_GLYPHS];	/* clipping aid */


static int
CollectCharacterInfo(
    NonTEGlyphInfo *glyphinfop,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    int maxascent )
{
    int i, w;

    w = 0;
    for (i = 0; i < nglyph; i++) {
        /*
         * Note that the glyph bitmap address is pre-adjusted, so that the
         * offset is the same for differently sized glyphs when writing a
         * text scanline.
         *
         * The width of each character should be carefully determined,
         * considering the leftSideBearing of the next character, since
         * characters can overlap horizontally.
         */
	glyphinfop[i].bitsp = (unsigned int *)(ppci[i]->bits) - 
			(maxascent - ppci[i]->metrics.ascent);

        if (i < nglyph - 1)
	    glyphinfop[i].width = ppci[i]->metrics.characterWidth +
	        ppci[i + 1]->metrics.leftSideBearing -
	        ppci[i]->metrics.leftSideBearing;
	else {
 	    glyphinfop[i].width = ppci[i]->metrics.rightSideBearing -
				  ppci[i]->metrics.leftSideBearing;
	    if(ppci[i]->metrics.characterWidth >  glyphinfop[i].width)
		glyphinfop[i].width = ppci[i]->metrics.characterWidth;
	}
	w += glyphinfop[i].width;
	GlyphWidths[i] = w;	/* store the end of the ith character */
	glyphinfop[i].firstline = maxascent - ppci[i]->metrics.ascent;
	glyphinfop[i].lastline = maxascent + ppci[i]->metrics.descent - 1;
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
    int skippix, skipglyph, width;
    int LeftText, RightText, TopText, BottomText;
    int LeftBox, RightBox, TopBox, BottomBox;
    int Left, Right, Top, Bottom, ytop, ybot;
    int nbox = REGION_NUM_RECTS(cclip);
    BoxPtr pbox = REGION_RECTS(cclip);

    width = CollectCharacterInfo(
		Glyphs, nglyph, ppci, FONTMAXBOUNDS(font, ascent));

    /* find our text dimensions */
    LeftText = xInit + (*ppci)->metrics.leftSideBearing;
    RightText = LeftText + width;
    TopText = yInit - FONTMAXBOUNDS(font,ascent);
    BottomText = yInit + FONTMAXBOUNDS(font,descent);

    /* find our backing rectangle dimensions */
    LeftBox = xInit;
    RightBox = LeftBox + width;
    TopBox = yInit - FONTASCENT(font);
    BottomBox = yInit + FONTDESCENT(font);

    Top = min(TopText, TopBox);
    Bottom = max(BottomText, BottomBox);
 
    /* get into the first band that may contain part of our string */
    while(nbox && (Top >= pbox->y2)) {
	pbox++; nbox--;
    }

    /* stop when the lower edge of the box is beyond our string */
    while(nbox && (Bottom >= pbox->y1)) {

	/* handle backing rect first */
	Left = max(LeftBox, pbox->x1);
	Right = min(RightBox, pbox->x2);
	if(Right > Left) {	    
	    ytop = max(TopBox, pbox->y1);
	    ybot = min(BottomBox, pbox->y2);

	    if(ybot > ytop) {
		(*infoRec->SetupForSolidFill)(pScrn, bg, rop, planemask);
		(*infoRec->SubsequentSolidFillRect)(pScrn, 
			Left, ytop, Right - Left, ybot - ytop);
	    }
	}

	/* now handle the glyphs */
	Left = max(LeftText, pbox->x1);
	Right = min(RightText, pbox->x2);
	if(Right > Left) {	    
	    ytop = max(TopText, pbox->y1);
	    ybot = min(BottomText, pbox->y2);

	    if(ybot > ytop) {
		skippix = Left - LeftText;

		skipglyph = 0;
		while( skippix >= GlyphWidths[skipglyph])
		    skipglyph++;

		if(skipglyph)
		   skippix -= GlyphWidths[skipglyph - 1];
	    
		(*infoRec->NonTEGlyphRenderer)(pScrn,
			Left, Right - Left, ytop, ybot - ytop, 
			skippix, ytop - TopText, Glyphs + skipglyph,
			fg, rop, planemask); 
	    }
	}

	nbox--; pbox++;
    }
}


static void
PolyGlyphBltNonTEColorExpansion(
   ScrnInfoPtr pScrn,
   int xInit, int yInit,
   FontPtr font,
   int fg, int rop,
   unsigned planemask,
   RegionPtr cclip,
   int nglyph,
   unsigned char* gBase,
   CharInfoPtr *ppci )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int skippix, skipglyph;
    int Left, Right, Top, Bottom;
    int LeftEdge, RightEdge, ytop, ybot;
    int nbox = REGION_NUM_RECTS(cclip);
    BoxPtr pbox = REGION_RECTS(cclip);

    /* find our text box */
    Left = xInit + (*ppci)->metrics.leftSideBearing;
    Right = Left + 
	CollectCharacterInfo(Glyphs, nglyph, ppci, FONTMAXBOUNDS(font, ascent));
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

	if(RightEdge > LeftEdge) {	/* we're drawing something */
	    ytop = max(Top, pbox->y1);
	    ybot = min(Bottom, pbox->y2);

	    skippix = LeftEdge - Left;

	    skipglyph = 0;
	    while( skippix >= GlyphWidths[skipglyph])
		skipglyph++;

	    if(skipglyph)
		skippix -= GlyphWidths[skipglyph - 1];
	    
	    (*infoRec->NonTEGlyphRenderer)(pScrn,
		LeftEdge, RightEdge - LeftEdge, ytop, ybot - ytop, 
		skippix, ytop - Top, Glyphs + skipglyph,
		fg, rop, planemask); 
	}

	nbox--; pbox++;
    }
}
