/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_state.c,v 1.3 2002/02/22 21:45:01 dawes Exp $ */
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
 *    Keith Whitwell <keithw@valinux.com>
 */

#include "radeon_context.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_tris.h"
#include "radeon_vb.h"
#include "radeon_tex.h"

#include "mmath.h"
#include "enums.h"
#include "colormac.h"

#include "swrast/swrast.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"


/* =============================================================
 * Alpha blending
 */

static void radeonAlphaFunc( GLcontext *ctx, GLenum func, GLchan ref )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );

   rmesa->state.hw.context.pp_misc &= ~(RADEON_ALPHA_TEST_OP_MASK |
					RADEON_REF_ALPHA_MASK);

   switch ( func ) {
   case GL_NEVER:
      rmesa->state.hw.context.pp_misc |= RADEON_ALPHA_TEST_FAIL;
      break;
   case GL_LESS:
      rmesa->state.hw.context.pp_misc |= RADEON_ALPHA_TEST_LESS;
      break;
   case GL_EQUAL:
      rmesa->state.hw.context.pp_misc |= RADEON_ALPHA_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      rmesa->state.hw.context.pp_misc |= RADEON_ALPHA_TEST_LEQUAL;
      break;
   case GL_GREATER:
      rmesa->state.hw.context.pp_misc |= RADEON_ALPHA_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      rmesa->state.hw.context.pp_misc |= RADEON_ALPHA_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      rmesa->state.hw.context.pp_misc |= RADEON_ALPHA_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      rmesa->state.hw.context.pp_misc |= RADEON_ALPHA_TEST_PASS;
      break;
   }

   rmesa->state.hw.context.pp_misc |= (ref & RADEON_REF_ALPHA_MASK);
}

static void radeonBlendEquation( GLcontext *ctx, GLenum mode )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint b = rmesa->state.hw.context.rb3d_blendcntl & ~RADEON_COMB_FCN_MASK;
   GLboolean fallback = GL_FALSE;

   switch ( mode ) {
   case GL_FUNC_ADD_EXT:
   case GL_LOGIC_OP:
      b |= RADEON_COMB_FCN_ADD_CLAMP;
      break;

   case GL_FUNC_SUBTRACT_EXT:
      b |= RADEON_COMB_FCN_SUB_CLAMP;
      break;

   default:
      fallback = GL_TRUE;
      break;
   }

   FALLBACK( rmesa, RADEON_FALLBACK_BLEND_EQ, fallback );
   if ( !fallback ) {
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
      rmesa->state.hw.context.rb3d_blendcntl = b;
      if ( ctx->Color.ColorLogicOpEnabled ) {
	 rmesa->state.hw.context.rb3d_cntl |=  RADEON_ROP_ENABLE;
      } else {
	 rmesa->state.hw.context.rb3d_cntl &= ~RADEON_ROP_ENABLE;
      }
   }
}

static void radeonBlendFunc( GLcontext *ctx, GLenum sfactor, GLenum dfactor )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint b = rmesa->state.hw.context.rb3d_blendcntl & ~(RADEON_SRC_BLEND_MASK |
							 RADEON_DST_BLEND_MASK);
   GLboolean fallback = GL_FALSE;

   switch ( ctx->Color.BlendSrcRGB ) {
   case GL_ZERO:
      b |= RADEON_SRC_BLEND_GL_ZERO;
      break;
   case GL_ONE:
      b |= RADEON_SRC_BLEND_GL_ONE;
      break;
   case GL_DST_COLOR:
      b |= RADEON_SRC_BLEND_GL_DST_COLOR;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      b |= RADEON_SRC_BLEND_GL_ONE_MINUS_DST_COLOR;
      break;
   case GL_SRC_ALPHA:
      b |= RADEON_SRC_BLEND_GL_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      b |= RADEON_SRC_BLEND_GL_ONE_MINUS_SRC_ALPHA;
      break;
   case GL_DST_ALPHA:
      b |= RADEON_SRC_BLEND_GL_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      b |= RADEON_SRC_BLEND_GL_ONE_MINUS_DST_ALPHA;
      break;
   case GL_SRC_ALPHA_SATURATE:
      b |= RADEON_SRC_BLEND_GL_SRC_ALPHA_SATURATE;
      break;
   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      fallback = GL_TRUE;
      break;
   }

   switch ( ctx->Color.BlendDstRGB ) {
   case GL_ZERO:
      b |= RADEON_DST_BLEND_GL_ZERO;
      break;
   case GL_ONE:
      b |= RADEON_DST_BLEND_GL_ONE;
      break;
   case GL_SRC_COLOR:
      b |= RADEON_DST_BLEND_GL_SRC_COLOR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      b |= RADEON_DST_BLEND_GL_ONE_MINUS_SRC_COLOR;
      break;
   case GL_SRC_ALPHA:
      b |= RADEON_DST_BLEND_GL_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      b |= RADEON_DST_BLEND_GL_ONE_MINUS_SRC_ALPHA;
      break;
   case GL_DST_ALPHA:
      b |= RADEON_DST_BLEND_GL_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      b |= RADEON_DST_BLEND_GL_ONE_MINUS_DST_ALPHA;
      break;
   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      fallback = GL_TRUE;
      break;
   }

   FALLBACK( rmesa, RADEON_FALLBACK_BLEND_FUNC, fallback );
   if ( !fallback ) {
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
      rmesa->state.hw.context.rb3d_blendcntl = b;
   }
}

static void radeonBlendFuncSeparate( GLcontext *ctx,
				     GLenum sfactorRGB, GLenum dfactorRGB,
				     GLenum sfactorA, GLenum dfactorA )
{
   radeonBlendFunc( ctx, sfactorRGB, dfactorRGB );
}


/* =============================================================
 * Depth testing
 */

static void radeonDepthFunc( GLcontext *ctx, GLenum func )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
   rmesa->state.hw.context.rb3d_zstencilcntl &= ~RADEON_Z_TEST_MASK;

   switch ( ctx->Depth.Func ) {
   case GL_NEVER:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_Z_TEST_NEVER;
      break;
   case GL_LESS:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_Z_TEST_LESS;
      break;
   case GL_EQUAL:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_Z_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_Z_TEST_LEQUAL;
      break;
   case GL_GREATER:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_Z_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_Z_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_Z_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_Z_TEST_ALWAYS;
      break;
   }
}


static void radeonDepthMask( GLcontext *ctx, GLboolean flag )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );

   if ( ctx->Depth.Mask ) {
      rmesa->state.hw.context.rb3d_zstencilcntl |=  RADEON_Z_WRITE_ENABLE;
   } else {
      rmesa->state.hw.context.rb3d_zstencilcntl &= ~RADEON_Z_WRITE_ENABLE;
   }
}

static void radeonClearDepth( GLcontext *ctx, GLclampd d )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint format = (rmesa->state.hw.context.rb3d_zstencilcntl &
		    RADEON_DEPTH_FORMAT_MASK);

   switch ( format ) {
   case RADEON_DEPTH_FORMAT_16BIT_INT_Z:
      rmesa->state.depth.clear = d * 0x0000ffff;
      break;
   case RADEON_DEPTH_FORMAT_24BIT_INT_Z:
      rmesa->state.depth.clear = d * 0x00ffffff;
      break;
   }
}


/* =============================================================
 * Fog
 */

static void radeonFogfv( GLcontext *ctx, GLenum pname, const GLfloat *param )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLchan c[4];

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
   UNCLAMPED_FLOAT_TO_RGB_CHAN( c, ctx->Fog.Color );
   rmesa->state.hw.context.pp_fog_color =
      radeonPackColor( 4, c[0], c[1], c[2], 0 );
}


/* =============================================================
 * Clipping
 */

static void radeonUpdateScissor( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if ( rmesa->dri.drawable ) {
      __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;

      int x = ctx->Scissor.X;
      int y = dPriv->h - ctx->Scissor.Y - ctx->Scissor.Height;
      int w = ctx->Scissor.X + ctx->Scissor.Width - 1;
      int h = dPriv->h - ctx->Scissor.Y - 1;

      rmesa->state.scissor.rect.x1 = x + dPriv->x;
      rmesa->state.scissor.rect.y1 = y + dPriv->y;
      rmesa->state.scissor.rect.x2 = w + dPriv->x + 1;
      rmesa->state.scissor.rect.y2 = h + dPriv->y + 1;

      if ( ctx->Scissor.Enabled )
	 rmesa->upload_cliprects = 1;
   }
}


static void radeonScissor( GLcontext *ctx,
			   GLint x, GLint y, GLsizei w, GLsizei h )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if ( ctx->Scissor.Enabled )
      RADEON_FIREVERTICES( rmesa );	/* don't pipeline cliprect changes */

   radeonUpdateScissor( ctx );
}


/* =============================================================
 * Culling
 */

static void radeonCullFace( GLcontext *ctx, GLenum unused )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint s = rmesa->state.hw.setup1.se_cntl;

   s |= RADEON_FFACE_SOLID | RADEON_BFACE_SOLID;

   if ( ctx->Polygon.CullFlag ) {
      switch ( ctx->Polygon.CullFaceMode ) {
      case GL_FRONT:
	 s &= ~RADEON_FFACE_SOLID;
	 break;
      case GL_BACK:
	 s &= ~RADEON_BFACE_SOLID;
	 break;
      case GL_FRONT_AND_BACK:
	 s &= ~(RADEON_FFACE_SOLID | RADEON_BFACE_SOLID);
	 break;
      }
   }

   if ( rmesa->state.hw.setup1.se_cntl != s ) {
      RADEON_STATECHANGE(rmesa, RADEON_UPLOAD_SETUP);
      rmesa->state.hw.setup1.se_cntl = s;
   }
}

static void radeonFrontFace( GLcontext *ctx, GLenum mode )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_SETUP );
   rmesa->state.hw.setup1.se_cntl &= ~RADEON_FFACE_CULL_DIR_MASK;

   switch ( mode ) {
   case GL_CW:
      rmesa->state.hw.setup1.se_cntl |= RADEON_FFACE_CULL_CW;
      break;
   case GL_CCW:
      rmesa->state.hw.setup1.se_cntl |= RADEON_FFACE_CULL_CCW;
      break;
   }
}


/* =============================================================
 * Line state
 */
static void radeonLineWidth( GLcontext *ctx, GLfloat widthf )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_LINE | RADEON_UPLOAD_SETUP );

   /* Line width is stored in U6.4 format.
    */
   rmesa->state.hw.line.se_line_width = (GLuint)(widthf * 16.0);
   if ( widthf > 1.0 ) {
      rmesa->state.hw.setup1.se_cntl |=  RADEON_WIDELINE_ENABLE;
   } else {
      rmesa->state.hw.setup1.se_cntl &= ~RADEON_WIDELINE_ENABLE;
   }
}

static void radeonLineStipple( GLcontext *ctx, GLint factor, GLushort pattern )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_LINE );

   rmesa->state.hw.line.re_line_pattern = ((((GLuint)factor & 0xff) << 16) |
					((GLuint)pattern));
}


/* =============================================================
 * Masks
 */
static void radeonColorMask( GLcontext *ctx,
			     GLboolean r, GLboolean g,
			     GLboolean b, GLboolean a )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint mask = radeonPackColor( rmesa->radeonScreen->cpp,
				  ctx->Color.ColorMask[RCOMP],
				  ctx->Color.ColorMask[GCOMP],
				  ctx->Color.ColorMask[BCOMP],
				  ctx->Color.ColorMask[ACOMP] );

   if ( rmesa->state.hw.mask.rb3d_planemask != mask ) {
      RADEON_STATECHANGE( rmesa,  RADEON_UPLOAD_MASKS );
      rmesa->state.hw.mask.rb3d_planemask = mask;
   }
}


/* =============================================================
 * Polygon state
 */

static void radeonPolygonOffset( GLcontext *ctx,
				 GLfloat factor, GLfloat units )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLfloat constant = units * rmesa->state.depth.scale;

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_ZBIAS );
   rmesa->state.hw.zbias.se_zbias_factor   = *(GLuint *)&factor;
   rmesa->state.hw.zbias.se_zbias_constant = *(GLuint *)&constant;
}

static void radeonPolygonStipple( GLcontext *ctx, const GLubyte *mask )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint i;

   /* Must flip pattern upside down.
    */
   for ( i = 0 ; i < 32 ; i++ ) {
      rmesa->state.stipple.mask[31 - i] = ((GLuint *) mask)[i];
   }

   RADEON_FIREVERTICES( rmesa );
   LOCK_HARDWARE( rmesa );

   /* FIXME: Use window x,y offsets into stipple RAM.
    */
   drmRadeonPolygonStipple( rmesa->dri.fd, rmesa->state.stipple.mask );
   UNLOCK_HARDWARE( rmesa );
}


/* =============================================================
 * Rendering attributes
 *
 * We really don't want to recalculate all this every time we bind a
 * texture.  These things shouldn't change all that often, so it makes
 * sense to break them out of the core texture state update routines.
 */

/* Examine lighting and texture state to determine if separate specular
 * should be enabled.
 */
static void radeonUpdateSpecular( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   CARD32 p = rmesa->state.hw.context.pp_cntl;

   if ( ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR &&
        ctx->Light.Enabled) {
      p |=  RADEON_SPECULAR_ENABLE;
   } else {
      p &= ~RADEON_SPECULAR_ENABLE;
   }

   if ( rmesa->state.hw.context.pp_cntl != p ) {
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
      rmesa->state.hw.context.pp_cntl = p;
   }
}


static void radeonLightModelfv( GLcontext *ctx, GLenum pname,
				const GLfloat *param )
{
   if ( pname == GL_LIGHT_MODEL_COLOR_CONTROL ) {
      radeonUpdateSpecular(ctx);
   }
}

static void radeonShadeModel( GLcontext *ctx, GLenum mode )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint s = rmesa->state.hw.setup1.se_cntl;

   s &= ~(RADEON_DIFFUSE_SHADE_MASK |
	  RADEON_ALPHA_SHADE_MASK |
	  RADEON_SPECULAR_SHADE_MASK |
	  RADEON_FOG_SHADE_MASK);

   switch ( mode ) {
   case GL_FLAT:
      s |= (RADEON_DIFFUSE_SHADE_FLAT |
	    RADEON_ALPHA_SHADE_FLAT |
	    RADEON_SPECULAR_SHADE_FLAT |
	    RADEON_FOG_SHADE_FLAT);
      break;
   case GL_SMOOTH:
      s |= (RADEON_DIFFUSE_SHADE_GOURAUD |
	    RADEON_ALPHA_SHADE_GOURAUD |
	    RADEON_SPECULAR_SHADE_GOURAUD |
	    RADEON_FOG_SHADE_GOURAUD);
      break;
   default:
      return;
   }

   if ( rmesa->state.hw.setup1.se_cntl != s ) {
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_SETUP );
      rmesa->state.hw.setup1.se_cntl = s;
   }
}


/* =============================================================
 * Stencil
 */

static void radeonStencilFunc( GLcontext *ctx, GLenum func,
			       GLint ref, GLuint mask )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint refmask = ((ctx->Stencil.Ref << RADEON_STENCIL_REF_SHIFT) |
		     (ctx->Stencil.ValueMask << RADEON_STENCIL_MASK_SHIFT));

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT | RADEON_UPLOAD_MASKS );

   rmesa->state.hw.context.rb3d_zstencilcntl &= ~RADEON_STENCIL_TEST_MASK;
   rmesa->state.hw.mask.rb3d_stencilrefmask &= ~(RADEON_STENCIL_REF_MASK|
					      RADEON_STENCIL_VALUE_MASK);

   switch ( ctx->Stencil.Function ) {
   case GL_NEVER:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_TEST_NEVER;
      break;
   case GL_LESS:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_TEST_LESS;
      break;
   case GL_EQUAL:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_TEST_LEQUAL;
      break;
   case GL_GREATER:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_TEST_ALWAYS;
      break;
   }

   rmesa->state.hw.mask.rb3d_stencilrefmask |= refmask;
}

static void radeonStencilMask( GLcontext *ctx, GLuint mask )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_MASKS );
   rmesa->state.hw.mask.rb3d_stencilrefmask &= ~RADEON_STENCIL_WRITE_MASK;

   rmesa->state.hw.mask.rb3d_stencilrefmask |=
      (ctx->Stencil.WriteMask << RADEON_STENCIL_WRITEMASK_SHIFT);
}

static void radeonStencilOp( GLcontext *ctx, GLenum fail,
			     GLenum zfail, GLenum zpass )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
   rmesa->state.hw.context.rb3d_zstencilcntl &= ~(RADEON_STENCIL_FAIL_MASK |
					       RADEON_STENCIL_ZFAIL_MASK |
					       RADEON_STENCIL_ZPASS_MASK);

   switch ( ctx->Stencil.FailFunc ) {
   case GL_KEEP:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_FAIL_KEEP;
      break;
   case GL_ZERO:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_FAIL_ZERO;
      break;
   case GL_REPLACE:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_FAIL_REPLACE;
      break;
   case GL_INCR:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_FAIL_INC;
      break;
   case GL_DECR:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_FAIL_DEC;
      break;
   case GL_INVERT:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_FAIL_INVERT;
      break;
   }

   switch ( ctx->Stencil.ZFailFunc ) {
   case GL_KEEP:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZFAIL_KEEP;
      break;
   case GL_ZERO:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZFAIL_ZERO;
      break;
   case GL_REPLACE:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZFAIL_REPLACE;
      break;
   case GL_INCR:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZFAIL_INC;
      break;
   case GL_DECR:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZFAIL_DEC;
      break;
   case GL_INVERT:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZFAIL_INVERT;
      break;
   }

   switch ( ctx->Stencil.ZPassFunc ) {
   case GL_KEEP:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZPASS_KEEP;
      break;
   case GL_ZERO:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZPASS_ZERO;
      break;
   case GL_REPLACE:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZPASS_REPLACE;
      break;
   case GL_INCR:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZPASS_INC;
      break;
   case GL_DECR:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZPASS_DEC;
      break;
   case GL_INVERT:
      rmesa->state.hw.context.rb3d_zstencilcntl |= RADEON_STENCIL_ZPASS_INVERT;
      break;
   }
}

static void radeonClearStencil( GLcontext *ctx, GLint s )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   rmesa->state.stencil.clear = ((GLuint) ctx->Stencil.Clear |
				 (0xff << RADEON_STENCIL_MASK_SHIFT) |
				 (ctx->Stencil.WriteMask << RADEON_STENCIL_WRITEMASK_SHIFT));
}


/* =============================================================
 * Window position and viewport transformation
 */

/*
 * To correctly position primitives:
 */
#define SUBPIXEL_X 0.125

void radeonUpdateWindow( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
   GLfloat xoffset = (GLfloat)dPriv->x;
   GLfloat yoffset = (GLfloat)dPriv->y + dPriv->h;
   const GLfloat *v = ctx->Viewport._WindowMap.m;

   GLfloat sx = v[MAT_SX];
   GLfloat tx = v[MAT_TX] + xoffset + SUBPIXEL_X;
   GLfloat sy = - v[MAT_SY];
   GLfloat ty = (- v[MAT_TY]) + yoffset;
   GLfloat sz = v[MAT_SZ] * rmesa->state.depth.scale;
   GLfloat tz = v[MAT_TZ] * rmesa->state.depth.scale;

/*     fprintf(stderr, "radeonUpdateWindow %d,%d %dx%d\n", */
/*  	   dPriv->x, dPriv->y, dPriv->w, dPriv->h); */

   RADEON_STATECHANGE(rmesa, RADEON_UPLOAD_VIEWPORT);
   rmesa->state.hw.viewport.se_vport_xscale  = *(GLuint *)&sx;
   rmesa->state.hw.viewport.se_vport_xoffset = *(GLuint *)&tx;
   rmesa->state.hw.viewport.se_vport_yscale  = *(GLuint *)&sy;
   rmesa->state.hw.viewport.se_vport_yoffset = *(GLuint *)&ty;
   rmesa->state.hw.viewport.se_vport_zscale  = *(GLuint *)&sz;
   rmesa->state.hw.viewport.se_vport_zoffset = *(GLuint *)&tz;
}



static void radeonViewport( GLcontext *ctx, GLint x, GLint y,
			    GLsizei width, GLsizei height )
{
   /* Don't pipeline viewport changes, conflict with window offset
    * setting below.  Could apply deltas to rescue pipelined viewport
    * values, or keep the originals hanging around.
    */
   RADEON_FIREVERTICES( RADEON_CONTEXT(ctx) );
   radeonUpdateWindow( ctx );
}

static void radeonDepthRange( GLcontext *ctx, GLclampd nearval,
			      GLclampd farval )
{
   radeonUpdateWindow( ctx );
}

void radeonUpdateViewportOffset( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
   GLfloat xoffset = (GLfloat)dPriv->x;
   GLfloat yoffset = (GLfloat)dPriv->y + dPriv->h;
   const GLfloat *v = ctx->Viewport._WindowMap.m;

   GLfloat tx = v[MAT_TX] + xoffset;
   GLfloat ty = (- v[MAT_TY]) + yoffset;

   if ( rmesa->state.hw.viewport.se_vport_xoffset != tx ||
	rmesa->state.hw.viewport.se_vport_yoffset != ty )
   {
      rmesa->state.hw.viewport.se_vport_xoffset = *(GLuint *)&tx;
      rmesa->state.hw.viewport.se_vport_yoffset = *(GLuint *)&ty;

      if (rmesa->store.statenr) {
	 int i;
	 rmesa->store.state[0].dirty |= RADEON_UPLOAD_VIEWPORT;
	 /* Note: assume vport x/yoffset are constant over the buffer:
	  */
	 for (i = 0 ; i < rmesa->store.statenr ; i++) {
	    rmesa->store.state[i].viewport.se_vport_xoffset = *(GLuint *)&tx;
	    rmesa->store.state[i].viewport.se_vport_yoffset = *(GLuint *)&ty;
	 }
      } else {
	 rmesa->state.hw.dirty |= RADEON_UPLOAD_VIEWPORT;
      }

      /* update polygon stipple x/y screen offset */
      {
         GLuint stx, sty;
         GLuint m = rmesa->state.hw.misc.re_misc;

         m &= ~(RADEON_STIPPLE_X_OFFSET_MASK |
                RADEON_STIPPLE_Y_OFFSET_MASK);

         /* add magic offsets, then invert */
         stx = 31 - ((rmesa->dri.drawable->x - 1) & RADEON_STIPPLE_COORD_MASK);
         sty = 31 - ((rmesa->dri.drawable->y + rmesa->dri.drawable->h - 1)
                     & RADEON_STIPPLE_COORD_MASK);

         m |= ((stx << RADEON_STIPPLE_X_OFFSET_SHIFT) |
               (sty << RADEON_STIPPLE_Y_OFFSET_SHIFT));

         if ( rmesa->state.hw.misc.re_misc != m ) {
            rmesa->state.hw.misc.re_misc = m;
            RADEON_STATECHANGE(rmesa, RADEON_UPLOAD_MISC);
         }
      }
   }

   radeonUpdateScissor( ctx );
}



/* =============================================================
 * Miscellaneous
 */

static void radeonClearColor( GLcontext *ctx, const GLchan c[4] )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   rmesa->state.color.clear = radeonPackColor( rmesa->radeonScreen->cpp,
					       c[0], c[1], c[2], c[3] );
}


static void radeonRenderMode( GLcontext *ctx, GLenum mode )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   FALLBACK( rmesa, RADEON_FALLBACK_RENDER_MODE, (mode != GL_RENDER) );
}


static GLuint radeon_rop_tab[] = {
   RADEON_ROP_CLEAR,
   RADEON_ROP_AND,
   RADEON_ROP_AND_REVERSE,
   RADEON_ROP_COPY,
   RADEON_ROP_AND_INVERTED,
   RADEON_ROP_NOOP,
   RADEON_ROP_XOR,
   RADEON_ROP_OR,
   RADEON_ROP_NOR,
   RADEON_ROP_EQUIV,
   RADEON_ROP_INVERT,
   RADEON_ROP_OR_REVERSE,
   RADEON_ROP_COPY_INVERTED,
   RADEON_ROP_OR_INVERTED,
   RADEON_ROP_NAND,
   RADEON_ROP_SET,
};

static void radeonLogicOpCode( GLcontext *ctx, GLenum opcode )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint rop = (GLuint)opcode - GL_CLEAR;

   ASSERT( rop < 16 );

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_MASKS );
   rmesa->state.hw.mask.rb3d_ropcntl = radeon_rop_tab[rop];
}


void radeonSetCliprects( radeonContextPtr rmesa, GLenum mode )
{
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;

   switch ( mode ) {
   case GL_FRONT_LEFT:
      rmesa->numClipRects = dPriv->numClipRects;
      rmesa->pClipRects = (XF86DRIClipRectPtr)dPriv->pClipRects;
      break;
   case GL_BACK_LEFT:
      if ( dPriv->numBackClipRects == 0 ) {
	 rmesa->numClipRects = dPriv->numClipRects;
	 rmesa->pClipRects = (XF86DRIClipRectPtr)dPriv->pClipRects;
      }
      else {
	 rmesa->numClipRects = dPriv->numBackClipRects;
	 rmesa->pClipRects = (XF86DRIClipRectPtr)dPriv->pBackClipRects;
      }
      break;
   default:
      return;
   }

   rmesa->upload_cliprects = 1;
}


static void radeonSetDrawBuffer( GLcontext *ctx, GLenum mode )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   RADEON_FIREVERTICES(rmesa);	/* don't pipeline cliprect changes */

   switch ( mode ) {
   case GL_FRONT_LEFT:
      FALLBACK( rmesa, RADEON_FALLBACK_DRAW_BUFFER, GL_FALSE );
      rmesa->state.color.drawOffset = rmesa->radeonScreen->frontOffset;
      rmesa->state.color.drawPitch  = rmesa->radeonScreen->frontPitch;
      radeonSetCliprects( rmesa, GL_FRONT_LEFT );
      break;
   case GL_BACK_LEFT:
      FALLBACK( rmesa, RADEON_FALLBACK_DRAW_BUFFER, GL_FALSE );
      rmesa->state.color.drawOffset = rmesa->radeonScreen->backOffset;
      rmesa->state.color.drawPitch  = rmesa->radeonScreen->backPitch;
      radeonSetCliprects( rmesa, GL_BACK_LEFT );
      break;
   default:
      FALLBACK( rmesa, RADEON_FALLBACK_DRAW_BUFFER, GL_TRUE );
      return;
   }

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
   rmesa->state.hw.context.rb3d_coloroffset = (rmesa->state.color.drawOffset &
					    RADEON_COLOROFFSET_MASK);
   rmesa->state.hw.context.rb3d_colorpitch = rmesa->state.color.drawPitch;
}


/* =============================================================
 * State enable/disable
 */

static void radeonEnable( GLcontext *ctx, GLenum cap, GLboolean state )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API )
      fprintf( stderr, __FUNCTION__"( %s = %s )\n",
	       _mesa_lookup_enum_by_nr( cap ),
	       state ? "GL_TRUE" : "GL_FALSE" );

   switch ( cap ) {
      /* Fast track this one...
       */
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
      break;

   case GL_ALPHA_TEST:
      RADEON_STATECHANGE(rmesa, RADEON_UPLOAD_CONTEXT);
      if (state) {
	 rmesa->state.hw.context.pp_cntl |= RADEON_ALPHA_TEST_ENABLE;
      } else {
	 rmesa->state.hw.context.pp_cntl &= ~RADEON_ALPHA_TEST_ENABLE;
      }
      break;

   case GL_BLEND:
      RADEON_STATECHANGE(rmesa, RADEON_UPLOAD_CONTEXT);
      if (state) {
	 rmesa->state.hw.context.rb3d_cntl |=  RADEON_ALPHA_BLEND_ENABLE;
      } else {
	 rmesa->state.hw.context.rb3d_cntl &= ~RADEON_ALPHA_BLEND_ENABLE;
      }
      break;

   case GL_CULL_FACE:
      radeonCullFace( ctx, 0 );
      break;

   case GL_DEPTH_TEST:
      RADEON_STATECHANGE(rmesa, RADEON_UPLOAD_CONTEXT);
      if ( state ) {
	 rmesa->state.hw.context.rb3d_cntl |=  RADEON_Z_ENABLE;
      } else {
	 rmesa->state.hw.context.rb3d_cntl &= ~RADEON_Z_ENABLE;
      }
      break;

   case GL_DITHER:
      RADEON_STATECHANGE(rmesa, RADEON_UPLOAD_CONTEXT);
      if ( state ) {
	 rmesa->state.hw.context.rb3d_cntl |=  RADEON_DITHER_ENABLE;
      } else {
	 rmesa->state.hw.context.rb3d_cntl &= ~RADEON_DITHER_ENABLE;
      }
      break;

   case GL_FOG:
      RADEON_STATECHANGE(rmesa, RADEON_UPLOAD_CONTEXT);
      if ( state ) {
	 rmesa->state.hw.context.pp_cntl |=  RADEON_FOG_ENABLE;
      } else {
	 rmesa->state.hw.context.pp_cntl &= ~RADEON_FOG_ENABLE;
      }
      break;

   case GL_LIGHTING:
      radeonUpdateSpecular(ctx);
      break;

   case GL_LINE_SMOOTH:
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
      if ( state ) {
	 rmesa->state.hw.context.pp_cntl |=  RADEON_ANTI_ALIAS_LINE;
      } else {
	 rmesa->state.hw.context.pp_cntl &= ~RADEON_ANTI_ALIAS_LINE;
      }
      break;

   case GL_LINE_STIPPLE:
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
      if ( state ) {
	 rmesa->state.hw.context.pp_cntl |=  RADEON_PATTERN_ENABLE;
      } else {
	 rmesa->state.hw.context.pp_cntl &= ~RADEON_PATTERN_ENABLE;
      }
      break;

   case GL_COLOR_LOGIC_OP:
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
      if ( state ) {
	 rmesa->state.hw.context.rb3d_cntl |=  RADEON_ROP_ENABLE;
      } else {
	 rmesa->state.hw.context.rb3d_cntl &= ~RADEON_ROP_ENABLE;
      }
      break;

   case GL_POLYGON_OFFSET_POINT:
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_SETUP );
      if ( state ) {
	 rmesa->state.hw.setup1.se_cntl |=  RADEON_ZBIAS_ENABLE_POINT;
      } else {
	 rmesa->state.hw.setup1.se_cntl &= ~RADEON_ZBIAS_ENABLE_POINT;
      }
      break;

   case GL_POLYGON_OFFSET_LINE:
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_SETUP );
      if ( state ) {
	 rmesa->state.hw.setup1.se_cntl |=  RADEON_ZBIAS_ENABLE_LINE;
      } else {
	 rmesa->state.hw.setup1.se_cntl &= ~RADEON_ZBIAS_ENABLE_LINE;
      }
      break;

   case GL_POLYGON_OFFSET_FILL:
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_SETUP );
      if ( state ) {
	 rmesa->state.hw.setup1.se_cntl |=  RADEON_ZBIAS_ENABLE_TRI;
      } else {
	 rmesa->state.hw.setup1.se_cntl &= ~RADEON_ZBIAS_ENABLE_TRI;
      }
      break;

   case GL_POLYGON_SMOOTH:
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
      if ( state ) {
	 rmesa->state.hw.context.pp_cntl |=  RADEON_ANTI_ALIAS_POLY;
      } else {
	 rmesa->state.hw.context.pp_cntl &= ~RADEON_ANTI_ALIAS_POLY;
      }
      break;

   case GL_POLYGON_STIPPLE:
      RADEON_STATECHANGE(rmesa, RADEON_UPLOAD_CONTEXT);
      if ( state ) {
	 rmesa->state.hw.context.pp_cntl |=  RADEON_STIPPLE_ENABLE;
      } else {
	 rmesa->state.hw.context.pp_cntl &= ~RADEON_STIPPLE_ENABLE;
      }
      break;

   case GL_SCISSOR_TEST:
      RADEON_FIREVERTICES( rmesa );
      rmesa->state.scissor.enabled = state;
      rmesa->upload_cliprects = 1;
      break;

   case GL_STENCIL_TEST:
      if ( rmesa->state.stencil.hwBuffer ) {
	 RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
	 if ( state ) {
	    rmesa->state.hw.context.rb3d_cntl |=  RADEON_STENCIL_ENABLE;
	 } else {
	    rmesa->state.hw.context.rb3d_cntl &= ~RADEON_STENCIL_ENABLE;
	 }
      } else {
	 FALLBACK( rmesa, RADEON_FALLBACK_STENCIL, state );
      }
      break;

   default:
      return;
   }
}


/* =============================================================
 * State initialization, management
 */

void radeonPrintDirty( const char *msg, GLuint state )
{
   fprintf( stderr,
	    "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s\n",
	    msg,
	    state,
	    (state & RADEON_UPLOAD_CONTEXT)     ? "context, " : "",
	    (state & RADEON_UPLOAD_LINE)        ? "line, " : "",
	    (state & RADEON_UPLOAD_BUMPMAP)     ? "bumpmap, " : "",
	    (state & RADEON_UPLOAD_MASKS)       ? "masks, " : "",
	    (state & RADEON_UPLOAD_VIEWPORT)    ? "viewport, " : "",
	    (state & RADEON_UPLOAD_SETUP)       ? "setup, " : "",
	    (state & RADEON_UPLOAD_TCL)         ? "tcl, " : "",
	    (state & RADEON_UPLOAD_MISC)        ? "misc, " : "",
	    (state & RADEON_UPLOAD_TEX0)        ? "tex0, " : "",
	    (state & RADEON_UPLOAD_TEX1)        ? "tex1, " : "",
	    (state & RADEON_UPLOAD_TEX2)        ? "tex2, " : "");
}





static void radeonInvalidateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   RADEON_CONTEXT(ctx)->NewGLState |= new_state;
}




/* Initialize the context's hardware state.
 */
void radeonInitState( radeonContextPtr rmesa )
{
   GLcontext *ctx = rmesa->glCtx;
   GLuint color_fmt, depth_fmt;

   switch ( rmesa->radeonScreen->cpp ) {
   case 2:
      color_fmt = RADEON_COLOR_FORMAT_RGB565;
      break;
   case 4:
      color_fmt = RADEON_COLOR_FORMAT_ARGB8888;
      break;
   default:
      fprintf( stderr, "Error: Unsupported pixel depth... exiting\n" );
      exit( -1 );
   }

   rmesa->state.color.clear = 0x00000000;

   switch ( ctx->Visual.depthBits ) {
   case 16:
      rmesa->state.depth.clear = 0x0000ffff;
      rmesa->state.depth.scale = 1.0 / (GLfloat)0xffff;
      depth_fmt = RADEON_DEPTH_FORMAT_16BIT_INT_Z;
      rmesa->state.stencil.clear = 0x00000000;
      break;
   case 24:
      rmesa->state.depth.clear = 0x00ffffff;
      rmesa->state.depth.scale = 1.0 / (GLfloat)0xffffff;
      depth_fmt = RADEON_DEPTH_FORMAT_24BIT_INT_Z;
      rmesa->state.stencil.clear = 0xffff0000;
      break;
   default:
      fprintf( stderr, "Error: Unsupported depth %d... exiting\n",
	       ctx->Visual.depthBits );
      exit( -1 );
   }

   /* Only have hw stencil when depth buffer is 24 bits deep */
   rmesa->state.stencil.hwBuffer = ( ctx->Visual.stencilBits > 0 &&
				     ctx->Visual.depthBits == 24 );

   rmesa->RenderIndex = ~0;
   rmesa->Fallback = 0;
   rmesa->render_primitive = GL_TRIANGLES;
   rmesa->hw_primitive = RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST;

   if ( ctx->Visual.doubleBufferMode ) {
      rmesa->state.color.drawOffset = rmesa->radeonScreen->backOffset;
      rmesa->state.color.drawPitch  = rmesa->radeonScreen->backPitch;
   } else {
      rmesa->state.color.drawOffset = rmesa->radeonScreen->frontOffset;
      rmesa->state.color.drawPitch  = rmesa->radeonScreen->frontPitch;
   }
   rmesa->state.pixel.readOffset = rmesa->state.color.drawOffset;
   rmesa->state.pixel.readPitch  = rmesa->state.color.drawPitch;

   /* Harware state:
    */
   rmesa->state.hw.context.pp_misc = (RADEON_ALPHA_TEST_PASS |
				   RADEON_CHROMA_FUNC_FAIL |
				   RADEON_CHROMA_KEY_NEAREST |
				   RADEON_SHADOW_FUNC_EQUAL |
				   RADEON_SHADOW_PASS_1 |
				   RADEON_RIGHT_HAND_CUBE_OGL);

   rmesa->state.hw.context.pp_fog_color = ((0x00000000 & RADEON_FOG_COLOR_MASK) |
					RADEON_FOG_VERTEX |
					RADEON_FOG_USE_DEPTH);

   rmesa->state.hw.context.re_solid_color = 0x00000000;

   rmesa->state.hw.context.rb3d_blendcntl = (RADEON_COMB_FCN_ADD_CLAMP |
					  RADEON_SRC_BLEND_GL_ONE |
					  RADEON_DST_BLEND_GL_ZERO );

   rmesa->state.hw.context.rb3d_depthoffset = rmesa->radeonScreen->depthOffset;

   rmesa->state.hw.context.rb3d_depthpitch = ((rmesa->radeonScreen->depthPitch &
					    RADEON_DEPTHPITCH_MASK) |
					   RADEON_DEPTH_ENDIAN_NO_SWAP);

   rmesa->state.hw.context.rb3d_zstencilcntl = (depth_fmt |
					     RADEON_Z_TEST_LESS |
					     RADEON_STENCIL_TEST_ALWAYS |
					     RADEON_STENCIL_FAIL_KEEP |
					     RADEON_STENCIL_ZPASS_KEEP |
					     RADEON_STENCIL_ZFAIL_KEEP |
					     RADEON_Z_WRITE_ENABLE);

   rmesa->state.hw.context.pp_cntl = (RADEON_SCISSOR_ENABLE |
				   RADEON_ANTI_ALIAS_NONE);

   rmesa->state.hw.context.rb3d_cntl = (RADEON_PLANE_MASK_ENABLE |
				     color_fmt |
				     RADEON_ZBLOCK16);

   rmesa->state.hw.context.rb3d_coloroffset = (rmesa->state.color.drawOffset &
					    RADEON_COLOROFFSET_MASK);

   rmesa->state.hw.context.re_width_height = ((0x7ff << RADEON_RE_WIDTH_SHIFT) |
					   (0x7ff << RADEON_RE_HEIGHT_SHIFT));

   rmesa->state.hw.context.rb3d_colorpitch = ((rmesa->state.color.drawPitch &
					    RADEON_COLORPITCH_MASK) |
					   RADEON_COLOR_ENDIAN_NO_SWAP);

   rmesa->state.hw.setup1.se_cntl = (RADEON_FFACE_CULL_CCW |
				 RADEON_BFACE_SOLID |
				 RADEON_FFACE_SOLID |
				 RADEON_FLAT_SHADE_VTX_LAST |
				 RADEON_DIFFUSE_SHADE_GOURAUD |
				 RADEON_ALPHA_SHADE_GOURAUD |
				 RADEON_SPECULAR_SHADE_GOURAUD |
				 RADEON_FOG_SHADE_GOURAUD |
				 RADEON_VPORT_XY_XFORM_ENABLE |
				 RADEON_VPORT_Z_XFORM_ENABLE |
				 RADEON_VTX_PIX_CENTER_OGL |
				 RADEON_ROUND_MODE_TRUNC |
				 RADEON_ROUND_PREC_8TH_PIX);

   rmesa->state.hw.vertex.se_coord_fmt = (
#if 1
      RADEON_VTX_XY_PRE_MULT_1_OVER_W0 |
      RADEON_VTX_Z_PRE_MULT_1_OVER_W0 |
#else
      RADEON_VTX_W0_IS_NOT_1_OVER_W0 |
#endif
      RADEON_TEX1_W_ROUTING_USE_Q1);

   rmesa->state.hw.setup2.se_cntl_status = (RADEON_VC_NO_SWAP |
					    RADEON_TCL_BYPASS);

   rmesa->state.hw.line.re_line_pattern = ((0x0000 & RADEON_LINE_PATTERN_MASK) |
					(0 << RADEON_LINE_REPEAT_COUNT_SHIFT) |
					(0 << RADEON_LINE_PATTERN_START_SHIFT) |
					RADEON_LINE_PATTERN_LITTLE_BIT_ORDER);

   rmesa->state.hw.line.re_line_state = ((0 << RADEON_LINE_CURRENT_PTR_SHIFT) |
				      (1 << RADEON_LINE_CURRENT_COUNT_SHIFT));

   rmesa->state.hw.line.se_line_width = (1 << 4);

   rmesa->state.hw.bumpmap.pp_lum_matrix = 0x00000000;
   rmesa->state.hw.bumpmap.pp_rot_matrix_0 = 0x00000000;
   rmesa->state.hw.bumpmap.pp_rot_matrix_1 = 0x00000000;

   rmesa->state.hw.mask.rb3d_stencilrefmask = ((0x00 << RADEON_STENCIL_REF_SHIFT) |
					       (0xff << RADEON_STENCIL_MASK_SHIFT) |
					       (0xff << RADEON_STENCIL_WRITEMASK_SHIFT));

   rmesa->state.hw.mask.rb3d_ropcntl = RADEON_ROP_COPY;
   rmesa->state.hw.mask.rb3d_planemask = 0xffffffff;

   rmesa->state.hw.viewport.se_vport_xscale  = 0x00000000;
   rmesa->state.hw.viewport.se_vport_xoffset = 0x00000000;
   rmesa->state.hw.viewport.se_vport_yscale  = 0x00000000;
   rmesa->state.hw.viewport.se_vport_yoffset = 0x00000000;
   rmesa->state.hw.viewport.se_vport_zscale  = 0x00000000;
   rmesa->state.hw.viewport.se_vport_zoffset = 0x00000000;

   rmesa->state.hw.misc.re_misc = ((0 << RADEON_STIPPLE_X_OFFSET_SHIFT) |
				   (0 << RADEON_STIPPLE_Y_OFFSET_SHIFT) |
				   RADEON_STIPPLE_BIG_BIT_ORDER);

   rmesa->state.hw.dirty = RADEON_UPLOAD_CONTEXT_ALL;
}

/* Initialize the driver's state functions.
 */
void radeonInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState		= radeonInvalidateState;

   ctx->Driver.SetDrawBuffer		= radeonSetDrawBuffer;

   ctx->Driver.AlphaFunc		= radeonAlphaFunc;
   ctx->Driver.BlendEquation		= radeonBlendEquation;
   ctx->Driver.BlendFunc		= radeonBlendFunc;
   ctx->Driver.BlendFuncSeparate	= radeonBlendFuncSeparate;
   ctx->Driver.ClearColor		= radeonClearColor;
   ctx->Driver.ClearDepth		= radeonClearDepth;
   ctx->Driver.ClearIndex		= NULL;
   ctx->Driver.ClearStencil		= radeonClearStencil;
   ctx->Driver.ColorMask		= radeonColorMask;
   ctx->Driver.CullFace			= radeonCullFace;
   ctx->Driver.DepthFunc		= radeonDepthFunc;
   ctx->Driver.DepthMask		= radeonDepthMask;
   ctx->Driver.DepthRange		= radeonDepthRange;
   ctx->Driver.Enable			= radeonEnable;
   ctx->Driver.Fogfv			= radeonFogfv;
   ctx->Driver.FrontFace		= radeonFrontFace;
   ctx->Driver.Hint			= NULL;
   ctx->Driver.IndexMask		= NULL;
   ctx->Driver.LightModelfv		= radeonLightModelfv;
   ctx->Driver.Lightfv			= NULL;
   ctx->Driver.LineStipple              = radeonLineStipple;
   ctx->Driver.LineWidth                = radeonLineWidth;
   ctx->Driver.LogicOpcode		= radeonLogicOpCode;
   ctx->Driver.PolygonMode		= NULL;
   ctx->Driver.PolygonOffset		= radeonPolygonOffset;
   ctx->Driver.PolygonStipple		= radeonPolygonStipple;
   ctx->Driver.RenderMode		= radeonRenderMode;
   ctx->Driver.Scissor			= radeonScissor;
   ctx->Driver.ShadeModel		= radeonShadeModel;
   ctx->Driver.StencilFunc		= radeonStencilFunc;
   ctx->Driver.StencilMask		= radeonStencilMask;
   ctx->Driver.StencilOp		= radeonStencilOp;
   ctx->Driver.Viewport			= radeonViewport;

   /* Pixel path fallbacks
    */
   ctx->Driver.Accum                    = _swrast_Accum;
   ctx->Driver.Bitmap                   = _swrast_Bitmap;
   ctx->Driver.CopyPixels               = _swrast_CopyPixels;
   ctx->Driver.DrawPixels               = _swrast_DrawPixels;
   ctx->Driver.ReadPixels               = _swrast_ReadPixels;
   ctx->Driver.ResizeBuffers            = _swrast_alloc_buffers;

   /* Swrast hooks for imaging extensions:
    */
   ctx->Driver.CopyColorTable		= _swrast_CopyColorTable;
   ctx->Driver.CopyColorSubTable	= _swrast_CopyColorSubTable;
   ctx->Driver.CopyConvolutionFilter1D	= _swrast_CopyConvolutionFilter1D;
   ctx->Driver.CopyConvolutionFilter2D	= _swrast_CopyConvolutionFilter2D;
}
