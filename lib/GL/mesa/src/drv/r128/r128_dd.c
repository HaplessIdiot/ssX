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
#include "r128_cce.h"
#include "r128_clear.h"
#include "r128_state.h"
#include "r128_swap.h"
#include "r128_vb.h"
#include "r128_pipeline.h"
#include "r128_dd.h"

/* Driver entry point for clearing color and ancillary buffers */
static GLbitfield r128DDClear(GLcontext *ctx, GLbitfield mask, GLboolean all,
			      GLint x, GLint y, GLint width, GLint height)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    if (mask & DD_FRONT_LEFT_BIT) {
	r128ClearColorBuffer(r128ctx, all, x, y, width, height,
			     r128ctx->r128Screen->fbX,
			     r128ctx->r128Screen->fbY);
	mask &= ~DD_FRONT_LEFT_BIT;
    }

    if (mask & DD_BACK_LEFT_BIT) {
	r128ClearColorBuffer(r128ctx, all, x, y, width, height,
			     r128ctx->r128Screen->backX,
			     r128ctx->r128Screen->backY);
	mask &= ~DD_BACK_LEFT_BIT;
    }

    if (mask & DD_DEPTH_BIT) {
	r128ClearDepthBuffer(r128ctx, all, x, y, width, height);
	mask &= ~DD_DEPTH_BIT;
    }

#if 0
    /* FIXME: Add stencil support */
    if (mask & DD_STENCIL_BIT) {
	r128ClearStencilBuffer(r128ctx, all, x, y, width, height);
	mask &= ~DD_STENCIL_BIT;
    }
#endif

    return mask;
}

/* Return the current color buffer size */
static void r128DDGetBufferSize(GLcontext *ctx, GLuint *width, GLuint *height)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    *width  = r128ctx->driDrawable->w;
    *height = r128ctx->driDrawable->h;
}

/* Return various strings for glGetString() */
static const GLubyte *r128DDGetString(GLcontext *ctx, GLenum name)
{
    switch (name) {
    case GL_VENDOR:
	return (GLubyte *)"Precision Insight, Inc.";
    case GL_RENDERER:
	return (GLubyte *)"Mesa DRI Rage128 20000613";
    default:
	return NULL;
    }
}

/* Send all commands to the hardware.  If vertex buffers or indirect
   buffers are in use, then we need to make sure they are sent to the
   hardware.  All commands that are normally sent to the ring are
   already considered `flushed'. */
static void r128DDFlush(GLcontext *ctx)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    r128FlushVertices(r128ctx);

#if ENABLE_PERF_BOXES
    if (r128ctx->boxes) {
	LOCK_HARDWARE(r128ctx);
	r128PerformanceBoxesLocked(r128ctx);
	UNLOCK_HARDWARE(r128ctx);
    }

    /* Log the performance counters if necessary */
    r128PerformanceCounters(r128ctx);
#endif
}

/* Make sure all commands have been sent to the hardware and have
   completed processing. */
static void r128DDFinish(GLcontext *ctx)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

#if ENABLE_PERF_BOXES
    /* Bump the performance counter */
    r128ctx->c_drawWaits++;
#endif

    r128DDFlush(ctx);
    r128WaitForIdle(r128ctx);
}

/* Return various parameters requested by Mesa (this is deprecated) */
static GLint r128DDGetParameteri(const GLcontext *ctx, GLint param)
{
    switch (param) {
       /* Hardware fog isn't working yet -- however returning zero
	* here means that it looks like fog is working in some cases.
	* It is less confusing to simply have it never work until it
	* is actually fixed.
	*/
    case DD_HAVE_HARDWARE_FOG: return 1;
    default:                   return 0;
    }
}

/* Initialize the extensions supported by this driver */
void r128DDInitExtensions(GLcontext *ctx)
{
    /* FIXME: Are there other extensions to enable/disable??? */
    gl_extensions_disable(ctx, "GL_EXT_shared_texture_palette");
    gl_extensions_disable(ctx, "GL_EXT_paletted_texture");
    gl_extensions_disable(ctx, "GL_EXT_point_parameters");
    gl_extensions_disable(ctx, "ARB_imaging");
    gl_extensions_disable(ctx, "GL_EXT_blend_minmax");
    gl_extensions_disable(ctx, "GL_EXT_blend_logic_op");
    gl_extensions_disable(ctx, "GL_EXT_blend_subtract");
    gl_extensions_disable(ctx, "GL_INGR_blend_func_separate");

    if (getenv("LIBGL_NO_MULTITEXTURE"))
	gl_extensions_disable(ctx, "GL_ARB_multitexture");
}

/* Initialize the driver's misc functions */
void r128DDInitDriverFuncs(GLcontext *ctx)
{
    ctx->Driver.Clear                = r128DDClear;

    ctx->Driver.GetBufferSize        = r128DDGetBufferSize;
    ctx->Driver.GetString            = r128DDGetString;
    ctx->Driver.Finish               = r128DDFinish;
    ctx->Driver.Flush                = r128DDFlush;

    ctx->Driver.Error                = NULL;
    ctx->Driver.GetParameteri        = r128DDGetParameteri;

    ctx->Driver.DrawPixels           = NULL;
    ctx->Driver.Bitmap               = NULL;

    ctx->Driver.RegisterVB           = r128DDRegisterVB;
    ctx->Driver.UnregisterVB         = r128DDUnregisterVB;
    ctx->Driver.BuildPrecalcPipeline = r128DDBuildPrecalcPipeline;
}
