/* $XFree86$ */ /* -*- c-basic-offset: 4 -*- */
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
#include "r128_vb.h"
#include "r128_tris.h"
#include "r128_state.h"

static triangle_func tri_tab[0x10];
static quad_func     quad_tab[0x10];
static line_func     line_tab[0x10];
static points_func   points_tab[0x10];

#define R128_COLOR(to, from)                                                \
do {                                                                        \
    (to)[0] = (from)[2];                                                    \
    (to)[1] = (from)[1];                                                    \
    (to)[2] = (from)[0];                                                    \
    (to)[3] = (from)[3];                                                    \
} while (0)

#define IND (0)
#define TAG(x) x
#include "r128_tritmp.h"

#define IND (R128_FLAT_BIT)
#define TAG(x) x##_flat
#include "r128_tritmp.h"

#define IND (R128_OFFSET_BIT)
#define TAG(x) x##_offset
#include "r128_tritmp.h"

#define IND (R128_OFFSET_BIT | R128_FLAT_BIT)
#define TAG(x) x##_offset_flat
#include "r128_tritmp.h"

#define IND (R128_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "r128_tritmp.h"

#define IND (R128_TWOSIDE_BIT | R128_FLAT_BIT)
#define TAG(x) x##_twoside_flat
#include "r128_tritmp.h"

#define IND (R128_TWOSIDE_BIT | R128_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "r128_tritmp.h"

#define IND (R128_TWOSIDE_BIT | R128_OFFSET_BIT | R128_FLAT_BIT)
#define TAG(x) x##_twoside_offset_flat
#include "r128_tritmp.h"

/* Initialize the table of points, line and triangle drawing functions */
void r128TriangleFuncsInit(void)
{
    init();
    init_flat();
    init_offset();
    init_offset_flat();
    init_twoside();
    init_twoside_flat();
    init_twoside_offset();
    init_twoside_offset_flat();
}

/* Setup the Point, Line, Triangle and Quad functions based on the
   current rendering state.  Wherever possible, use the hardware to
   render the primitive.  Otherwise, fallback to software rendering. */
void r128ChooseRenderState(GLcontext *ctx)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);
    GLuint         flags   = ctx->TriangleCaps;

    /* KW: Includes handling of SWonly rendering:
     */
    if (r128ctx->Fallback)
       return;

    r128ctx->IndirectTriangles = 0;

    if (flags) {
	CARD32 index    = 0;
	CARD32 shared   = 0;
 	CARD32 fallback = R128_FALLBACK_BIT;

	/* KW: I'd prefer to remove SWfallbackDisable & just use
	 * R128_FALLBACK_BIT throughout this routine.
	 */
  	if (r128ctx->SWfallbackDisable) fallback = 0;

	if (flags & DD_FLATSHADE)               shared |= R128_FLAT_BIT;
	if (flags & DD_TRI_LIGHT_TWOSIDE)       shared |= R128_TWOSIDE_BIT;

	/* TODO: Fix mesa so that these can be handled in
	 * r128ctx->Fallback.
	 */
	if (flags & (DD_MULTIDRAW |
		     DD_SELECT    |
		     DD_FEEDBACK))              shared |= R128_FALLBACK_BIT;

	/* FIXME: Only enable in 16 bpp mode after we have hardware
           stencil support */
	if (flags & DD_STENCIL)                 shared |= R128_FALLBACK_BIT;

	/* Setup PointFunc */
	index = shared;
	if (flags & DD_POINT_SMOOTH)            index  |= fallback;

	r128ctx->RenderIndex = index;
	r128ctx->PointsFunc  = points_tab[index];
	if (index & R128_FALLBACK_BIT)
	    r128ctx->IndirectTriangles |= DD_POINT_SW_RASTERIZE;

	/* Setup LineFunc */
	index = shared;
	if (flags & DD_LINE_SMOOTH)             index  |= fallback;
	if (flags & DD_LINE_STIPPLE)            index  |= fallback;

	r128ctx->RenderIndex |= index;
	r128ctx->LineFunc     = line_tab[index];
	if (index & R128_FALLBACK_BIT)
	    r128ctx->IndirectTriangles |= DD_LINE_SW_RASTERIZE;

	/* Setup TriangleFunc and QuadFunc */
	index = shared;
	if (flags & DD_TRI_OFFSET)              index  |= R128_OFFSET_BIT;
	if (flags & DD_TRI_SMOOTH)              index  |= fallback;
	if (flags & DD_TRI_UNFILLED)            index  |= fallback;
	if (flags & DD_TRI_STIPPLE)             index  |= fallback;

	r128ctx->RenderIndex  |= index;
	r128ctx->TriangleFunc  = tri_tab[index];
	r128ctx->QuadFunc      = quad_tab[index];
	if (index & R128_FALLBACK_BIT)
	    r128ctx->IndirectTriangles |= (DD_TRI_SW_RASTERIZE |
					   DD_QUAD_SW_RASTERIZE);
    } else if (r128ctx->RenderIndex) {
	r128ctx->RenderIndex  = 0;
	r128ctx->PointsFunc   = points_tab[0];
	r128ctx->LineFunc     = line_tab[0];
	r128ctx->TriangleFunc = tri_tab[0];
	r128ctx->QuadFunc     = quad_tab[0];
    }
}
