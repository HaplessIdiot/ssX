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

#include "r128_init.h"
#include "r128_mesa.h"
#include "r128_xmesa.h"
#include "r128_context.h"
#include "r128_lock.h"
#include "r128_reg.h"
#include "r128_cce.h"
#include "r128_state.h"
#include "r128_vb.h"
#include "r128_tex.h"
#include "r128_swap.h"


#if ENABLE_PERF_BOXES

/*
 * Add hardware commands to draw a filled box for the debugging display.
 */
static void r128ClearBoxLocked( r128ContextPtr r128ctx,
				int x, int y, int w, int h,
				int r, int g, int b )
{
    int dst_bpp;
    int color;

    switch ( r128ctx->r128Screen->bpp ) {
    case 8:
	dst_bpp = R128_GMC_DST_8BPP_CI;
	break;
    case 16:
	if ( r128ctx->r128Screen->depth == 15 ) {
	    dst_bpp = R128_GMC_DST_15BPP;
	} else {
	    dst_bpp = R128_GMC_DST_16BPP;
	}
	break;
    case 24:
	dst_bpp = R128_GMC_DST_24BPP;
	break;
    case 32:
    default:
	dst_bpp = R128_GMC_DST_32BPP;
	break;
    }

    x += r128ctx->driDrawable->x + r128ctx->drawX;
    y += r128ctx->driDrawable->y + r128ctx->drawY;

    color = r128PackColor( r128ctx->r128Screen->bpp, r, g, b, 0 );

    /* Temporarily disable Z and stencil buffer and texture mapping modes */
    R128CCE0( R128_CCE_PACKET0, R128_TEX_CNTL_C, 0 );
    R128CCE( r128ctx->regs.tex_cntl_c & ~(R128_Z_ENABLE |
					  R128_STENCIL_ENABLE |
					  R128_TEXMAP_ENABLE) );

    R128CCE3( R128_CCE_PACKET3_CNTL_PAINT_MULTI, 3 );
    R128CCE( R128_GMC_BRUSH_SOLID_COLOR
	     | dst_bpp
	     | R128_GMC_SRC_DATATYPE_COLOR
	     | R128_ROP3_P
	     | R128_GMC_3D_FCN_EN		/* FIXME?? */
	     | R128_GMC_CLR_CMP_CNTL_DIS	/* FIXME?? */
	     | R128_AUX_CLIP_DIS );		/* FIXME?? */
    R128CCE( color );
    R128CCE( (x << 16) | y );
    R128CCE( (w << 16) | h );

    R128CCE_SUBMIT_PACKET();
}

/*
 * Draw some small boxes in the corner of the buffer based on some
 * performance information.
 */
void r128PerformanceBoxesLocked( r128ContextPtr r128ctx )
{
    int		w, t;

    if ( !r128ctx->boxes ) {
	return;
    }

    /* Draw a box to show we are active, so if it is't seen
     * it means that it is completely software based rendering
     */
    if ( R128CCE_USE_RING_BUFFER( r128ctx->r128Screen->CCEMode ) ) {
	r128ClearBoxLocked( r128ctx, 4, 4, 8, 8, 255, 0, 255 );
    } else {
	r128ClearBoxLocked( r128ctx, 4, 4, 8, 8, 255, 255, 255 );
    }

    /* Draw a red box if we had to wait for drawing to complete
     * (software render or texture swap)
     */
    if ( r128ctx->c_drawWaits ) {
	r128ClearBoxLocked( r128ctx, 16, 4, 8, 8, 255, 0, 0 );
    }

    /* Draw a yellow box if textures were swapped */
    if ( r128ctx->c_textureSwaps ) {
	r128ClearBoxLocked( r128ctx, 40, 4, 8, 8, 255, 255, 0 );
    }

    /* Draw a green box if we had to wait for the previous frame
     * to complete (full utilization)
     */
    if ( !r128ctx->hardwareWentIdle ) {
	r128ClearBoxLocked( r128ctx, 64, 4, 8, 8, 0, 255, 0 );
    }

    /* Draw bars to represent the utilization of the vertex buffers */
    r128ClearBoxLocked( r128ctx, 4, 16, 252, 4, 32, 32, 32 );
    t = r128ctx->r128Screen->vbRgn.size / r128ctx->r128Screen->vbBufSize;
    w = 252 * r128ctx->c_vertexBuffers / t;
    if ( w < 1 ) {
	w = 1;
    }
    if ( r128ctx->c_vertexBuffers > t ) {
	r128ClearBoxLocked( r128ctx, 4, 16, 252, 4, 255, 32, 32 );
    } else {
	r128ClearBoxLocked( r128ctx, 4, 16, w, 4, 196, 128, 128 );
    }
}

#endif /* ENABLE_PERF_BOXES */


static void delay( void ) {
/* Prevent an optimizing compiler from removing a spin loop */
}

/* Throttle the frame rate -- only allow one pending swap buffers
 * request at a time.
 * GTH: We probably don't want a timeout here, as we can wait as
 * long as we want for a frame to complete.  If it never does, then
 * the card has locked.
 */
static int r128WaitForFrameCompletion( r128ContextPtr r128ctx )
{
    unsigned char *R128MMIO = r128ctx->r128Screen->mmio;
    CARD32 swapAge;
    int i;
    int wait = 0;

    while ( 1 ) {
	swapAge = INREG( R128_SWAP_AGE_REG );
	if ( r128ctx->lastSwapAge <= swapAge ) {
	    break;
	}

	/* Spin in place a bit so we aren't hammering the register */
	wait++;
	for ( i = 0 ; i < 256 ; i++ ) {
	    delay();
	}
    }

    /* Increment the frame counter */
    r128ctx->lastSwapAge = ++swapAge;

    return wait;
}



/* Copy the back color buffer to the front color buffer */
void r128SwapBuffers(r128ContextPtr r128ctx)
{
    __DRIdrawablePrivate *dPriv = r128ctx->driDrawable;
    int                   nc;
    XF86DRIClipRectPtr    c;
    int                   dst_bpp;

    switch (r128ctx->r128Screen->bpp) {
    case 8:
	dst_bpp = R128_GMC_DST_8BPP_CI;
	break;
    case 16:
	if (r128ctx->r128Screen->depth == 15) dst_bpp = R128_GMC_DST_15BPP;
	else                                  dst_bpp = R128_GMC_DST_16BPP;
	break;
    case 24:
	dst_bpp = R128_GMC_DST_24BPP;
	break;
    case 32:
    default:
	dst_bpp = R128_GMC_DST_32BPP;
	break;
    }

    LOCK_HARDWARE(r128ctx);

    /* Flush any outstanding vertex buffers */
    r128FlushVerticesLocked(r128ctx);

    /* Throttle the frame rate -- only allow one pending swap buffers
     * request at a time.
     */
    if ( !r128WaitForFrameCompletion(r128ctx) ) {
	r128ctx->hardwareWentIdle = 1;
    } else {
	r128ctx->hardwareWentIdle = 0;
    }

#if ENABLE_PERF_BOXES
    /* Draw the performance boxes */
    r128PerformanceBoxesLocked(r128ctx);
#endif

    /* Init the clip rects here in case they changed during the
       LOCK_HARDWARE macro */
    c  = dPriv->pClipRects;
    nc = dPriv->numClipRects;

    /* Cycle through the clip rects */
    while (nc--) {
	int fx = c[nc].x1;
	int fy = c[nc].y1;
	int fw = c[nc].x2 - fx;
	int fh = c[nc].y2 - fy;
	int bx = fx + r128ctx->r128Screen->backX;
	int by = fy + r128ctx->r128Screen->backY;

	fx += r128ctx->r128Screen->fbX;
	fy += r128ctx->r128Screen->fbY;

	R128CCE3(R128_CCE_PACKET3_CNTL_BITBLT_MULTI, 3);
	R128CCE(R128_GMC_BRUSH_NONE
		| R128_GMC_SRC_DATATYPE_COLOR
		| R128_DP_SRC_SOURCE_MEMORY
		| dst_bpp
		| R128_ROP3_S);
	R128CCE((bx << 16) | by);
	R128CCE((fx << 16) | fy);
	R128CCE((fw << 16) | fh);
    }

    R128CCE0(R128_CCE_PACKET0, R128_SWAP_AGE_REG, 0);
    R128CCE(r128ctx->lastSwapAge);

    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
/*      r128ctx->dirty_context |= R128_CTX_ALL_DIRTY; */
    r128ctx->dirty_context |= (R128_CTX_MISC        |
			       R128_CTX_ENGINESTATE |
			       R128_CTX_ALPHASTATE);

    R128CCE_SUBMIT_PACKET();

    UNLOCK_HARDWARE(r128ctx);

#if ENABLE_PERF_BOXES
    /* Log the performance counters if necessary */
    r128PerformanceCounters(r128ctx);
#endif
}
