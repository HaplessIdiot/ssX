/* $XFree86$ */
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

#ifndef _R128_LOCK_H_
#define _R128_LOCK_H_

#ifdef GLX_DIRECT_RENDERING

/* Turn DEBUG_LOCKING on to find locking conflicts (see r128_init.h) */
#if DEBUG_LOCKING
extern char *prevLockFile;
extern int   prevLockLine;

#define DEBUG_LOCK()                                                    \
    do {                                                                \
	prevLockFile = (__FILE__);                                      \
	prevLockLine = (__LINE__);                                      \
    } while (0)

#define DEBUG_RESET()                                                   \
    do {                                                                \
	prevLockFile = 0;                                               \
	prevLockLine = 0;                                               \
    } while (0)

#define DEBUG_CHECK_LOCK()                                              \
    do {                                                                \
	if (prevLockFile) {                                             \
	    fprintf(stderr,                                             \
		    "LOCK SET!\n\tPrevious %s:%d\n\tCurrent: %s:%d\n",  \
		    prevLockFile, prevLockLine, __FILE__, __LINE__);    \
	    exit(1);                                                    \
	}                                                               \
    } while (0)

#else

#define DEBUG_LOCK()
#define DEBUG_RESET()
#define DEBUG_CHECK_LOCK()

#endif

/*
 * !!! We may want to separate locks from locks with validation.  This
 * could be used to improve performance for those things commands that
 * do not do any drawing !!!
 */

/* Lock the hardware using the current context */
#define LOCK_HARDWARE(CC)                                               \
    do {                                                                \
	char                  __ret = 0;                                \
	__DRIcontextPrivate  *cPriv = CC->driContext;                   \
	__DRIscreenPrivate   *sPriv = CC->r128Screen->driScreen;        \
                                                                        \
	DEBUG_CHECK_LOCK();                                             \
	DRM_CAS(&sPriv->pSAREA->lock, cPriv->hHWContext,                \
		DRM_LOCK_HELD|cPriv->hHWContext, __ret);                \
	if (__ret) {                                                    \
	    /* We lost the context, so we need to request the lock from \
               the kernel and update our state. */                      \
	    drmGetLock(sPriv->fd, cPriv->hHWContext, 0);                \
	    XMesaUpdateState(cPriv);                                    \
	}                                                               \
	DEBUG_LOCK();                                                   \
    } while (0)

/* Unlock the hardware using the current context */
#define UNLOCK_HARDWARE(CC)                                             \
    do {                                                                \
	__DRIcontextPrivate  *cPriv = CC->driContext;                   \
	__DRIscreenPrivate   *sPriv = CC->r128Screen->driScreen;        \
                                                                        \
	DRM_UNLOCK(sPriv->fd, &sPriv->pSAREA->lock, cPriv->hHWContext); \
	DEBUG_RESET();                                                  \
    } while (0)

/*
 * This pair of macros makes a loop over the drawing operations, so it
 * is not self contained and does not have the nice single statement
 * semantics of most macros.
 */
#define BEGIN_CLIP_LOOP(CC)                                             \
    do {                                                                \
	__DRIdrawablePrivate *_dPriv = CC->driDrawable;                 \
	XF86DRIClipRectPtr    _pc = _dPriv->pClipRects;                 \
	int                   _nc, _sc;                                 \
                                                                        \
	for (_nc = _dPriv->numClipRects; _nc > 0; _nc -= 3, _pc += 3) { \
	    _sc = (_nc <= 3) ? _nc : 3;                                 \
	    r128SetClipRects(CC, _pc, _sc)

/* FIXME: This should be a function call to turn off aux clipping */
#define END_CLIP_LOOP(CC)                                               \
	    R128CCE0(R128_CCE_PACKET0, R128_AUX_SC_CNTL, 0);            \
	    R128CCE(0x00000000);                                        \
	    R128CCE_SUBMIT_PACKET();                                    \
	}                                                               \
    } while (0)

#endif
#endif /* _R128_LOCK_H_ */
