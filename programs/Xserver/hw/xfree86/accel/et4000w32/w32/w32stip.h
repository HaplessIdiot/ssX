/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/et4000w32/w32/w32stip.h,v 3.0 1994/09/11 00:42:26 dawes Exp $ */
/*******************************************************************************
                        Copyright 1994 by Glenn G. Lai

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyr notice appear in all copies and that
both that copyr notice and this permission notice appear in
supporting documentation, and that the name of Glenn G. Lai not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

Glenn G. Lai DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

Glenn G. Lai
P.O. Box 4314
Austin, Tx 78765
glenn@cs.utexas.edu)
8/9/94
*******************************************************************************/
#ifndef W32_STIP_H
#define W32_STIP_H

#include "w32blt.h"


/* the following needs some polishing */
#define W32_INIT_OPAQUE_STIPPLE(OP, MASK, FOREGROUND, BACKGROUND, DST_OFFSET) \
	W32_INIT_STIPPLE(0xf0, OP, MASK, FOREGROUND, BACKGROUND, DST_OFFSET)


#define W32_INIT_TR_STIPPLE(OP, MASK, FOREGROUND, BACKGROUND, DST_OFFSET) \
	W32_INIT_STIPPLE(0xaa, OP, MASK, FOREGROUND, BACKGROUND, DST_OFFSET)


#define W32_INIT_STIPPLE(BK_OP, OP, MASK, FOREGROUND, BACKGROUND, DST_OFFSET) \
{ \
    if (W32OrW32i) \
    { \
	*ACL_FOREGROUND_RASTER_OPERATION= W32OpTable[OP]; \
    } \
    else \
    { /* The following works only for the w32p */ \
	*ACL_FOREGROUND_RASTER_OPERATION= \
	    (0xf0 & W32OpTable[OP]) | (0x0f & 0xaa); \
	*ACL_PATTERN_WRAP		= 0x02; \
	*ACL_PATTERN_Y_OFFSET		= 0x3; \
	*MBP0 				= W32Pattern; \
	*(LongP)W32Buffer 		= MASK; \
    } \
    *ACL_Y_COUNT = 0; \
    *ACL_BACKGROUND_RASTER_OPERATION	= BK_OP; \
    *ACL_ROUTING_CONTROL		= 0x2; \
    if (W32) *ACL_VIRTUAL_BUS_SIZE	= 0x0; \
    *ACL_XY_DIRECTION			= 0; \
    *ACL_DESTINATION_Y_OFFSET		= DST_OFFSET; \
    *ACL_SOURCE_WRAP			= 0x12; \
    *ACL_SOURCE_Y_OFFSET		= 0x3; \
    *ACL_SOURCE_ADDRESS			= W32Foreground; \
    *MBP0 				= W32Foreground; \
    *(LongP)W32Buffer	 		= FOREGROUND; \
    *(LongP)(W32Buffer + 4) 		= FOREGROUND; \
    *ACL_PATTERN_WRAP			= 0x12; \
    *ACL_PATTERN_Y_OFFSET		= 0x3; \
    *ACL_PATTERN_ADDRESS		= W32Background; \
    *MBP0 				= W32Background; \
    *(LongP)W32Buffer	 		= BACKGROUND; \
    *(LongP)(W32Buffer + 4) 		= BACKGROUND; \
}


#define W32_INIT_STIPPLE_BLT(MASK, OFFSET, WIDTH, BASE) \
{ \
    W32_INIT_BLT(GXcopy, MASK, OFFSET, OFFSET, 1, 1) \
    *ACL_SOURCE_ADDRESS = BASE; \
    *ACL_X_COUNT = WIDTH; \
}


#define W32_STIPPLE_BLT(DST, Y) \
{ \
    *ACL_Y_COUNT = Y; \
    START_ACL(DST) \
}


#endif /* W32_STIP_H */
