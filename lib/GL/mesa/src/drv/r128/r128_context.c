/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_context.c,v 1.1 2000/06/17 00:03:04 martin Exp $ */
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

#include <stdlib.h>

#include "r128_init.h"
#include "r128_xmesa.h"
#include "r128_context.h"
#include "r128_cce.h"
#include "r128_dd.h"
#include "r128_state.h"
#include "r128_span.h"
#include "r128_tex.h"
#include "r128_vb.h"
#include "r128_pipeline.h"

#include "context.h"
#include "simple_list.h"

/* Create the device specific context */
GLboolean r128CreateContext(Display *dpy, GLvisual *glVisual,
			    __DRIcontextPrivate *driContextPriv)
{
    r128ContextPtr  r128ctx;
    r128ScreenPtr   r128scrn;
    GLcontext      *glCtx    = driContextPriv->mesaContext;
    int             i;
    char           *v;

    r128ctx = (r128ContextPtr)Xmalloc(sizeof(*r128ctx));
    if (!r128ctx) return GL_FALSE;

    /* Initialize r128Context */
    r128ctx->glCtx         = glCtx;
    r128ctx->display       = dpy;
    r128ctx->driContext    = driContextPriv;
    r128ctx->driDrawable   = NULL; /* Set by XMesaMakeCurrent */

    r128scrn = r128ctx->r128Screen =
	(r128ScreenPtr)(driContextPriv->driScreenPriv->private);

    r128ctx->CurrentTexObj[0] = NULL;
    r128ctx->CurrentTexObj[1] = NULL;
    make_empty_list(&r128ctx->SwappedOut);
    for (i = 0; i < r128scrn->NRTexHeaps; i++) {
	make_empty_list(&r128ctx->TexObjList[i]);
	r128ctx->texHeap[i] = mmInit(0, r128scrn->texSize[i]);
	r128ctx->lastTexAge[i] = -1;
    }

    r128ctx->lastSwapAge      = 0;

    r128ctx->useFastPath      = GL_FALSE;

    r128ctx->vert_buf         = NULL;

    r128ctx->CCEbuf           = (CARD32 *)malloc(sizeof(*r128ctx->CCEbuf) *
						 r128scrn->ringEntries);
    r128ctx->CCEcount         = 0;

    if ((v = getenv("LIBGL_CCE_TIMEOUT")))
	r128ctx->CCEtimeout   = strtoul(v, NULL, 10);
    else
	r128ctx->CCEtimeout   = (R128_DEFAULT_TOTAL_CCE_TIMEOUT /
				 R128_DEFAULT_CCE_TIMEOUT);
    if (r128ctx->CCEtimeout <= 0) r128ctx->CCEtimeout = 1;

    /* Initialize GLcontext */
    glCtx->DriverCtx = (void *)r128ctx;

    r128DDInitExtensions(glCtx);

    r128DDInitDriverFuncs(glCtx);
    r128DDInitStateFuncs(glCtx);
    r128DDInitSpanFuncs(glCtx);
    r128DDInitTextureFuncs(glCtx);

    /* Do this after initialization of r128ctx->Fallback in InitState().
     */
    if (getenv("LIBGL_SOFTWARE_RENDERING"))
	r128ctx->Fallback  |= R128_FALLBACK_SWONLY;

    /* KW: No vertex buffer code for PCI cards -- for now fallback to
     * software always.
     * GTH: There is support there, but I'm seeing strange behaviour
     * with it enabled.  I'll leave the software fallbacks in place
     * for now.
     */
    if (r128scrn->IsPCI)
	r128ctx->Fallback  |= R128_FALLBACK_SWONLY;

    if (getenv("LIBGL_NO_SOFTWARE_FALLBACKS"))
	r128ctx->SWfallbackDisable = GL_TRUE;
    else
	r128ctx->SWfallbackDisable = GL_FALSE;

    glCtx->Driver.TriangleCaps = (DD_TRI_CULL
				  | DD_TRI_LIGHT_TWOSIDE
				  | DD_TRI_OFFSET);

    /* Ask Mesa to clip fog coordinates for us
     */
    glCtx->TriangleCaps |= DD_CLIP_FOG_COORD;

    /* Reset Mesa's current 2D texture pointers to the driver's textures */
    glCtx->Shared->DefaultD[2][0].DriverData = NULL;
    glCtx->Shared->DefaultD[2][1].DriverData = NULL;

    /* KW: Set the maximum texture size small enough that we can
     * guarentee that both texture units can bind a maximal texture
     * and have them both in on-card memory at once.  (Kevin or
     * Gareth: Please check these numbers are OK)
     */
    if (r128scrn->texSize[0] < 2*1024*1024) {
       glCtx->Const.MaxTextureLevels = 9;
       glCtx->Const.MaxTextureSize = 1<<8;
    } else if (r128scrn->texSize[0] < 8*1024*1024) {
       glCtx->Const.MaxTextureLevels = 10;
       glCtx->Const.MaxTextureSize = 1<<9;
    } else {
       glCtx->Const.MaxTextureLevels = 11;
       glCtx->Const.MaxTextureSize = 1<<10;
    }

    glCtx->Const.MaxTextureUnits = 2;

#if ENABLE_PERF_BOXES
    if (getenv("LIBGL_PERFORMANCE_BOXES"))
	r128ctx->boxes = 1;
    else
	r128ctx->boxes = 0;
#endif

    /* If Mesa has current a vertex buffer, make sure the driver's VB
       data is up to date */
    if (glCtx->VB) r128DDRegisterVB(glCtx->VB);

    /* Register the fast path */
    if (glCtx->NrPipelineStages)
	glCtx->NrPipelineStages =
	    r128DDRegisterPipelineStages(glCtx->PipelineStage,
					 glCtx->PipelineStage,
					 glCtx->NrPipelineStages);

    r128DDInitState(r128ctx);

    driContextPriv->driverPrivate = (void *)r128ctx;

    return GL_TRUE;
}

/* Destroy the device specific context */
void r128DestroyContext(r128ContextPtr r128ctx)
{
    if (r128ctx) {
	r128TexObjPtr t, next_t;
	int           i;

	free(r128ctx->CCEbuf);

	for (i = 0; i < r128ctx->r128Screen->NRTexHeaps; i++) {
	    foreach_s (t, next_t, &r128ctx->TexObjList[i])
		r128DestroyTexObj(r128ctx, t);
	}

	foreach_s (t, next_t, &r128ctx->SwappedOut)
	    r128DestroyTexObj(r128ctx, t);

	Xfree(r128ctx);
    }
}

/* Load the device specific context into the hardware.  The actual
   setting of the hardware state is done in the r128UpdateHWState(). */
r128ContextPtr r128MakeCurrent(r128ContextPtr oldCtx,
			       r128ContextPtr newCtx,
			       __DRIdrawablePrivate *dPriv)
{
    if (oldCtx) {
	if (!R128CCE_USE_RING_BUFFER(newCtx->r128Screen->CCEMode))
	    newCtx->dirty         |= R128_REQUIRE_QUIESCENCE;
	if (oldCtx != newCtx) {
	    newCtx->dirty         |= R128_UPDATE_CONTEXT;
	    newCtx->dirty_context |= R128_CTX_ALL_DIRTY;
	}
	if (oldCtx->driDrawable != dPriv)
	    newCtx->dirty         |= R128_UPDATE_WINPOS;
    } else {
	newCtx->dirty             |= R128_UPDATE_CONTEXT;
	newCtx->dirty_context     |= R128_CTX_ALL_DIRTY;
    }

    newCtx->driDrawable = dPriv;

    return newCtx;
}
