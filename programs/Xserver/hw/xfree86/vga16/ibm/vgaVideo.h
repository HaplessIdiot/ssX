/* $XConsortium: vgaVideo.h,v 1.1 94/03/28 21:40:14 dpw Exp $ */
/*
 * Copyright IBM Corporation 1987,1988,1989
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that 
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/
/***********************************************************
		Copyright IBM Corporation 1987,1988

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* Header: /andrew/X11/r3src/earlyRelease/server/ddx/ibm/vga/RCS/vgaVideo.h,v 6.3 88/11/10 22:51:31 paul Exp */
/* Source: /andrew/X11/r3src/earlyRelease/server/ddx/ibm/vga/RCS/vgaVideo.h,v */

#include "misc.h"	/* GJA -- for pointer data type */
#ifdef lint
#if defined(volatile)
#undef volatile
#endif
#define volatile /**/
#if defined(const)
#undef const
#endif
#define const /**/
#if defined(signed)
#undef signed
#endif
#define signed /**/
#endif

/*
 * References to all pc ( i.e. '286 ) memory in the
 * regions used by the [ev]ga server ( the 128K windows )
 * MUST be long-word ( i.e. 32-bit ) reads or writes.
 * This definition will change for other memory architectures
 * ( e.g. AIX-Rt )
 */
typedef unsigned char VideoAdapterObject ;
typedef volatile VideoAdapterObject *VideoMemoryPtr ;
typedef volatile VideoAdapterObject *VgaMemoryPtr ;
#if !defined(BITMAP_BIT_ORDER)
#define BITMAP_BIT_ORDER MSBFirst
#endif

#if !defined(IMAGE_BYTE_ORDER)
#define IMAGE_BYTE_ORDER LSBFirst
#endif

	/* GJA -- Connect IBM code with XFree86 */
extern pointer vgaBase;
#ifdef UNBANKED_VGA16
#define SCREEN_ADDR ((int)vgaBase)
#else
extern pointer vgaVirtBase;
#define SCREEN_ADDR ((int)vgaVirtBase)
#endif

#define VIDEO_MEMORY_BASE ( (volatile unsigned char *) ( SCREEN_ADDR ) )
#define VIDBASE VIDEO_MEMORY_BASE


/* Bit Ordering Macros */
#if !defined(SCRLEFT8)
#define SCRLEFT8(lw, n)	( (unsigned char) (((unsigned char) lw) << (n)) )
#endif
#if !defined(SCRRIGHT8)
#define SCRRIGHT8(lw, n)	( (unsigned char) (((unsigned char)lw) >> (n)) )
#endif
/* These work ONLY on 8-bit wide Quantities !! */
#define LeftmostBit ( SCRLEFT8( 0xFF, 7 ) & 0xFF )
#define RightmostBit ( SCRRIGHT8( 0xFF, 7 ) & 0xFF )

/*
 * [ev]ga video screen defines & macros
 */
#define PIX_PER_BYTE 8

#define xGA_BLACK_PIXEL 0
#define xGA_WHITE_PIXEL 1

#define xGA_MAXPLANES 4
#define xGA_ALLPLANES 0xFL

#define COLOR_TUBE 1

#define VGA_BLACK_PIXEL xGA_BLACK_PIXEL
#define VGA_WHITE_PIXEL xGA_WHITE_PIXEL

#define VGA_MAXPLANES xGA_MAXPLANES
#define VGA_ALLPLANES xGA_ALLPLANES

#define VIDEO_MEM_BYTES_PER_PLANE ( 96 * 1024 )

extern int BYTES_PER_LINE;
extern int MAX_ROW;
extern int MAX_COLUMN;
extern int MAX_OFFSCREEN_ROW;

#define ROW_OFFSET( PIXEL_X_VALUE ) ( ( PIXEL_X_VALUE ) >> 3 )
#define BIT_OFFSET( PIXEL_X_VALUE ) ( ( PIXEL_X_VALUE ) & 0x7 )
#define BYTE_OFFSET( PIXEL_X_VALUE, PIXEL_Y_VALUE ) \
	( ( ( PIXEL_Y_VALUE ) * BYTES_PER_LINE ) + ( ( PIXEL_X_VALUE ) >> 3 ) )
#define SCREENADDRESS( PIXEL_X_VALUE, PIXEL_Y_VALUE ) \
	( VIDEO_MEMORY_BASE + BYTE_OFFSET( PIXEL_X_VALUE, PIXEL_Y_VALUE ) )


