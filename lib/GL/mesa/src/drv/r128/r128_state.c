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
#include "r128_state.h"
#include "r128_reg.h"
#include "r128_cce.h"
#include "r128_tris.h"
#include "r128_vb.h"
#include "r128_tex.h"

#include "mmath.h"
#include "pb.h"

#define INTERESTED (~(NEW_MODELVIEW      | \
		      NEW_PROJECTION     | \
		      NEW_TEXTURE_MATRIX | \
		      NEW_USER_CLIP      | \
		      NEW_CLIENT_STATE   | \
		      NEW_TEXTURE_ENABLE))

static void r128DDUpdateState(GLcontext *ctx)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    if (R128_DEBUG_FLAGS & DEBUG_VERBOSE_API) {
	fprintf(stderr, "r128DDUpdateState(%p)\n", ctx);
    }

    /* Need to do this here to detect texture fallbacks before
     * setting triangle functions.
     */
    if (r128ctx->dirty & R128_UPDATE_TEXSTATE)
	r128UpdateHWState(r128ctx);

    if (ctx->NewState & INTERESTED) {
	r128ChooseRenderState(ctx);
	r128ChooseRasterSetupFunc(ctx);
    }

    if (!r128ctx->Fallback) {
	ctx->IndirectTriangles   &= ~DD_SW_RASTERIZE;
	ctx->IndirectTriangles   |= r128ctx->IndirectTriangles;

	ctx->Driver.PointsFunc    = r128ctx->PointsFunc;
	ctx->Driver.LineFunc      = r128ctx->LineFunc;
	ctx->Driver.TriangleFunc  = r128ctx->TriangleFunc;
	ctx->Driver.QuadFunc      = r128ctx->QuadFunc;
	ctx->Driver.RectFunc      = NULL;
    }
}

static void r128DDUpdateHWState(GLcontext *ctx)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    /* FIXME: state is being updated too often */
    if (r128ctx->dirty)
	r128UpdateHWState(r128ctx);
}


/* This is called when mesa switches between rendering triangle
 * primitives (such as GL_POLYGON, GL_QUADS, GL_TRIANGLE_STRIP, etc),
 * and lines, points and bitmaps.
 *
 * As the r128 uses triangles to render lines and points, it is
 * necessary to turn off hardware culling when rendering these
 * primitives.
 */
static void r128DDReducedPrimitiveChange(GLcontext *ctx, GLenum prim)
{
   r128ContextPtr r128ctx = R128_CONTEXT(ctx);
   CARD32         f       = r128ctx->regs.pm4_vc_fpu_setup;

   f |= R128_BACKFACE_SOLID | R128_FRONTFACE_SOLID;

   /* Should really use an intermediate value to hold this rather
    * than recalculating all the time.
    */
   if (prim == GL_POLYGON && ctx->Polygon.CullFlag) {
      switch (ctx->Polygon.CullFaceMode) {
      case GL_FRONT:          f &= ~R128_FRONTFACE_SOLID;   break;
      case GL_BACK:           f &= ~R128_BACKFACE_SOLID;    break;
      case GL_FRONT_AND_BACK: f &= ~(R128_BACKFACE_SOLID |
				     R128_FRONTFACE_SOLID); break;
      default:
      }
   }

    if (r128ctx->regs.pm4_vc_fpu_setup != f) {
	r128FlushVertices(r128ctx);
	r128ctx->regs.pm4_vc_fpu_setup = f;

	/* FIXME: Load into hardware now??? */
	r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	r128ctx->dirty_context |= R128_CTX_SETUPSTATE;
    }
}

static void r128DDClearColor(GLcontext *ctx,
                             GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    r128ctx->ClearColor = r128PackColor(r128ctx->r128Screen->depth,
					r, g, b, a);
}

static void r128DDColor(GLcontext *ctx,
			GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    r128ctx->Color = r128PackColor(r128ctx->r128Screen->depth, r, g, b, a);
}

static GLboolean r128DDSetDrawBuffer(GLcontext *ctx, GLenum mode)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);
    int            x       = r128ctx->driDrawable->x;
    int            y       = r128ctx->driDrawable->y;
    int            found;

    r128ctx->Fallback &= ~R128_FALLBACK_DRAW_BUFFER;

    switch (mode) {
    case GL_FRONT_LEFT:
	r128ctx->drawX = r128ctx->r128Screen->fbX;
	r128ctx->drawY = r128ctx->r128Screen->fbY;
	found = GL_TRUE;
	break;
    case GL_BACK_LEFT:
	r128ctx->drawX = r128ctx->r128Screen->backX;
	r128ctx->drawY = r128ctx->r128Screen->backY;
	found = GL_TRUE;
	break;
    default:
	r128ctx->Fallback |= R128_FALLBACK_DRAW_BUFFER;
	found = GL_FALSE;
	break;
    }

    x += r128ctx->drawX;
    y += r128ctx->drawY;

    r128FlushVertices(r128ctx);

    r128ctx->regs.window_xy_offset = ((y << R128_WINDOW_Y_SHIFT) |
				      (x << R128_WINDOW_X_SHIFT));

    /* Recalculate the Z buffer offset since we might be drawing to the
       back buffer and window_xy_offset affects both color buffer and
       depth drawing */
    r128ctx->regs.z_offset_c        = ((r128ctx->r128Screen->depthX -
					r128ctx->drawX) *
				       (r128ctx->r128Screen->bpp/8) +
				       (r128ctx->r128Screen->depthY -
					r128ctx->drawY) *
				       r128ctx->r128Screen->fbStride);

    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
    r128ctx->dirty_context |= R128_CTX_WIN_Z_POS;
    return found;
}

static void r128DDSetReadBuffer(GLcontext *ctx,
				GLframebuffer *colorBuffer,
				GLenum mode)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    r128ctx->Fallback &= ~R128_FALLBACK_READ_BUFFER;

    switch (mode) {
    case GL_FRONT_LEFT:
	r128ctx->readX = r128ctx->r128Screen->fbX;
	r128ctx->readY = r128ctx->r128Screen->fbY;
	break;
    case GL_BACK_LEFT:
	r128ctx->readX = r128ctx->r128Screen->backX;
	r128ctx->readY = r128ctx->r128Screen->backY;
	break;
    default:
	r128ctx->Fallback |= R128_FALLBACK_READ_BUFFER;
	break;
    }
}

static GLboolean r128DDColorMask(GLcontext *ctx,
				 GLboolean r, GLboolean g,
				 GLboolean b, GLboolean a)
{
    r128ContextPtr r128ctx  = R128_CONTEXT(ctx);

    GLuint mask = r128PackColor(r128ctx->r128Screen->pixel_code,
				ctx->Color.ColorMask[RCOMP],
				ctx->Color.ColorMask[GCOMP],
				ctx->Color.ColorMask[BCOMP],
				ctx->Color.ColorMask[ACOMP]);

    if (r128ctx->regs.plane_3d_mask_c != mask) {
	r128FlushVertices(r128ctx);
	r128ctx->regs.plane_3d_mask_c = mask;

	/* FIXME: Load into hardware now??? */
	r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	r128ctx->dirty_context |= R128_CTX_MISC;
    }

    return GL_TRUE;
}


static void r128DDDither(GLcontext *ctx, GLboolean enable)
{
}

static void r128DDAlphaFunc(GLcontext *ctx, GLenum func, GLclampf ref)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);
    CARD32         a       = r128ctx->regs.misc_3d_state_cntl_reg;

    a &= ~(R128_ALPHA_TEST_MASK | R128_REF_ALPHA_MASK);
    a |= ctx->Color.AlphaRef & R128_REF_ALPHA_MASK;

    switch (func) {
    case GL_NEVER:    a |= R128_ALPHA_TEST_NEVER;        break;
    case GL_LESS:     a |= R128_ALPHA_TEST_LESS;         break;
    case GL_LEQUAL:   a |= R128_ALPHA_TEST_LESSEQUAL;    break;
    case GL_EQUAL:    a |= R128_ALPHA_TEST_EQUAL;        break;
    case GL_GEQUAL:   a |= R128_ALPHA_TEST_GREATEREQUAL; break;
    case GL_GREATER:  a |= R128_ALPHA_TEST_GREATER;      break;
    case GL_NOTEQUAL: a |= R128_ALPHA_TEST_NEQUAL;       break;
    case GL_ALWAYS:   a |= R128_ALPHA_TEST_ALWAYS;
	break;
    default:
	/* ERROR!!! */
	return;
    }

    if (r128ctx->regs.misc_3d_state_cntl_reg != a) {
	r128FlushVertices(r128ctx);
	r128ctx->regs.misc_3d_state_cntl_reg = a;

	/* FIXME: Load into hardware now??? */
	r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	r128ctx->dirty_context |= R128_CTX_ALPHASTATE;
    }
}

static void r128DDBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);
    CARD32         b       = r128ctx->regs.misc_3d_state_cntl_reg;

    b &= ~(R128_ALPHA_BLEND_SRC_MASK | R128_ALPHA_BLEND_DST_MASK);

    switch (sfactor) {
    case GL_ZERO:                     b |= R128_ALPHA_BLEND_SRC_ZERO;
	break;
    case GL_ONE:                      b |= R128_ALPHA_BLEND_SRC_ONE;
	break;
    case GL_DST_COLOR:                b |= R128_ALPHA_BLEND_SRC_DESTCOLOR;
	break;
    case GL_ONE_MINUS_DST_COLOR:      b |= R128_ALPHA_BLEND_SRC_INVDESTCOLOR;
	break;
    case GL_SRC_ALPHA:                b |= R128_ALPHA_BLEND_SRC_SRCALPHA;
	break;
    case GL_ONE_MINUS_SRC_ALPHA:      b |= R128_ALPHA_BLEND_SRC_INVSRCALPHA;
	break;
    case GL_DST_ALPHA:                b |= R128_ALPHA_BLEND_SRC_DESTALPHA;
	break;
    case GL_ONE_MINUS_DST_ALPHA:      b |= R128_ALPHA_BLEND_SRC_INVDESTALPHA;
	break;
    case GL_SRC_ALPHA_SATURATE:       b |= R128_ALPHA_BLEND_SRC_SRCALPHASAT;
	break;
#if 0
	/* FIXME: These are not supported directly by the Rage 128.
           They could be emulated using something like the TexEnv
           modes. */
    case GL_CONSTANT_COLOR:           b |= 0;
	break;
    case GL_ONE_MINUS_CONSTANT_COLOR: b |= 0;
	break;
    case GL_CONSTANT_ALPHA:           b |= 0;
	break;
    case GL_ONE_MINUS_CONSTANT_ALPHA: b |= 0;
	break;
#endif
    default:
	/* ERROR!!! */
	return;
    }

    switch (dfactor) {
    case GL_ZERO:                     b |= R128_ALPHA_BLEND_DST_ZERO;
	break;
    case GL_ONE:                      b |= R128_ALPHA_BLEND_DST_ONE;
	break;
    case GL_SRC_COLOR:                b |= R128_ALPHA_BLEND_DST_SRCCOLOR;
	break;
    case GL_ONE_MINUS_SRC_COLOR:      b |= R128_ALPHA_BLEND_DST_INVSRCCOLOR;
	break;
    case GL_SRC_ALPHA:                b |= R128_ALPHA_BLEND_DST_SRCALPHA;
	break;
    case GL_ONE_MINUS_SRC_ALPHA:      b |= R128_ALPHA_BLEND_DST_INVSRCALPHA;
	break;
    case GL_DST_ALPHA:                b |= R128_ALPHA_BLEND_DST_DESTALPHA;
	break;
    case GL_ONE_MINUS_DST_ALPHA:      b |= R128_ALPHA_BLEND_DST_INVDESTALPHA;
	break;
#if 0
	/* FIXME: These are not supported directly by the Rage 128.
           They could be emulated using something like the TexEnv
           modes. */
    case GL_CONSTANT_COLOR:           b |= 0;
	break;
    case GL_ONE_MINUS_CONSTANT_COLOR: b |= 0;
	break;
    case GL_CONSTANT_ALPHA:           b |= 0;
	break;
    case GL_ONE_MINUS_CONSTANT_ALPHA: b |= 0;
	break;
#endif
    default:
	/* ERROR!!! */
	return;
    }

    if (r128ctx->regs.misc_3d_state_cntl_reg != b) {
	r128FlushVertices(r128ctx);
	r128ctx->regs.misc_3d_state_cntl_reg = b;

	/* FIXME: Load into hardware now??? */
	r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	r128ctx->dirty_context |= R128_CTX_ALPHASTATE;
    }
}

static void r128DDBlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB,
				    GLenum dfactorRGB, GLenum sfactorA,
				    GLenum dfactorA)
{
    if (sfactorRGB != sfactorA || dfactorRGB != dfactorA) {
	/* ERROR!!! */
	return;
    }

    r128DDBlendFunc(ctx, sfactorRGB, dfactorRGB);
}

static void r128DDClearDepth(GLcontext *ctx, GLclampd d)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    switch (r128ctx->regs.z_sten_cntl_c & R128_Z_PIX_WIDTH_MASK) {
    case R128_Z_PIX_WIDTH_16: r128ctx->ClearDepth = d * 0x0000ffff; break;
    case R128_Z_PIX_WIDTH_24: r128ctx->ClearDepth = d * 0x00ffffff; break;
    case R128_Z_PIX_WIDTH_32: r128ctx->ClearDepth = d * 0xffffffff; break;
    default: return;
    }
}

static void r128DDCullFace(GLcontext *ctx, GLenum mode)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);
    CARD32         f       = r128ctx->regs.pm4_vc_fpu_setup;

    if (!ctx->Polygon.CullFlag) return;

    f &= ~(R128_BACKFACE_MASK | R128_FRONTFACE_MASK);

    switch (mode) {
    case GL_FRONT:          f |= R128_BACKFACE_SOLID;  break;
    case GL_BACK:           f |= R128_FRONTFACE_SOLID; break;
    case GL_FRONT_AND_BACK:                            break;
    default: return;
    }

    if (r128ctx->regs.pm4_vc_fpu_setup != f) {
	r128FlushVertices(r128ctx);
	r128ctx->regs.pm4_vc_fpu_setup = f;

	/* FIXME: Load into hardware now??? */
	r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	r128ctx->dirty_context |= R128_CTX_SETUPSTATE;
    }
}

static void r128DDFrontFace(GLcontext *ctx, GLenum mode)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);
    CARD32         f       = r128ctx->regs.pm4_vc_fpu_setup;

    f &= ~R128_FRONT_DIR_MASK;

    switch (mode) {
    case GL_CW:  f |= R128_FRONT_DIR_CW;  break;
    case GL_CCW: f |= R128_FRONT_DIR_CCW; break;
    default: return;
    }

    if (r128ctx->regs.pm4_vc_fpu_setup != f) {
	r128FlushVertices(r128ctx);
	r128ctx->regs.pm4_vc_fpu_setup = f;

	/* FIXME: Load into hardware now??? */
	r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	r128ctx->dirty_context |= R128_CTX_SETUPSTATE;
    }
}

static void r128DDDepthFunc(GLcontext *ctx, GLenum func)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);
    CARD32         z       = r128ctx->regs.z_sten_cntl_c;

    z &= ~R128_Z_TEST_MASK;

    switch (func) {
    case GL_NEVER:    z |= R128_Z_TEST_NEVER;        break;
    case GL_LESS:     z |= R128_Z_TEST_LESS;         break;
    case GL_LEQUAL:   z |= R128_Z_TEST_LESSEQUAL;    break;
    case GL_EQUAL:    z |= R128_Z_TEST_EQUAL;        break;
    case GL_GEQUAL:   z |= R128_Z_TEST_GREATEREQUAL; break;
    case GL_GREATER:  z |= R128_Z_TEST_GREATER;      break;
    case GL_NOTEQUAL: z |= R128_Z_TEST_NEQUAL;       break;
    case GL_ALWAYS:   z |= R128_Z_TEST_ALWAYS;       break;
    default: return;
    }

    if (r128ctx->regs.z_sten_cntl_c != z) {
	r128FlushVertices(r128ctx);
	r128ctx->regs.z_sten_cntl_c = z;

	/* FIXME: Load into hardware now??? */
	r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	r128ctx->dirty_context |= R128_CTX_ZSTENSTATE;
    }
}

static void r128DDDepthMask(GLcontext *ctx, GLboolean flag)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);
    CARD32         t       = r128ctx->regs.tex_cntl_c;

    if (flag) t |=  R128_Z_WRITE_ENABLE;
    else      t &= ~R128_Z_WRITE_ENABLE;

    if (r128ctx->regs.tex_cntl_c != t) {
	r128FlushVertices(r128ctx);
	r128ctx->regs.tex_cntl_c = t;

	/* FIXME: Load into hardware now??? */
	r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	r128ctx->dirty_context |= R128_CTX_ENGINESTATE;
    }
}

static void r128DDLightModelfv(GLcontext *ctx, GLenum pname,
			       const GLfloat *param)
{
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL)
   {
      r128ContextPtr r128ctx = R128_CONTEXT( ctx );
      CARD32         t       = r128ctx->regs.tex_cntl_c;

      if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
	 t |=  R128_SPEC_LIGHT_ENABLE;
      else
	 t &= ~R128_SPEC_LIGHT_ENABLE;

      if (r128ctx->regs.tex_cntl_c != t) {
	 r128FlushVertices(r128ctx);
	 r128ctx->regs.tex_cntl_c = t;

	 /* FIXME: Load into hardware now??? */
	 r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	 r128ctx->dirty_context |= R128_CTX_ENGINESTATE;
      }
   }
}


static void r128DDEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);
    CARD32         t       = r128ctx->regs.tex_cntl_c;
    CARD32         f       = r128ctx->regs.pm4_vc_fpu_setup;

    if (R128_DEBUG_FLAGS & DEBUG_VERBOSE_API) {
	fprintf(stderr, "r128DDEnable( %p, 0x%x = %s )\n",
		ctx, cap, state ? "GL_TRUE" : "GL_FALSE");
    }

    switch (cap) {
    case GL_ALPHA_TEST:
	if (state) t |=  R128_ALPHA_TEST_ENABLE;
	else       t &= ~R128_ALPHA_TEST_ENABLE;
	break;

    case GL_AUTO_NORMAL: return;

    case GL_BLEND:
	if (state) t |=  R128_ALPHA_ENABLE;
	else       t &= ~R128_ALPHA_ENABLE;
	break;

    case GL_CLIP_PLANE0:
    case GL_CLIP_PLANE1:
    case GL_CLIP_PLANE2:
    case GL_CLIP_PLANE3:
    case GL_CLIP_PLANE4:
    case GL_CLIP_PLANE5:
    case GL_COLOR_MATERIAL: return;

    case GL_CULL_FACE:
        if (ctx->PB->primitive == GL_POLYGON) {
	    f &= ~(R128_BACKFACE_MASK | R128_FRONTFACE_MASK);
	    if (state) {
	        switch (ctx->Polygon.CullFaceMode) {
	        case GL_FRONT:          f |= R128_BACKFACE_SOLID;  break;
	        case GL_BACK:           f |= R128_FRONTFACE_SOLID; break;
	        case GL_FRONT_AND_BACK:                            break;
	        default: return;
	        }
	    } else {
	        f |= R128_BACKFACE_SOLID | R128_FRONTFACE_SOLID;
	    }
        }
	break;

    case GL_DEPTH_TEST:
	if (state) t |=  R128_Z_ENABLE;
	else       t &= ~R128_Z_ENABLE;
	break;

    case GL_DITHER:
	if (state) t |=  R128_DITHER_ENABLE;
	else       t &= ~R128_DITHER_ENABLE;
	break;

    case GL_FOG:
	if (state) t |=  R128_FOG_ENABLE;
	else       t &= ~R128_FOG_ENABLE;
	break;

    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
    case GL_LIGHTING:
    case GL_LINE_SMOOTH:
    case GL_LINE_STIPPLE:
    case GL_INDEX_LOGIC_OP:
    case GL_COLOR_LOGIC_OP:
    case GL_MAP1_COLOR_4:
    case GL_MAP1_INDEX:
    case GL_MAP1_NORMAL:
    case GL_MAP1_TEXTURE_COORD_1:
    case GL_MAP1_TEXTURE_COORD_2:
    case GL_MAP1_TEXTURE_COORD_3:
    case GL_MAP1_TEXTURE_COORD_4:
    case GL_MAP1_VERTEX_3:
    case GL_MAP1_VERTEX_4:
    case GL_MAP2_COLOR_4:
    case GL_MAP2_INDEX:
    case GL_MAP2_NORMAL:
    case GL_MAP2_TEXTURE_COORD_1:
    case GL_MAP2_TEXTURE_COORD_2:
    case GL_MAP2_TEXTURE_COORD_3:
    case GL_MAP2_TEXTURE_COORD_4:
    case GL_MAP2_VERTEX_3:
    case GL_MAP2_VERTEX_4:
    case GL_NORMALIZE:
    case GL_POINT_SMOOTH:
    case GL_POLYGON_SMOOTH:
    case GL_POLYGON_STIPPLE:
    case GL_POLYGON_OFFSET_POINT:
    case GL_POLYGON_OFFSET_LINE:
    case GL_POLYGON_OFFSET_FILL:
    case GL_RESCALE_NORMAL_EXT: return;

    case GL_SCISSOR_TEST:
	/* FIXME: Hook up the software scissor */
#if 0
	r128ctx->Scissor = state;
#endif
	break;

    case GL_SHARED_TEXTURE_PALETTE_EXT:
    case GL_STENCIL_TEST: return;

    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
    case GL_TEXTURE_3D:
	/* This is handled in r128UpdateTex[01]State() */
	r128ctx->dirty |= R128_UPDATE_TEXSTATE;
	break;

    case GL_TEXTURE_GEN_Q:
    case GL_TEXTURE_GEN_R:
    case GL_TEXTURE_GEN_S:
    case GL_TEXTURE_GEN_T: return;

	/* Client state */
    case GL_VERTEX_ARRAY:
    case GL_NORMAL_ARRAY:
    case GL_COLOR_ARRAY:
    case GL_INDEX_ARRAY:
    case GL_TEXTURE_COORD_ARRAY:
    case GL_EDGE_FLAG_ARRAY: return;

    default: return;
    }

    if (r128ctx->regs.tex_cntl_c != t) {
	r128FlushVertices(r128ctx);
	r128ctx->regs.tex_cntl_c = t;

	/* FIXME: Load into hardware now??? */
	r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	r128ctx->dirty_context |= R128_CTX_ENGINESTATE;
    }
    if (r128ctx->regs.pm4_vc_fpu_setup != f) {
	r128FlushVertices(r128ctx);
	r128ctx->regs.pm4_vc_fpu_setup = f;

	/* FIXME: Load into hardware now??? */
	r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	r128ctx->dirty_context |= R128_CTX_SETUPSTATE;
    }

}

static void r128DDFogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);
    GLubyte        c[4];
    CARD32         col;
    floatTOint     fog;
    GLenum         mode;

    if (R128_DEBUG_FLAGS & DEBUG_VERBOSE_API) {
	fprintf(stderr, "r128DDFogfv(%p, 0x%x)\n", ctx, pname);
    }

    switch (pname) {
    case GL_FOG_MODE:
	mode = (GLenum)(GLint)*param;
	if (r128ctx->FogMode != mode) {
	    r128FlushVertices(r128ctx);
	    r128ctx->FogMode = mode;

	    /* FIXME: Load into hardware now??? */
	    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	    r128ctx->dirty_context |= R128_CTX_FOGTABLE;
	}
	break;

    case GL_FOG_DENSITY:
	fog.f = *param;
	if (r128ctx->regs.fog_3d_table_density != fog.i) {
	    r128FlushVertices(r128ctx);
	    r128ctx->regs.fog_3d_table_density = fog.i;

	    /* FIXME: Load into hardware now??? */
	    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	    r128ctx->dirty_context |= R128_CTX_FOGSTATE;
	}
	break;

    case GL_FOG_START:
	fog.f = *param;
	if (r128ctx->regs.fog_3d_table_start != fog.i) {
	    r128FlushVertices(r128ctx);
	    r128ctx->regs.fog_3d_table_start = fog.i;

	    /* FIXME: Load into hardware now??? */
	    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	    r128ctx->dirty_context |= R128_CTX_FOGSTATE;
	}
	break;

    case GL_FOG_END:
	fog.f = *param;
	if (r128ctx->regs.fog_3d_table_end != fog.i) {
	    r128FlushVertices(r128ctx);
	    r128ctx->regs.fog_3d_table_end = fog.i;

	    /* FIXME: Load into hardware now??? */
	    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	    r128ctx->dirty_context |= R128_CTX_FOGSTATE;
	}
	break;

    case GL_FOG_COLOR:
	FLOAT_RGBA_TO_UBYTE_RGBA(c, ctx->Fog.Color);
	col = r128PackColor(32, c[0], c[1], c[2], c[3]);
	if (r128ctx->regs.fog_color_c != col) {
	    r128FlushVertices(r128ctx);
	    r128ctx->regs.fog_color_c = col;

	    /* FIXME: Load into hardware now??? */
	    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
	    r128ctx->dirty_context |= R128_CTX_FOGSTATE;
	}
	break;

    default:
	return;
    }
}

static void r128DDScissor(GLcontext *ctx,
			  GLint x, GLint y, GLsizei w, GLsizei h)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    r128ctx->ScissorRect.x1 = x;
    r128ctx->ScissorRect.y1 = r128ctx->driDrawable->h - (y + h);
    r128ctx->ScissorRect.x2 = x + w;
    r128ctx->ScissorRect.y2 = r128ctx->driDrawable->h - y;
}

/* Initialize the driver's state functions */
void r128DDInitStateFuncs(GLcontext *ctx)
{
    ctx->Driver.UpdateState            = r128DDUpdateState;

    ctx->Driver.ClearIndex             = NULL;
    ctx->Driver.ClearColor             = r128DDClearColor;
    ctx->Driver.Index                  = NULL;
    ctx->Driver.Color                  = r128DDColor;
    ctx->Driver.SetDrawBuffer          = r128DDSetDrawBuffer;
    ctx->Driver.SetReadBuffer          = r128DDSetReadBuffer;

    ctx->Driver.IndexMask              = NULL;
    ctx->Driver.ColorMask              = r128DDColorMask;
    ctx->Driver.LogicOp                = NULL;
    ctx->Driver.Dither                 = r128DDDither;

    ctx->Driver.NearFar                = NULL;

    ctx->Driver.RenderStart            = r128DDUpdateHWState;
    ctx->Driver.RenderFinish           = NULL;
    ctx->Driver.RasterSetup            = NULL;

    ctx->Driver.RenderVBClippedTab     = NULL;
    ctx->Driver.RenderVBCulledTab      = NULL;
    ctx->Driver.RenderVBRawTab         = NULL;

    ctx->Driver.ReducedPrimitiveChange = r128DDReducedPrimitiveChange;
    ctx->Driver.MultipassFunc          = NULL;

    ctx->Driver.AlphaFunc              = r128DDAlphaFunc;
    ctx->Driver.BlendEquation          = NULL;
    ctx->Driver.BlendFunc              = r128DDBlendFunc;
    ctx->Driver.BlendFuncSeparate      = r128DDBlendFuncSeparate;
    ctx->Driver.ClearDepth             = r128DDClearDepth;
    ctx->Driver.CullFace               = r128DDCullFace;
    ctx->Driver.FrontFace              = r128DDFrontFace;
    ctx->Driver.DepthFunc              = r128DDDepthFunc;
    ctx->Driver.DepthMask              = r128DDDepthMask;
    ctx->Driver.DepthRange             = NULL;
    ctx->Driver.Enable                 = r128DDEnable;
    ctx->Driver.Fogfv                  = r128DDFogfv;
    ctx->Driver.Hint                   = NULL;
    ctx->Driver.Lightfv                = NULL;
    ctx->Driver.LightModelfv           = r128DDLightModelfv;
    ctx->Driver.PolygonMode            = NULL;
    ctx->Driver.Scissor                = r128DDScissor;
    ctx->Driver.ShadeModel             = NULL;
    ctx->Driver.ClearStencil           = NULL;
    ctx->Driver.StencilFunc            = NULL;
    ctx->Driver.StencilMask            = NULL;
    ctx->Driver.StencilOp              = NULL;
    ctx->Driver.Viewport               = NULL;
}

/* Initialize the context's hardware state */
void r128DDInitState(r128ContextPtr r128ctx)
{
    int    dst_bpp, depth_bpp, pitch, i;
    CARD32 depthClear;

    pitch = r128ctx->r128Screen->fbStride / r128ctx->r128Screen->bpp;

    switch (r128ctx->r128Screen->pixel_code) {
    case  8: dst_bpp = R128_GMC_DST_8BPP_CI; break;
    case 15: dst_bpp = R128_GMC_DST_15BPP;   break;
    case 16: dst_bpp = R128_GMC_DST_16BPP;   break;
    case 24: dst_bpp = R128_GMC_DST_24BPP;   break;
    case 32: dst_bpp = R128_GMC_DST_32BPP;   break;
    default:
	fprintf(stderr, "Error: Unsupported pixel depth %d... exiting\n",
		r128ctx->r128Screen->pixel_code);
	exit(-1);
    }

    /* FIXME: Figure out how to use 16bpp depth buffer in 32bpp mode */
    switch (r128ctx->glCtx->Visual->DepthBits) {
    case 16: depthClear = 0x0000ffff; depth_bpp  = R128_Z_PIX_WIDTH_16; break;
    case 24: depthClear = 0x00ffffff; depth_bpp  = R128_Z_PIX_WIDTH_24; break;
    case 32: depthClear = 0xffffffff; depth_bpp  = R128_Z_PIX_WIDTH_32; break;
    default:
	fprintf(stderr, "Error: Unsupported depth %d... exiting\n",
		r128ctx->r128Screen->bpp);
	exit(-1);
	break;
    }

    /* Precalculate the depth scale while we're here */
    switch (r128ctx->glCtx->Visual->DepthBits) {
    case 16: r128ctx->depth_scale = 1.0 / 65536.0; break;
    case 24: r128ctx->depth_scale = 1.0 / 16777216.0; break;
    case 32: r128ctx->depth_scale = 1.0 / 4294967296.0; break;
    default: r128ctx->depth_scale = 1.0 / 65536.0; break;
    }

    r128ctx->dirty             = R128_ALL_DIRTY;
    r128ctx->dirty_context     = R128_CTX_ALL_DIRTY;

    r128ctx->RenderIndex       = R128_FALLBACK_BIT;
    r128ctx->PointsFunc        = NULL;
    r128ctx->LineFunc          = NULL;
    r128ctx->TriangleFunc      = NULL;
    r128ctx->QuadFunc          = NULL;

    r128ctx->IndirectTriangles = 0;
    r128ctx->Fallback          = 0;

    if (r128ctx->glCtx->Visual->DBflag) {
	r128ctx->drawX         = r128ctx->r128Screen->backX;
	r128ctx->drawY         = r128ctx->r128Screen->backY;
	r128ctx->readX         = r128ctx->r128Screen->backX;
	r128ctx->readY         = r128ctx->r128Screen->backY;
    } else {
	r128ctx->drawX         = r128ctx->r128Screen->fbX;
	r128ctx->drawY         = r128ctx->r128Screen->fbY;
	r128ctx->readX         = r128ctx->r128Screen->fbX;
	r128ctx->readY         = r128ctx->r128Screen->fbY;
    }

    r128ctx->ClearColor = 0x00000000;
    r128ctx->ClearDepth = depthClear;

    r128ctx->regs.scale_3d_cntl =
	R128_SCALE_DITHER_TABLE         |
	R128_TEX_CACHE_SIZE_FULL        |
	R128_DITHER_INIT_RESET          |
	R128_SCALE_3D_TEXMAP_SHADE      |
	R128_SCALE_PIX_REPLICATE        |
     /* R128_TEX_CACHE_SPLIT            | */
	R128_ALPHA_COMB_ADD_CLAMP       |
	R128_FOG_TABLE                  |
	R128_ALPHA_BLEND_SRC_ONE        |
	R128_ALPHA_BLEND_DST_ZERO       |
	R128_ALPHA_TEST_ALWAYS          |
	R128_COMPOSITE_SHADOW_CMP_EQUAL |
	R128_TEX_MAP_ALPHA_IN_TEXTURE   |
	R128_TEX_CACHE_LINE_SIZE_8QW;

    r128ctx->regs.dst_pitch_offset_c = pitch << R128_PITCH_SHIFT;

    r128ctx->regs.dp_gui_master_cntl =
	R128_GMC_DST_PITCH_OFFSET_CNTL |
	R128_GMC_DST_CLIPPING          |
	R128_GMC_BRUSH_SOLID_COLOR     |
	dst_bpp                        |
	R128_GMC_SRC_DATATYPE_COLOR    |
	R128_GMC_BYTE_MSB_TO_LSB       |
	R128_GMC_CONVERSION_TEMP_6500  |
	R128_ROP3_S                    |
	R128_DP_SRC_SOURCE_MEMORY      |
	R128_GMC_3D_FCN_EN             |
	R128_GMC_CLR_CMP_CNTL_DIS      |
	R128_AUX_CLIP_DIS              |
	R128_GMC_WR_MSK_DIS;

    r128ctx->regs.sc_top_left_c     = 0x00000000;
    r128ctx->regs.sc_bottom_right_c = 0x1fff1fff;

    r128ctx->regs.aux_sc_cntl       = 0x00000000;

    r128ctx->regs.aux1_sc_left      = 0x00000000;
    r128ctx->regs.aux1_sc_right     = 0x00001fff;
    r128ctx->regs.aux1_sc_top       = 0x00000000;
    r128ctx->regs.aux1_sc_bottom    = 0x00001fff;

    r128ctx->regs.aux2_sc_left      = 0x00000000;
    r128ctx->regs.aux2_sc_right     = 0x00001fff;
    r128ctx->regs.aux2_sc_top       = 0x00000000;
    r128ctx->regs.aux2_sc_bottom    = 0x00001fff;

    r128ctx->regs.aux3_sc_left      = 0x00000000;
    r128ctx->regs.aux3_sc_right     = 0x00001fff;
    r128ctx->regs.aux3_sc_top       = 0x00000000;
    r128ctx->regs.aux3_sc_bottom    = 0x00001fff;

    r128ctx->regs.z_offset_c        = (r128ctx->r128Screen->depthX *
				       (r128ctx->r128Screen->bpp/8) +
				       r128ctx->r128Screen->depthY *
				       r128ctx->r128Screen->fbStride);
    r128ctx->regs.z_pitch_c         = pitch;

    r128ctx->regs.z_sten_cntl_c =
	depth_bpp                |
	R128_Z_TEST_LESS         |
	R128_STENCIL_TEST_ALWAYS |
	R128_STENCIL_S_FAIL_KEEP |
	R128_STENCIL_ZPASS_KEEP  |
	R128_STENCIL_ZFAIL_KEEP;

    r128ctx->regs.tex_cntl_c =
	R128_Z_WRITE_ENABLE          |
	R128_SHADE_ENABLE            |
	R128_DITHER_ENABLE           |
	R128_ALPHA_IN_TEX_COMPLETE_A |
	R128_LIGHT_DIS               |
	R128_ALPHA_LIGHT_DIS         |
	R128_TEX_CACHE_FLUSH         |
	(0x0f << R128_LOD_BIAS_SHIFT);

    r128ctx->regs.misc_3d_state_cntl_reg =
	R128_MISC_SCALE_3D_TEXMAP_SHADE |
	R128_MISC_SCALE_PIX_REPLICATE   |
	R128_ALPHA_COMB_ADD_CLAMP       |
	R128_FOG_TABLE                  |
	R128_ALPHA_BLEND_SRC_ONE        |
	R128_ALPHA_BLEND_DST_ZERO       |
	R128_ALPHA_TEST_ALWAYS;

    r128ctx->regs.texture_clr_cmp_clr_c = 0x00000000;
    r128ctx->regs.texture_clr_cmp_msk_c = 0xffffffff;

    r128ctx->regs.prim_tex_cntl_c =
	R128_MIN_BLEND_NEAREST |
	R128_MAG_BLEND_NEAREST |
	R128_MIP_MAP_DISABLE   |
	R128_TEX_CLAMP_S_WRAP  |
	R128_TEX_CLAMP_T_WRAP;

    r128ctx->regs.prim_texture_combine_cntl_c =
	R128_COMB_MODULATE          |
	R128_COLOR_FACTOR_TEX       |
	R128_INPUT_FACTOR_INT_COLOR |
	R128_COMB_ALPHA_COPY        |
	R128_ALPHA_FACTOR_TEX_ALPHA |
	R128_INP_FACTOR_A_INT_ALPHA;

    r128ctx->regs.tex_size_pitch_c     =
	(0 << R128_TEX_PITCH_SHIFT)      |
	(0 << R128_TEX_SIZE_SHIFT)       |
	(0 << R128_TEX_HEIGHT_SHIFT)     |
	(0 << R128_TEX_MIN_SIZE_SHIFT)   |
	(0 << R128_SEC_TEX_PITCH_SHIFT)  |
	(0 << R128_SEC_TEX_SIZE_SHIFT)   |
	(0 << R128_SEC_TEX_HEIGHT_SHIFT) |
	(0 << R128_SEC_TEX_MIN_SIZE_SHIFT);

    for (i = 0; i < R128_TEX_MAXLEVELS; i++)
	r128ctx->regs.prim_tex_offset[i]  = 0x00000000;

    r128ctx->regs.sec_tex_cntl_c =
	R128_SEC_SELECT_PRIM_ST;

    r128ctx->regs.sec_tex_combine_cntl_c =
	R128_COMB_DIS                |
	R128_COLOR_FACTOR_TEX        |
	R128_INPUT_FACTOR_PREV_COLOR |
	R128_COMB_ALPHA_DIS          |
	R128_ALPHA_FACTOR_TEX_ALPHA  |
	R128_INP_FACTOR_A_PREV_ALPHA;

    for (i = 0; i < R128_TEX_MAXLEVELS; i++)
	r128ctx->regs.sec_tex_offset[i] = 0x00000000;

    r128ctx->regs.constant_color_c            = 0x00ffffff;
    r128ctx->regs.prim_texture_border_color_c = 0x00ffffff;
    r128ctx->regs.sec_texture_border_color_c  = 0x00ffffff;
    r128ctx->regs.sten_ref_mask_c             = 0xffff0000;
    r128ctx->regs.plane_3d_mask_c             = 0xffffffff;

    r128ctx->regs.setup_cntl =
	R128_COLOR_GOURAUD         |
	R128_PRIM_TYPE_TRI         |
#if 1
	/* FIXME: Let r128 multiply? */
	R128_TEXTURE_ST_MULT_W     |
#else
	/* FIXME: Or, pre multiply? */
	R128_TEXTURE_ST_DIRECT     |
#endif
	R128_STARTING_VERTEX_1     |
	R128_ENDING_VERTEX_3       |
	R128_SU_POLY_LINE_NOT_LAST |
	R128_SUB_PIX_4BITS;

    r128ctx->regs.pm4_vc_fpu_setup =
	R128_FRONT_DIR_CCW         |
	R128_BACKFACE_SOLID        |
	R128_FRONTFACE_SOLID       |
	R128_FPU_COLOR_GOURAUD     |
	R128_FPU_SUB_PIX_4BITS     |
	R128_FPU_MODE_3D           |
	R128_TRAP_BITS_DISABLE     |
	R128_XFACTOR_2             |
	R128_YFACTOR_2             |
	R128_FLAT_SHADE_VERTEX_OGL |
	R128_FPU_ROUND_TRUNCATE    |
	R128_WM_SEL_8DW;

    r128ctx->FogMode                   = GL_EXP;
    r128ctx->regs.fog_color_c          = 0x00808080;
    r128ctx->regs.fog_3d_table_start   = 0x00000000;
    r128ctx->regs.fog_3d_table_end     = 0xffffffff;
    r128ctx->regs.fog_3d_table_density = 0x00000000;

    r128ctx->regs.window_xy_offset     = 0x00000000;

    r128ctx->regs.dp_write_mask        = 0xffffffff;

    r128ctx->regs.pc_gui_ctlstat       = R128_PC_FLUSH_GUI;

    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
    r128ctx->dirty_context |= R128_CTX_ALL_DIRTY;
}

/* Upload the fog table for the current fog mode */
static void r128UploadFogTable(r128ContextPtr r128ctx)
{
    int i;

    R128CCE0(R128_CCE_PACKET0, R128_FOG_TABLE_INDEX, 0);
    R128CCE(0x00000000);

    R128CCE0(R128_CCE_PACKET0_ONE_REG_WR, R128_FOG_TABLE_DATA, 255);


    if (0)
       fprintf(stderr, "uploading fog table for %s\n",
	       gl_lookup_enum_by_nr(r128ctx->FogMode));


    /* KW: I'm not sure we're doing enough here - shouldn't density
     *  play a role in calculating the exp and exp2 tables -- is the
     *  card really doing exponent calculations of its own?  Need to
     *  see the spec...
     */
    switch (r128ctx->FogMode) {
    case GL_LINEAR:
	for (i = 0; i < 256; i++) {
	    R128CCE(255 - i);
	}
	break;
    case GL_EXP:
	for (i = 0; i < 256; i++) {
	   float arg = (255 - i)/255.0;
	   float exparg = (exp(arg) - 1) / (M_E - 1); /* range [0,1] */
	   int result;
	   FLOAT_COLOR_TO_UBYTE_COLOR(result, exparg);
	   R128CCE(result);
	}
	break;
    case GL_EXP2:
	for (i = 0; i < 256; i++) {
	   float arg = (255 - i)/255.0;
	   float exparg = (exp(arg*arg) - 1) / (M_E - 1); /* range [0,1] */
	   int result;
	   FLOAT_COLOR_TO_UBYTE_COLOR(result, exparg);
	   R128CCE(result);
	}
	break;
    }
}

/* Load the current context's state into the hardware */
/* NOTE: This function is only called while holding the hardware lock */
static void r128LoadContext(r128ContextPtr r128ctx)
{
    int i;
    int tex_size_pitch_done = GL_FALSE;

    if (R128_DEBUG_FLAGS & DEBUG_VERBOSE_API) {
	fprintf(stderr, "r128LoadContext(%p)\n", r128ctx->glCtx);
    }

#if 0
    r128ctx->dirty_context = R128_CTX_ALL_DIRTY;
#endif

#if 1
    /* FIXME: Why do these need to be updated even when they don't change? */
    r128ctx->dirty_context |= (R128_CTX_MISC        |
			       R128_CTX_ENGINESTATE |
			       R128_CTX_ALPHASTATE);
#endif

#if 1
    /* FIXME: Is this _really_ needed? */
    if (r128ctx->dirty_context)
	if (!R128CCE_USE_RING_BUFFER(r128ctx->r128Screen->CCEMode))
	    r128WaitForIdleLocked(r128ctx);
#endif

    if (r128ctx->dirty_context & R128_CTX_MISC) {
	R128CCE1(R128_CCE_PACKET1, R128_SCALE_3D_CNTL, R128_DP_WRITE_MASK);
	R128CCE(r128ctx->regs.scale_3d_cntl);
	R128CCE(r128ctx->regs.dp_write_mask);

	R128CCE0(R128_CCE_PACKET0, R128_DST_PITCH_OFFSET_C, 1);
	R128CCE(r128ctx->regs.dst_pitch_offset_c);
	R128CCE(r128ctx->regs.dp_gui_master_cntl);

	R128CCE0(R128_CCE_PACKET0, R128_TEXTURE_CLR_CMP_CLR_C, 1);
	R128CCE(r128ctx->regs.texture_clr_cmp_clr_c);
	R128CCE(r128ctx->regs.texture_clr_cmp_msk_c);

	R128CCE0(R128_CCE_PACKET0, R128_STEN_REF_MASK_C, 1);
	R128CCE(r128ctx->regs.sten_ref_mask_c);
	R128CCE(r128ctx->regs.plane_3d_mask_c);
    }

    if (r128ctx->dirty_context & R128_CTX_ENGINESTATE) {
	R128CCE0(R128_CCE_PACKET0, R128_TEX_CNTL_C, 0);
	R128CCE(r128ctx->regs.tex_cntl_c);
    }

    if (r128ctx->dirty_context & R128_CTX_TEX0STATE) {
	R128CCE0(R128_CCE_PACKET0, R128_PRIM_TEX_CNTL_C, 2+R128_TEX_MAXLEVELS);
	R128CCE(r128ctx->regs.prim_tex_cntl_c);
	R128CCE(r128ctx->regs.prim_texture_combine_cntl_c);
	R128CCE(r128ctx->regs.tex_size_pitch_c);
	for (i = 0; i < R128_TEX_MAXLEVELS; i++)
	    R128CCE(r128ctx->regs.prim_tex_offset[i]);

	R128CCE0(R128_CCE_PACKET0, R128_PRIM_TEXTURE_BORDER_COLOR_C, 0);
	R128CCE(r128ctx->regs.prim_texture_border_color_c);

	tex_size_pitch_done = GL_TRUE;
    }

    if (r128ctx->dirty_context & R128_CTX_TEX1STATE) {
	if (!tex_size_pitch_done) {
	    R128CCE0(R128_CCE_PACKET0, R128_TEX_SIZE_PITCH_C, 0);
	    R128CCE(r128ctx->regs.tex_size_pitch_c);
	}

	R128CCE0(R128_CCE_PACKET0, R128_SEC_TEX_CNTL_C, 1+R128_TEX_MAXLEVELS);
	R128CCE(r128ctx->regs.sec_tex_cntl_c);
	R128CCE(r128ctx->regs.sec_tex_combine_cntl_c);
	for (i = 0; i < R128_TEX_MAXLEVELS; i++)
	    R128CCE(r128ctx->regs.sec_tex_offset[i]);

	R128CCE0(R128_CCE_PACKET0, R128_SEC_TEXTURE_BORDER_COLOR_C, 0);
	R128CCE(r128ctx->regs.sec_texture_border_color_c);
    }

    if (r128ctx->dirty_context & R128_CTX_TEXENVSTATE) {
	R128CCE0(R128_CCE_PACKET0, R128_CONSTANT_COLOR_C, 0);
	R128CCE(r128ctx->regs.constant_color_c);
    }

    if (r128ctx->dirty_context & R128_CTX_FOGSTATE) {
	R128CCE0(R128_CCE_PACKET0, R128_FOG_3D_TABLE_START, 1);
	R128CCE(r128ctx->regs.fog_3d_table_start);
	R128CCE(r128ctx->regs.fog_3d_table_end);

	R128CCE1(R128_CCE_PACKET1,
		 R128_FOG_COLOR_C, R128_FOG_3D_TABLE_DENSITY);
	R128CCE(r128ctx->regs.fog_color_c);
	R128CCE(r128ctx->regs.fog_3d_table_density);
    }

    if (r128ctx->dirty_context & R128_CTX_FOGTABLE) {
	r128UploadFogTable(r128ctx);
    }

    if (r128ctx->dirty_context & R128_CTX_ZSTENSTATE) {
	R128CCE0(R128_CCE_PACKET0, R128_Z_STEN_CNTL_C, 0);
	R128CCE(r128ctx->regs.z_sten_cntl_c);
    }

    if (r128ctx->dirty_context & R128_CTX_SCISSORS) {
	R128CCE0(R128_CCE_PACKET0, R128_SC_TOP_LEFT_C, 1);
	R128CCE(r128ctx->regs.sc_top_left_c);
	R128CCE(r128ctx->regs.sc_bottom_right_c);
    }

    if (r128ctx->dirty_context & (R128_CTX_ALPHASTATE |
				  R128_CTX_FOGSTATE)) {
	R128CCE0(R128_CCE_PACKET0, R128_MISC_3D_STATE_CNTL_REG, 0);
	R128CCE(r128ctx->regs.misc_3d_state_cntl_reg);
    }

    if (r128ctx->dirty_context & R128_CTX_SETUPSTATE) {
	R128CCE1(R128_CCE_PACKET1, R128_SETUP_CNTL, R128_PM4_VC_FPU_SETUP);
	R128CCE(r128ctx->regs.setup_cntl);
	R128CCE(r128ctx->regs.pm4_vc_fpu_setup);
    }

    if (r128ctx->dirty_context & R128_CTX_WIN_Z_POS) {
	R128CCE0(R128_CCE_PACKET0, R128_WINDOW_XY_OFFSET, 0);
	R128CCE(r128ctx->regs.window_xy_offset);

	R128CCE0(R128_CCE_PACKET0, R128_Z_OFFSET_C, 1);
	R128CCE(r128ctx->regs.z_offset_c);
	R128CCE(r128ctx->regs.z_pitch_c);
    }

#if 0
    if (r128ctx->dirty_context & R128_CTX_FLUSH_PIX_CACHE) {
	R128CCE0(R128_CCE_PACKET0, R128_PC_GUI_CTLSTAT, 0);
	R128CCE(r128ctx->regs.pc_gui_ctlstat);
    }
#endif

    R128CCE_SUBMIT_PACKET();

    /* Turn off the texture cache flushing */
    r128ctx->regs.tex_cntl_c     &= ~R128_TEX_CACHE_FLUSH;

    /* Turn off the pixel cache flushing */
    r128ctx->regs.pc_gui_ctlstat &= ~R128_PC_FLUSH_ALL;

    r128ctx->dirty_context = R128_CTX_CLEAN;
}

/* Set the hardware clip rects for drawing to the current color buffer */
/* NOTE: This function is only called while holding the hardware lock */
void r128SetClipRects(r128ContextPtr r128ctx,
		      XF86DRIClipRectPtr pc, int nc)
{
    if (!pc) return;

    /* Clear any previous auxiliary scissors */
    r128ctx->regs.aux_sc_cntl = 0x00000000;

    switch (nc) {
    case 3:
	R128CCE0(R128_CCE_PACKET0, R128_AUX3_SC_LEFT, 3);
	R128CCE(pc[2].x1   + r128ctx->drawX);
	R128CCE(pc[2].x2-1 + r128ctx->drawX);
	R128CCE(pc[2].y1   + r128ctx->drawY);
	R128CCE(pc[2].y2-1 + r128ctx->drawY);

	r128ctx->regs.aux_sc_cntl |= R128_AUX3_SC_EN | R128_AUX3_SC_MODE_OR;

    case 2:
	R128CCE0(R128_CCE_PACKET0, R128_AUX2_SC_LEFT, 3);
	R128CCE(pc[1].x1   + r128ctx->drawX);
	R128CCE(pc[1].x2-1 + r128ctx->drawX);
	R128CCE(pc[1].y1   + r128ctx->drawY);
	R128CCE(pc[1].y2-1 + r128ctx->drawY);

	r128ctx->regs.aux_sc_cntl |= R128_AUX2_SC_EN | R128_AUX2_SC_MODE_OR;

    case 1:
	R128CCE0(R128_CCE_PACKET0, R128_AUX1_SC_LEFT, 3);
	R128CCE(pc[0].x1   + r128ctx->drawX);
	R128CCE(pc[0].x2-1 + r128ctx->drawX);
	R128CCE(pc[0].y1   + r128ctx->drawY);
	R128CCE(pc[0].y2-1 + r128ctx->drawY);

	r128ctx->regs.aux_sc_cntl |= R128_AUX1_SC_EN | R128_AUX1_SC_MODE_OR;
	break;

    default:
	return;
    }

    R128CCE0(R128_CCE_PACKET0, R128_AUX_SC_CNTL, 0);
    R128CCE(r128ctx->regs.aux_sc_cntl);

    R128CCE_SUBMIT_PACKET();
}

/* Update the driver's notion of the window position */
/* NOTE: This function is only called while holding the hardware lock */
static void r128UpdateWindowPosition(r128ContextPtr r128ctx)
{
    int x = r128ctx->driDrawable->x + r128ctx->drawX;
    int y = r128ctx->driDrawable->y + r128ctx->drawY;

#if 0
    /* FIXME: Is this _really_ needed? */
    R128CCE_FLUSH_VB(r128ctx);
#endif
    r128ctx->regs.window_xy_offset = ((y << R128_WINDOW_Y_SHIFT) |
				      (x << R128_WINDOW_X_SHIFT));

    /* Recalculate the Z buffer offset since we might be drawing to the
       back buffer and window_xy_offset affects both color buffer and
       depth drawing */
    r128ctx->regs.z_offset_c        = ((r128ctx->r128Screen->depthX -
					r128ctx->drawX) *
				       (r128ctx->r128Screen->bpp/8) +
				       (r128ctx->r128Screen->depthY -
					r128ctx->drawY) *
				       r128ctx->r128Screen->fbStride);

    r128ctx->dirty         |= R128_UPDATE_CONTEXT;
    r128ctx->dirty_context |= R128_CTX_WIN_Z_POS;
}

/* Update the hardware state */
/* NOTE: This function is only called while holding the hardware lock */
static void r128UpdateHWStateLocked(r128ContextPtr r128ctx)
{
    if (r128ctx->dirty & R128_REQUIRE_QUIESCENCE)
	r128WaitForIdleLocked(r128ctx);

    /* Update any state that might have changed recently */

    /* Update the clip rects */
    if (r128ctx->dirty & R128_UPDATE_WINPOS)
	r128UpdateWindowPosition(r128ctx);

    /* Update texture state and then upload the images */
    /* Note: Texture images can only be updated after the state has been set */
    if (r128ctx->dirty & R128_UPDATE_TEXSTATE)
	r128UpdateTextureState(r128ctx);
    if (r128ctx->dirty & R128_UPDATE_TEX0IMAGES)
        r128UploadTexImages(r128ctx, r128ctx->CurrentTexObj[0]);
    if (r128ctx->dirty & R128_UPDATE_TEX1IMAGES)
        r128UploadTexImages(r128ctx, r128ctx->CurrentTexObj[1]);

    /* Load the state into the hardware */
    /* Note: This must be done after all other state has been set */
    if (r128ctx->dirty & R128_UPDATE_CONTEXT)
	r128LoadContext(r128ctx);

    r128ctx->dirty = R128_CLEAN;
}

/* Update the hardware state */
void r128UpdateHWState(r128ContextPtr r128ctx)
{
    LOCK_HARDWARE(r128ctx);
    r128UpdateHWStateLocked(r128ctx);
    UNLOCK_HARDWARE(r128ctx);
}

/* Update the driver's state */
/* NOTE: This function is only called while holding the hardware lock */
void r128UpdateState(r128ContextPtr r128ctx, int winMoved)
{
    R128SAREAPrivPtr sarea = r128ctx->r128Screen->SAREA;
    int              i;

    if (sarea->ctxOwner != r128ctx->driContext->hHWContext) {
	sarea->ctxOwner = r128ctx->driContext->hHWContext;
	r128ctx->dirty_context = R128_CTX_ALL_DIRTY;
	r128LoadContext(r128ctx);
    }

    for (i = 0; i < r128ctx->r128Screen->NRTexHeaps; i++)
	r128AgeTextures(r128ctx, i);

    if (winMoved) r128ctx->dirty |= R128_UPDATE_WINPOS;
}
