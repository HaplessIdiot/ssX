/*
 * Copyright (c) 1989  X Consortium
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Except as contained in this notice, the name of the X Consortium shall not
 * be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the X
 * Consortium.
 */

/* $XFree86$ */

/*
 * Modified for Cirrus by Harm Hanemaayer (hhanemaa@cs.ruu.nl).
 *
 * We accelerate straightforward text writing for fonts with widths up to 32
 * pixels.
 */

/*
 * Modified for Chips by David Bateman (dbateman@ee.uts.edu.au)
 */

/*
 * TEGblt - ImageText expanded glyph fonts only.  For
 * 8/16/24 bit displays, in Copy mode with no clipping.
 */

#include        "vga256.h"
#include	"xf86.h"
#include	"vga.h"		       /* For vgaBase. */
#include        "vgaBank.h"
#include	"xf86_OSlib.h"

#include "compiler.h"

#include "ct_driver.h"

#ifdef CHIPS_MMIO
#ifdef CHIPS_HIQV
#include "ct_BltHiQV.h"
#else
#include "ct_BlitMM.h"
#endif
#else
#include "ct_Blitter.h"
#endif

#ifndef CHIPS_MMIO /* Only define this once */
unsigned int byte_reversed3[256] =
{
    0x000000, 0xE00000, 0x1C0000, 0xFC0000, 0x038000, 0xE38000, 0x1F8000, 0xFF8000,
    0x007000, 0xE07000, 0x1C7000, 0xFC7000, 0x03F000, 0xE3F000, 0x1FF000, 0xFFF000,
    0x000E00, 0xE00E00, 0x1C0E00, 0xFC0E00, 0x038E00, 0xE38E00, 0x1F8E00, 0xFF8E00,
    0x007E00, 0xE07E00, 0x1C7E00, 0xFC7E00, 0x03FE00, 0xE3FE00, 0x1FFE00, 0xFFFE00,
    0x0001C0, 0xE001C0, 0x1C01C0, 0xFC01C0, 0x0381C0, 0xE381C0, 0x1F81C0, 0xFF81C0,
    0x0071C0, 0xE071C0, 0x1C71C0, 0xFC71C0, 0x03F1C0, 0xE3F1C0, 0x1FF1C0, 0xFFF1C0,
    0x000FC0, 0xE00FC0, 0x1C0FC0, 0xFC0FC0, 0x038FC0, 0xE38FC0, 0x1F8FC0, 0xFF8FC0,
    0x007FC0, 0xE07FC0, 0x1C7FC0, 0xFC7FC0, 0x03FFC0, 0xE3FFC0, 0x1FFFC0, 0xFFFFC0,
    0x000038, 0xE00038, 0x1C0038, 0xFC0038, 0x038038, 0xE38038, 0x1F8038, 0xFF8038,
    0x007038, 0xE07038, 0x1C7038, 0xFC7038, 0x03F038, 0xE3F038, 0x1FF038, 0xFFF038,
    0x000E38, 0xE00E38, 0x1C0E38, 0xFC0E38, 0x038E38, 0xE38E38, 0x1F8E38, 0xFF8E38,
    0x007E38, 0xE07E38, 0x1C7E38, 0xFC7E38, 0x03FE38, 0xE3FE38, 0x1FFE38, 0xFFFE38,
    0x0001F8, 0xE001F8, 0x1C01F8, 0xFC01F8, 0x0381F8, 0xE381F8, 0x1F81F8, 0xFF81F8,
    0x0071F8, 0xE071F8, 0x1C71F8, 0xFC71F8, 0x03F1F8, 0xE3F1F8, 0x1FF1F8, 0xFFF1F8,
    0x000FF8, 0xE00FF8, 0x1C0FF8, 0xFC0FF8, 0x038FF8, 0xE38FF8, 0x1F8FF8, 0xFF8FF8,
    0x007FF8, 0xE07FF8, 0x1C7FF8, 0xFC7FF8, 0x03FFF8, 0xE3FFF8, 0x1FFFF8, 0xFFFFF8,
    0x000007, 0xE00007, 0x1C0007, 0xFC0007, 0x038007, 0xE38007, 0x1F8007, 0xFF8007,
    0x007007, 0xE07007, 0x1C7007, 0xFC7007, 0x03F007, 0xE3F007, 0x1FF007, 0xFFF007,
    0x000E07, 0xE00E07, 0x1C0E07, 0xFC0E07, 0x038E07, 0xE38E07, 0x1F8E07, 0xFF8E07,
    0x007E07, 0xE07E07, 0x1C7E07, 0xFC7E07, 0x03FE07, 0xE3FE07, 0x1FFE07, 0xFFFE07,
    0x0001C7, 0xE001C7, 0x1C01C7, 0xFC01C7, 0x0381C7, 0xE381C7, 0x1F81C7, 0xFF81C7,
    0x0071C7, 0xE071C7, 0x1C71C7, 0xFC71C7, 0x03F1C7, 0xE3F1C7, 0x1FF1C7, 0xFFF1C7,
    0x000FC7, 0xE00FC7, 0x1C0FC7, 0xFC0FC7, 0x038FC7, 0xE38FC7, 0x1F8FC7, 0xFF8FC7,
    0x007FC7, 0xE07FC7, 0x1C7FC7, 0xFC7FC7, 0x03FFC7, 0xE3FFC7, 0x1FFFC7, 0xFFFFC7,
    0x00003F, 0xE0003F, 0x1C003F, 0xFC003F, 0x03803F, 0xE3803F, 0x1F803F, 0xFF803F,
    0x00703F, 0xE0703F, 0x1C703F, 0xFC703F, 0x03F03F, 0xE3F03F, 0x1FF03F, 0xFFF03F,
    0x000E3F, 0xE00E3F, 0x1C0E3F, 0xFC0E3F, 0x038E3F, 0xE38E3F, 0x1F8E3F, 0xFF8E3F,
    0x007E3F, 0xE07E3F, 0x1C7E3F, 0xFC7E3F, 0x03FE3F, 0xE3FE3F, 0x1FFE3F, 0xFFFE3F,
    0x0001FF, 0xE001FF, 0x1C01FF, 0xFC01FF, 0x0381FF, 0xE381FF, 0x1F81FF, 0xFF81FF,
    0x0071FF, 0xE071FF, 0x1C71FF, 0xFC71FF, 0x03F1FF, 0xE3F1FF, 0x1FF1FF, 0xFFF1FF,
    0x000FFF, 0xE00FFF, 0x1C0FFF, 0xFC0FFF, 0x038FFF, 0xE38FFF, 0x1F8FFF, 0xFF8FFF,
    0x007FFF, 0xE07FFF, 0x1C7FFF, 0xFC7FFF, 0x03FFFF, 0xE3FFFF, 0x1FFFFF, 0xFFFFFF,
};
#endif

/* 
 * This function collects normal character bitmaps from the server.
 */

static void 
CollectCharacters(glyphp, nglyph, pglyphBase, ppci)
    unsigned long **glyphp;
    unsigned int nglyph;
    CharInfoPtr *ppci;
    unsigned char *pglyphBase;
{
    int i;

    for (i = 0; i < nglyph; i++) {
	glyphp[i] = (unsigned long *)FONTGLYPHBITS(pglyphBase,
	    *ppci++);
    }
}

#ifdef CHIPS_MMIO
#ifdef CHIPS_HIQV
void
ctHiQVImageGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase)
#else
void
ctMMIOImageGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase)
#endif
#else
void
ctcfbImageGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase)
#endif
    DrawablePtr pDrawable;
    GC *pGC;
    int xInit, yInit;
    unsigned int nglyph;
    CharInfoPtr *ppci;		       /* array of character info */
    unsigned char *pglyphBase;	       /* start of array of glyphs */
{
    FontPtr pfont = pGC->font;
    unsigned long *pdstBase;
    int widthDst;
    int widthGlyph;
    int h;
    int x, y;
    BoxRec bbox;		       /* for clipping */

    int glyphWidth;		       /* Character width in pixels. */
    int glyphWidthBytes;	       /* Character width in bytes (padded). */
    int i;

    /* Characters are padded to 4 bytes. */
    unsigned long **glyphp;
    int destaddr, blitwidth;

#ifdef DEBUG
#ifdef CHIPS_MMIO
    ErrorF("CHIPS:ctMMIOImageGlyphBlt(%d, %d, %d)\n", xInit, yInit, nglyph);
#else
    ErrorF("CHIPS:ctcfbImageGlyphBlt(%d, %d, %d)\n", xInit, yInit, nglyph);
#endif
#endif

    glyphWidth = FONTMAXBOUNDS(pfont, characterWidth);
    glyphWidthBytes = GLYPHWIDTHBYTESPADDED(*ppci);
    widthDst = vga256InfoRec.displayWidth * vgaBytesPerPixel;

    /* We only accelerate fonts 32 or less pixels wide. */
    /* Let cfb handle writing into offscreen pixmap. */
    if (vgaBitsPerPixel == 8 && (pDrawable->type != DRAWABLE_WINDOW ||
	    !xf86VTSema)) {
	cfbImageGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
	    pglyphBase);
	return;
    }
    if (vgaBitsPerPixel == 16 && (pDrawable->type != DRAWABLE_WINDOW ||
	    !xf86VTSema)) {
	cfb16TEGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci,
	    pglyphBase);
	return;
    }
    if (vgaBitsPerPixel == 24 && (pDrawable->type != DRAWABLE_WINDOW ||
	    !xf86VTSema)) {
	cfb24TEGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci,
	    pglyphBase);
	return;
    }
    if (glyphWidthBytes != 4 || glyphWidth > 32) {
	switch (vgaBitsPerPixel) {
	case 8:
	    vga256TEGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph,
		ppci, pglyphBase);
	    break;
	case 16:
	    cfb16TEGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph,
		ppci, pglyphBase);
	    break;
	case 24:
	    cfb24TEGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph,
		ppci, pglyphBase);
	    break;
	}
	return;
    }
    h = FONTASCENT(pfont) + FONTDESCENT(pfont);

    if ((h | glyphWidth) == 0)
	return;

    x = xInit + FONTMAXBOUNDS(pfont, leftSideBearing) + pDrawable->x;
    y = yInit - FONTASCENT(pfont) + pDrawable->y;
    bbox.x1 = x;
    bbox.x2 = x + (glyphWidth * nglyph);
    bbox.y1 = y;
    bbox.y2 = y + h;

    switch (RECT_IN_REGION(pGC->pScreen, cfbGetCompositeClip(pGC), &bbox)) {
    case rgnPART:
	switch (vgaBitsPerPixel) {
	case 8:
	    vga256TEGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph,
		ppci, pglyphBase);
	    break;
	case 16:
	    cfb16TEGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph,
		ppci, pglyphBase);
	    break;
	case 24:
	    cfb24TEGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph,
		ppci, pglyphBase);
	    break;
	}
    case rgnOUT:
	return;
    }

    /* Allocate list of pointers to glyph bitmaps. */
    glyphp = (unsigned long **)ALLOCATE_LOCAL(nglyph *
	sizeof(unsigned long *));

    destaddr = y * widthDst + (x * vgaBytesPerPixel);

    ctSETDSTADDR(destaddr);
    ctSETSRCADDR(0);
    ctSETPITCH(0, widthDst);

    switch (vgaBitsPerPixel) {
    case 16:
	ctSETBGCOLOR16(pGC->bgPixel);
	ctSETFGCOLOR16(pGC->fgPixel);
	break;
    case 24:
#ifdef CHIPS_HIQV
	    ctSETBGCOLOR24(pGC->bgPixel);
	    ctSETFGCOLOR24(pGC->fgPixel);
#else
	{
	    /* The 6554x Blitter can only handle 8/16bpp fills 
	     * directly, Though you can do a grey fill, by a little
	     * bit of magic with the 8bpp fill */
	    unsigned char red, green, blue;

	    red = pGC->bgPixel & 0xFF;
	    green = (pGC->bgPixel >> 8) & 0xFF;
	    blue = (pGC->bgPixel >> 16) & 0xFF;
	    if (red ^ green || green ^ blue || blue ^ red) {
		cfb24TEGlyphBlt(pDrawable, pGC, xInit, yInit,
		    nglyph, ppci, pglyphBase);
		return;
	    } else {
		ctSETBGCOLOR8(pGC->bgPixel);
	    }
	    red = pGC->fgPixel & 0xFF;
	    green = (pGC->fgPixel >> 8) & 0xFF;
	    blue = (pGC->fgPixel >> 16) & 0xFF;
	    if (red ^ green || green ^ blue || blue ^ red) {
		cfb24TEGlyphBlt(pDrawable, pGC, xInit, yInit,
		    nglyph, ppci, pglyphBase);
		return;
	    } else {
		ctSETFGCOLOR8(pGC->fgPixel);
	    }
	}
#endif
	break;
    default:			       /* 8 */
	ctSETBGCOLOR8(pGC->bgPixel);
	ctSETFGCOLOR8(pGC->fgPixel);
	break;
    }

    ctSETROP(ctAluConv[GXcopy & 0xf] | ctSRCMONO | ctSRCSYSTEM |
	ctTOP2BOTTOM | ctLEFT2RIGHT);
#ifdef CHIPS_HIQV
#if 1
    ctSETMONOCTL(ctDWORDALIGN);
#else
    ctSETMONOCTL(ctBITALIGN);
#endif
#endif

    blitwidth = glyphWidth * nglyph;
    ctSETHEIGHTWIDTHGO(h, blitwidth * vgaBytesPerPixel);

    /* Write bitmap to video memory (for BitBlt engine to process). */
    /* Gather bytes until we have a dword to write. Doubleword is   */
    /* LSByte first, and MSBit first in each byte, as required for  */
    /* the blit data. */

    CollectCharacters(glyphp, nglyph, pglyphBase, ppci);
    if (!ctisHiQV32 && (vgaBitsPerPixel == 24)) {
	ctTransferText24(nglyph, h, glyphp, glyphWidth, ctBltDataWindow);
    } else {
#ifdef CHIPS_HIQV
#if 1
	ctTransferText(nglyph, h, glyphp, glyphWidth, ctBltDataWindow);
	if (((((blitwidth + 31) & ~31) >> 5) * h) & 0x1)
	    *(unsigned int *)ctBltDataWindow = 0;
#else
	ctTransferTextHiQV(nglyph, h, glyphp, glyphWidth, ctBltDataWindow);
	if (((((blitwidth * h) + 31) & ~31) >> 5) & 0x1)
	    *(unsigned int *)ctBltDataWindow = 0;
#endif
#else
	ctTransferText(nglyph, h, glyphp, glyphWidth, ctBltDataWindow);
#endif
    }
    
#ifdef CHIPS_HIQV
    usleep(50);
    while(*(volatile unsigned int *)(ctMMIOBase + 0x10) & (0x80000000)) {
	ErrorF("Flush\n");
	*(unsigned int *)ctBltDataWindow = 0;
	usleep(5);
    }
#else
    ctBLTWAIT;
#endif
    DEALLOCATE_LOCAL(glyphp);
}

/*
 * Transparent text.
 */

#ifdef CHIPS_MMIO
#ifdef CHIPS_HIQV
void 
ctHiQVPolyGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase)
#else
void 
ctMMIOPolyGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase)
#endif
#else
void 
ctcfbPolyGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase)
#endif
    DrawablePtr pDrawable;
    GC *pGC;
    int xInit, yInit;
    unsigned int nglyph;
    CharInfoPtr *ppci;		       /* array of character info */
    unsigned char *pglyphBase;	       /* start of array of glyphs */
{
    FontPtr pfont = pGC->font;
    unsigned long *pdstBase;
    int widthDst;
    int widthGlyph;
    int h;
    int x, y;
    BoxRec bbox;		       /* for clipping */

    int glyphWidth;		       /* Character width in pixels. */
    int glyphWidthBytes;	       /* Character width in bytes (padded). */
    int i;

    /* Characters are padded to 4 bytes. */
    unsigned long **glyphp;
    int shift, line;
    unsigned dworddata;
    int destaddr, blitwidth;

#ifdef DEBUG
#ifdef CHIPS_MMIO
    ErrorF("CHIPS:ctMMIOPolyGlyphBlt(%d, %d, %d)\n", xInit, yInit, nglyph);
#else
    ErrorF("CHIPS:ctcfbPolyGlyphBlt(%d, %d, %d)\n", xInit, yInit, nglyph);
#endif
#endif

    /* Let cfb handle writing into offscreen pixmap. */
    if (vgaBitsPerPixel == 8 && (pDrawable->type != DRAWABLE_WINDOW ||
	    !xf86VTSema)) {
	vga256PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
	    pglyphBase);
	return;
    }
    if (vgaBitsPerPixel == 16 && (pDrawable->type != DRAWABLE_WINDOW ||
	    !xf86VTSema)) {
	cfb16PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
	    pglyphBase);
	return;
    }
    if (vgaBitsPerPixel == 24 && (pDrawable->type != DRAWABLE_WINDOW ||
	    !xf86VTSema)) {
	cfb24PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
	    pglyphBase);
	return;
    }

#if 0 /* This doesn't work with multiple depths */
    cfbGetLongWidthAndPointer(pDrawable, widthDst, pdstBase)
    widthDst *= 4;		       /* Convert to bytes. */
    if (!CHECKSCREEN(pdstBase)) {
	switch (vgaBitsPerPixel) {
	  case 8:
	    cfbPolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
	    break;
	  case 16:
	    cfb16PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
	    break;
	  case 24:
	    cfb24PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
	    break;
	}
	return;
    }
#else
    widthDst = vga256InfoRec.displayWidth * vgaBytesPerPixel;
#endif

    glyphWidth = FONTMAXBOUNDS(pfont, characterWidth);
    glyphWidthBytes = GLYPHWIDTHBYTESPADDED(*ppci);

    h = FONTASCENT(pfont) + FONTDESCENT(pfont);
    if ((h | glyphWidth) == 0)
	return;

    /* We only do GXcopy polyglyph. */
    if ((pGC->alu != GXcopy) || (glyphWidthBytes != 4 || glyphWidth > 32)) {
	if (pGC->alu == GXcopy) {
	    switch (vgaBitsPerPixel) {
	      case 8:
		vga256PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	      case 16:
		cfb16PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	      case 24:
		cfb24PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	    }
	} else {
	    switch (vgaBitsPerPixel) {
	      case 8:
		vga256PolyGlyphRop8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	      case 16:
		cfb16PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	      case 24:
		cfb24PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	    }
	}
	return;
    }

    if (pGC->fillStyle != FillSolid) {
	miPolyGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci,
	    pglyphBase);
	return;
    }
    
    if (FONTMAXBOUNDS(pGC->font, rightSideBearing) -
	FONTMINBOUNDS(pGC->font, leftSideBearing) > 32 ||
	FONTMINBOUNDS(pGC->font, characterWidth) < 0) {
	miPolyGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci,
	    pglyphBase);
	return;
    }

    x = xInit + FONTMAXBOUNDS(pfont, leftSideBearing) + pDrawable->x;
    y = yInit - FONTASCENT(pfont) + pDrawable->y;
    bbox.x1 = x;
    bbox.x2 = x + (glyphWidth * nglyph);
    bbox.y1 = y;
    bbox.y2 = y + h;

    switch (RECT_IN_REGION(pGC->pScreen, cfbGetCompositeClip(pGC), &bbox)) {
    case rgnPART:
	if (pGC->alu == GXcopy) {
	    switch (vgaBitsPerPixel) {
	      case 8:
		vga256PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	      case 16:
		cfb16PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	      case 24:
		cfb24PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	    }
	} else {
	    switch (vgaBitsPerPixel) {
	      case 8:
		vga256PolyGlyphRop8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	      case 16:
		cfb16PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	      case 24:
		cfb24PolyGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
			     pglyphBase);
		break;
	    }
	}
    case rgnOUT:
	return;
    }

    /* Allocate list of pointers to glyph bitmaps. */
    glyphp = (unsigned long **)ALLOCATE_LOCAL(nglyph *
	sizeof(unsigned long *));

    destaddr = y * widthDst + (x * vgaBytesPerPixel);

    switch (vgaBitsPerPixel) {
    case 16:
	ctSETFGCOLOR16(pGC->fgPixel);
	break;
    case 24:
#ifdef CHIPS_HIQV
	ctSETFGCOLOR24(pGC->fgPixel);
#else
	{
	    /* The 6554x Blitter can only handle 8/16bpp fills 
	     * directly, Though you can do a grey fill, by a little
	     * bit of magic with the 8bpp fill */
	    unsigned char red, green, blue;

	    red = pGC->fgPixel & 0xFF;
	    green = (pGC->fgPixel >> 8) & 0xFF;
	    blue = (pGC->fgPixel >> 16) & 0xFF;
	    if (red ^ green || green ^ blue || blue ^ red) {
		cfb24TEGlyphBlt(pDrawable, pGC, xInit, yInit,
		    nglyph, ppci, pglyphBase);
		return;
	    } else {
		ctSETFGCOLOR8(pGC->fgPixel);
	    }
	}
#endif
	break;
    default:			       /* 8 */
	ctSETFGCOLOR8(pGC->fgPixel);
	break;
    }

    ctSETDSTADDR(destaddr);
    ctSETSRCADDR(0);
    ctSETPITCH(0, widthDst);
    ctSETROP(ctAluConv[GXcopy & 0xf] | ctSRCMONO | ctSRCSYSTEM |
	ctBGTRANSPARENT | ctTOP2BOTTOM | ctLEFT2RIGHT);
#ifdef CHIPS_HIQV
#if 1
    ctSETMONOCTL(ctDWORDALIGN);
#else
    ctSETMONOCTL(ctBITALIGN);
#endif
#endif
    blitwidth = glyphWidth * nglyph;
    ctSETHEIGHTWIDTHGO(h, blitwidth * vgaBytesPerPixel);
    CollectCharacters(glyphp, nglyph, pglyphBase, ppci);
    if (!ctisHiQV32 && (vgaBitsPerPixel == 24)) {
	ctTransferText24(nglyph, h, glyphp, glyphWidth, ctBltDataWindow);
    } else {
#ifdef CHIPS_HIQV
#if 1
	ctTransferText(nglyph, h, glyphp, glyphWidth, ctBltDataWindow);
	if (((((blitwidth + 31) & ~31) >> 5) * h) & 0x1)
	    *(unsigned int *)ctBltDataWindow = 0;
#else
	ctTransferTextHiQV(nglyph, h, glyphp, glyphWidth, ctBltDataWindow);
	if (((((blitwidth * h) + 31) & ~31) >> 5) & 0x1)
	    *(unsigned int *)ctBltDataWindow = 0;
#endif
#else
	ctTransferText(nglyph, h, glyphp, glyphWidth, ctBltDataWindow);
#endif
    }
#ifdef CHIPS_HIQV
    usleep(50);
    while(*(volatile unsigned int *)(ctMMIOBase + 0x10) & (0x80000000)) {
	ErrorF("Flush\n");
	*(unsigned int *)ctBltDataWindow = 0;
	usleep(5);
    }
#else
    ctBLTWAIT;
#endif
    DEALLOCATE_LOCAL(glyphp);
    return;
}

/*
 * Low-level text transfer routines. Compile it only once
 */

/* 
 * Note that the 6554x machines use 8bpp colour expansion for 24bpp.
 * Hence, 3 bits will need to be written for every bit in the image
 * glyph. I'm uncertain what effect this will have on the speed of
 * this routine. The test for the special case however slows up the
 * other colour expansions. Hence the 6554x 24bpp case is implemented
 * with its own text transfer routine
 */

#ifndef CHIPS_MMIO

#if 0 /* Not used, now in ct_textblt.s */
void
ctTransferText(nglyph, h, glyphp, glyphwidth, base)
    int nglyph;
    int h;
    unsigned long **glyphp;
    unsigned char *base;
{
    int shift;
    unsigned long dworddata;
    unsigned char *data;
    int i, line, databytes;

    /* Other character widths. Tricky, bit order is very awkward.  */
    /* We maintain a word straightforwardly LSB, and do the */
    /* bit order converting when writing 32-bit double words. */

#ifdef DEBUG
    ErrorF("CHIPS:ctTransferText(%d, %d, %d, 0x%X)\n", nglyph, h,
	glyphwidth, base);
#endif

    dworddata = 0;
    line = 0;
    shift = 0;
    data = (unsigned char *)ALLOCATE_LOCAL((nglyph * glyphwidth + 7) & ~7);
    while (line < h) {
	databytes = 0;
	i = 0;
	while (i < nglyph) {
	    dworddata += glyphp[i][line] << shift;
	    shift += glyphwidth;
	    if (shift >= 16) {
		/* Store the current word */
		*(unsigned short *)(data + databytes) =
		    byte_reversed[dworddata & 0xff] +
		    (byte_reversed[(dworddata & 0xff00) >> 8] << 8);
		databytes += 2;
		shift -= 16;
		dworddata >>= 16;
	    }
	    i++;
	}
	if (shift > 0) {
	    /* Make sure last bits of scanline are padded to byte
	     * boundary. */
	    shift = (shift + 7) & ~7;
	    *(unsigned short *)(data + databytes) =
	        byte_reversed[dworddata & 0xff] +
		(byte_reversed[(dworddata & 0xff00) >> 8] << 8);
	    databytes += 2;
	    shift = 0;
	    dworddata = 0;
	}
	if (databytes & 0x3) {
	    /* Pad the data line to a double word boundary */
	    *(unsigned short *)(data + databytes) = 0;
	    databytes += 2;
	}
	memcpy(base, data, databytes);
	line++;
    }
    DEALLOCATE_LOCAL(data);
}
#endif

void
ctTransferText24(nglyph, h, glyphp, glyphwidth, base)
    int nglyph;
    int h;
    unsigned long **glyphp;
    unsigned char *base;
{
    int shift;
    unsigned long dworddata;
    unsigned char *data;
    int i, line, databytes;

    /* Other character widths. Tricky, bit order is very awkward.  */
    /* We maintain a word straightforwardly LSB, and do the */
    /* bit order converting when writing 32-bit double words. */

#ifdef DEBUG
    ErrorF("CHIPS:ctTransferText24(%d, %d, %d, 0x%X)\n", nglyph, h,
	glyphwidth, base);
#endif

    dworddata = 0;
    line = 0;
    shift = 0;
    data = (unsigned char *)ALLOCATE_LOCAL(3 * ((nglyph * glyphwidth + 7) 
	    & ~7));
    while (line < h) {
	databytes = 0;
	i = 0;
	while (i < nglyph) {
	    dworddata += glyphp[i][line] << shift;
	    shift += glyphwidth;
	    if (shift >= 16) {
		/* Store the current word */
		*(unsigned long *)(data + 3 * databytes) =
		    (byte_reversed3[dworddata & 0xff] +
		    (byte_reversed3[(dworddata & 0xff00) >> 8] << 24)
		    & 0xffffffff);
		*(unsigned short *)(data + 3 * databytes + 4) =
		    (byte_reversed3[(dworddata & 0xff00) >> 8] >> 8);
		databytes += 2;
		shift -= 16;
		dworddata >>= 16;
	    }
	    i++;
	}
	if (shift > 0) {
	    /* Make sure last bits of scanline are padded to byte
	     * boundary. */
	    shift = (shift + 7) & ~7;
	    *(unsigned long *)(data + 3 * databytes) =
	        (byte_reversed3[dworddata & 0xff] +
		(byte_reversed3[(dworddata & 0xff00) >> 8] << 24)
		& 0xffffffff);
	    *(unsigned short *)(data + 3 * databytes + 4) =
	        (byte_reversed3[(dworddata & 0xff00) >> 8] >> 8);
	    databytes += 2;
	    shift = 0;
	    dworddata = 0;
	}
	if (databytes & 0x3) {
	    /* Pad the data line to a double word boundary */
	    *(unsigned long *)(data + 3 * databytes) = 0;
	    *(unsigned short *)(data + 3 * databytes + 4) = 0;
	    databytes += 2;
	}
	memcpy(base, data, 3 * databytes);
	line++;
    }
    DEALLOCATE_LOCAL(data);
}

#endif
