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
#include "r128_vb.h"
#include "r128_fastpath.h"
#include "r128_pipeline.h"

#include "types.h"

static struct gl_pipeline_stage r128FastStage = {
    "R128 Fast Path",
    (PIPE_OP_VERT_XFORM   |
     PIPE_OP_RAST_SETUP_0 |
     PIPE_OP_RAST_SETUP_1 |
     PIPE_OP_RENDER),
    PIPE_PRECALC,
    0, 0, 0, 0, 0, 0, 0, 0, 0,   
    r128FastPath
};

/* Build the PRECALC pipeline with our stage, if possible.  Otherwise,
   return GL_FALSE */
GLboolean r128DDBuildPrecalcPipeline(GLcontext *ctx)
{
    r128ContextPtr      r128ctx = R128_CONTEXT(ctx);
    struct gl_pipeline *pipe    = &ctx->CVA.pre;

    if (r128ctx->RenderIndex == 0 &&
	!(ctx->Enabled & (TEXTURE0_3D     |
			  TEXTURE1_3D     |
			  ENABLE_TEXMAT0  |
			  ENABLE_TEXMAT1  |
			  ENABLE_TEXGEN0  |
			  ENABLE_TEXGEN1  |
			  ENABLE_USERCLIP |
			  ENABLE_LIGHT    |
			  ENABLE_FOG)) &&
	(ctx->Array.Flags & (VERT_OBJ_234 |
			     VERT_TEX0_4  |
			     VERT_TEX1_4  |
			     VERT_ELT)) == (VERT_OBJ_23 |
					    VERT_ELT)) {
	pipe->stages[0]  = &r128FastStage;
	pipe->stages[1]  = 0;
	pipe->new_inputs = ctx->RenderFlags & VERT_DATA;
	pipe->ops        = pipe->stages[0]->ops;

	r128ctx->useFastPath = GL_TRUE;
	return GL_TRUE;
    }

    if (r128ctx->useFastPath) {
	r128ctx->useFastPath = GL_FALSE;

	ctx->CVA.VB->ClipOrMask   = 0;
	ctx->CVA.VB->ClipAndMask  = CLIP_ALL_BITS;
	ctx->Array.NewArrayState |= ctx->Array.Summary;
    }

    return GL_FALSE;
}


/* Register the pipeline with our stages included */
GLuint r128RegisterPipelineStages(struct gl_pipeline_stage *out,
				  const struct gl_pipeline_stage *in,
				  GLuint nr)
{
    int i;

    for (i = 0; i < nr; i++) {
	out[i] = in[i];
	switch (in[i].ops) {
	case PIPE_OP_RAST_SETUP_0:
	    out[i].cva_state_change = (NEW_LIGHTING  |
				       NEW_TEXTURING |
				       NEW_RASTER_OPS);
	    out[i].state_change     = ~0;
	    out[i].check            = r128CheckPartialRasterSetup;
	    out[i].run              = r128PartialRasterSetup;
	    break;

	case PIPE_OP_RAST_SETUP_0|PIPE_OP_RAST_SETUP_1:
	    out[i].run              = r128DoRasterSetup;
	    break;
	}
    }

    return nr;
}
