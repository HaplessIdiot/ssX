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

#ifndef _R128_SWAP_H_
#define _R128_SWAP_H_

#ifdef GLX_DIRECT_RENDERING

extern void r128SwapBuffers(r128ContextPtr r128ctx);

#if ENABLE_PERF_BOXES
extern void r128PerformanceBoxesLocked(r128ContextPtr r128ctx);

static __inline void r128PerformanceCounters( r128ContextPtr r128ctx )
{
    r128ctx->c_clears = 0;
    r128ctx->c_drawWaits = 0;

    r128ctx->c_textureSwaps = 0;
    r128ctx->c_textureBytes = 0;

    r128ctx->c_vertexBuffers = 0;
}
#endif

#endif
#endif /* _R128_SWAP_H_ */
