/* $XConsortium: cir_driver.h,v 1.1 94/03/28 21:48:52 dpw Exp $ */
/* $XFree86$ */
/*
 *
 * Copyright 1993 by Simon P. Cooper, New Brunswick, New Jersey, USA.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Simon P. Cooper not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Simon P. Cooper makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * SIMON P. COOPER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL SIMON P. COOPER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Simon P. Cooper, <scooper@vizlab.rutgers.edu>
 * Modified: Harm Hanemaayer, <hhanemaa@cs.ruu.nl>
 *
 * Id: cir_driver.h,v 0.7 1993/09/16 01:07:25 scooper Exp
 */

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

extern void CirrusFillRectSolidCopy();		/* GXcopy fill. */
extern void CirrusFillRectSolidGeneral();	/* Non-GXcopy fill. */
/* In cir_blt.c: */
extern void CirrusBitBlt();

/* LowlevelFuncs: */

extern void CirrusDoBitbltCopy();
extern void CirrusFillBoxSolid();

extern void CirrusFillRectOpaqueStippled32();
extern void CirrusFillRectTransparentStippled32();
extern void CirrusFillRectTile();

/* Higher level functions: */

extern void CirrusSegmentSS();
extern void CirrusLineSS();

extern void CirrusImageGlyphBlt();
extern void CirrusPolyGlyphBlt();

extern void CirrusPolyFillRect();
extern void CirrusFillSpans();

/* Low-level graphics display functions: */

/* In cir_blt.c: */
extern void CirrusBitBlt();

/* In cir_imageblt.s:  */
#if NeedFunctionPrototypes
extern void CirrusImageWriteTransfer( int w, int h, void *srcaddr,
	int srcwidth, void *vaddr );
extern void CirrusImageReadTransfer( int w, int h, void *srcaddr,
	int srcwidth, void *vaddr );
#else
extern void CirrusImageWriteTransfer();
extern void CirrusImageReadTransfer();
#endif

/* In cir_colorexp.c: */
#if NeedFunctionPrototypes
extern void CirrusColorExpand32bitFill( int x, int y, int w, int h,
	unsigned long *bits, int sh, int sox, int soy, int bg, int fg,
	int destpitch );
extern void Cirrus32bitFillSmall( int x, int y, int w, int h,
	unsigned long *bits, int sh, int sox, int soy, int bg, int fg,
	int destpitch );
extern void CirrusLatchedBitBlt( int x1, int y1, int x2, int y2,
	int w, int h, int destpitch );
extern void CirrusLatchedBitBltReversed( int x1, int y1, int x2, int y2,
	int w, int h, int destpitch );
extern void CirrusSimpleBitBlt( int x1, int y1, int x2, int y2,
  	int w, int h, int destpitch );
extern void CirrusColorExpandFillTile32( int x, int y, int w, int h,
	unsigned char *src, int tpitch, int twidth, int theight, int toy,
	int destpitch );
#else
extern void CirrusColorExpand32bitFill();
extern void Cirrus32bitFillSmall();
extern void CirrusLatchedBitBlt();
extern void CirrusLatchedBitBltReversed();
extern void CirrusSimpleBitBlt();
extern void CirrusColorExpandFillTile32();
#endif
/* In cir_blitter.c: */
#if NeedFunctionPrototypes
extern void CirrusBLTColorExpand8x8PatternFill( unsigned destaddr, int fg,
	int bg,	int w, int h, int destpitch, int rop, unsigned long pword1,
	unsigned long pword2 );
extern void CirrusBLT8x8PatternFill( unsigned destaddr, int w, int h,
	void *pattern, int destpitch, int rop );
extern void CirrusBLT16x16PatternFill( unsigned destaddr, int w, int h,
	unsigned char *pattern, int destpitch, int rop );
extern void CirrusBLTBitBlt( unsigned dstAddr, unsigned srcAddr,
	int dstPitch, int srcPitch, int w, int h, int dir );
extern void CirrusBLTWaitUntilFinished(void);	
#else
extern void CirrusBLTColorExpand8x8PatternFill();
extern void CirrusBLT8x8PatternFill();
extern void CirrusBLT16x16PatternFill();
extern void CirrusBLTBitBlt();
extern void CirrusBLTWaitUntilFinished();
#endif
/* In cir_im.c: */
extern void CirrusImageWrite();
extern void CirrusImageRead();
#if NeedPrototypes
extern void CirrusWriteBitmap( int x, int y, int w, int h,
	unsigned char *srcp, int bwidth, int bw, int bh, int box, int boy,
	int bg, int fg, int destpitch, int alu );
#else
extern void CirrusWriteBitmap();
#endif

_XFUNCPROTOEND

/* Card type variables; used for selection of accel routines. */

extern int cirrusChip;
extern int cirrusBusType;
extern Bool cirrusUseBLTEngine;

extern int CirrusMemTop;
extern int cirrusBankShift;

#define CLGD5420    0
#define CLGD5422    1
#define CLGD5424    2
#define CLGD5426    3
#define CLGD5428    4
#define CLGD5429    5
#define CLGD6205    6
#define CLGD6215    7
#define CLGD6225    8
#define CLGD6235    9
#define CLGD543x    10
#define LASTCLGD    CLGD543x

#define CIRRUS_SLOWBUS 0
#define CIRRUS_FASTBUS 1


#define CROP_0			0x00	/*     0 */
#define CROP_1			0x0E	/*     1 */
#define CROP_SRC		0x0D	/*     S */
#define CROP_DST		0x06	/*     D */
#define CROP_NOT_SRC		0xD0	/*    ~S */
#define CROP_NOT_DST		0x0B	/*    ~D */
#define CROP_AND		0x05	/*   S.D */
#define CROP_SRC_AND_NOT_DST	0x09	/*  S.~D */
#define CROP_NOT_SRC_AND_DST	0x50	/*  ~S.D */
#define CROP_NOR		0x90	/* ~S.~D */
#define CROP_OR			0x6D	/*   S+D */
#define CROP_SRC_OR_NOT_DST	0xAD	/*  S+~D */
#define CROP_NOT_SRC_OR_DST	0xD6	/*  ~S+D */
#define CROP_NAND		0xDA	/* ~S+~D */
#define CROP_XOR		0x59	/*  S~=D */
#define CROP_XNOR		0x95	/*   S=D */

/* Array that maps from alu to Cirrus ROP. */

extern int cirrus_rop[];

/* Look-up table for per-byte bit order reversal. */

extern unsigned char byte_reversed[];


#define HAVE543X() (cirrusChip == CLGD543x)

#define HAVEBITBLTENGINE() (cirrusUseBLTEngine)

#define SETWRITEMODE(n) \
	{ \
		unsigned char tmp; \
		outb(0x3ce, 0x05); \
		tmp = inb(0x3cf) & 0xf8; \
		outb(0x3cf, tmp | (n)); \
	}

#define SETFOREGROUNDCOLOR(c) \
	outw(0x3ce, 0x01 + ((c) << 8));

#define SETBACKGROUNDCOLOR(c) \
	outw(0x3ce, 0x00 + ((c) << 8));

#define SETPIXELMASK(m) \
	outw(0x3c4, 0x02 + ((m) << 8));

#define EIGHTDATALATCHES	0x08
#define EXTENDEDWRITEMODES	0x04
#define BY8ADDRESSING		0x02
#define DOUBLEBANKED		0x01
#define SINGLEBANKED		0x00

#define SETMODEEXTENSIONS(m) \
	{ \
		unsigned char tmp; \
		outb(0x3ce, 0x0b); \
		tmp = inb(0x3cf) & 0xe0; \
		outb(0x3cf, tmp | (m)); \
	}

/* We use a seperate banking routine with 16K granularity for some accel. */
/* functions. Knows about different bank granularity for 2Mb cards. */

#define setwritebank(n) \
	outw(0x3ce, 0x0a + ((n) << cirrusBankShift));

#define setreadbank(n) \
	outw(0x3ce, 0x09 + ((n) << cirrusBankShift));

#define setbank setreadbank


#if !defined(__GNUC__) || defined(NO_INLINE)
#undef __inline__
#define __inline__ /**/
#endif
