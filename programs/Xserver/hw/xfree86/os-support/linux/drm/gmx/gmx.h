/* gmx.h -- Example GLINT GMX driver stub -*- linux-c -*-
 * Created: Wed Jan 13 13:18:05 1999 by faith@precisioninsight.com
 * Revised: Fri Mar 19 14:31:18 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * $PI: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/gmx/gmx.h,v 1.3 1999/03/26 17:47:01 faith Exp $
 * $XFree86$
 * 
 */

#ifndef _GMX_H_
#define _GMX_H_
#ifdef __KERNEL__
#include "drm.h"

#define GMX_DEBUG_CODE 2	  /* Include debugging code (if > 1, then
                                     turn on printing by default) */
#define GMX_NAME   "gmx"

#define GMX_FLAG_TRACE 0x0001
#define GMX_FLAG_DEBUG 0x0002

				/* Macros to make printk easier */
#define GMX_ERROR(fmt, arg...) printk(KERN_ERR "[" GMX_NAME "] " fmt, ##arg)
#define GMX_INFO(fmt, arg...)  printk(KERN_INFO "[" GMX_NAME "] " fmt, ##arg)

#if GMX_DEBUG_CODE
#define GMX_TRACE(fmt, arg...)                                            \
	do {                                                              \
		if (gmx_flags|GMX_FLAG_TRACE)                             \
			printk(KERN_DEBUG                                 \
			       "[" GMX_NAME ":" __FUNCTION__ "] " fmt,    \
			       ##arg);                                    \
	} while (0)
#define GMX_DEBUG(fmt, arg...)                                            \
	do {                                                              \
		if (gmx_flags|GMX_FLAG_DEBUG)                             \
			printk(KERN_DEBUG                                 \
			       "[" GMX_NAME ":" __FUNCTION__ "] " fmt,    \
			       ##arg);                                    \
	} while (0)
#else
#define GMX_TRACE(fmt, arg...) do { } while (0)
#define GMX_DEBUG(fmt, arg...) do { } while (0)
#endif

extern void __init gmx_setup(char *str, int *ints);
extern int         gmx_init(void);
extern void        gmx_cleanup(void);


#endif
#endif
