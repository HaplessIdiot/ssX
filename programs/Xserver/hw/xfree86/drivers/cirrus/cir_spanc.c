/* $XFree86: $ */
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
 * This file contains portable versions of the assembly routines in cir_span.s
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
#include "cir_driver.h"
#include "cir_inline.h"
#include "cir_span.h"

extern pointer vgaBase;

void
CirrusLatchCopySpans(srcp, destp, bcount, n, destpitch)
	unsigned char *srcp;
	unsigned char *destp;
	int bcount;
	int n;
	int destpitch;
{
	int i;
	for (i = 0; i < n; i++) {
		int j;
		for (j = 0; j < bcount; j++)
			*(destp + j) = *(srcp + j);
		srcp += destpitch;
		destp += destpitch;
	}
}


void
CirrusColorExpandWriteSpans(destp, leftmask, leftbcount,
			    midlcount, rightbcount, rightmask, h, destpitch)
	unsigned char *destp;
	int leftmask, leftbcount, midlcount, rightbcount, rightmask;
	int h;
	int destpitch;
{
	while (h > 0) {
		unsigned char *destpsave;
		int i;
		destpsave = destp;
		if (leftbcount > 0) {
			*destp = leftmask;
			destp++;
			for (i = 0; i < leftbcount - 1; i++) {
				*destp = 0xff;
				destp++;
			}
		}
		for (i = 0; i < midlcount; i++) {
			*(unsigned long *)destp = 0xffffffff;
			destp += 4;
		}
		if (rightbcount > 1) 
			for (i = 0; i < rightbcount - 1; i++) {
				*destp = 0xff;
				destp++;
			}
		if (rightbcount > 0)
			*destp = rightmask;
		destp = destpsave + destpitch;
		h--;
	}
}


void
CirrusColorExpandWriteStippleSpans(destp, stippleleftmask, leftbcount,
				   midlcount, rightbcount, stipplerightmask, h, stippleword, destpitch)
	unsigned char *destp;
	int stippleleftmask, leftbcount, midlcount, rightbcount;
	int stipplerightmask;
	int h;
	unsigned long stippleword;
	int destpitch;
{
	while (h > 0) {
		unsigned char *destpsave;
		int i;
		destpsave = destp;
		switch (leftbcount) {
		case 1 :
			*destp = stippleleftmask;
			destp++;
			break;
		case 2 :
			*destp = stippleleftmask;
			*(destp + 1) = stippleword >> 24;
			destp += 2;
			break;
		case 3 :
			*destp = stippleleftmask;
			*(destp + 1) = stippleword >> 16;
			*(destp + 2) = stippleword >> 24;
			destp += 3;
			break;
		case 4 :
			*destp = stippleleftmask;
			*(destp + 1) = stippleword >> 8;
			*(unsigned short *)(destp + 2) = stippleword >> 16;
			destp += 4;
			break;
		}
		for (i = 0; i < midlcount; i++) {
			*(unsigned long *)destp = stippleword;
			destp += 4;
		}
		switch (rightbcount) {
		case 1 :
			*destp = stipplerightmask;
			break;
		case 2 :
			*destp = stippleword;
			*(destp + 1) = stipplerightmask;
			break;
		case 3 :
			*(unsigned short *)destp = stippleword;
			*(destp + 2) = stipplerightmask;
			break;
		case 4 :
			*(unsigned short *)destp = stippleword;
			*(destp + 2) = stippleword >> 16;
			*(destp + 3) = stipplerightmask;
			break;
		}
		destp = destpsave + destpitch;
		h--;
	}
}

/*
 * This function uses the 8 data latches for fast multiple-of-8 pixel wide
 * tile fill. Divides into banking regions.
 * Supports tilewidths of 8, 16, 24, 32, 40, 48, 56 and 64.
 *
 * Arguments:
 * x, y		Coordinates of the destination area.
 * w, h		Size of the area.
 * vtileaddr	Offset in video memory of tile.
 * tpitch	Width of a tile line in bytes.
 * tbytes	Width of tile / 8.
 * theight	Height of tile.
 * toy		Tile y-origin.
 *
 * w must be >= 32.
 * Tile must be aligned to start of scanline.
 */
void
CirrusLatchWriteTileSpans(destp, count, tbytes, linecount, vpitch)
	unsigned char *destp;
	int tbytes, count, linecount, vpitch;
{
	int i;
	for (i = 0; i < linecount; i++) {
		int j;
		for (j = 0; j < count; j++)
			*(destp + j * tbytes) = 0;
		destp += vpitch;
	}
}
