/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgastate.c,v 1.3 2000/06/21 12:10:59 tsi Exp $ */

#include <stdio.h>

#include "types.h"
#include "pb.h"
#include "dd.h"

#include "mm.h"
#include "mgalib.h"
#include "mgadd.h"
#include "mgastate.h"
#include "mgatex.h"
#include "mgalog.h"
#include "mgavb.h"
#include "mgatris.h"
#include "mgaregs.h"
#include "mgabuffers.h"

/* Some outstanding problems with accelerating logic ops...
 */
#if defined(ACCEL_ROP)
static GLuint mgarop_NoBLK[16] = {
   DC_atype_rpl  | 0x00000000, DC_atype_rstr | 0x00080000, 
   DC_atype_rstr | 0x00040000, DC_atype_rpl  | 0x000c0000,
   DC_atype_rstr | 0x00020000, DC_atype_rstr | 0x000a0000, 
   DC_atype_rstr | 0x00060000, DC_atype_rstr | 0x000e0000,
   DC_atype_rstr | 0x00010000, DC_atype_rstr | 0x00090000, 
   DC_atype_rstr | 0x00050000, DC_atype_rstr | 0x000d0000,
   DC_atype_rpl  | 0x00030000, DC_atype_rstr | 0x000b0000, 
   DC_atype_rstr | 0x00070000, DC_atype_rpl  | 0x000f0000
};
#endif


static void mgaUpdateZMode(const GLcontext *ctx)
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   int zmode = 0;

   if (ctx->Depth.Test) {
      switch(ctx->Depth.Func)  { 
      case GL_NEVER:
	 zmode = DC_zmode_nozcmp; break;
      case GL_ALWAYS:  
	 zmode = DC_zmode_nozcmp; break;
      case GL_LESS:
	 zmode = DC_zmode_zlt; break; 
      case GL_LEQUAL:
	 zmode = DC_zmode_zlte; break;
      case GL_EQUAL:
	 zmode = DC_zmode_ze; break;
      case GL_GREATER:
	 zmode = DC_zmode_zgt; break;
      case GL_GEQUAL:
	 zmode = DC_zmode_zgte; break;
      case GL_NOTEQUAL:
	 zmode = DC_zmode_zne; break;
      default:
	 break;
      }
      if (ctx->Depth.Mask)
	 zmode |= DC_atype_zi;
      else
	 zmode |= DC_atype_i;
   } else {
      zmode |= DC_zmode_nozcmp | DC_atype_i;
   }

#if defined(ACCEL_ROP)
   mmesa->Setup[MGA_CTXREG_DWGCTL] &= DC_bop_MASK;
   if (ctx->Color.ColorLogicOpEnabled) 
      zmode |= mgarop_NoBLK[(ctx->Color.LogicOp)&0xf];
   else 
      zmode |= mgarop_NoBLK[GL_COPY & 0xf];
#endif

   mmesa->Setup[MGA_CTXREG_DWGCTL] &= DC_zmode_MASK & DC_atype_MASK; 
   mmesa->Setup[MGA_CTXREG_DWGCTL] |= zmode;
   mmesa->dirty |= MGA_UPLOAD_CTX;
}


static void mgaDDAlphaFunc(GLcontext *ctx, GLenum func, GLclampf ref)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}


static void mgaDDBlendEquation(GLcontext *ctx, GLenum mode) 
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}

static void mgaDDBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}

static void mgaDDBlendFuncSeparate( GLcontext *ctx, GLenum sfactorRGB, 
				    GLenum dfactorRGB, GLenum sfactorA,
				    GLenum dfactorA )
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}



static void mgaDDLightModelfv(GLcontext *ctx, GLenum pname, 
			      const GLfloat *param)
{
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
      FLUSH_BATCH( MGA_CONTEXT(ctx) );
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
   }
}


static void mgaDDShadeModel(GLcontext *ctx, GLenum mode)
{
   if (1) {
      FLUSH_BATCH( MGA_CONTEXT(ctx) );
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE; 
      mgaMsg(8, "mgaDDShadeModel: %x\n", mode);
   }
}


static void mgaDDDepthFunc(GLcontext *ctx, GLenum func)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_DEPTH;
}

static void mgaDDDepthMask(GLcontext *ctx, GLboolean flag)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_DEPTH;
}

#if defined(ACCEL_ROP)
static void mgaDDLogicOp( GLcontext *ctx, GLenum opcode )
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_DEPTH;
}
#else
static void mgaDDLogicOp( GLcontext *ctx, GLenum opcode )
{
   if (ctx->Color.ColorLogicOpEnabled) {
      FLUSH_BATCH( MGA_CONTEXT(ctx) );
   
      if (opcode == GL_COPY)
	 MGA_CONTEXT(ctx)->Fallback &= ~MGA_FALLBACK_LOGICOP;
      else
	 MGA_CONTEXT(ctx)->Fallback |= MGA_FALLBACK_LOGICOP;
   }
}
#endif


static void mgaUpdateFogAttrib( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   mgaUI32 color = MGAPACKCOLOR888((mgaUI8)(ctx->Fog.Color[0]*255.0F), 
				   (mgaUI8)(ctx->Fog.Color[1]*255.0F), 
				   (mgaUI8)(ctx->Fog.Color[2]*255.0F));

   if (color != mmesa->Setup[MGA_CTXREG_FOGCOLOR]) 
      mmesa->Setup[MGA_CTXREG_FOGCOLOR] = color;

   mmesa->Setup[MGA_CTXREG_MACCESS] &= ~MA_fogen_enable;
   if (ctx->FogMode == FOG_FRAGMENT) 
      mmesa->Setup[MGA_CTXREG_MACCESS] |= MA_fogen_enable;

   mmesa->dirty |= MGA_UPLOAD_CTX;
}

static void mgaDDFogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_FOG;
}




/* =============================================================
 * Alpha blending
 */


static void mgaUpdateAlphaMode(GLcontext *ctx)
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;
   int a = 0;

   /* determine source of alpha for blending and testing */
   if ( !ctx->Texture.Enabled ) 
      a |= AC_alphasel_diffused;
   else {
      switch (ctx->Texture.Unit[0].EnvMode) {
      case GL_DECAL:
      case GL_REPLACE:
	 a |= AC_alphasel_fromtex;
	 break;
      case GL_BLEND:
      case GL_MODULATE:
	 a |= AC_alphasel_modulated;
	 break;
      default:
	 break;
      }
   }


   /* alpha test control.
    */
   if (ctx->Color.AlphaEnabled) {
      GLubyte ref = ctx->Color.AlphaRef;
      switch (ctx->Color.AlphaFunc) {
      case GL_NEVER:		
	 a |= AC_atmode_alt;
	 ref = 0;
	 break;
      case GL_LESS:
	 a |= AC_atmode_alt; 
	 break;
      case GL_GEQUAL:
	 a |= AC_atmode_agte; 
	 break;
      case GL_LEQUAL:  
	 a |= AC_atmode_alte; 
	 break;
      case GL_GREATER:  
	 a |= AC_atmode_agt; 
	 break;
      case GL_NOTEQUAL: 
	 a |= AC_atmode_ane; 
	 break;
      case GL_EQUAL: 
	 a |= AC_atmode_ae; 
	 break;
      case GL_ALWAYS:
	 a |= AC_atmode_noacmp; 
	 break;
      default:
	 break;
      }
      a |= MGA_FIELD(AC_atref,ref);
   }

   /* blending control */
   if (ctx->Color.BlendEnabled) {
      switch (ctx->Color.BlendSrcRGB) {
      case GL_ZERO:
	 a |= AC_src_zero; break;
      case GL_SRC_ALPHA:
	 a |= AC_src_src_alpha; break;
      case GL_ONE:
	 a |= AC_src_one; break;
      case GL_DST_COLOR:
	 a |= AC_src_dst_color; break;
      case GL_ONE_MINUS_DST_COLOR:
	 a |= AC_src_om_dst_color; break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 a |= AC_src_om_src_alpha; break;
      case GL_DST_ALPHA:
	 if (mgaScreen->cpp == 4)
	    a |= AC_src_dst_alpha;
	 else
	    a |= AC_src_one;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 if (mgaScreen->cpp == 4)
	    a |= AC_src_om_dst_alpha; 
	 else 
	    a |= AC_src_zero;
	 break;
      case GL_SRC_ALPHA_SATURATE:
	 a |= AC_src_src_alpha_sat; 
	 break;
      default:		/* never happens */
	 break;
      }

      switch (ctx->Color.BlendDstRGB) {
      case GL_SRC_ALPHA:
	 a |= AC_dst_src_alpha; break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 a |= AC_dst_om_src_alpha; break;
      case GL_ZERO:
	 a |= AC_dst_zero; break;
      case GL_ONE:
	 a |= AC_dst_one; break;
      case GL_SRC_COLOR:
	 a |= AC_dst_src_color; break;
      case GL_ONE_MINUS_SRC_COLOR:
	 a |= AC_dst_om_src_color; break;
      case GL_DST_ALPHA:
	 if (mgaScreen->cpp == 4)
	    a |= AC_dst_dst_alpha;
	 else
	    a |= AC_dst_one;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 if (mgaScreen->cpp == 4)
	    a |= AC_dst_om_dst_alpha;
	 else
	    a |= AC_dst_zero;
	 break;
      default:		/* never happens */
	 break;
      }
   } else {
      a |= AC_src_one|AC_dst_zero;
   }

   mmesa->Setup[MGA_CTXREG_ALPHACTRL] = (AC_amode_alpha_channel | 
					AC_astipple_disable | 
					AC_aten_disable | 
					AC_atmode_noacmp | 
					a);
 
   mmesa->dirty |= MGA_UPLOAD_CTX;
}



/* =============================================================
 * Hardware clipping
 */

void mgaUpdateClipping(const GLcontext *ctx)
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   if (mmesa->driDrawable)
   {
      int x1 = mmesa->driDrawable->x + ctx->Scissor.X;
      int y1 = mmesa->driDrawable->y + mmesa->driDrawable->h - (ctx->Scissor.Y+
								ctx->Scissor.Height);
      int x2 = mmesa->driDrawable->x + ctx->Scissor.X+ctx->Scissor.Width;
      int y2 = mmesa->driDrawable->y + mmesa->driDrawable->h - ctx->Scissor.Y;

      if (x1 < 0) x1 = 0;
      if (y1 < 0) y1 = 0;
      if (x2 < 0) x2 = 0;
      if (y2 < 0) y2 = 0;

      mmesa->scissor_rect.x1 = x1;
      mmesa->scissor_rect.y1 = y1;
      mmesa->scissor_rect.x2 = x2;
      mmesa->scissor_rect.y2 = y2;

      if (MGA_DEBUG&DEBUG_VERBOSE_2D)
	 fprintf(stderr, "SET SCISSOR %d,%d-%d,%d\n", 
		 mmesa->scissor_rect.x1,
		 mmesa->scissor_rect.y1,
		 mmesa->scissor_rect.x2,
		 mmesa->scissor_rect.y2);

      mmesa->dirty |= MGA_UPLOAD_CLIPRECTS;
   }
}


static void mgaDDScissor( GLcontext *ctx, GLint x, GLint y, 
			  GLsizei w, GLsizei h )
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_CLIP;
}


/* ======================================================================
 * New stuff for DRI state.
 */

static void mgaDDDither(GLcontext *ctx, GLboolean enable)
{
}




static void mgaDDSetColor(GLcontext *ctx, 
			   GLubyte r, GLubyte g,
			   GLubyte b, GLubyte a )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   mmesa->MonoColor = mgaPackColor( mmesa->mgaScreen->cpp, r, g, b, a );
}


static void mgaDDClearColor(GLcontext *ctx, 
			     GLubyte r, GLubyte g,
			     GLubyte b, GLubyte a )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   mmesa->ClearColor = mgaPackColor( mmesa->mgaScreen->cpp, r, g, b, a );
}


/* =============================================================
 * Culling
 */

#define _CULL_DISABLE 0
#define _CULL_NEGATIVE ((1<<11)|(1<<5)|(1<<16))
#define _CULL_POSITIVE (1<<11)


static void mgaUpdateCull( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint mode = _CULL_DISABLE;
   
   if (ctx->Polygon.CullFlag && 
       ctx->PB->primitive == GL_POLYGON &&       
       ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK) 
   {
      mode = _CULL_NEGATIVE;
      if (ctx->Polygon.CullFaceMode == GL_FRONT)
	 mode ^= (_CULL_POSITIVE ^ _CULL_NEGATIVE);
      if (ctx->Polygon.FrontFace != GL_CCW)
	 mode ^= (_CULL_POSITIVE ^ _CULL_NEGATIVE);
      if (mmesa->multitex)
	 mode ^= (_CULL_POSITIVE ^ _CULL_NEGATIVE); /* why??? */
   }

   mmesa->Setup[MGA_CTXREG_WFLAG] = mode;
   mmesa->dirty |= MGA_UPLOAD_CTX;
}


static void mgaDDCullFaceFrontFace(GLcontext *ctx, GLenum mode)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_CULL;
}




/* =============================================================
 * Color masks
 */

/* Mesa calls this from the wrong place:
 */
static GLboolean mgaDDColorMask(GLcontext *ctx, 
				GLboolean r, GLboolean g, 
				GLboolean b, GLboolean a )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;	


   GLuint mask = mgaPackColor(mgaScreen->cpp,
			      ctx->Color.ColorMask[RCOMP],
			      ctx->Color.ColorMask[GCOMP],
			      ctx->Color.ColorMask[BCOMP],
			      ctx->Color.ColorMask[ACOMP]);
  
   if (mgaScreen->cpp == 2)
      mask = mask | (mask << 16);

   if (mmesa->Setup[MGA_CTXREG_PLNWT] != mask) {
      FLUSH_BATCH( MGA_CONTEXT(ctx) );
      mmesa->Setup[MGA_CTXREG_PLNWT] = mask;      
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_MASK; 
      mmesa->dirty |= MGA_UPLOAD_CTX;
   }

   return 1;
}

/* =============================================================
 * Polygon stipple 
 * 
 * The mga supports a subset of possible 4x4 stipples natively, GL
 * wants 32x32.  Fortunately stipple is usually a repeating pattern.
 */
static int mgaStipples[16] = {
   0xffff,
   0xa5a5,
   0x5a5a,
   0xa0a0,
   0x5050,
   0x0a0a,
   0x0505,
   0x8020,
   0x0401,
   0x1040,
   0x0208,
   0x0802,
   0x4010,
   0x0104,
   0x2080,
   0x0000
};

static void mgaDDPolygonStipple( GLcontext *ctx, const GLubyte *mask )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   const GLubyte *m = mask;
   GLubyte p[4];
   int i,j,k;
   int active = (ctx->Polygon.StippleFlag && ctx->PB->primitive == GL_POLYGON);
   GLuint stipple;

   FLUSH_BATCH(mmesa);
   ctx->Driver.TriangleCaps |= DD_TRI_STIPPLE;

   if (active) {
      mmesa->dirty |= MGA_UPLOAD_CTX;
      mmesa->Setup[MGA_CTXREG_DWGCTL] &= ~(0xf<<20);
   }

   p[0] = mask[0] & 0xf; p[0] |= p[0] << 4;
   p[1] = mask[4] & 0xf; p[1] |= p[1] << 4;
   p[2] = mask[8] & 0xf; p[2] |= p[2] << 4;
   p[3] = mask[12] & 0xf; p[3] |= p[3] << 4;

   for (k = 0 ; k < 8 ; k++)
      for (j = 0 ; j < 4; j++) 
	 for (i = 0 ; i < 4 ; i++) 
	    if (*m++ != p[j]) {
	       ctx->Driver.TriangleCaps &= ~DD_TRI_STIPPLE;
	       return;
	    }

   stipple = ( ((p[0] & 0xf) << 0) |
	       ((p[1] & 0xf) << 4) |
	       ((p[2] & 0xf) << 8) |
	       ((p[3] & 0xf) << 12) );   

   for (i = 0 ; i < 16 ; i++)
      if (mgaStipples[i] == stipple) {
	 mmesa->poly_stipple = i<<20;
	 break;
      }

   if (i == 16) {
      ctx->Driver.TriangleCaps &= ~DD_TRI_STIPPLE;
      return;
   }
   
   if (active) {
      mmesa->Setup[MGA_CTXREG_DWGCTL] &= ~(0xf<<20);
      mmesa->Setup[MGA_CTXREG_DWGCTL] |= mmesa->poly_stipple;
   }
}

/* =============================================================
 */




static void mgaDDPrintDirty( const char *msg, GLuint state )
{
   fprintf(stderr, "%s (0x%x): %s%s%s%s%s%s%s\n",
	   msg,
	   (unsigned int) state,
	   (state & MGA_WAIT_AGE) ? "wait-age, " : "",
	   (state & MGA_UPLOAD_TEX0IMAGE)  ? "upload-tex0-img, " : "",
	   (state & MGA_UPLOAD_TEX1IMAGE)  ? "upload-tex1-img, " : "",
	   (state & MGA_UPLOAD_CTX)        ? "upload-ctx, " : "",
	   (state & MGA_UPLOAD_TEX0)       ? "upload-tex0, " : "",
	   (state & MGA_UPLOAD_TEX1)       ? "upload-tex1, " : "",
	   (state & MGA_UPLOAD_PIPE)       ? "upload-pipe, " : ""
	   );
}


/* Push the state into the sarea and/or texture memory.
 */
void mgaEmitHwStateLocked( mgaContextPtr mmesa )
{
   if (MGA_DEBUG & DEBUG_VERBOSE_MSG)
      mgaDDPrintDirty( "mgaEmitHwStateLocked", mmesa->dirty );

   if ((mmesa->dirty & MGA_UPLOAD_TEX0IMAGE) && mmesa->CurrentTexObj[0])
      mgaUploadTexImages(mmesa, mmesa->CurrentTexObj[0]);
   
   if ((mmesa->dirty & MGA_UPLOAD_TEX1IMAGE) && mmesa->CurrentTexObj[1])
      mgaUploadTexImages(mmesa, mmesa->CurrentTexObj[1]);
  
   if (mmesa->dirty & MGA_UPLOAD_CTX) 
      memcpy( mmesa->sarea->ContextState, 
	      mmesa->Setup,
	      sizeof(mmesa->Setup));

   if ((mmesa->dirty & MGA_UPLOAD_TEX0) && mmesa->CurrentTexObj[0])
	 memcpy(mmesa->sarea->TexState[0],
		mmesa->CurrentTexObj[0]->Setup,
		sizeof(mmesa->sarea->TexState[0]));

   if ((mmesa->dirty & MGA_UPLOAD_TEX1) && mmesa->CurrentTexObj[1])
	 memcpy(mmesa->sarea->TexState[1],
		mmesa->CurrentTexObj[1]->Setup,
		sizeof(mmesa->sarea->TexState[1]));

   mmesa->sarea->WarpPipe = mmesa->warp_pipe;
   mmesa->sarea->dirty |= mmesa->dirty;

#if 0
   mgaPrintSetupFlags("warp pipe", mmesa->sarea->WarpPipe);
   fprintf(stderr, "in mgaEmitHwStateLocked: dirty now %x\n",
	   mmesa->sarea->dirty);
#endif

   mmesa->dirty &= (MGA_UPLOAD_CLIPRECTS|MGA_WAIT_AGE);
}



/* =============================================================
 */

static void mgaDDEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );

   switch(cap) {
   case GL_ALPHA_TEST:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MGA_NEW_ALPHA;
      break;
   case GL_BLEND:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MGA_NEW_ALPHA;
      break;
   case GL_DEPTH_TEST:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MGA_NEW_DEPTH;
      break;
   case GL_SCISSOR_TEST:
      FLUSH_BATCH( mmesa );
      mmesa->scissor = state;
      mmesa->new_state |= MGA_NEW_CLIP;
      break;
   case GL_FOG:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MGA_NEW_FOG;
      break;
   case GL_CULL_FACE:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MGA_NEW_CULL;
      break;
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= (MGA_NEW_TEXTURE|MGA_NEW_ALPHA);
      break;
   case GL_POLYGON_STIPPLE:
      if ((ctx->Driver.TriangleCaps & DD_TRI_STIPPLE) &&
	  ctx->PB->primitive == GL_POLYGON) 
      {
	 FLUSH_BATCH(mmesa);
	 mmesa->dirty |= MGA_UPLOAD_CTX;
	 mmesa->Setup[MGA_CTXREG_DWGCTL] &= ~(0xf<<20);
	 if (state)
	    mmesa->Setup[MGA_CTXREG_DWGCTL] |= mmesa->poly_stipple;
      }
      break;
   case GL_COLOR_LOGIC_OP:
   case GL_INDEX_LOGIC_OP:
      FLUSH_BATCH( mmesa );
#if !defined(ACCEL_ROP)
      mmesa->Fallback &= ~MGA_FALLBACK_LOGICOP;
      if (state && ctx->Color.LogicOp != GL_COPY)
	 mmesa->Fallback |= MGA_FALLBACK_LOGICOP;
#else
      mmesa->new_state |= MGA_NEW_DEPTH;
#endif
      break;
   default:
      break; 
   }    
}


/* =============================================================
 */

/* Just need to note that it has changed - the kernel will do the
 * upload the next time we fire a dma buffer.
 */
static void mgaWarpUpdateState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   int index = mmesa->setupindex;
 
   index &= ~(MGA_WIN_BIT|MGA_TEX0_BIT|MGA_RGBA_BIT);
   index |= MGA_ALPHA_BIT | MGA_SPEC_BIT | MGA_FOG_BIT;

   if (index != mmesa->warp_pipe)
   {
      FLUSH_BATCH(mmesa);      
      mmesa->warp_pipe = index;
      mmesa->new_state |= MGA_NEW_WARP;
      mmesa->dirty |= MGA_UPLOAD_PIPE;
   }
}



/* =============================================================
 */

static void mgaDDPrintState( const char *msg, GLuint state )
{
   mgaMsg(1, "%s (0x%x): %s%s%s%s%s%s%s%s\n",
	   msg,
	   state,
	   (state & MGA_NEW_DEPTH)   ? "depth, " : "",
	   (state & MGA_NEW_ALPHA)   ? "alpha, " : "",
	   (state & MGA_NEW_FOG)     ? "fog, " : "",
	   (state & MGA_NEW_CLIP)    ? "clip, " : "",
	   (state & MGA_NEW_MASK)    ? "colormask, " : "",
	   (state & MGA_NEW_CULL)    ? "cull, " : "",
	   (state & MGA_NEW_TEXTURE) ? "texture, " : "",
	   (state & MGA_NEW_CONTEXT) ? "context, " : "");
}

void mgaDDUpdateHwState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   int new_state = mmesa->new_state;

   if (new_state) 
   {
      FLUSH_BATCH( mmesa );

      mmesa->new_state = 0;

      if (MESA_VERBOSE&VERBOSE_DRIVER)
	 mgaDDPrintState("UpdateHwState", new_state);

      if (new_state & MGA_NEW_DEPTH)
	 mgaUpdateZMode(ctx);

      if (new_state & MGA_NEW_ALPHA)
	 mgaUpdateAlphaMode(ctx);

      if (new_state & MGA_NEW_FOG)
	 mgaUpdateFogAttrib(ctx);

      if (new_state & MGA_NEW_CLIP)
	 mgaUpdateClipping(ctx);

      if (new_state & (MGA_NEW_WARP|MGA_NEW_CULL))
	 mgaUpdateCull(ctx);

      if (new_state & (MGA_NEW_WARP|MGA_NEW_TEXTURE))
	 mgaUpdateTextureState(ctx);
   }
}







void mgaDDReducedPrimitiveChange( GLcontext *ctx, GLenum prim )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );
   mgaUpdateCull(ctx);   

   if (ctx->Polygon.StippleFlag && (ctx->Driver.TriangleCaps & DD_TRI_STIPPLE))
   {
      mmesa->dirty |= MGA_UPLOAD_CTX;
      mmesa->Setup[MGA_CTXREG_DWGCTL] &= ~(0xf<<20);
      if (ctx->PB->primitive == GL_POLYGON)
	 mmesa->Setup[MGA_CTXREG_DWGCTL] |= mmesa->poly_stipple;
   }
}


#define INTERESTED (~(NEW_MODELVIEW|NEW_PROJECTION|\
                      NEW_TEXTURE_MATRIX|\
                      NEW_USER_CLIP|NEW_CLIENT_STATE|\
                      NEW_TEXTURE_ENABLE))

void mgaDDUpdateState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaglx.c_setupPointers++;
   
   if (ctx->NewState & INTERESTED) {
      mgaDDChooseRenderState(ctx);  
      mgaChooseRasterSetupFunc(ctx);
      mgaWarpUpdateState(ctx);
   }     

   /* Have to do this here to detect texture fallbacks in time:
    */
   if (mmesa->new_state & MGA_NEW_TEXTURE)
      mgaDDUpdateHwState( ctx );

   if (0) fprintf(stderr, "fallback %x indirect %x\n", mmesa->Fallback, 
		  mmesa->IndirectTriangles);
   
   if (!mmesa->Fallback) {
      ctx->IndirectTriangles &= ~DD_SW_RASTERIZE;
      ctx->IndirectTriangles |= mmesa->IndirectTriangles;

      ctx->Driver.PointsFunc=mmesa->PointsFunc;
      ctx->Driver.LineFunc=mmesa->LineFunc;
      ctx->Driver.TriangleFunc=mmesa->TriangleFunc;
      ctx->Driver.QuadFunc=mmesa->QuadFunc;
   }
}



void mgaInitState( mgaContextPtr mmesa )
{
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;
   GLcontext *ctx = mmesa->glCtx;

   if (ctx->Color.DriverDrawBuffer == GL_BACK_LEFT) {
      mmesa->draw_buffer = MGA_BACK;
      mmesa->read_buffer = MGA_BACK;
      mmesa->drawOffset = mmesa->mgaScreen->backOffset;
      mmesa->readOffset = mmesa->mgaScreen->backOffset;
      mmesa->Setup[MGA_CTXREG_DSTORG] = mgaScreen->backOffset;
   } else {
      mmesa->drawOffset = mmesa->mgaScreen->frontOffset;
      mmesa->readOffset = mmesa->mgaScreen->frontOffset;
      mmesa->draw_buffer = MGA_FRONT;
      mmesa->read_buffer = MGA_FRONT;
      mmesa->Setup[MGA_CTXREG_DSTORG] = mgaScreen->frontOffset;
   }

   if (mgaScreen->cpp == 2) 
      mmesa->Setup[MGA_CTXREG_MACCESS] = (MA_pwidth_16 |
					  MA_zwidth_16 |
					  MA_memreset_disable |
					  MA_fogen_disable |
					  MA_tlutload_disable |
					  MA_nodither_disable |
					  MA_dit555_disable);
   else
      mmesa->Setup[MGA_CTXREG_MACCESS] = (MA_pwidth_32 |
					  MA_zwidth_32 | /* stencil? */
					  MA_memreset_disable |
					  MA_fogen_disable |
					  MA_tlutload_disable |
					  MA_nodither_enable |
					  MA_dit555_disable);

   mmesa->Setup[MGA_CTXREG_DWGCTL] = (DC_opcod_trap |
				      DC_atype_i |
				      DC_linear_xy |
				      DC_zmode_nozcmp |
				      DC_solid_disable |
				      DC_arzero_disable |
				      DC_sgnzero_disable |
				      DC_shftzero_enable |
				      (0xC << DC_bop_SHIFT) |
				      (0x0 << DC_trans_SHIFT) |
				      DC_bltmod_bmonolef |
				      DC_pattern_disable |
				      DC_transc_disable |
				      DC_clipdis_disable);


   mmesa->Setup[MGA_CTXREG_PLNWT] = ~0;
   mmesa->Setup[MGA_CTXREG_ALPHACTRL] = ( AC_src_one |
					  AC_dst_zero | 
					  AC_amode_FCOL | 
					  AC_astipple_disable | 
					  AC_aten_disable | 
					  AC_atmode_noacmp |
					  AC_alphasel_fromtex );

   mmesa->Setup[MGA_CTXREG_FOGCOLOR] = 
      MGAPACKCOLOR888((GLubyte)(ctx->Fog.Color[0]*255.0F), 
		      (GLubyte)(ctx->Fog.Color[1]*255.0F), 
		      (GLubyte)(ctx->Fog.Color[2]*255.0F));

   mmesa->Setup[MGA_CTXREG_WFLAG] = 0;
   mmesa->Setup[MGA_CTXREG_TDUAL0] = 0;
   mmesa->Setup[MGA_CTXREG_TDUAL1] = 0;
   mmesa->Setup[MGA_CTXREG_FCOL] = 0;
   mmesa->new_state = ~0;
}


void mgaDDInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState = mgaDDUpdateState;
   ctx->Driver.Enable = mgaDDEnable;
   ctx->Driver.LightModelfv = mgaDDLightModelfv;
   ctx->Driver.AlphaFunc = mgaDDAlphaFunc;
   ctx->Driver.BlendEquation = mgaDDBlendEquation;
   ctx->Driver.BlendFunc = mgaDDBlendFunc;
   ctx->Driver.BlendFuncSeparate = mgaDDBlendFuncSeparate;
   ctx->Driver.DepthFunc = mgaDDDepthFunc;
   ctx->Driver.DepthMask = mgaDDDepthMask;
   ctx->Driver.Fogfv = mgaDDFogfv;
   ctx->Driver.Scissor = mgaDDScissor;
   ctx->Driver.ShadeModel = mgaDDShadeModel;
   ctx->Driver.CullFace = mgaDDCullFaceFrontFace;
   ctx->Driver.FrontFace = mgaDDCullFaceFrontFace;
   ctx->Driver.ColorMask = mgaDDColorMask;
   ctx->Driver.ReducedPrimitiveChange = mgaDDReducedPrimitiveChange;
   ctx->Driver.RenderStart = mgaDDUpdateHwState; 
   ctx->Driver.RenderFinish = 0; 

   ctx->Driver.SetDrawBuffer = mgaDDSetDrawBuffer;
   ctx->Driver.SetReadBuffer = mgaDDSetReadBuffer;
   ctx->Driver.Color = mgaDDSetColor;
   ctx->Driver.ClearColor = mgaDDClearColor;
   ctx->Driver.Dither = mgaDDDither;
   ctx->Driver.LogicOpcode = mgaDDLogicOp;

   ctx->Driver.PolygonStipple = mgaDDPolygonStipple;

   ctx->Driver.Index = 0;
   ctx->Driver.ClearIndex = 0;
   ctx->Driver.IndexMask = 0;

}

