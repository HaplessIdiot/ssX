/* $XConsortium: cir_teblt8.c,v 1.2 94/04/17 20:32:34 dpw Exp $ */
/* $XFree86$ */
/*
 * TEGblt - ImageText expanded glyph fonts only.  For
 * 8 bit displays, in Copy mode with no clipping.
 */


/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
*/

/*
 * Modified for Cirrus by Harm Hanemaayer (hhanemaa@cs.ruu.nl).
 *
 * We accelerate straightforward text writing for fonts with widths up to 16
 * pixels.
 */
 

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"servermd.h"
#include	"cfb.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"cfbmskbits.h"
#include	"cfb8bit.h"
#include	"xf86.h"
#include	"vga.h"	/* For vgaBase. */
#include        "vgaBank.h"
/* #include        "vgaFasm.h" */
#include        "cfbfuncs.h"

#include "compiler.h"

#include "cir_driver.h"
#include "cir_blitter.h"


extern void speedupcfbTEGlyphBlt8();	/* Doesn't support clipping. */
extern void cfbImageGlyphBlt8();
extern void miPolyGlyphBlt();


void CirrusTransferTextWidth8();
void CirrusTransferTextWidth6();
void CirrusTransferText();		/* General, for widths <= 16. */


void CirrusImageGlyphBlt(pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	xInit, yInit;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    FontPtr		pfont = pGC->font;
    unsigned long	*pdstBase;
    int			widthDst;
    int			widthGlyph;
    int			h;
    int			x, y;
    BoxRec		bbox;		/* for clipping */

    	int glyphWidth;		/* Character width in pixels. */
    	int glyphWidthBytes;	/* Character width in bytes (padded). */
	int i;
	/* Characters are padded to 4 bytes. */
	unsigned long **glyphp;
	int shift, line;
	unsigned dworddata;
	int destaddr, blitwidth;

	glyphWidth = FONTMAXBOUNDS(pfont,characterWidth);
	glyphWidthBytes = GLYPHWIDTHBYTESPADDED(*ppci);

	cfbGetLongWidthAndPointer(pDrawable, widthDst, pdstBase)
	widthDst *= 4;		/* Convert to bytes. */

	glyphWidth = FONTMAXBOUNDS(pfont,characterWidth);
	glyphWidthBytes = GLYPHWIDTHBYTESPADDED(*ppci);

	/* We only accelerate fonts 16 or less pixels wide. */
	/* Let cfb handle writing into offscreen pixmap. */
	if (glyphWidthBytes != 4 || glyphWidth > 16 || !CHECKSCREEN(pdstBase)
	|| !xf86VTSema) {
	        cfbImageGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci,
	        	pglyphBase);
		return;	        
	}

    h = FONTASCENT(pfont) + FONTDESCENT(pfont);

    if ((h | glyphWidth) == 0) return;

    x = xInit + FONTMAXBOUNDS(pfont,leftSideBearing) + pDrawable->x;
    y = yInit - FONTASCENT(pfont) + pDrawable->y;
    bbox.x1 = x;
    bbox.x2 = x + (glyphWidth * nglyph);
    bbox.y1 = y;
    bbox.y2 = y + h;

    switch ((*pGC->pScreen->RectIn)(
                ((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip, &bbox))
    {
      case rgnPART:
	cfbImageGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase);
      case rgnOUT:
	return;
    }

    if (!cfb8CheckPixels (pGC->fgPixel, pGC->bgPixel))
	cfb8SetPixels (pGC->fgPixel, pGC->bgPixel);

	/* Collect list of pointers to glyph bitmaps. */
	glyphp = (unsigned long **)ALLOCATE_LOCAL(nglyph * sizeof(unsigned long *));
	for (i = 0; i < nglyph; i++ ) {
		glyphp[i] = (unsigned long *)FONTGLYPHBITS(pglyphBase, *ppci++);
	}

	destaddr = y * widthDst + x;
	SETDESTADDR(destaddr);
	SETDESTPITCH(widthDst);
	SETSRCADDR(0);
	SETSRCPITCH(0);
	blitwidth = glyphWidth * nglyph;
	SETWIDTH(blitwidth);
	SETHEIGHT(h);

	SETBACKGROUNDCOLOR(pGC->bgPixel);
	SETFOREGROUNDCOLOR(pGC->fgPixel);

	SETBLTMODE(SYSTEMSRC | COLOREXPAND);
	SETROP(CROP_SRC);
	STARTBLT();

	/* Problem: must synthesize bitmap. The current code works reasonably
	 * efficiently for 6 and 8 pixel wide fonts, other widths (up to 16)
	 * are less efficiently handled.
	 */

	/* Write bitmap to video memory (for BitBlt engine to process). */
	/* Gather bytes until we have a dword to write. Doubleword is   */
	/* LSByte first, and MSBit first in each byte, as required for  */
	/* the blit data. */

	switch (glyphWidth) {
	case 8 :
		/* 8 pixel wide font, easier and faster. */
		CirrusTransferTextWidth8(nglyph, h, glyphp);
		break;
	case 6 :
		CirrusTransferTextWidth6(nglyph, h, glyphp);
		break;
	default :
		CirrusTransferText(nglyph, h, glyphp, glyphWidth, vgaBase);
		break;
	}

	WAITUNTILFINISHED();

	SETBACKGROUNDCOLOR(0x0f);
	SETFOREGROUNDCOLOR(0);
	
	DEALLOCATE_LOCAL(glyphp);
}


/*
 * Low-level text transfer routines.
 */

void CirrusTransferTextWidth8(nglyph, h, glyphp)
	int nglyph;
	int h;
	unsigned long **glyphp;
{
	int shift;
	unsigned long dworddata;
	int i, line;

	shift = 0;
	dworddata = 0;
	line = 0;
	while (line < h) {
		i = 0;
		while (i < nglyph) {
			if (shift == 0 && nglyph - i >= 8) {
				/* Unroll loop. */
				dworddata = byte_reversed[glyphp[i][line]];
				dworddata += byte_reversed[glyphp[i + 1][line]] << 8;
				dworddata += byte_reversed[glyphp[i + 2][line]] << 16;
				dworddata += byte_reversed[glyphp[i + 3][line]] << 24;
				*(unsigned long *)vgaBase = dworddata;
				dworddata = byte_reversed[glyphp[i + 4][line]];
				dworddata += byte_reversed[glyphp[i + 5][line]] << 8;
				dworddata += byte_reversed[glyphp[i + 6][line]] << 16;
				dworddata += byte_reversed[glyphp[i + 7][line]] << 24;
				*(unsigned long *)vgaBase = dworddata;
				i += 8;
				dworddata = 0;
			}
			else {
				dworddata += byte_reversed[glyphp[i][line]] << shift;
				shift += 8;
				i++;
			}
			if (shift == 32) {
				/* Write the dword. */
				*(unsigned long *)vgaBase = dworddata;
				shift = 0;
				dworddata = 0;
			}
		}
		line++;
	}
	if (shift != 0)
		*(unsigned long *)vgaBase = dworddata;
}


void CirrusTransferTextWidth6(nglyph, h, glyphp)
	int nglyph;
	int h;
	unsigned long **glyphp;
{
	int shift;
	unsigned long dworddata;
	int i, line;

	/* Special case for fixed font. Tricky, bit order is very awkward. */
	/* We maintain a word straightforwardly LSB, and do the bit order  */
	/* converting when writing 16-bit words. */

	dworddata = 0;
	line = 0;
	shift = 0;
	while (line < h) {
		i = 0;
		while (i < nglyph) {
			if (shift == 0 && nglyph - i >= 8) {
				/* Speed up 8 character chunks. */
				/* Bit order conversion is done directly. */
				/* 3 16-bit words written (48 bits). */
				unsigned byte2, byte3, byte6, byte7;
				dworddata = byte_reversed[glyphp[i][line]];
				byte2 = byte_reversed[glyphp[i + 1][line]];
				dworddata += byte2 >> 6;
				dworddata += (byte2 & 0x3c) << 10;
				byte3 = byte_reversed[glyphp[i + 2][line]];
				dworddata += (byte3 & 0xf0) << 4;
				dworddata += (byte3 & 0x0c) << 20;
				dworddata += byte_reversed[glyphp[i + 3][line]] << 14;
				dworddata += byte_reversed[glyphp[i + 4][line]] << 24;
				byte6 = byte_reversed[glyphp[i + 5][line]];
				dworddata += (byte6 & 0xc0) << 18;
				*(unsigned long *)vgaBase = dworddata;
				dworddata = (byte6 & 0x3c) << 2;
				byte7 = byte_reversed[glyphp[i + 6][line]];
				dworddata += (byte7 & 0xf0) >> 4;
				dworddata += (byte7 & 0x0c) << 12;
				dworddata += byte_reversed[glyphp[i + 7][line]] << 6;
				*(unsigned short *)vgaBase = dworddata;
				i += 8;
				dworddata = 0;
			}
			else {
				dworddata += glyphp[i][line] << shift;
				shift += 6;
				i++;
			}
			if (shift >= 16) {
				/* Write a 16-bit word. */
				*(unsigned short *)vgaBase =
					byte_reversed[dworddata & 0xff] +
					(byte_reversed[(dworddata & 0xff00) >> 8] << 8);
				shift -= 16;
				dworddata >>= 16;
			}
		}
		if (shift > 0) {
			/* Make sure last bits of scanline are padded to byte
			 * boundary. */
			shift = (shift + 7) & ~7;
			if (shift >= 16) {
				/* Write a 16-bit word. */
				*(unsigned short *)vgaBase =
					byte_reversed[dworddata & 0xff] +
					(byte_reversed[(dworddata & 0xff00) >> 8] << 8);
				shift -= 16;
				dworddata >>= 16;
			}
		}
		line++;
	}

	{
		/* There are (shift) bits left. */
		unsigned data;
		int bytes;
		data = byte_reversed[dworddata & 0xff] +
			(byte_reversed[(dworddata & 0xff00) >> 8] << 8);
		/* Number of bytes of real bitmap data. */
		bytes = ((nglyph * 6 + 7) >> 3) * h;
		/* We must transfer a multiple of 4 bytes in total. */
		if ((bytes - ((shift + 7) >> 3)) & 2)
			*(unsigned short *)vgaBase = data;
		else
			if (shift != 0)
				*(unsigned long *)vgaBase = data;
	}
}


#if 0	/* Replaced by assembler routine in cir_textblt.s. */

void CirrusTransferText(nglyph, h, glyphp, glyphwidth, base)
	int nglyph;
	int h;
	unsigned long **glyphp;
	unsigned char *base;
{
	int shift;
	unsigned long dworddata;
	int i, line;

	/* Other character widths. Tricky, bit order is very awkward.  */
	/* We maintain a word straightforwardly LSB, and do the */
	/* bit order converting when writing 16-bit words. */

	dworddata = 0;
	line = 0;
	shift = 0;
	while (line < h) {
		i = 0;
		while (i < nglyph) {
			dworddata += glyphp[i][line] << shift;
			shift += glyphwidth;
			if (shift >= 16) {
				/* Write a 16-bit word. */
				*(unsigned short *)base =
					byte_reversed[dworddata & 0xff] +
					(byte_reversed[(dworddata & 0xff00) >> 8] << 8);
				shift -= 16;
				dworddata >>= 16;
			}
			i++;
		}
		if (shift > 0) {
			/* Make sure last bits of scanline are padded to byte
			 * boundary. */
			shift = (shift + 7) & ~7;
			if (shift >= 16) {
				/* Write a 16-bit word. */
				*(unsigned short *)base =
					byte_reversed[dworddata & 0xff] +
					(byte_reversed[(dworddata & 0xff00) >> 8] << 8);
				shift -= 16;
				dworddata >>= 16;
			}
		}
		line++;
	}

	{
		/* There are (shift) bits left. */
		unsigned data;
		int bytes;
		data = byte_reversed[dworddata & 0xff] +
			(byte_reversed[(dworddata & 0xff00) >> 8] << 8);
		/* Number of bytes of real bitmap data. */
		bytes = ((nglyph * glyphwidth + 7) >> 3) * h;
		/* We must transfer a multiple of 4 bytes in total. */
		if ((bytes - ((shift + 7) >> 3)) & 2)
			*(unsigned short *)base = data;
		else
			if (shift != 0)
				*(unsigned long *)base = data;
	}
}

#endif
