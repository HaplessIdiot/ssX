/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_vtxfmt.c,v 1.1 2002/02/22 21:45:01 dawes Exp $ */
/*
 * Copyright 2000, 2001 VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 */


#include "glheader.h"
#include "api_noop.h"
#include "colormac.h"
#include "context.h"
#include "light.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"
#include "simple_list.h"
#include "vtxfmt.h"

#include "math/m_xform.h"
#include "tnl/tnl.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_vb.h"
#include "radeon_vtxfmt.h"


#define VERTEX				radeonVertex
#define TNL_VERTEX			radeonTnlVertex


#define LINTERP( T, A, B )		((A) + (T) * ((B) - (A)))

#define INTERP_RGBA( t, out, a, b )					\
do {									\
   GLint i;								\
   for ( i = 0 ; i < 4 ; i++ ) {					\
      GLfloat fa = UBYTE_TO_FLOAT( a[i] );				\
      GLfloat fb = UBYTE_TO_FLOAT( b[i] );				\
      GLfloat fo = LINTERP( t, fa, fb );				\
      UNCLAMPED_FLOAT_TO_UBYTE( out[i], fo );				\
   }									\
} while (0)




/* ================================================================
 * Color functions:  Always update ctx->Current.*
 */

/* ================================================================
 * Material functions:
 */

static __inline void radeon_recalc_base_color( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct gl_light *light;

   COPY_3V( rmesa->state.light.base_color, ctx->Light._BaseColor[0] );
   foreach ( light, &ctx->Light.EnabledList ) {
      ACC_3V( rmesa->state.light.base_color, light->_MatAmbient[0] );
   }
    
   UNCLAMPED_FLOAT_TO_UBYTE( rmesa->state.light.base_alpha, 
			     ctx->Light.Material[0].Diffuse[3] );
}


/* ================================================================
 * Normal functions:
 */

struct radeon_norm_tab {
   void (*normal3f_multi)( GLfloat x, GLfloat y, GLfloat z );
   void (*normal3fv_multi)( const GLfloat *v );
   void (*normal3f_single)( GLfloat x, GLfloat y, GLfloat z );
   void (*normal3fv_single)( const GLfloat *v );
};

static struct radeon_norm_tab norm_tab[0x4];


#define HAVE_HW_LIGHTING 0

#define GET_CURRENT_VERTEX						\
   GET_CURRENT_CONTEXT(ctx);						\
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);			\
   radeonTnlVertexPtr v = rmesa->imm.v0

#define CURRENT_NORMAL			rmesa->state.current.normal
#define BASE_COLOR			rmesa->state.light.base_color
#define BASE_ALPHA			rmesa->state.light.base_alpha

#define VERT_COLOR( COMP )		v->color[COMP]


#define IND (0)
#define TAG(x) radeon_##x
#define PRESERVE_NORMAL_DEFS
#include "tnl_dd/t_dd_imm_napi.h"

#define IND (NORM_RESCALE)
#define TAG(x) radeon_##x##_rescale
#define PRESERVE_NORMAL_DEFS
#include "tnl_dd/t_dd_imm_napi.h"

#define IND (NORM_NORMALIZE)
#define TAG(x) radeon_##x##_normalize
#include "tnl_dd/t_dd_imm_napi.h"


static void radeon_init_norm_funcs( void )
{
   radeon_init_norm();
   radeon_init_norm_rescale();
   radeon_init_norm_normalize();
}

static void radeon_choose_Normal3f( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint index;

   if ( ctx->Light.Enabled ) {
      if ( ctx->Transform.Normalize ) {
	 index = NORM_NORMALIZE;
      }
      else if ( !ctx->Transform.RescaleNormals &&
		ctx->_ModelViewInvScale != 1.0 ) {
	 index = NORM_RESCALE;
      }
      else {
	 index = 0;
      }

      if ( ctx->Light.EnabledList.next == ctx->Light.EnabledList.prev ) {
	 ctx->Exec->Normal3f  = norm_tab[index].normal3f_single;
      } else {
	 ctx->Exec->Normal3f  = norm_tab[index].normal3f_multi;
      }
   } else {
      ctx->Exec->Normal3f  = _mesa_noop_Normal3f;
   }

   glNormal3f( x, y, z );
}

static void radeon_choose_Normal3fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint index;

   if ( ctx->Light.Enabled ) {
      if ( ctx->Transform.Normalize ) {
	 index = NORM_NORMALIZE;
      }
      else if ( !ctx->Transform.RescaleNormals &&
		ctx->_ModelViewInvScale != 1.0 ) {
	 index = NORM_RESCALE;
      }
      else {
	 index = 0;
      }

      if ( ctx->Light.EnabledList.next == ctx->Light.EnabledList.prev ) {
	 ctx->Exec->Normal3fv = norm_tab[index].normal3fv_single;
      } else {
	 ctx->Exec->Normal3fv = norm_tab[index].normal3fv_multi;
      }
   } else {
      ctx->Exec->Normal3fv = _mesa_noop_Normal3fv;
   }

   glNormal3fv( v );
}




/* ================================================================
 * Texture functions:
 */

#define GET_CURRENT							\
   GET_CURRENT_CONTEXT(ctx);						\
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx)

#define NUM_TEXTURE_UNITS		RADEON_MAX_TEXTURE_UNITS
#define DO_PROJ_TEX

#define CURRENT_TEXTURE( unit )		rmesa->state.current.texture[unit]

#define TAG(x) radeon_##x
#include "tnl_dd/t_dd_imm_tapi.h"



/* ================================================================
 * Vertex functions:
 */

#define GET_CURRENT_VERTEX						\
   GET_CURRENT_CONTEXT(ctx);						\
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);			\
   radeonTnlVertexPtr v = rmesa->imm.v0

#define CURRENT_VERTEX			v->obj
#define SAVE_VERTEX			rmesa->imm.save_vertex( ctx, v )

#define TAG(x) radeon_##x
#include "tnl_dd/t_dd_imm_vapi.h"




struct radeon_vert_tab {
   void (*save_vertex)( GLcontext *ctx, radeonTnlVertexPtr v );
   void (*interpolate_vertex)( GLfloat t,
			       radeonTnlVertex *O,
			       const radeonTnlVertex *I,
			       const radeonTnlVertex *J );
};

static struct radeon_vert_tab vert_tab[0xf];

#define VTX_NORMAL	0x0
#define VTX_RGBA	0x1
#define VTX_SPEC	0x2
#define VTX_TEX0	0x4
#define VTX_TEX1	0x8

#define LOCAL_VARS							\
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx)

#define CURRENT_COLOR			rmesa->state.current.color
#define CURRENT_SPECULAR		rmesa->state.current.specular

#define CURRENT_NORMAL( COMP )		rmesa->state.current.normal[COMP]
#define CURRENT_TEXTURE( U, COMP )	rmesa->state.current.texture[U][COMP]

#define FLUSH_VERTEX			rmesa->imm.flush_vertex( ctx, v );


#define IND (VTX_NORMAL)
#define TAG(x) radeon_##x##_NORMAL
#define PRESERVE_VERTEX_DEFS
#include "tnl_dd/t_dd_imm_vertex.h"

#define IND (VTX_NORMAL|VTX_TEX0)
#define TAG(x) radeon_##x##_NORMAL_TEX0
#define PRESERVE_VERTEX_DEFS
#include "tnl_dd/t_dd_imm_vertex.h"

#define IND (VTX_NORMAL|VTX_TEX0|VTX_TEX1)
#define TAG(x) radeon_##x##_NORMAL_TEX0_TEX1
#define PRESERVE_VERTEX_DEFS
#include "tnl_dd/t_dd_imm_vertex.h"

#define IND (VTX_RGBA)
#define TAG(x) radeon_##x##_RGBA
#define PRESERVE_VERTEX_DEFS
#include "tnl_dd/t_dd_imm_vertex.h"

#define IND (VTX_RGBA|VTX_TEX0)
#define TAG(x) radeon_##x##_RGBA_TEX0
#define PRESERVE_VERTEX_DEFS
#include "tnl_dd/t_dd_imm_vertex.h"

#define IND (VTX_RGBA|VTX_TEX1)
#define TAG(x) radeon_##x##_RGBA_TEX0_TEX1
#include "tnl_dd/t_dd_imm_vertex.h"


static void radeon_init_vert_funcs( void )
{
   radeon_init_vert_NORMAL();
   radeon_init_vert_NORMAL_TEX0();
   radeon_init_vert_NORMAL_TEX0_TEX1();
   radeon_init_vert_RGBA();
   radeon_init_vert_RGBA_TEX0();
   radeon_init_vert_RGBA_TEX0_TEX1();
}







#define LOCAL_VARS							\
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx)

#define FLUSH_VERTEX			rmesa->imm.flush_vertex

#define IMM_VERTEX( V )			rmesa->imm.V
#define IMM_VERTICES( n )		rmesa->imm.vertices[n]


/* TINY_VERTEX_FORMAT:
 */
#define GET_VERTEX_SPACE( n ) radeonAllocDmaLow( rmesa, n * 16, __FUNCTION__ )

#define EMIT_VERTEX( vb, v )						\
do {									\
   vb[0] = *(GLuint *)&(v->clip[0]);					\
   vb[1] = *(GLuint *)&(v->clip[1]);					\
   vb[2] = *(GLuint *)&(v->clip[2]);					\
   vb[3] = *(GLuint *)&(v->color);					\
   vb += 4;								\
} while (0)

#define TAG(x) radeon_##x##_tiny
#define PRESERVE_PRIM_DEFS
#include "tnl_dd/t_dd_imm_primtmp.h"


/* NOTEX_VERTEX_FORMAT:
 */
#define GET_VERTEX_SPACE( n ) radeonAllocDmaLow( rmesa, n * 24, __FUNCTION__ )

#define EMIT_VERTEX( vb, v )						\
do {									\
   vb[0] = *(GLuint *)&(v->clip[0]);					\
   vb[1] = *(GLuint *)&(v->clip[1]);					\
   vb[2] = *(GLuint *)&(v->clip[2]);					\
   vb[3] = *(GLuint *)&(v->clip[3]);					\
   vb[4] = *(GLuint *)&(v->color);					\
   vb[5] = *(GLuint *)&(v->specular);					\
   vb += 6;								\
} while (0)

#define TAG(x) radeon_##x##_notex
#define PRESERVE_PRIM_DEFS
#include "tnl_dd/t_dd_imm_primtmp.h"


/* TEX0_VERTEX_FORMAT:
 */
#define GET_VERTEX_SPACE( n ) radeonAllocDmaLow( rmesa, n * 32, __FUNCTION__ )

#define EMIT_VERTEX( vb, v )						\
do {									\
   vb[0] = *(GLuint *)&(v->clip[0]);					\
   vb[1] = *(GLuint *)&(v->clip[1]);					\
   vb[2] = *(GLuint *)&(v->clip[2]);					\
   vb[3] = *(GLuint *)&(v->clip[3]);					\
   vb[4] = *(GLuint *)&(v->color);					\
   vb[5] = *(GLuint *)&(v->specular);					\
   vb[6] = *(GLuint *)&(v->texture[0][0]);				\
   vb[7] = *(GLuint *)&(v->texture[0][1]);				\
   vb += 8;								\
} while (0)

#define TAG(x) radeon_##x##_tex0
#define PRESERVE_PRIM_DEFS
#include "tnl_dd/t_dd_imm_primtmp.h"


/* TEX1_VERTEX_FORMAT:
 */
#define GET_VERTEX_SPACE( n ) radeonAllocDmaLow( rmesa, n * 40, __FUNCTION__ )

#define EMIT_VERTEX( vb, v )						\
do {									\
   vb[0] = *(GLuint *)&(v->clip[0]);					\
   vb[1] = *(GLuint *)&(v->clip[1]);					\
   vb[2] = *(GLuint *)&(v->clip[2]);					\
   vb[3] = *(GLuint *)&(v->clip[3]);					\
   vb[4] = *(GLuint *)&(v->color);					\
   vb[5] = *(GLuint *)&(v->specular);					\
   vb[6] = *(GLuint *)&(v->texture[0][0]);				\
   vb[7] = *(GLuint *)&(v->texture[0][1]);				\
   vb[8] = *(GLuint *)&(v->texture[1][0]);				\
   vb[9] = *(GLuint *)&(v->texture[1][1]);				\
   vb += 10;								\
} while (0)

#define TAG(x) radeon_##x##_tex1
#define PRESERVE_PRIM_DEFS
#include "tnl_dd/t_dd_imm_primtmp.h"







/* Bzzt: Material changes are lost on fallback.
 */
static void radeon_Materialfv( GLenum face, GLenum pname,
			       const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_noop_Materialfv( face, pname, params );
   radeon_recalc_base_color( ctx );
}





/* ================================================================
 * Fallback functions:
 */

static void radeon_do_fallback( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct radeon_current_state *current = &rmesa->state.current;

   /* Tell tnl to restore its exec vtxfmt, rehook its driver callbacks
    * and revive internal state that depended on those callbacks:
    */
   _tnl_wakeup_exec( ctx );

   /* Replay enough vertices that the current primitive is continued
    * correctly:
    */
   if ( rmesa->imm.prim != PRIM_OUTSIDE_BEGIN_END ) {
      glBegin( rmesa->imm.prim );
      /*rmesa->fire_on_fallback( ctx );*/
   }

   /* Replay the current, partially complete vertex:
    */
   if ( current->texture[0][3] == 1.0 ) {
      glMultiTexCoord3fvARB( GL_TEXTURE0_ARB, current->texture[0] );
   } else {
      glMultiTexCoord4fvARB( GL_TEXTURE0_ARB, current->texture[0] );
   }

   if ( current->texture[1][3] == 1.0 ) {
      glMultiTexCoord3fvARB( GL_TEXTURE1_ARB, current->texture[1] );
   } else {
      glMultiTexCoord4fvARB( GL_TEXTURE1_ARB, current->texture[1] );
   }

   /* FIXME: Secondary color, fog coord...
    */

   if ( ctx->Light.Enabled ) {
      glColor4fv( ctx->Current.Color );	/* Catch ColorMaterial */
      glNormal3fv( current->normal );
   } else {
      glColor4ubv( current->color );
   }
}

#define PRE_LOOPBACK( FUNC ) do {					\
   GET_CURRENT_CONTEXT(ctx);						\
   radeon_do_fallback( ctx );						\
} while (0)

#define TAG(x) radeon_fallback_##x
#include "vtxfmt_tmp.h"






static void radeon_Begin( GLenum prim )
{
   GET_CURRENT_CONTEXT(ctx);
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if ( prim > GL_POLYGON ) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glBegin" );
      return;
   }
   if ( rmesa->imm.prim != PRIM_OUTSIDE_BEGIN_END ) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      return;
   }

   ctx->Driver.NeedFlush |= (FLUSH_STORED_VERTICES |
			     FLUSH_UPDATE_CURRENT);


   radeonChooseVertexState( ctx );


   rmesa->imm.prim = prim;
   rmesa->imm.v0 = &rmesa->imm.vertices[0];

   rmesa->imm.save_vertex = radeon_save_vertex_RGBA;
   rmesa->imm.flush_vertex = rmesa->imm.flush_tab[prim];
}

static void radeon_End( void )
{
   GET_CURRENT_CONTEXT(ctx);
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if ( rmesa->imm.prim == PRIM_OUTSIDE_BEGIN_END ) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glEnd" );
      return;
   }

   rmesa->imm.prim = PRIM_OUTSIDE_BEGIN_END;

   ctx->Driver.NeedFlush &= ~(FLUSH_STORED_VERTICES |
			      FLUSH_UPDATE_CURRENT);
}






void radeonInitTnlModule( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLvertexformat *vfmt = &(rmesa->imm.vtxfmt);

   return;

   radeon_init_norm_funcs();
   radeon_init_vert_funcs();

   MEMSET( vfmt, 0, sizeof(GLvertexformat) );

   /* Handled fully in supported states:
    */
   vfmt->ArrayElement = NULL;				/* FIXME: ... */
   vfmt->Color3f = radeon_choose_Color3f;
   vfmt->Color3fv = radeon_choose_Color3fv;
   vfmt->Color3ub = radeon_choose_Color3ub;
   vfmt->Color3ubv = radeon_choose_Color3ubv;
   vfmt->Color4f = radeon_choose_Color4f;
   vfmt->Color4fv = radeon_choose_Color4fv;
   vfmt->Color4ub = radeon_choose_Color4ub;
   vfmt->Color4ubv = radeon_choose_Color4ubv;
   vfmt->FogCoordfvEXT = radeon_FogCoordfvEXT;
   vfmt->FogCoordfEXT = radeon_FogCoordfEXT;
   vfmt->Materialfv = radeon_Materialfv;
   vfmt->MultiTexCoord1fARB = radeon_MultiTexCoord1fARB;
   vfmt->MultiTexCoord1fvARB = radeon_MultiTexCoord1fvARB;
   vfmt->MultiTexCoord2fARB = radeon_MultiTexCoord2fARB;
   vfmt->MultiTexCoord2fvARB = radeon_MultiTexCoord2fvARB;
   vfmt->MultiTexCoord3fARB = radeon_MultiTexCoord3fARB;
   vfmt->MultiTexCoord3fvARB = radeon_MultiTexCoord3fvARB;
   vfmt->MultiTexCoord4fARB = radeon_MultiTexCoord4fARB;
   vfmt->MultiTexCoord4fvARB = radeon_MultiTexCoord4fvARB;
   vfmt->Normal3f = radeon_choose_Normal3f;
   vfmt->Normal3fv = radeon_choose_Normal3fv;
   vfmt->SecondaryColor3ubEXT = radeon_SecondaryColor3ubEXT;
   vfmt->SecondaryColor3ubvEXT = radeon_SecondaryColor3ubvEXT;
   vfmt->SecondaryColor3fEXT = radeon_SecondaryColor3fEXT;
   vfmt->SecondaryColor3fvEXT = radeon_SecondaryColor3fvEXT;
   vfmt->TexCoord1f = radeon_TexCoord1f;
   vfmt->TexCoord1fv = radeon_TexCoord1fv;
   vfmt->TexCoord2f = radeon_TexCoord2f;
   vfmt->TexCoord2fv = radeon_TexCoord2fv;
   vfmt->TexCoord3f = radeon_TexCoord3f;
   vfmt->TexCoord3fv = radeon_TexCoord3fv;
   vfmt->TexCoord4f = radeon_TexCoord4f;
   vfmt->TexCoord4fv = radeon_TexCoord4fv;
   vfmt->Vertex2f = radeon_Vertex2f;
   vfmt->Vertex2fv = radeon_Vertex2fv;
   vfmt->Vertex3f = radeon_Vertex3f;
   vfmt->Vertex3fv = radeon_Vertex3fv;
   vfmt->Vertex4f = radeon_Vertex4f;
   vfmt->Vertex4fv = radeon_Vertex4fv;

   vfmt->Begin = radeon_Begin;
   vfmt->End = radeon_End;

   vfmt->Rectf = _mesa_noop_Rectf;			/* generic helper */

   vfmt->DrawArrays = NULL;
   vfmt->DrawElements = NULL;
   vfmt->DrawRangeElements = _mesa_noop_DrawRangeElements; /* discard range */


   /* Not active in supported states; just keep ctx->Current uptodate:
    */
   vfmt->EdgeFlag = _mesa_noop_EdgeFlag;
   vfmt->EdgeFlagv = _mesa_noop_EdgeFlagv;
   vfmt->Indexi = _mesa_noop_Indexi;
   vfmt->Indexiv = _mesa_noop_Indexiv;


   /* Active but unsupported -- fallback if we receive these:
    *
    * All of these fallbacks can be fixed with additional code, except
    * CallList, unless we build a play_immediate_noop() command which
    * turns an immediate back into glBegin/glEnd commands...
    */
   vfmt->CallList = radeon_fallback_CallList;
   vfmt->EvalCoord1f = radeon_fallback_EvalCoord1f;
   vfmt->EvalCoord1fv = radeon_fallback_EvalCoord1fv;
   vfmt->EvalCoord2f = radeon_fallback_EvalCoord2f;
   vfmt->EvalCoord2fv = radeon_fallback_EvalCoord2fv;
   vfmt->EvalMesh1 = radeon_fallback_EvalMesh1;
   vfmt->EvalMesh2 = radeon_fallback_EvalMesh2;
   vfmt->EvalPoint1 = radeon_fallback_EvalPoint1;
   vfmt->EvalPoint2 = radeon_fallback_EvalPoint2;


   rmesa->imm.prim = PRIM_OUTSIDE_BEGIN_END;

   /* THIS IS A HACK!
    */
   _mesa_install_exec_vtxfmt( ctx, vfmt );
}






#if 0



static void radeon_Begin( GLenum prim )
{
   GET_CURRENT_CONTEXT(ctx);
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   radeon_prim *tab = &radeon_prim_tab[(int)prim];

   if ( prim > GL_POLYGON ) {
      gl_error( ctx, GL_INVALID_ENUM, "glBegin" );
      return;
   }

   if ( rmesa->prim != PRIM_OUTSIDE_BEGIN_END ) {
      gl_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      return;
   }

   if ( tab->fire_on_vertex ) {
      rmesa->fire_on_vertex = tab->fire_on_vertex;
      rmesa->fire_on_end = tab->fire_on_end;
      rmesa->fire_on_fallback = tab->fire_on_fallback;
      rmesa->vert = &(rmesa->cache[0]);
      rmesa->prim = prim;
      ctx->Driver.NeedFlush |= (FLUSH_INSIDE_BEGIN_END |
				FLUSH_STORED_VERTICES);
   } else {
      radeon_fallback_vtxfmt( ctx );
   }
}

static void radeon_End( void )
{
   GET_CURRENT_CONTEXT(ctx);
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if ( rmesa->prim == PRIM_OUTSIDE_BEGIN_END ) {
      gl_error( ctx, GL_INVALID_OPERATION, "glEnd" );
      return;
   }

   rmesa->fire_on_end( ctx );
   rmesa->prim = PRIM_OUTSIDE_BEGIN_END;

   ctx->Exec->Vertex3fv = radeon_noop_Vertex3fv;
   ctx->Exec->Vertex3f = radeon_noop_Vertex3f;
   ctx->Exec->Vertex2f = radeon_noop_Vertex2f;

   ctx->Driver.NeedFlush &= ~(FLUSH_INSIDE_BEGIN_END |
			      FLUSH_STORED_VERTICES);
}




static GLboolean radeon_flush_vtxfmt( GLcontext *ctx, GLuint flags )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if ( fxMesa->prim != PRIM_OUTSIDE_BEGIN_END )
      return GL_FALSE;

   /* Outside begin/end.  All vertices will already be flushed, just
    * update ctx->Current.
    */
   if ( flags & FLUSH_UPDATE_CURRENT ) {
      radeonClipVertexPtr v = &(RADEON_CONTEXT(ctx)->Current);
      COPY_2FV( ctx->Current.Texcoord[0], v->texcoord[0] );
      COPY_2FV( ctx->Current.Texcoord[1], v->texcoord[1] );
      if ( rmesa->accel_light == ACCEL_LIGHT ) {
	 COPY_3FV( ctx->Current.Normal, v->normal );
      } else {
	 ctx->Current.Color[RCOMP] = UBYTE_TO_CHAN( v->v.color.red );
	 ctx->Current.Color[GCOMP] = UBYTE_TO_CHAN( v->v.color.green );
	 ctx->Current.Color[BCOMP] = UBYTE_TO_CHAN( v->v.color.blue );
	 ctx->Current.Color[ACOMP] = UBYTE_TO_CHAN( v->v.color.alpha );

	 if ( ctx->Light.ColorMaterialEnabled )
	    _mesa_update_color_material( ctx, ctx->Current.Color );
      }
   }

   /* Could clear this flag and set it from each 'choose' function,
    * maybe, but there isn't much of a penalty for leaving it set:
    */
   ctx->Driver.NeedFlush = FLUSH_UPDATE_CURRENT;
   return GL_TRUE;
}

void radeon_update_lighting( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if ( !ctx->Light.Enabled ) {
      rmesa->accel_light = ACCEL_NO_LIGHT;
   }
   else if ( !ctx->Light._NeedVertices && !ctx->Light.Model.TwoSide ) {
      rmesa->accel_light = ACCEL_LIGHT;
      radeon_recalc_basecolor( ctx );
   }
   else {
      radeon->accel_light = 0;
   }
}


/* How to fallback:
 *   - install default vertex format
 *   - call glBegin
 *   - revive stalled vertices (may be reordered).
 *   - re-issue call that caused fallback.
 */
#endif
