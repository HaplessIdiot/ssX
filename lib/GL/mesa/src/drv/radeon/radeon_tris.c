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

#include <stdio.h>
#include <math.h>

#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "colormac.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "radeon_tris.h"
#include "radeon_state.h"
#include "radeon_tex.h"
#include "radeon_vb.h"
#include "radeon_ioctl.h"

static const GLuint hw_prim[GL_POLYGON+1] = {
   RADEON_CP_VC_CNTL_PRIM_TYPE_POINT,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST
};

static void radeonRasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void radeonRenderPrimitive( GLcontext *ctx, GLenum prim );
static void radeonResetLineStipple( GLcontext *ctx );


/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/


#if defined(USE_X86_ASM)
#define COPY_DWORDS( j, vb, vertsize, v )				\
do {									\
	int __tmp;							\
	__asm__ __volatile__( "rep ; movsl"				\
			      : "=%c" (j), "=D" (vb), "=S" (__tmp)	\
			      : "0" (vertsize),				\
			        "D" ((long)vb),				\
			        "S" ((long)v) );			\
} while (0)
#else
#define COPY_DWORDS( j, vb, vertsize, v )				\
do {									\
   for ( j = 0 ; j < vertsize ; j++ )					\
      vb[j] = ((GLuint *)v)[j];						\
   vb += vertsize;							\
} while (0)
#endif

static __inline void radeon_draw_quad( radeonContextPtr rmesa,
				       radeonVertexPtr v0,
				       radeonVertexPtr v1,
				       radeonVertexPtr v2,
				       radeonVertexPtr v3 )
{
   GLuint vertsize = rmesa->vertex_size;
   GLuint *vb = (GLuint *)radeonAllocDmaLow( rmesa, 6 * vertsize * 4,
					     __FUNCTION__);
   GLuint j;

   rmesa->num_verts += 6;
   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v3 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
   COPY_DWORDS( j, vb, vertsize, v3 );
}


static __inline void radeon_draw_triangle( radeonContextPtr rmesa,
					   radeonVertexPtr v0,
					   radeonVertexPtr v1,
					   radeonVertexPtr v2 )
{
   GLuint vertsize = rmesa->vertex_size;
   GLuint *vb = (GLuint *)radeonAllocDmaLow( rmesa, 3 * vertsize * 4,
					     __FUNCTION__);
   GLuint j;

   /*
   printf("radeon_draw_triangle\n");
   radeon_print_vertex(rmesa->glCtx, v0);
   radeon_print_vertex(rmesa->glCtx, v1);
   radeon_print_vertex(rmesa->glCtx, v2);
   */
   rmesa->num_verts += 3;
   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
}

static __inline void radeon_draw_line( radeonContextPtr rmesa,
				       radeonVertexPtr v0,
				       radeonVertexPtr v1 )
{
   GLuint vertsize = rmesa->vertex_size;
   GLuint *vb = (GLuint *)radeonAllocDmaLow( rmesa, 2 * vertsize * 4,
					     __FUNCTION__);
   GLuint j;

   rmesa->num_verts += 2;
   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
}

static __inline void radeon_draw_point( radeonContextPtr rmesa,
					radeonVertexPtr v0 )
{
   int vertsize = rmesa->vertex_size;
   GLuint *vb = (GLuint *)radeonAllocDmaLow( rmesa, vertsize * 4,
					     __FUNCTION__);
   int j;

   /*
   printf("radeon_draw_point\n");
   radeon_print_vertex(rmesa->glCtx, v0);
   */
   rmesa->num_verts += 1;
   COPY_DWORDS( j, vb, vertsize, v0 );
}

/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define QUAD( a, b, c, d ) radeon_draw_quad( rmesa, a, b, c, d )
#define TRI( a, b, c )     radeon_draw_triangle( rmesa, a, b, c )
#define LINE( a, b )       radeon_draw_line( rmesa, a, b )
#define POINT( a )         radeon_draw_point( rmesa, a )

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define RADEON_TWOSIDE_BIT	0x01
#define RADEON_UNFILLED_BIT	0x02
#define RADEON_MAX_TRIFUNC	0x04


static struct {
   points_func	        points;
   line_func		line;
   triangle_func	triangle;
   quad_func		quad;
} rast_tab[RADEON_MAX_TRIFUNC];


#define DO_FALLBACK  0
#define DO_OFFSET    0
#define DO_UNFILLED (IND & RADEON_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & RADEON_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA   1
#define HAVE_SPEC   1
#define HAVE_INDEX  0
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define VERTEX radeonVertex
#define TAB rast_tab

#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a < 0)
#define GET_VERTEX(e) (rmesa->verts + (e<<rmesa->vertex_stride_shift))


#define VERT_SET_RGBA( v, c )    v->ui[coloroffset] = *(GLuint *)c
#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]

#define VERT_SET_SPEC( v, c )    if (havespec) COPY_3V(v->ub4[5], c )
#define VERT_COPY_SPEC( v0, v1 ) if (havespec) COPY_3V(v0->ub4[5], v1->ub4[5])
#define VERT_SAVE_SPEC( idx )    if (havespec) spec[idx] = v[idx]->ui[5]
#define VERT_RESTORE_SPEC( idx ) if (havespec) v[idx]->ui[5] = spec[idx]

#define LOCAL_VARS(n)							\
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);			\
   GLuint color[n], spec[n];						\
   GLuint coloroffset = (rmesa->vertex_size == 4 ? 3 : 4);		\
   GLboolean havespec = (rmesa->vertex_size > 4);			\
   (void) color; (void) spec; (void) coloroffset; (void) havespec;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) if (rmesa->hw_primitive != hw_prim[x]) \
                        radeonRasterPrimitive( ctx, hw_prim[x] )
#define RENDER_PRIMITIVE rmesa->render_primitive
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND


/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/


#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (RADEON_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (RADEON_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (RADEON_TWOSIDE_BIT|RADEON_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"


static void init_rast_tab( void )
{
   init();
   init_twoside();
   init_unfilled();
   init_twoside_unfilled();
}



/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define VERT(x) (radeonVertex *)(radeonverts + (x << shift))
#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      radeon_draw_point( rmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   radeon_draw_line( rmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   radeon_draw_triangle( rmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   radeon_draw_quad( rmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) do {					\
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);	\
   radeonRenderPrimitive( ctx, x );			\
} while (0)
#undef LOCAL_VARS
#define LOCAL_VARS						\
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);		\
   const GLuint shift = rmesa->vertex_stride_shift;		\
   const char *radeonverts = (char *)rmesa->verts;		\
   const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   (void) elt; (void) stipple;
#define RESET_STIPPLE	if ( stipple ) radeonResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) radeon_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) radeon_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"


/**********************************************************************/
/*                    Render clipped primitives                       */
/**********************************************************************/

static void radeonRenderClippedPoly( GLcontext *ctx, const GLuint *elts,
				     GLuint n )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   /* Render the new vertices as an unclipped polygon.
    */
   {
      GLuint *tmp = VB->Elts;
      VB->Elts = (GLuint *)elts;
      tnl->Driver.Render.PrimTabElts[GL_POLYGON]( ctx, 0, n, PRIM_BEGIN|PRIM_END );
      VB->Elts = tmp;
   }
}

static void radeonFastRenderClippedPoly( GLcontext *ctx, const GLuint *elts,
					 GLuint n )
{
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );
   GLuint vertsize = rmesa->vertex_size;
   GLuint *vb = radeonAllocDmaLow( rmesa, (n-2) * 3 * 4 * vertsize,
				   __FUNCTION__);
   GLubyte *radeonverts = (GLubyte *)rmesa->verts;
   const GLuint shift = rmesa->vertex_stride_shift;
   const GLuint *start = (const GLuint *)VERT(elts[0]);
   int i,j;

   rmesa->num_verts += (n-2) * 3;

   for (i = 2 ; i < n ; i++) {
      COPY_DWORDS( j, vb, vertsize, start );
      COPY_DWORDS( j, vb, vertsize, VERT(elts[i-1]) );
      COPY_DWORDS( j, vb, vertsize, VERT(elts[i]) );
   }
}




/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

#define _RADEON_NEW_RENDER_STATE (_DD_NEW_TRI_LIGHT_TWOSIDE |		\
			          _DD_NEW_TRI_UNFILLED)

#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)


static void radeonChooseRenderState(GLcontext *ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (flags & DD_TRI_LIGHT_TWOSIDE) index |= RADEON_TWOSIDE_BIT;
   if (flags & DD_TRI_UNFILLED)      index |= RADEON_UNFILLED_BIT;

   if (index != rmesa->RenderIndex) {
      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.ClippedLine = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = radeon_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = radeon_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = radeonFastRenderClippedPoly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = radeonRenderClippedPoly;
      }

      rmesa->RenderIndex = index;
   }
}

/**********************************************************************/
/*                 Validate state at pipeline start                   */
/**********************************************************************/

static void radeonWrapRunPipeline( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   /* Validate state:
    */
   if (rmesa->NewGLState) {
      if (rmesa->NewGLState & _NEW_TEXTURE)
	 radeonUpdateTextureState( ctx );

      if (!rmesa->Fallback) {
	 if (rmesa->NewGLState & _RADEON_NEW_VERTEX_STATE)
	    radeonChooseVertexState( ctx );

	 if (rmesa->NewGLState & _RADEON_NEW_RENDER_STATE)
	    radeonChooseRenderState( ctx );
      }

      rmesa->NewGLState = 0;
   }

   /* Run the pipeline.
    */ 
   _tnl_run_pipeline( ctx );

   /* Nothing left to do.
    */
}

/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/


static void radeonRasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   if (rmesa->hw_primitive != hwprim) {
      RADEON_STATECHANGE( rmesa, 0 );
      rmesa->hw_primitive = hwprim;
   }
}

static void radeonRenderPrimitive( GLcontext *ctx, GLenum prim )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint hw = hw_prim[prim];
   rmesa->render_primitive = prim;
   if (prim >= GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;
   radeonRasterPrimitive( ctx, hw );
}

static void radeonRenderFinish( GLcontext *ctx )
{
}

static void radeonResetLineStipple( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   /* Reset the hardware stipple counter.
    */
   fprintf(stderr, "%s\n", __FUNCTION__);
   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_LINE );
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

static char *fallbackStrings[] = {
   "Texture mode",
   "glDrawBuffer(GL_FRONT_AND_BACK)",
   "glEnable(GL_STENCIL) without hw stencil buffer",
   "glRenderMode(selection or feedback)",
   "glBlendEquation",
   "glBlendFunc(mode != ADD)"
};


static char *getFallbackString(GLuint bit)
{
   int i = 0;
   while (bit > 1) {
      i++;
      bit >>= 1;
   }
   return fallbackStrings[i];
}


void radeonFallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = rmesa->Fallback;

   if (mode) {
      rmesa->Fallback |= bit;
      if (oldfallback == 0) {
	 RADEON_FIREVERTICES( rmesa );
	 _swsetup_Wakeup( ctx );
	 _tnl_need_projected_coords( ctx, GL_TRUE );
	 rmesa->RenderIndex = ~0;
         if (rmesa->debugFallbacks) {
            fprintf(stderr, "Radeon begin software fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
   else {
      rmesa->Fallback &= ~bit;
      if (oldfallback == bit) {
         /*printf("End fallback 0x%x\n", bit);*/
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = radeonCheckTexSizes;
	 tnl->Driver.Render.PrimitiveNotify = radeonRenderPrimitive;
	 tnl->Driver.Render.Finish = radeonRenderFinish;
	 tnl->Driver.Render.BuildVertices = radeonBuildVertices;
	 tnl->Driver.Render.ResetLineStipple = radeonResetLineStipple;
	 rmesa->NewGLState |= (_RADEON_NEW_RENDER_STATE|
			       _RADEON_NEW_VERTEX_STATE);
         if (rmesa->debugFallbacks) {
            fprintf(stderr, "Radeon end software fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void radeonInitTriFuncs( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }

   tnl->Driver.RunPipeline = radeonWrapRunPipeline;
   tnl->Driver.Render.Start = radeonCheckTexSizes;
   tnl->Driver.Render.Finish = radeonRenderFinish;
   tnl->Driver.Render.PrimitiveNotify = radeonRenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = radeonResetLineStipple;
   tnl->Driver.Render.BuildVertices = radeonBuildVertices;

/*     radeonFallback( ctx, 0x100000, 1 ); */
}
