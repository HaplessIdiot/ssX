/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_init.h,v 1.1 2000/06/17 00:03:05 martin Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *
 */

#ifndef _R128_INIT_H_
#define _R128_INIT_H_

#ifdef GLX_DIRECT_RENDERING

#include <X11/Xlibint.h>

#include "types.h"
#include "xf86drm.h"
#include "dri_tmm.h"
#include "dri_mesaint.h"

#include "r128_screen.h"
#include "r128_context.h"

#define DEBUG			0
#define DEBUG_LOCKING		0
#define ENABLE_PERF_BOXES	0


#if DEBUG

#include <stdio.h>
#define R128_DEBUG(p)                                                       \
    do {                                                                    \
	printf p ;                                                          \
	fflush(stdout);                                                     \
    } while (0)

extern int R128_DEBUG_FLAGS;

#else

#define R128_DEBUG_FLAGS            0

#endif

#define DEBUG_VERBOSE_2D            0x0001
#define DEBUG_VERBOSE_CCE           0x0008
#define DEBUG_VERBOSE_OUTREG        0x0010
#define DEBUG_ALWAYS_SYNC           0x0040
#define DEBUG_VERBOSE_MSG           0x0080
#define DEBUG_NO_OUTRING            0x0100
#define DEBUG_NO_OUTREG             0x0200
#define DEBUG_VERBOSE_API           0x0400
#define DEBUG_VALIDATE_RING         0x0800
#define DEBUG_VERBOSE_LRU           0x1000
#define DEBUG_VERBOSE_DRI           0x2000
#define DEBUG_VERBOSE_IOCTL         0x4000

#endif
#endif /* _R128_INIT_H_ */
