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
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#include <stdlib.h>

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_span.h"
#include "radeon_tex.h"
#include "radeon_tris.h"
#include "radeon_vb.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "array_cache/acache.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "context.h"
#include "simple_list.h"
#include "mem.h"
#include "matrix.h"
#include "extensions.h"
#if defined(USE_X86_ASM)
#include "X86/common_x86_asm.h"
#endif

#define RADEON_DATE	"20020221"

#ifndef RADEON_DEBUG
int RADEON_DEBUG = (0
/*		    | DEBUG_ALWAYS_SYNC */
/*		    | DEBUG_VERBOSE_API */
/*		    | DEBUG_VERBOSE_MSG */
/*		    | DEBUG_VERBOSE_LRU */
/*		    | DEBUG_VERBOSE_DRI */
/*		    | DEBUG_VERBOSE_IOCTL */
/*		    | DEBUG_VERBOSE_2D */
/*		    | DEBUG_VERBOSE_TEXTURE */
   );
#endif



/* Return the width and height of the current color buffer.
 */
static void radeonGetBufferSize( GLcontext *ctx,
				   GLuint *width, GLuint *height )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   LOCK_HARDWARE( rmesa );
   *width  = rmesa->dri.drawable->w;
   *height = rmesa->dri.drawable->h;
   UNLOCK_HARDWARE( rmesa );
}

/* Return various strings for glGetString().
 */
static const GLubyte *radeonGetString( GLcontext *ctx, GLenum name )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   static char buffer[128];

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *)"VA Linux Systems, Inc.";

   case GL_RENDERER:
      sprintf( buffer, "Mesa DRI Radeon " RADEON_DATE );

      /* Append any chipset-specific information.  None yet.
       */

      /* Append any AGP-specific information.
       */
      switch ( rmesa->radeonScreen->AGPMode ) {
      case 1:
	 strncat( buffer, " AGP 1x", 7 );
	 break;
      case 2:
	 strncat( buffer, " AGP 2x", 7 );
	 break;
      case 4:
	 strncat( buffer, " AGP 4x", 7 );
	 break;
      }

      /* Append any CPU-specific information.
       */
#ifdef USE_X86_ASM
      if ( _mesa_x86_cpu_features ) {
	 strncat( buffer, " x86", 4 );
      }
#ifdef USE_MMX_ASM
      if ( cpu_has_mmx ) {
	 strncat( buffer, "/MMX", 4 );
      }
#endif
#ifdef USE_3DNOW_ASM
      if ( cpu_has_3dnow ) {
	 strncat( buffer, "/3DNow!", 7 );
      }
#endif
#ifdef USE_SSE_ASM
      if ( cpu_has_xmm ) {
	 strncat( buffer, "/SSE", 4 );
      }
#endif
#endif
      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}

/* Send all commands to the hardware.  If vertex buffers or indirect
 * buffers are in use, then we need to make sure they are sent to the
 * hardware.  All commands that are normally sent to the ring are
 * already considered `flushed'.
 */
static void radeonFlush( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   RADEON_FIREVERTICES( rmesa );

   if ( rmesa->boxes ) {
      LOCK_HARDWARE( rmesa );
      radeonPerformanceBoxesLocked( rmesa );
      UNLOCK_HARDWARE( rmesa );
   }

   /* Log the performance counters if necessary */
   radeonPerformanceCounters( rmesa );
}

/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
static void radeonFinish( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   /* Bump the performance counter */
   rmesa->c_drawWaits++;
   radeonFlush( ctx );
   LOCK_HARDWARE( rmesa );
   radeonWaitForIdleLocked( rmesa );
   UNLOCK_HARDWARE( rmesa );
}


/* Initialize the extensions supported by this driver.
 */
static void radeonInitExtensions( GLcontext *ctx )
{
   _mesa_enable_imaging_extensions( ctx );

   _mesa_enable_extension( ctx, "GL_ARB_multitexture" );
   _mesa_enable_extension( ctx, "GL_ARB_texture_env_add" );

   _mesa_enable_extension( ctx, "GL_EXT_blend_logic_op" );
   _mesa_enable_extension( ctx, "GL_EXT_texture_env_add" );
   _mesa_enable_extension( ctx, "GL_EXT_texture_env_combine" );
   _mesa_enable_extension( ctx, "GL_EXT_texture_env_dot3" );
   _mesa_enable_extension( ctx, "GL_EXT_texture_filter_anisotropic" );
   _mesa_enable_extension( ctx, "GL_EXT_texture_lod_bias" );

}

extern const struct gl_pipeline_stage _radeon_render_stage;
extern const struct gl_pipeline_stage _radeon_tcl_render_stage;

static const struct gl_pipeline_stage *radeon_pipeline[] = {
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
				/* REMOVE: point attenuation stage */
#if 1
   &_radeon_render_stage,	/* ADD: unclipped rastersetup-to-dma */
#endif
   &_tnl_render_stage,
   0,
};



/* Initialize the driver's misc functions.
 */
static void radeonInitDriverFuncs( GLcontext *ctx )
{
    ctx->Driver.GetBufferSize		= radeonGetBufferSize;
    ctx->Driver.GetString		= radeonGetString;
    ctx->Driver.Finish			= radeonFinish;
    ctx->Driver.Flush			= radeonFlush;

    ctx->Driver.Error			= NULL;
    ctx->Driver.DrawPixels		= NULL;
    ctx->Driver.Bitmap			= NULL;
}



/* Create the device specific context.
 */
static GLboolean
radeonCreateContext( Display *dpy, const __GLcontextModes *glVisual,
                     __DRIcontextPrivate *driContextPriv,
                     void *sharedContextPrivate)
{
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   radeonScreenPtr radeonScreen = (radeonScreenPtr)(sPriv->private);
   radeonContextPtr rmesa;
   GLcontext *ctx, *shareCtx;
   int i;

   assert(dpy);
   assert(glVisual);
   assert(driContextPriv);
   assert(radeonScreen);

   /* Allocate the Radeon context */
   rmesa = (radeonContextPtr) CALLOC( sizeof(*rmesa) );
   if ( !rmesa )
      return GL_FALSE;

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((radeonContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;
   rmesa->glCtx = _mesa_create_context(glVisual, shareCtx, rmesa, GL_TRUE);
   if (!rmesa->glCtx) {
      FREE(rmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = rmesa;

   /* Init radeon context data */
   rmesa->dri.display = dpy;
   rmesa->dri.context = driContextPriv;
   rmesa->dri.screen = sPriv;
   rmesa->dri.drawable = NULL; /* Set by XMesaMakeCurrent */
   rmesa->dri.hwContext = driContextPriv->hHWContext;
   rmesa->dri.hwLock = &sPriv->pSAREA->lock;
   rmesa->dri.fd = sPriv->fd;

   rmesa->radeonScreen = radeonScreen;
   rmesa->sarea = (RADEONSAREAPrivPtr)((GLubyte *)sPriv->pSAREA +
				       radeonScreen->sarea_priv_offset);


   for ( i = 0 ; i < radeonScreen->numTexHeaps ; i++ ) {
      make_empty_list( &rmesa->texture.objects[i] );
      rmesa->texture.heap[i] = mmInit( 0, radeonScreen->texSize[i] );
      rmesa->texture.age[i] = -1;
   }
   rmesa->texture.numHeaps = radeonScreen->numTexHeaps;
   make_empty_list( &rmesa->texture.swapped );

   rmesa->RenderIndex = ~0;
   rmesa->state.hw.dirty = RADEON_UPLOAD_CONTEXT_ALL;
   rmesa->upload_cliprects = 1;

   /* KW: Set the maximum texture size small enough that we can
    * guarentee that both texture units can bind a maximal texture
    * and have them both in on-card memory at once.
    * Test for 2 textures * 4 bytes/texel * size * size.
    */
   ctx = rmesa->glCtx;
   if (radeonScreen->texSize[RADEON_CARD_HEAP] >= 2 * 4 * 2048 * 2048) {
      ctx->Const.MaxTextureLevels = 12; /* 2048x2048 */
   }
   else if (radeonScreen->texSize[RADEON_CARD_HEAP] >= 2 * 4 * 1024 * 1024) {
      ctx->Const.MaxTextureLevels = 11; /* 1024x1024 */
   }
   else if (radeonScreen->texSize[RADEON_CARD_HEAP] >= 2 * 4 * 512 * 512) {
      ctx->Const.MaxTextureLevels = 10; /* 512x512 */
   }
   else {
      ctx->Const.MaxTextureLevels = 9; /* 256x256 */
   }

   ctx->Const.MaxTextureUnits = 2;
   ctx->Const.MaxTextureMaxAnisotropy = 16.0;

   /* No wide points.
    */
   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 1.0;
   ctx->Const.MaxPointSizeAA = 1.0;

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 10.0;
   ctx->Const.MaxLineWidthAA = 10.0;
   ctx->Const.LineWidthGranularity = 0.0625;

   if (getenv("LIBGL_PERFORMANCE_BOXES"))
      rmesa->boxes = 1;
   else
      rmesa->boxes = 0;

   {
      const char *debug = getenv("LIBGL_DEBUG");
      if (debug && strstr(debug, "fallbacks")) {
         rmesa->debugFallbacks = GL_TRUE;
      }
   }

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _ac_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );


   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, radeon_pipeline );

   /* Configure swrast to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );

   radeonInitVB( ctx );
   radeonInitExtensions( ctx );
   radeonInitDriverFuncs( ctx );
   radeonInitIoctlFuncs( ctx );
   radeonInitStateFuncs( ctx );
   radeonInitSpanFuncs( ctx );
   radeonInitTextureFuncs( ctx );
   radeonInitTriFuncs( ctx );

   radeonInitState( rmesa );

   return GL_TRUE;
}


/* Destroy the device specific context.
 */
/* Destroy the Mesa and driver specific context data.
 */
static void
radeonDestroyContext( __DRIcontextPrivate *driContextPriv )
{
   GET_CURRENT_CONTEXT(ctx);
   radeonContextPtr rmesa = (radeonContextPtr) driContextPriv->driverPrivate;
   radeonContextPtr current = ctx ? RADEON_CONTEXT(ctx) : NULL;

   /* check if we're deleting the currently bound context */
   if (rmesa == current) {
      _mesa_make_current2(NULL, NULL, NULL);
   }

   /* Free radeon context resources */
   assert(rmesa); /* should never be null */
   if ( rmesa ) {
      if (rmesa->glCtx->Shared->RefCount == 1) {
         /* This share group is about to go away, free our private
          * texture object data.
          */
         radeonTexObjPtr t, next_t;
         int i;

         for ( i = 0 ; i < rmesa->texture.numHeaps ; i++ ) {
            foreach_s ( t, next_t, &rmesa->texture.objects[i] ) {
               radeonDestroyTexObj( rmesa, t );
            }
            mmDestroy( rmesa->texture.heap[i] );
         }

         foreach_s ( t, next_t, &rmesa->texture.swapped ) {
            radeonDestroyTexObj( rmesa, t );
         }
      }

      _swsetup_DestroyContext( rmesa->glCtx );
      _tnl_DestroyContext( rmesa->glCtx );
      _ac_DestroyContext( rmesa->glCtx );
      _swrast_DestroyContext( rmesa->glCtx );

      radeonFreeVB( rmesa->glCtx );

      /* free the Mesa context */
      rmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context( rmesa->glCtx );

      FREE( rmesa );
   }

#if 0
   /* Use this to force shared object profiling. */
   glx_fini_prof();
#endif
}


/* Initialize the driver specific screen private data.
 */
static GLboolean
radeonInitDriver( __DRIscreenPrivate *sPriv )
{
   sPriv->private = (void *) radeonCreateScreen( sPriv );
   if ( !sPriv->private ) {
      radeonDestroyScreen( sPriv );
      return GL_FALSE;
   }

   return GL_TRUE;
}


/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
radeonCreateBuffer( Display *dpy,
                    __DRIscreenPrivate *driScrnPriv,
                    __DRIdrawablePrivate *driDrawPriv,
                    const __GLcontextModes *mesaVis,
                    GLboolean isPixmap )
{
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      const GLboolean swDepth = GL_FALSE;
      const GLboolean swAlpha = GL_FALSE;
      const GLboolean swAccum = mesaVis->accumRedBits > 0;
      const GLboolean swStencil = mesaVis->stencilBits > 0 &&
         mesaVis->depthBits != 24;
      driDrawPriv->driverPrivate = (void *)
         _mesa_create_framebuffer( mesaVis,
                                   swDepth,
                                   swStencil,
                                   swAccum,
                                   swAlpha );
      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
radeonDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}



static void
radeonSwapBuffers(Display *dpy, void *drawablePrivate)
{
   __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *) drawablePrivate;
   (void) dpy;

   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      radeonContextPtr rmesa;
      GLcontext *ctx;
      rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = rmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_swapbuffers( ctx );  /* flush pending rendering comands */
         if ( rmesa->doPageFlip ) {
            radeonPageFlip( dPriv );
         }
         else {
            radeonCopyBuffer( dPriv );
         }
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "radeonSwapBuffers: drawable has no context!\n");
   }
}


/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
static GLboolean
radeonMakeCurrent( __DRIcontextPrivate *driContextPriv,
                   __DRIdrawablePrivate *driDrawPriv,
                   __DRIdrawablePrivate *driReadPriv )
{
   if ( driContextPriv ) {
      GET_CURRENT_CONTEXT(ctx);
      radeonContextPtr oldRadeonCtx = ctx ? RADEON_CONTEXT(ctx) : NULL;
      radeonContextPtr newRadeonCtx = (radeonContextPtr) driContextPriv->driverPrivate;

      if ( newRadeonCtx != oldRadeonCtx ) {
	 newRadeonCtx->state.hw.dirty = RADEON_UPLOAD_CONTEXT_ALL;
	 if ( newRadeonCtx->state.texture.unit[0].texobj )
	    newRadeonCtx->state.hw.dirty |= RADEON_UPLOAD_TEX0;
	 if ( newRadeonCtx->state.texture.unit[1].texobj )
	    newRadeonCtx->state.hw.dirty |= RADEON_UPLOAD_TEX1;
      }

      if ( newRadeonCtx->dri.drawable != driDrawPriv ) {
	 newRadeonCtx->dri.drawable = driDrawPriv;
	 newRadeonCtx->upload_cliprects = 1;
	 radeonUpdateWindow( newRadeonCtx->glCtx );
	 radeonUpdateViewportOffset( newRadeonCtx->glCtx );
      }

      _mesa_make_current2( newRadeonCtx->glCtx,
			   (GLframebuffer *) driDrawPriv->driverPrivate,
			   (GLframebuffer *) driReadPriv->driverPrivate );

      if ( !newRadeonCtx->glCtx->Viewport.Width ) {
	 _mesa_set_viewport( newRadeonCtx->glCtx, 0, 0,
			     driDrawPriv->w, driDrawPriv->h );
      }
   } else {
      _mesa_make_current( 0, 0 );
   }

   return GL_TRUE;
}

/* Force the context `c' to be unbound from its buffer.
 */
static GLboolean
radeonUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}

/* Initialize the fullscreen mode.
 */
static GLboolean
radeonOpenFullScreen( __DRIcontextPrivate *driContextPriv )
{
#if 0
   radeonContextPtr rmesa = (radeonContextPtr)driContextPriv->driverPrivate;
   GLint ret;

   /* FIXME: Do we need to check this?
    */
   if ( !rmesa->glCtx->Visual.doubleBufferMode )
      return GL_TRUE;

   LOCK_HARDWARE( rmesa );
   radeonWaitForIdleLocked( rmesa );

   /* Ignore errors.  If this fails, we simply don't do page flipping.
    */
   ret = drmRadeonFullScreen( rmesa->driFd, GL_TRUE );

   UNLOCK_HARDWARE( rmesa );

   rmesa->doPageFlip = ( ret == 0 );
#endif
   return GL_TRUE;
}

/* Shut down the fullscreen mode.
 */
static GLboolean
radeonCloseFullScreen( __DRIcontextPrivate *driContextPriv )
{
#if 0
   radeonContextPtr rmesa = (radeonContextPtr)driContextPriv->driverPrivate;

   LOCK_HARDWARE( rmesa );
   radeonWaitForIdleLocked( rmesa );

   /* Don't care if this fails, we're not page flipping anymore.
    */
   drmRadeonFullScreen( rmesa->driFd, GL_FALSE );

   UNLOCK_HARDWARE( rmesa );

   rmesa->doPageFlip = GL_FALSE;
   rmesa->currentPage = 0;
#endif
   return GL_TRUE;
}


/* This function is called by libGL.so as soon as libGL.so is loaded.
 * This is where we'd register new extension functions with the dispatcher.
 */
void
__driRegisterExtensions( void )
{
}



static struct __DriverAPIRec radeonAPI = {
   radeonInitDriver,
   radeonDestroyScreen,
   radeonCreateContext,
   radeonDestroyContext,
   radeonCreateBuffer,
   radeonDestroyBuffer,
   radeonSwapBuffers,
   radeonMakeCurrent,
   radeonUnbindContext,
   radeonOpenFullScreen,
   radeonCloseFullScreen
};



/*
 * This is the bootstrap function for the driver.
 * The __driCreateScreen name is the symbol that libGL.so fetches.
 * Return:  pointer to a __DRIscreenPrivate.
 */
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                        int numConfigs, __GLXvisualConfig *config)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &radeonAPI);
   return (void *) psp;
}
