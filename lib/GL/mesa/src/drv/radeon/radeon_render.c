/* $XFree86$ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keithw@valinux.com>
 *
 */


#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "mem.h"
#include "mtypes.h"
#include "mmath.h"

#include "tnl/t_context.h"

#include "radeon_context.h"
#include "radeon_tris.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_vb.h"

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0
#define HAVE_POLYGONS    0

#define HAVE_ELTS        1

static const GLuint hw_prim[GL_POLYGON+1] = {
   RADEON_CP_VC_CNTL_PRIM_TYPE_POINT,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   0,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE_STRIP,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_STRIP,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN,
   0,
   0,
   0
};

static __inline void radeonDmaPrimitive( radeonContextPtr rmesa, GLenum prim )
{
   RADEON_STATECHANGE( rmesa, 0 );
   rmesa->hw_primitive = hw_prim[prim];
}

static __inline void radeonEltPrimitive( radeonContextPtr rmesa, GLenum prim )
{
   RADEON_STATECHANGE( rmesa, 0 );
   rmesa->hw_primitive = hw_prim[prim] | RADEON_CP_VC_CNTL_PRIM_WALK_IND;
}


static void VERT_FALLBACK( GLcontext *ctx,
			   GLuint start,
			   GLuint count,
			   GLuint flags )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.PrimitiveNotify( ctx, flags & PRIM_MODE_MASK );
   tnl->Driver.Render.BuildVertices( ctx, start, count, ~0 );
   tnl->Driver.Render.PrimTabVerts[flags&PRIM_MODE_MASK]( ctx, start, count, flags );
   RADEON_CONTEXT(ctx)->SetupNewInputs = VERT_CLIP;
}

static void ELT_FALLBACK( GLcontext *ctx,
			  GLuint start,
			  GLuint count,
			  GLuint flags )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.PrimitiveNotify( ctx, flags & PRIM_MODE_MASK );
   tnl->Driver.Render.BuildVertices( ctx, 0, tnl->vb.Count, ~0 ); /* argh! */
   tnl->Driver.Render.PrimTabElts[flags&PRIM_MODE_MASK]( ctx, start, count, flags );
   RADEON_CONTEXT(ctx)->SetupNewInputs = VERT_CLIP;
}


#define LOCAL_VARS radeonContextPtr rmesa = RADEON_CONTEXT(ctx)
#define ELTS_VARS  GLushort *dest
#define INIT( prim ) radeonDmaPrimitive( rmesa, prim )
#define ELT_INIT(prim) radeonEltPrimitive( rmesa, prim )
#define NEW_PRIMITIVE()  RADEON_STATECHANGE( rmesa, 0 )
#define NEW_BUFFER()  RADEON_FIREVERTICES( rmesa )
#define GET_CURRENT_VB_MAX_ELTS() \
  (((int)rmesa->dma.high - (int)rmesa->dma.low - 20) / 2)
#define GET_CURRENT_VB_MAX_VERTS() \
  (((int)rmesa->dma.high - (int)rmesa->dma.low) / (rmesa->vertex_size*4))
#define GET_SUBSEQUENT_VB_MAX_ELTS() \
  ((RADEON_BUFFER_SIZE - 20) / 2)
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
  RADEON_BUFFER_SIZE / (rmesa->vertex_size * 4)

#define ALLOC_ELTS_NEW_PRIMITIVE(nr) do {			\
   dest = (GLushort *)radeonAllocDmaLow( rmesa, 20 + nr * 2,	\
					 __FUNCTION__);		\
   dest += RADEON_INDEX_PRIM_OFFSET / sizeof(*dest);		\
} while (0)

#define ALLOC_ELTS(nr) do {					\
   if (rmesa->dma.low == rmesa->dma.last) {		\
      ALLOC_ELTS_NEW_PRIMITIVE( nr );				\
   } else {							\
      dest = (GLushort *)radeonAllocDmaLow( rmesa, nr * 2,	\
					    __FUNCTION__);	\
   }								\
} while (0)

#define EMIT_ELT(offset, x) (dest)[offset] = (GLushort) (x)
#if defined(__i386__)
#define EMIT_TWO_ELTS(offset, x, y)  *(GLuint *)(dest+offset) = ((y)<<16)|(x);
#endif
#define INCR_ELTS( nr ) dest += nr
#define RELEASE_ELT_VERTS() radeonReleaseRetainedBuffer( rmesa )
#define EMIT_VERTS( ctx, j, nr ) \
  radeon_emit_contiguous_verts(ctx, j, (j)+(nr))
#define EMIT_INDEXED_VERTS( ctx, start, count ) \
  radeon_emit_indexed_verts( ctx, start, count )


#define TAG(x) radeon_##x
#include "tnl_dd/t_dd_dmatmp.h"


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


static GLboolean radeon_run_render( GLcontext *ctx,
				  struct gl_pipeline_stage *stage )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i, length, flags = 0;
   render_func *tab;

   if (rmesa->dma.retained && (!VB->Elts || stage->changed_inputs)) 
      radeonReleaseRetainedBuffer( rmesa );

   if (VB->ClipOrMask ||	     /* No clipping */
       rmesa->RenderIndex != 0 ||    /* No per-vertex manipulations */
       ctx->Line.StippleFlag)        /* GH: THIS IS A HACK!!! */
      return GL_TRUE;		

   if (VB->Elts) {
      tab = TAG(render_tab_elts);
      if (!rmesa->dma.retained)
	 if (!TAG(emit_elt_verts)(ctx, 0, VB->Count))
	    return GL_TRUE;	/* too many vertices */
   } else {
      tab = TAG(render_tab_verts);
   }


   tnl->Driver.Render.Start( ctx );

   for (i = VB->FirstPrimitive ; !(flags & PRIM_LAST) ; i += length)
   {
      flags = VB->Primitive[i];
      length = VB->PrimitiveLength[i];
      if (length)
	 tab[flags & PRIM_MODE_MASK]( ctx, i, i + length, flags );
   }

   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;		/* finished the pipe */
}


static void radeon_check_render( GLcontext *ctx,
				 struct gl_pipeline_stage *stage )
{
   GLuint inputs = VERT_CLIP|VERT_RGBA;

   if (ctx->RenderMode == GL_RENDER) {
      if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
	 inputs |= VERT_SPEC_RGB;

      if (ctx->Texture.Unit[0]._ReallyEnabled)
	 inputs |= VERT_TEX(0);

      if (ctx->Texture.Unit[1]._ReallyEnabled)
	 inputs |= VERT_TEX(1);

      if (ctx->Fog.Enabled)
	 inputs |= VERT_FOG_COORD;
   }

   stage->inputs = inputs;
}


static void dtr( struct gl_pipeline_stage *stage )
{
   (void)stage;
}


const struct gl_pipeline_stage _radeon_render_stage =
{
   "radeon render",
   (_DD_NEW_SEPARATE_SPECULAR |
    _NEW_TEXTURE|
    _NEW_FOG|
    _NEW_RENDERMODE),		/* re-check (new inputs) */
   0,				/* re-run (always runs) */
   GL_TRUE,			/* active */
   0, 0,			/* inputs (set in check_render), outputs */
   0, 0,			/* changed_inputs, private */
   dtr,				/* destructor */
   radeon_check_render,		/* check - initially set to alloc data */
   radeon_run_render		/* run */
};
