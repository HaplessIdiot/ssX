/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_clear.c,v 1.1 2000/06/17 00:03:04 martin Exp $ */
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
#include "r128_clear.h"

/* Clear the depth buffer */
void r128ClearDepthBuffer(r128ContextPtr r128ctx, GLboolean all,
			  GLint cx, GLint cy, GLint cw, GLint ch)
{
    __DRIdrawablePrivate *dPriv    = r128ctx->driDrawable;
    int                   nc;
    XF86DRIClipRectPtr    c;
    int                   dst_bpp;
    CARD32                write_mask;

    if (!(r128ctx->regs.tex_cntl_c & R128_Z_WRITE_ENABLE)) return;

    switch (r128ctx->regs.z_sten_cntl_c & R128_Z_PIX_WIDTH_MASK) {
    case R128_Z_PIX_WIDTH_16:
	write_mask = 0x0000ffff;
	dst_bpp    = R128_GMC_DST_16BPP;
	break;
    case R128_Z_PIX_WIDTH_24:
	write_mask = 0x00ffffff;
	dst_bpp    = R128_GMC_DST_32BPP;
	break;
    case R128_Z_PIX_WIDTH_32:
	write_mask = 0xffffffff;
	dst_bpp    = R128_GMC_DST_32BPP;
	break;
    default: return;
    }

#if ENABLE_PERF_BOXES
    /* Bump the performance counter */
    r128ctx->c_clears++;
#endif

    cx += dPriv->x;
    cy  = dPriv->y + dPriv->h - cy - ch;

    LOCK_HARDWARE(r128ctx);

    /* Flush any outstanding vertex buffers */
    r128FlushVerticesLocked(r128ctx);

    /* Init the clip rects here in case they changed during the
       LOCK_HARDWARE macro */
    c  = dPriv->pClipRects;
    nc = dPriv->numClipRects;

    /* Set the write mask so that we _only_ clear the Z buffer */
    R128CCE0(R128_CCE_PACKET0, R128_DP_WRITE_MASK, 0);
    R128CCE(write_mask);

    /* Cycle through the clip rects */
    while (nc--) {
	int x = c[nc].x1;
	int y = c[nc].y1;
	int w = c[nc].x2 - x;
	int h = c[nc].y2 - y;

	if (!all) {
	    if (x < cx) w -= cx - x, x = cx;
	    if (y < cy) h -= cy - y, y = cy;

	    if (x + w > cx + cw) w = cx + cw - x;
	    if (y + h > cy + ch) h = cy + ch - y;

	    if (w <= 0 || h <= 0) continue;
	}

	x += r128ctx->r128Screen->depthX;
	y += r128ctx->r128Screen->depthY;

	R128CCE3(R128_CCE_PACKET3_CNTL_PAINT_MULTI, 3);
	R128CCE(R128_GMC_BRUSH_SOLID_COLOR
		| dst_bpp
		| R128_GMC_SRC_DATATYPE_COLOR
		| R128_ROP3_P
		| R128_GMC_CLR_CMP_CNTL_DIS
		| R128_GMC_AUX_CLIP_DIS);
	R128CCE(r128ctx->ClearDepth);
	R128CCE((x << 16) | y);
	R128CCE((w << 16) | h);
    }

    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
    r128ctx->dirty_context |= (R128_CTX_MISC        |
			       R128_CTX_ENGINESTATE |
			       R128_CTX_ALPHASTATE);

    R128CCE_SUBMIT_PACKET();

    UNLOCK_HARDWARE(r128ctx);
}

/* Clear a color buffer */
void r128ClearColorBuffer(r128ContextPtr r128ctx, GLboolean all,
			  GLint cx, GLint cy, GLint cw, GLint ch,
			  GLint drawX, GLint drawY)
{
    __DRIdrawablePrivate *dPriv    = r128ctx->driDrawable;
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

#if ENABLE_PERF_BOXES
    /* Bump the performance counter */
    r128ctx->c_clears++;
#endif

    cx += dPriv->x;
    cy  = dPriv->y + dPriv->h - cy - ch;

    LOCK_HARDWARE(r128ctx);

    /* Flush any outstanding vertex buffers */
    r128FlushVerticesLocked(r128ctx);

    /* Init the clip rects here in case they changed during the
       LOCK_HARDWARE macro */
    c  = dPriv->pClipRects;
    nc = dPriv->numClipRects;

    /* Cycle through the clip rects */
    while (nc--) {
	int x = c[nc].x1;
	int y = c[nc].y1;
	int w = c[nc].x2 - x;
	int h = c[nc].y2 - y;

	if (!all) {
	    if (x < cx) w -= cx - x, x = cx;
	    if (y < cy) h -= cy - y, y = cy;

	    if (x + w > cx + cw) w = cx + cw - x;
	    if (y + h > cy + ch) h = cy + ch - h;

	    if (w <= 0 || h <= 0) continue;
	}

	x += drawX;
	y += drawY;

	R128CCE3(R128_CCE_PACKET3_CNTL_PAINT_MULTI, 3);
	R128CCE(R128_GMC_BRUSH_SOLID_COLOR
		| dst_bpp
		| R128_GMC_SRC_DATATYPE_COLOR
		| R128_ROP3_P
		| R128_GMC_CLR_CMP_CNTL_DIS
		| R128_GMC_AUX_CLIP_DIS);
	R128CCE(r128ctx->ClearColor);
	R128CCE((x << 16) | y);
	R128CCE((w << 16) | h);
    }

    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
    r128ctx->dirty_context |= (R128_CTX_MISC        |
			       R128_CTX_ENGINESTATE |
			       R128_CTX_ALPHASTATE);

    R128CCE_SUBMIT_PACKET();

    UNLOCK_HARDWARE(r128ctx);
}
