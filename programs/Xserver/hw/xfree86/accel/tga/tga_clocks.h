
/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/tga/tga_clocks.h,v 3.0 1996/09/22 05:04:37 dawes Exp $ */
/*
 * Copyright 1995,96 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 */

/* ICS1562 Clocks */

/* There must be some algorithm for this, but for now I use the
   documented ones that I have access to. 
*/

typedef struct {
	int clock;
	unsigned char ics_bits[7];
} ICS1562ClockRec;

ICS1562ClockRec ICS1562ClockTab[] = {
	135000,{0x80, 0x00, 0x80, 0xA4, 0x28, 0x60, 0xB0,},	/* 135.000MHz */
	130808,{0x80, 0x00, 0x80, 0xA4, 0x04, 0xC0, 0xA8,},	/* 130.808MHz */
	119840,{0x80, 0x00, 0x80, 0x24, 0x98, 0xC0, 0x48,},	/* 119.840MHz */
	110000,{0x80, 0x00, 0x80, 0x24, 0xF0, 0x20, 0x30,},	/* 110.000MHz */
	108180,{0x80, 0x00, 0x80, 0x24, 0xA8, 0x20, 0x88,},	/* 108.180MHz */
	104000,{0x80, 0x00, 0x80, 0x24, 0xA8, 0x60, 0x48,},	/* 104.000MHz */
	 92980,{0x80, 0x00, 0x80, 0x24, 0x30, 0x00, 0xB0,},	/*  92.980MHz */
	 75000,{0x80, 0x00, 0x80, 0x24, 0x88, 0x40, 0x28,},	/*  75.000MHz */
	 74370,{0x80, 0x00, 0x80, 0x24, 0x78, 0x80, 0xC4,},	/*  74.370MHz */
	 69000,{0x80, 0x00, 0x80, 0x24, 0xB0, 0xC0, 0x88,},	/*  69.000MHz */
	 65000,{0x80, 0x00, 0x80, 0x24, 0x48, 0x20, 0x98,},	/*  65.000MHz */
	 50000,{0x80, 0x00, 0x80, 0x24, 0x88, 0x80, 0x78,},	/*  50.000MHz */
	 40000,{0x80, 0x00, 0x80, 0x24, 0x70, 0xA0, 0x84,},	/*  40.000MHz */
	 31500,{0x80, 0x04, 0x80, 0x24, 0xB0, 0x20, 0xC8,},	/*  31.500MHz */
#if 0	/* This is a documented 25.175MHz clock */
	 25175,{0x80, 0x04, 0x80, 0x24, 0x88, 0x80, 0x78,},	/*  25.175MHz */
#endif
	/* This is the 25.175MHz clock that Jay Estabrook chose for Linux */
         25175,{0x80, 0x04, 0x00, 0x24, 0x44, 0x80, 0xB8,},
};

#define Num_TGAStaticClocks 15
