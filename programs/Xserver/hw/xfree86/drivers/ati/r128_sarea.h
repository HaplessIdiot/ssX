/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/r128_sarea.h,v 1.1 2000/11/02 16:55:38 tsi Exp $ */
/*
 * Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario, 
 *                      Precision Insight, Inc., Cedar Park, Texas, and
 *                      VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL ATI, PRECISION INSIGHT, VA LINUX
 * SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *
 */

#ifndef _R128_SAREA_H_
#define _R128_SAREA_H_

/* There are 2 heaps (local/AGP).  Each region within a heap is a
   minimum of 64k, and there are at most 64 of them per heap. */
#define R128_LOCAL_TEX_HEAP       0
#define R128_AGP_TEX_HEAP         1
#define R128_NR_TEX_HEAPS         2
#define R128_NR_TEX_REGIONS      64
#define R128_LOG_TEX_GRANULARITY 16

typedef struct {
    unsigned char next, prev; /* indices to form a circular LRU  */
    unsigned char in_use;     /* owned by a client, or free? */
    int           age;        /* tracked by clients to update local LRU's */
} R128TexRegion;

typedef struct {
    /* Maintain an LRU of contiguous regions of texture space.  If you
     * think you own a region of texture memory, and it has an age
     * different to the one you set, then you are mistaken and it has
     * been stolen by another client.  If global texAge hasn't changed,
     * there is no need to walk the list.
     *
     * These regions can be used as a proxy for the fine-grained texture
     * information of other clients - by maintaining them in the same
     * lru which is used to age their own textures, clients have an
     * approximate lru for the whole of global texture space, and can
     * make informed decisions as to which areas to kick out.  There is
     * no need to choose whether to kick out your own texture or someone
     * else's - simply eject them all in LRU order.
     */
				/* Last elt is sentinal */
    R128TexRegion texList[R128_NR_TEX_HEAPS][R128_NR_TEX_REGIONS+1];
				/* last time texture was uploaded */
    int           texAge[R128_NR_TEX_HEAPS];

    int           ctxOwner;     /* last context to upload state */

    CARD32        ringWrite;    /* current ring buffer write index */
} R128SAREAPriv, *R128SAREAPrivPtr;

#endif
