/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir_textbltc.c,v 1.3 1998/01/24 16:58:01 hohndel Exp $ */

#include        "vga256.h"
#include        "xf86.h"
#include        "vga.h" /* For vgaBase. */
#include        "vgaBank.h"

#include "compiler.h"

#include "cir_driver.h"
#include "cirBlitter.h"
#include "cir_inline.h"

/*
 * Portable low-level text transfer routines.
 *
 * The next two routines are the equivalent of the those found in
 * cir_textblt.s.  They are not currently used because they need
 * to reimplemented as 'C' functions (or at a minimum, one general
 * 'C 'function that does transfers 32 bits at a time.
 *
 * Until they are rewritten, non-ix86 platforms will use the
 * general routine below (CirrusTransferText) which still
 * uses 16 bit transfers...
 *
 * Actually, I'm not sure if any of these are still used by the xaa
 * driver, but they are apparently still referenced somewhere... 
 */
void
CirrusTransferText32bit( int nglyph, int height, unsigned long **glyphp,
     int glyphwidth, void *vaddr )
{
	ErrorF("CirrusTransferText32bit: Not implemented\n");
}

void
CirrusTransferText32bitSpecial( int nglyph, int height, unsigned long **glyphp,
     int glyphwidth, void *vaddr )
{
	ErrorF("CirrusTransferText32bitSpecial: Not Implemented\n");
}

void
CirrusTransferText(nglyph, h, glyphp, glyphwidth, base)
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
