/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaTEText.c,v 1.1.2.2 1998/06/04 17:36:19 dawes Exp $ */

/********************************************************************

   In this file we have GC level replacements for PolyText8/16,
   ImageText8/16, ImageGlyphBlt and PolyGlyphBlt for TE (fixed) fonts.
   The idea is that everything in this file is device independent.
   The mentioned GCOps are merely wrappers for XAAGlyphBltTEColorExpansion
   which calculates the boxes containing arbitrarily clipped text
   and passes them to the TEGlyphRenderer which will usually be a lower 
   level XAA function which renders these clipped glyphs using
   the basic color expansion functions exported by the chipset driver.
   The TEGlyphRenderer itself may optionally be driver supplied to
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


static void XAAGlyphBltTEColorExpansion(ScrnInfoPtr pScrn, int xInit,
			int yInit, FontPtr font, int fg, int bg, int rop,
			unsigned int planemask, RegionPtr cclip, int nglyph,
			unsigned char* gBase, CharInfoPtr *ppci);


/********************************************************************

   GC level replacements for PolyText8/16 and ImageText8/16
   for TE fonts when using color expansion.

********************************************************************/


int
XAAPolyText8TEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    char *chars )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    unsigned long n;

    CharInfoPtr charinfo[255];	/* encoding only has 1 byte for count */

    GetGlyphs(pGC->font, (unsigned long)count, (unsigned char *)chars,
	      Linear8Bit, &n, charinfo);

    /* we have divorced XAAGlyphBltTEColorExpansion from the drawable */
    if(n) XAAGlyphBltTEColorExpansion(
	infoRec->pScrn, x + pDraw->x, y + pDraw->y,
	pGC->font, pGC->fgPixel, -1, pGC->alu, pGC->planemask, 
	pGC->pCompositeClip, n, FONTGLYPHS(pGC->font), charinfo);

    return (x + (n * FONTMAXBOUNDS(pGC->font, characterWidth)));
}


int
XAAPolyText16TEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    unsigned short *chars )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    unsigned long n;

    CharInfoPtr charinfo[255];	/* encoding only has 1 byte for count */

    GetGlyphs(pGC->font, (unsigned long)count, (unsigned char *)chars,
	      (FONTLASTROW(pGC->font) == 0) ? Linear16Bit : TwoD16Bit,
	      &n, charinfo);

    if(n) XAAGlyphBltTEColorExpansion(
	infoRec->pScrn, x + pDraw->x, y + pDraw->y,
	pGC->font, pGC->fgPixel, -1, pGC->alu, pGC->planemask, 
	pGC->pCompositeClip, n, FONTGLYPHS(pGC->font), charinfo);

    return (x + (n * FONTMAXBOUNDS(pGC->font, characterWidth)));
}


void
XAAImageText8TEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    char *chars )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    unsigned long n;

    CharInfoPtr charinfo[255];	/* encoding only has 1 byte for count */

    GetGlyphs(pGC->font, (unsigned long)count, (unsigned char *)chars,
	      Linear8Bit, &n, charinfo);

    if(n) XAAGlyphBltTEColorExpansion(
	infoRec->pScrn, x + pDraw->x, y + pDraw->y,
	pGC->font, pGC->fgPixel, pGC->bgPixel, GXcopy, pGC->planemask, 
	pGC->pCompositeClip, n, FONTGLYPHS(pGC->font), charinfo);
}


void
XAAImageText16TEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    unsigned short *chars )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    unsigned long n;
    FontPtr font = pGC->font;
    CharInfoPtr charinfo[255];	/* encoding only has 1 byte for count */

    GetGlyphs(font, (unsigned long)count, (unsigned char *)chars,
	      (FONTLASTROW(pGC->font) == 0) ? Linear16Bit : TwoD16Bit,
	      &n, charinfo);

    if(n) XAAGlyphBltTEColorExpansion(
	infoRec->pScrn, x + pDraw->x, y + pDraw->y,
	pGC->font, pGC->fgPixel, pGC->bgPixel, GXcopy, pGC->planemask, 
	pGC->pCompositeClip, n, FONTGLYPHS(pGC->font), charinfo);
}



/********************************************************************

   GC level replacements for ImageGlyphBlt and PolyGlyphBlt for
   TE fonts when using color expansion.

********************************************************************/


void
XAAImageGlyphBltTEColorExpansion(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int xInit, int yInit,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

    XAAGlyphBltTEColorExpansion(
	infoRec->pScrn, xInit + pDrawable->x, yInit + pDrawable->y,
	pGC->font, pGC->fgPixel, pGC->bgPixel, GXcopy, pGC->planemask, 
	pGC->pCompositeClip, nglyph, (unsigned char*)pglyphBase, ppci);
}

void
XAAPolyGlyphBltTEColorExpansion(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int xInit, int yInit,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

    XAAGlyphBltTEColorExpansion(
	infoRec->pScrn, xInit + pDrawable->x, yInit + pDrawable->y,
	pGC->font, pGC->fgPixel, -1, pGC->alu, pGC->planemask, 
	pGC->pCompositeClip, nglyph, (unsigned char*)pglyphBase, ppci);
}




/********************************************************************

   XAAGlyphBltTEColorExpansion -

   This guy computes the clipped pieces of text and sends it to
   the lower-level function which will handle acceleration of 
   arbitrarily clipped text.
  
********************************************************************/

#define MAX_GLYPHS	(256 + 6)

/* this should be OK until we multi-thread */
static unsigned int *Glyphs[MAX_GLYPHS];


static unsigned int **
CollectGlyphs(
    unsigned int **glyphp,
    unsigned int nglyph,
    unsigned char *pglyphBase,
    CharInfoPtr *ppci )
{
    int count;

    for(count = 0; count < nglyph; count++) 
 	glyphp[count] = (unsigned int*)FONTGLYPHBITS(pglyphBase,*ppci++);

    /* our new unrolled TE code only writes DWORDS at a time so it can read
	up to 6 characters past the last one we're displaying */
    glyphp[count + 0] = glyphp[0];
    glyphp[count + 1] = glyphp[0];
    glyphp[count + 2] = glyphp[0];
    glyphp[count + 3] = glyphp[0];
    glyphp[count + 4] = glyphp[0];
    glyphp[count + 5] = glyphp[0];

    return glyphp;
}

static void
XAAGlyphBltTEColorExpansion(
   ScrnInfoPtr pScrn,
   int xInit, int yInit,
   FontPtr font,
   int fg, int bg,
   int rop,
   unsigned int planemask,
   RegionPtr cclip,
   int nglyph,
   unsigned char* gBase,
   CharInfoPtr *ppci )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int skippix, skipglyphs;
    int Left, Right, Top, Bottom;
    int LeftEdge, RightEdge, ytop, ybot;
    int nbox = REGION_NUM_RECTS(cclip);
    BoxPtr pbox = REGION_RECTS(cclip);
    unsigned int **glyphs = NULL; 
    int glyphWidth = FONTMAXBOUNDS(font, characterWidth);

    /* find the size of the box */
    Left = xInit;
    Right = Left + (glyphWidth * nglyph);
    Top = yInit - FONTASCENT(font);
    Bottom =  yInit + FONTDESCENT(font);

    /* get into the first band that may contain part of our string */
    while(nbox && (Top >= pbox->y2)) {
	pbox++; nbox--;
    }

    /* stop when the lower edge of the box is beyond our string */
    while(nbox && (Bottom >= pbox->y1)) {
	LeftEdge = max(Left, pbox->x1);
	RightEdge = min(Right, pbox->x2);

	if(RightEdge > LeftEdge) {	/* we have something to draw */
	    ytop = max(Top, pbox->y1);
	    ybot = min(Bottom, pbox->y2);
	    
	    if((skippix = LeftEdge - Left)) {
		skipglyphs = skippix/glyphWidth;
		skippix %= glyphWidth;
	    } else skipglyphs = 0;

	    if(!glyphs) glyphs = CollectGlyphs(Glyphs, nglyph, gBase, ppci);

   /* x, y, w, h, skipleft, skiptop, glyphp, glyphWidth, fg, bg, rop, pm */

	    (*infoRec->TEGlyphRenderer)( pScrn, 
		LeftEdge, ytop, RightEdge - LeftEdge, ybot - ytop, 
		skippix, ytop - Top, glyphs + skipglyphs, glyphWidth, 
		fg, bg, rop, planemask); 
	}

	nbox--; pbox++;
    }
}




