/* $XConsortium: cir_colexp.c,v 1.1 94/03/28 21:48:36 dpw Exp $ */
/* $XFree86$ */
/*
 *
 * Copyright 1994 by H. Hanemaayer, Utrecht, The Netherlands
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of H. Hanemaayer not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  H. Hanemaayer makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * H. HANEMAAYER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL H. HANEMAAYER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  H. Hanemaayer, <hhanemaa@cs.ruu.nl>
 *
 */


/*
 * This file contains the low level accelerated functions that use color
 * expansion/extended write mode and some more framebuffer functions that
 * take advantage of the 16K bank granularity.
 */
 

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "cfb.h"
#include "cfbmskbits.h"
#include "cfbrrop.h"
#include "mergerop.h"
#include "xf86.h"
#include "vgaBank.h"
#include "vga.h"	/* For vgaInfoRec. */
#include "xf86_HWlib.h"

#ifndef __GNUC__
#undef __volatile__
#define __volatile__ volatile
#endif

#include "compiler.h"


extern pointer vgaBase;

#include "cir_driver.h"
#include "cir_inline.h"


#if __GNUC__ > 1 && defined(GCCUSESGAS)

static __inline__ void latchedcopy8( unsigned char *srcp, unsigned char *destp ) {
/* This is critical code. gcc produces unnecessary overhead with the */
/* C equivalent. */
	__asm__ __volatile__(
	"movb (%0),%%al\n\t"
	"movb %%al,(%1)\n\t"
	"movb 1(%0),%%al\n\t"
	"movb %%al,1(%1)\n\t"
	"movb 2(%0),%%al\n\t"
	"movb %%al,2(%1)\n\t"
	"movb 3(%0),%%al\n\t"
	"movb %%al,3(%1)\n\t"
	"movb 4(%0),%%al\n\t"
	"movb %%al,4(%1)\n\t"
	"movb 5(%0),%%al\n\t"
	"movb %%al,5(%1)\n\t"
	"movb 6(%0),%%al\n\t"
	"movb %%al,6(%1)\n\t"
	"movb 7(%0),%%al\n\t"
	"movb %%al,7(%1)\n\t"
	: : "r" (srcp), "r" (destp) : "ax"
	);
}

static __inline__ void latchedwrite8step4( unsigned char *destp ) {
	__asm__ __volatile__(
	"movb $0,(%0)\n\t"
	"movb $0,4(%0)\n\t"
	"movb $0,8(%0)\n\t"
	"movb $0,12(%0)\n\t"
	"movb $0,16(%0)\n\t"
	"movb $0,20(%0)\n\t"
	"movb $0,24(%0)\n\t"
	"movb $0,28(%0)\n\t"
	: : "r" (destp) : "ax"
	);
}

#else /* !defined(__GNUC__) */

static void latchedcopy8( unsigned char *srcp, unsigned char *destp ) {
/* This is optimal if the compiler stores the temporary in a register; */
/* gcc 2.4.5 does not manage this when inlined. */
	*(destp + 0) = *(srcp + 0);
	*(destp + 1) = *(srcp + 1);
	*(destp + 2) = *(srcp + 2);
	*(destp + 3) = *(srcp + 3);
	*(destp + 4) = *(srcp + 4);
	*(destp + 5) = *(srcp + 5);
	*(destp + 6) = *(srcp + 6);
	*(destp + 7) = *(srcp + 7);
}

static void latchedwrite8step4( unsigned char *destp ) {
/* This produces somewhat horrible code because of lack of registers when
 * inlined. */
	*destp = 0;	/* Latch write. */
	*(destp + 4) = 0;
	*(destp + 8) = 0;
	*(destp + 12) = 0;
	*(destp + 16) = 0;
	*(destp + 20) = 0;
	*(destp + 24) = 0;
	*(destp + 28) = 0;
}

#endif


/* Table with bit-reversed equivalent for each possible byte. */

unsigned char byte_reversed[256] = {
	0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,
	0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,0xf0,
	0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8,
	0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,0xf8,
	0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,
	0x14,0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4,
	0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec,
	0x1c,0x9c,0x5c,0xdc,0x3c,0xbc,0x7c,0xfc,
	0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2,
	0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2,
	0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,
	0x1a,0x9a,0x5a,0xda,0x3a,0xba,0x7a,0xfa,
	0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6,
	0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6,
	0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,
	0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,
	0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,0xe1,
	0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1,
	0x09,0x89,0x49,0xc9,0x29,0xa9,0x69,0xe9,
	0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,
	0x05,0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5,
	0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5,
	0x0d,0x8d,0x4d,0xcd,0x2d,0xad,0x6d,0xed,
	0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd,
	0x03,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3,
	0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,
	0x0b,0x8b,0x4b,0xcb,0x2b,0xab,0x6b,0xeb,
	0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb,
	0x07,0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7,
	0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,
	0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,
	0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,0xff,
};

/* Bit masks for left edge (indexed with first left-to-right bit number). */

static unsigned char leftbitmask[8] = {
	0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01
};

/* Bit masks for right edge (indexed with number of pixels left). */

static unsigned char rightbitmask[8] = {
	0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe
};


/*
 * This low-level function fills an area with a 32 bits wide monochrome
 * pattern of given height. The pattern scanlines are stored as 32-bit
 * integers, with the lowest-order bit corresponding to the leftmost pixel.
 * It has to reverse the per-byte bit ordering because the Cirrus color
 * expansion works that way.
 *
 * This function should work on all chipsets, i.e. 5420 up to 5429.
 * On local bus and fast processor, it can be significantly faster than the
 * 5426 bitblt engine for large solid fills.
 *
 * Arguments:
 * x, y		Coordinates of fill area.
 * w, h		Size of fill area.
 * bits_in	Pointer to array of 32-bit monochrome scanlines.
 * sh		Height of the fill pattern (size of array).
 * sox, soy	Coordinates of the origin of the pattern.
 * fg		Foreground color (bit 1 in bitmap data).
 * bg		Background color (bit 0 in bitmap data).
 * destpitch	Scanline width of screen; must be a multiple of 8.
 *
 * The function assumes vgaBase to be the address of the frame buffer window,
 * with a 32K write window at offset 0x8000. The width must be greater or
 * equal to 32.
 *
 * There's some optimization potential left; the color expansion itself
 * can be very fast (bandwidth up to 100Mpix/s) so overhead is costly.
 */

/* For solid fills, on a 486 VLB at 40 MHz, is it optimal at about
 * width >= 200) (that is, better than the 5426 blit engine).
 * It appears it can also be faster on the ISA bus.
 */

#ifdef __STDC__
void CirrusColorExpand32bitFill( int x, int y, int w, int h,
unsigned long *bits_in, int sh, int sox, int soy, int bg, int fg,
int destpitch )
#else
void CirrusColorExpand32bitFill( x, y, w, h, bits_in, sh, sox, soy, bg,
fg, destpitch )
	int x, y, w, h;
	unsigned long *bits_in;
	int sh, sox, soy, bg, fg, destpitch;
#endif
{
	unsigned long *new_bits;
	int i, j;
	int destaddr;
	unsigned char *destp;
	int bitoffset;
	int syindex;
	int bank;
	unsigned char *base;	/* Video window base address. */
	char filltype;		/* Opaque, solid or transparent. */

#define OPAQUE 0
#define SOLID 1
#define TRANSPARENT 2

	/* Reverse per-byte bit order. */
	new_bits = (unsigned long *)ALLOCATE_LOCAL(sh * 4);
	filltype = SOLID;
	for (i = 0; i < sh; i++) {
		unsigned long bits;
		if (bits_in[i] == 0xffffffff)
			new_bits[i] = 0xffffffff;
		else {
			filltype = OPAQUE;
			/* Rotate so that data is correctly aligned to */
			/* origin for writing dwords to framebuffer. */
			bits = rotateleft(32 - (sox & 31), bits_in[i]);
			/* Reverse each of the four bytes. */
			((unsigned char *)new_bits)[i * 4] =
				byte_reversed[(unsigned char)bits];
			((unsigned char *)new_bits)[i * 4 + 1] =
				byte_reversed[(unsigned char)(bits >> 8)];
			((unsigned char *)new_bits)[i * 4 + 2] =
				byte_reversed[(unsigned char)(bits >> 16)];
			((unsigned char *)new_bits)[i * 4 + 3] =
				byte_reversed[(unsigned char)(bits >> 24)];
		}
	}
	if (bg == -1)
		filltype = TRANSPARENT;

	base = (unsigned char *)vgaBase + 0x8000;	/* Write window. */
	destaddr = y * destpitch + x;

	/* Enable extended write modes and BY8 addressing. */
	/* Every addressing byte corresponds to 8 pixels. */
	SETMODEEXTENSIONS(EXTENDEDWRITEMODES | BY8ADDRESSING | DOUBLEBANKED);

	if (filltype == OPAQUE) {
		/* Set extended write mode 5, which writes both foreground */
		/* and background color, and will use the pixel mask */
		/* for edges. */
		SETWRITEMODE(5);
		SETBACKGROUNDCOLOR(bg);
	}
	else {
		/* Set extended write mode 4, which writes the foreground */
		/* at the pixels indicates by the CPU data. */
		/* This will avoid costly OUTs at the edges for solid fills*/
		/* and transparent stipples. */
		SETWRITEMODE(4);
	}

	SETPIXELMASK(0xff);
	SETFOREGROUNDCOLOR(fg);

	/* Bit offset of leftmost pixel of area to be filled. */
	bitoffset = destaddr & 7;

	destaddr >>= 3;			/* Divide address by 8. */

	bank = destaddr >> 14;		/* 16K units. */
	setwritebank(bank);
	destaddr &= 0x3fff;

	syindex = (y - soy) % sh;	/* y index into source bitmap. */

	for (j = 0; j < h; j++) {
		union {
			unsigned long dword;
			unsigned char byte[4];
		} bits;
		int count;

		if (destaddr >= 0x4000) {
			/* 16K granularity is very helpful here. */
			/* Because of the 32K window, we completely avoid */
			/* page breaks within scanlines. */
			bank++;
			setwritebank(bank);
			destaddr -= 0x4000;
		}
		destp = base + destaddr;

		/* Rotate stipple bits so that is correctly aligned for */
		/* writing aligned dwords in this scanline. It is a bit */
		/* tricky because scanlines don't necessarily start on */
		/* a dword boundary because of BY8 addressing. */
		if (filltype == SOLID)
			bits.dword = 0xffffffff;
		else
			bits.dword = rotateleft(((destaddr - (x >> 3)) & 3) << 3,
				new_bits[syindex]);

		count = w;		/* Number of pixels left. */

		/* Do first byte (left edge). */
		if (bitoffset != 0) {
			if (filltype == SOLID) {
				*destp = leftbitmask[bitoffset];
			}
			else
			if (filltype == TRANSPARENT) {
				*destp = bits.byte[(unsigned)destp & 3]
					& leftbitmask[bitoffset];
			}
			else {
				SETPIXELMASK(leftbitmask[bitoffset]);
				*destp = bits.byte[(unsigned)destp & 3];
				SETPIXELMASK(0xff);
			}
			destp++;
			count -= 8 - bitoffset;
		}

		/* Fill to dword boundary. */
		switch ((unsigned)destp & 3) {
		case 1 :
			*destp = bits.byte[1];
			*(unsigned short *)(destp + 1) =
				*(unsigned short *)(&bits.byte[2]);
			destp += 3;
			count -= 24;
			break;
		case 2 :
			*(unsigned short *)destp =
				*(unsigned short *)(&bits.byte[2]);
			destp += 2;
			count -= 16;
			break;
		case 3 :
			*destp = bits.byte[3];
			destp++;
			count -= 8;
		}

		/* Fill dwords (32 pixels). */
		__memsetlong((unsigned long *)destp, bits.dword, count / 32);
		destp += (count / 32) * 4;
		count &= 31;

		/* Fill remaining bytes (8 pixels). */
		switch (count >> 3) {
		case 1 :
			*destp = bits.byte[0];
			destp++;
			count -= 8;
			break;
		case 2 :
			*(unsigned short *)destp =
				*(unsigned short *)(&bits.byte[0]);
			destp += 2;
			count -= 16;
			break;
		case 3 :
			*(unsigned short *)destp =
				*(unsigned short *)(&bits.byte[0]);
			*(destp + 2) = bits.byte[2];
			destp += 3;
			count -= 24;
		}

		/* Do last byte (right edge). */
		if (count != 0)
			if (filltype == SOLID)
				*destp = rightbitmask[count];
			else
			if (filltype == TRANSPARENT)
				*destp = bits.byte[(unsigned)destp & 3]
					& rightbitmask[count];
			else {
				SETPIXELMASK(rightbitmask[count]);
				*destp = bits.byte[(unsigned)destp & 3];
				SETPIXELMASK(0xff);
			}

		destaddr += (destpitch >> 3);

		syindex++;
		if (syindex == sh)
			syindex = 0;	/* Wrap pattern vertically. */
	}

	/* Disable extended write modes and BY8 addressing. */
	SETMODEEXTENSIONS(DOUBLEBANKED);

	SETWRITEMODE(0);

	SETFOREGROUNDCOLOR(0x00);	/* Disable set/reset. */

	DEALLOCATE_LOCAL(new_bits);
}


/*
 * This is the special function for small widths (< 32).
 * Just uses the banked plain framebuffer.
 * It is somewhat faster than the cfb.banked equivalents (it takes
 * advantage of the 16K bank granularity).
 *
 * Currently not used because of bug (see xgc time percentage scrollbar).
 */

#ifdef __STDC__
void Cirrus32bitFillSmall( int x, int y, int w, int h,
unsigned long *bits_in, int sh, int sox, int soy, int bg, int fg,
int destpitch )
#else
void Cirrus32bitFillSmall( x, y, w, h, bits_in, sh, sox, soy, bg, fg,
destpitch )
	int x, y, w, h;
	unsigned long *bits_in;
	int sh, sox, soy, bg, fg, destpitch;
#endif
{
	int j;
	int destaddr;
	unsigned char *destp;
	int syindex;
	int bank;
	unsigned char *base;	/* Video window base address. */
	unsigned char color[2];

	base = (unsigned char *)vgaBase + 0x8000;	/* Write window. */
	destaddr = y * destpitch + x;

	bank = destaddr >> 14;
	setwritebank(bank);
	destaddr &= 0x3fff;

	color[0] = bg;
	color[1] = fg;

	syindex = (y - soy) % sh;	/* y index into source bitmap. */

	for (j = 0; j < h; j++) {
		unsigned long bits;
		int count;
		
		bits = rotateleft(32 - (sox & 31), bits_in[syindex]);

		if (destaddr >= 0x4000) {
			bank++;
			setwritebank(bank);
			destaddr -= 0x4000;
		}
		destp = base + destaddr;

		count = w;
		if (bits == 0xffffffff)
			__memset(destp, fg, w);
		else {
			while (count > 8) {
				*destp = color[bits & 1]; bits >>= 1;
				*(destp + 1) = color[bits & 1]; bits >>= 1;
				*(destp + 2) = color[bits & 1]; bits >>= 1;
				*(destp + 3) = color[bits & 1]; bits >>= 1;
				*(destp + 4) = color[bits & 1]; bits >>= 1;
				*(destp + 5) = color[bits & 1]; bits >>= 1;
				*(destp + 6) = color[bits & 1]; bits >>= 1;
				*(destp + 7) = color[bits & 1]; bits >>= 1;
				destp += 8;
				count -= 8;
			}
			while (count > 0) {
				*destp = color[bits & 1];
				bits >>= 1;
				destp++;
				count--;
			}
		}

		destaddr += destpitch;

		syindex++;
		if (syindex == sh)
			syindex = 0;	/* Wrap pattern vertically. */
	}
}


/*
 * This is a bitblt function. It takes advantage of the 8 data latches that
 * can be enabled in BY8 addressing mode to do efficient vertical bitblts.
 * I believe this makes scrolling bearable on the chips that don't
 * have the bitblt engine, i.e. <= 5424. On a local bus, it may even
 * rival the bitblt engine in speed.
 *
 * Arguments:
 * x1, y1	Coordinates of source area.
 * x2, y2	Coordinates of destination area.
 * w, h		Size of area to be copied.
 * destpitch	Scanline width of screen in bytes.
 *
 * x1 must be equal to x2 (actually works if (x1 % 8 == x2 % 8)).
 * Copies from top to bottom. For overlapping areas, correct if
 * (y1 > y2 || (y1 == y2 && x1 > x2).
 *
 * The data latches work similar to VGA write mode 1 (for planar modes).
 * Basically, a read from display memory fills the 8 latches with 8 pixels,
 * and subsequent writes (CPU data written doesn't matter) will each write
 * the 8 pixels stored in the latches.
 */
 
/* #define USE_MEMCPYB */

#ifdef __STDC__
void CirrusLatchedBitBlt( int x1, int y1, int x2, int y2, int w, int h,
int destpitch )
#else
void CirrusLatchedBitBlt( x1, y1, x2, y2, w, h, destpitch )
	int x1, y1, x2, y2, w, h, destpitch;
#endif
{
	int j;
	int destaddr, srcaddr;
	unsigned char *destp, *srcp;
	int writebank, readbank;
	int bitoffset;

	unsigned char *base;	/* Video window base address. */

	base = vgaBase;			/* Read window. */
					/* Write window is at offset 0x8000 */
	destaddr = y2 * destpitch + x2;
	srcaddr = y1 * destpitch + x1;

	/* Enable extended write modes, BY8 addressing, and 8 byte data */
	/* latches. Every addressing byte corresponds to 8 pixels. */
	SETMODEEXTENSIONS(EXTENDEDWRITEMODES | BY8ADDRESSING |
		EIGHTDATALATCHES | DOUBLEBANKED);

	SETWRITEMODE(1);

	SETPIXELMASK(0xff);
	SETFOREGROUNDCOLOR(0);		/* Disable set/reset. */

	/* Bit offset of leftmost pixel of area to be filled. */
	bitoffset = destaddr & 7;

	destaddr >>= 3;			/* Divide address by 8. */
	srcaddr >>= 3;

	writebank = destaddr >> 14;	/* 16K units. */
	setwritebank(writebank);
	readbank = srcaddr >> 14;
	setreadbank(readbank);
	destaddr &= 0x3fff;
	srcaddr &= 0x3fff;

	for (j = 0; j < h; j++) {
		int count;

		if (destaddr >= 0x4000) {
			/* 16K granularity is very helpful here. */
			/* Because of the 32K window, we completely avoid */
			/* page breaks within scanlines. */
			writebank++;
			setwritebank(writebank);
			destaddr -= 0x4000;
		}
		if (srcaddr >= 0x4000) {
			readbank++;
			setreadbank(readbank);
			srcaddr -= 0x4000;
		}
		/* Address in write window. */
		destp = base + 0x8000 + destaddr;
		/* Address in read window. */
		srcp = base + srcaddr;

		count = w;		/* Number of pixels left. */

		/* Do first byte (left edge). */
		if (bitoffset != 0) {
			SETPIXELMASK(leftbitmask[bitoffset]);
			/* Write mode 1 latch read/write. */
			*destp = *srcp;
			SETPIXELMASK(0xff);
			destp++;
			srcp++;
			count -= 8 - bitoffset;
		}

#ifndef USE_MEMCPYB
		/* Using rep movsb here would seem appropriate, but I */
		/* think some recent non-Intel chips actually try */
		/* to optimize that instruction by writing words, which */
		/* would break this. */
		while (count >= 64) {
			/* Write mode 1 latch read/write. */
			latchedcopy8(srcp, destp);
			destp += 8;
			srcp += 8;
			count -= 64;
		}
		while (count >= 8) {
			/* Write mode 1 latch read/write. */
			*destp = *srcp;
			destp++;
			srcp++;
			count -= 8;
		}
#else
		__memcpyb(destp, srcp, count >> 3);
		destp += count >> 3;
		srcp += count >> 3;
		count &= 7;
#endif

		/* Do last byte (right edge). */
		if (count != 0) {
			SETPIXELMASK(rightbitmask[count]);
			/* Write mode 1 latch read/write. */
			*destp = *srcp;
			SETPIXELMASK(0xff);
		}

		destaddr += (destpitch >> 3);
		srcaddr += (destpitch >> 3);
	}

	/* Disable extended write modes and BY8 addressing. */
	SETMODEEXTENSIONS(DOUBLEBANKED);

	SETWRITEMODE(0);

	SETFOREGROUNDCOLOR(0x00);	/* Disable set/reset. */
}


void CirrusLatchedBitBltReversed( x1, y1, x2, y2, w, h, destpitch )
	int x1, y1, x2, y2, w, h, destpitch;
{
	int j;
	int destaddr, srcaddr;
	unsigned char *destp, *srcp;
	int writebank, readbank;
	int bitoffset;

	unsigned char *base;	/* Video window base address. */

	base = vgaBase;			/* Read window. */
					/* Write window is at offset 0x8000 */
	destaddr = (y2 + h - 1) * destpitch + x2;
	srcaddr = (y1 + h - 1) * destpitch + x1;

	/* Enable extended write modes, BY8 addressing, and 8 byte data */
	/* latches. Every addressing byte corresponds to 8 pixels. */
	SETMODEEXTENSIONS(EXTENDEDWRITEMODES | BY8ADDRESSING |
		EIGHTDATALATCHES | DOUBLEBANKED);

	SETWRITEMODE(1);

	SETPIXELMASK(0xff);
	SETFOREGROUNDCOLOR(0);		/* Disable set/reset. */

	/* Bit offset of leftmost pixel of area to be filled. */
	bitoffset = destaddr & 7;

	destaddr >>= 3;			/* Divide address by 8. */
	srcaddr >>= 3;

	writebank = destaddr >> 14;	/* 16K units. */
	setwritebank(writebank);
	readbank = srcaddr >> 14;
	setreadbank(readbank);
	destaddr &= 0x3fff;
	srcaddr &= 0x3fff;

	for (j = 0; j < h; j++) {
		int count;

		if (destaddr < 0) {
			/* 16K granularity is very helpful here. */
			/* Because of the 32K window, we completely avoid */
			/* page breaks within scanlines. */
			writebank--;
			setwritebank(writebank);
			destaddr += 0x4000;
		}
		if (srcaddr < 0) {
			readbank--;
			setreadbank(readbank);
			srcaddr += 0x4000;
		}
		/* Address in write window. */
		destp = base + 0x8000 + destaddr;
		/* Address in read window. */
		srcp = base + srcaddr;

		count = w;		/* Number of pixels left. */

		/* Do first byte (left edge). */
		if (bitoffset != 0) {
			SETPIXELMASK(leftbitmask[bitoffset]);
			/* Write mode 1 latch read/write. */
			*destp = *srcp;
			SETPIXELMASK(0xff);
			destp++;
			srcp++;
			count -= 8 - bitoffset;
		}

#ifndef USE_MEMCPYB
		/* Using rep movsb here would seem appropriate, but I */
		/* think some recent non-Intel chips actually try */
		/* to optimize that instruction by writing words, which */
		/* would break this. */
		while (count >= 64) {
			/* Write mode 1 latch read/write. */
			latchedcopy8(srcp, destp);
			destp += 8;
			srcp += 8;
			count -= 64;
		}
		while (count >= 8) {
			/* Write mode 1 latch read/write. */
			*destp = *srcp;
			destp++;
			srcp++;
			count -= 8;
		}
#else
		__memcpyb(destp, srcp, count >> 3);
		destp += count >> 3;
		srcp += count >> 3;
		count &= 7;
#endif

		/* Do last byte (right edge). */
		if (count != 0) {
			SETPIXELMASK(rightbitmask[count]);
			/* Write mode 1 latch read/write. */
			*destp = *srcp;
			SETPIXELMASK(0xff);
		}

		destaddr -= (destpitch >> 3);
		srcaddr -= (destpitch >> 3);
	}

	/* Disable extended write modes and BY8 addressing. */
	SETMODEEXTENSIONS(DOUBLEBANKED);

	SETWRITEMODE(0);

	SETFOREGROUNDCOLOR(0x00);	/* Disable set/reset. */
}


/*
 * Conventional framebuffer bitblt; no (x1 & 7 == x2 & 7) restriction.
 */

#ifdef __STDC__
void CirrusSimpleBitBlt( int x1, int y1, int x2, int y2, int w, int h,
int destpitch )
#else
void CirrusSimpleBitBlt( x1, y1, x2, y2, w, h, destpitch )
	int x1, y1, x2, y2, w, h, destpitch;
#endif
{
	int j;
	int destaddr, srcaddr;
	unsigned char *destp, *srcp;
	int writebank, readbank;
	int bitoffset;
	int syindex;

	unsigned char *base;	/* Video window base address. */
	int saveGRB;

	base = vgaBase;			/* Read window. */
					/* Write window is at offset 0x8000 */
	destaddr = y2 * destpitch + x2;
	srcaddr = y1 * destpitch + x1;

	writebank = destaddr >> 14;	/* 16K units. */
	setwritebank(writebank);
	readbank = srcaddr >> 14;
	setreadbank(readbank);
	destaddr &= 0x3fff;
	srcaddr &= 0x3fff;

	for (j = 0; j < h; j++) {
		if (destaddr >= 0x4000) {
			/* 16K granularity is very helpful here. */
			/* Because of the 32K window, we completely avoid */
			/* page breaks within scanlines. */
			writebank++;
			setwritebank(writebank);
			destaddr -= 0x4000;
		}
		if (srcaddr >= 0x4000) {
			readbank++;
			setreadbank(readbank);
			srcaddr -= 0x4000;
		}
		/* Address in write window. */
		destp = base + 0x8000 + destaddr;
		/* Address in read window. */
		srcp = base + srcaddr;

		__memcpy(destp, srcp, w);

		destaddr += destpitch;
		srcaddr += destpitch;
	}
}


/*
 * This function uses the 8 data latches for fast 32 pixel wide tile fill.
 *
 * Arguments:
 * x, y		Coordinates of the destination area.
 * w, h		Size of the area.
 * src		Pointer to tile.
 * tpitch	Width of a tile line in bytes.
 * twidth	Width of tile (should be 32).
 * theight	Height of tile.
 * toy		Tile y-origin.
 * [tox]	Tile x-origin. [currently excluded]
 *
 * This functions is limited.
 * It only works for x coordinates that are a multiple of 8, and
 * width also a multiple of 8.
 * The width must be >= 32.
 * x-origin must correspond with the x coordinate.
 */

#ifdef __STDC__
extern void CirrusColorExpandFillTile32( int x, int y, int w, int h,
unsigned char *src, int tpitch, int twidth, int theight, int toy, int destpitch )
#else
extern void CirrusColorExpandFillTile32( x, y, w, h, src, tpitch, twidth,
theight, toy, destpitch )
	int x, y, w, h;
	unsigned char *src;
	int tpitch, twidth, theight, toy, destpitch;
#endif
{
	int i, k;
	int destaddr;
	register unsigned char *destp;
	int writebank;
	int tyindex;
	unsigned char *vtilep, *vtilelinep;
	unsigned char *base;	/* Video window base address. */
	int saveGRB;
	int tcount, chunk8_tcount[4];

	base = vgaBase;			/* Read window. */
					/* Write window is at offset 0x8000 */

	/* First write the tile at the top of memory. */
	/* We use max. 256 bytes of memory. */
        setwritebank(CirrusMemTop >> 14);
	for (i = 0; i < theight; i++) {
		if (i >= h)
			break;
		__memcpy(base + 0x8000 + (CirrusMemTop & 0x3fff) + i * 32,
			src + i * tpitch, twidth);
	}

	/* Pointer to tile in read window in BY8 addressing mode. */
	setreadbank((CirrusMemTop >> 3) >> 14);
	vtilep = base + ((CirrusMemTop >> 3) & 0x3fff);

	destaddr = y * destpitch + x;

	/* Enable extended write modes, BY8 addressing, and 8 byte data */
	/* latches. Every addressing byte corresponds to 8 pixels. */
	SETMODEEXTENSIONS(EXTENDEDWRITEMODES | BY8ADDRESSING |
		EIGHTDATALATCHES | DOUBLEBANKED);

	SETWRITEMODE(1);

	SETPIXELMASK(0xff);
	SETFOREGROUNDCOLOR(0);		/* Disable set/reset. */

	destaddr >>= 3;			/* Divide address by 8. */

	writebank = destaddr >> 14;	/* 16K units. */
	setwritebank(writebank);

	/* Current tile line index. */
	tyindex = (y - toy) % theight;

	vtilelinep = vtilep + tyindex * 4;

	/* Number of full tile 'widths'. */
	tcount = w / 32;
	/* Calculate how many full 8 pixels chunks we can write per */
	/* scanline for each of the 4 chunks in the tile line. */
	for (i = 0; i < 4; i++) {
		int cnt;
		cnt = tcount;
		if (i < ((w >> 3) & 3))
			/* Partial tile includes this 8 pixel chunk. */
			cnt++;
		chunk8_tcount[i] = cnt;
	}

	for (i = 0; i < h; i++) {
		unsigned char *linep;

		if (destaddr >= 0x4000) {
			/* 16K granularity is very helpful here. */
			/* Because of the 32K window, we completely avoid */
			/* page breaks within scanlines. */
			writebank++;
			setwritebank(writebank);
			destaddr -= 0x4000;
		}
		/* Address in write window. */
		linep = base + 0x8000 + destaddr;

		/* vtilelinep is pointer to tile line in video memory. */

		for (k = 0; k < 4; k++) {
		/* Handle tile line pixels (k * 8) to ((k + 1) * 8 - 1). */
			unsigned char j;
			__volatile__ unsigned char tmp;
			/* Read 8 tile pixels into latches. */
			tmp = vtilelinep[k];
			/* Now write them at every 32th pixel offset */
			/* in this scanline. */
			destp = linep + k;
			j = chunk8_tcount[k];
			while (j >= 8) {
				latchedwrite8step4(destp);
				destp += 32;
				j -= 8;
			}
			switch (j) {
			/* Fall through. */
			case 7 : *destp = 0; destp += 4;
			case 6 : *destp = 0; destp += 4;
			case 5 : *destp = 0; destp += 4;
			case 4 : *destp = 0; destp += 4;
			case 3 : *destp = 0; destp += 4;
			case 2 : *destp = 0; destp += 4;
			case 1 : *destp = 0;
			case 0 : break;
			}
		}

		destaddr += (destpitch >> 3);

		tyindex++;
		vtilelinep += 4;
		if (tyindex == theight) {
			tyindex = 0;
			vtilelinep = vtilep;
		}
	}

	/* Disable extended write modes and BY8 addressing. */
	SETMODEEXTENSIONS(DOUBLEBANKED);

	SETWRITEMODE(0);

	SETFOREGROUNDCOLOR(0x00);	/* Disable set/reset. */
}
