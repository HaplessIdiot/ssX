/* Id: misc.c,v 1.3 1999/02/26 08:52:35 martin Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Log: misc.c,v $
 * Revision 1.3  1999/02/26 08:52:35  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.25  1999/02/24 22:48:06  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.24  1999/02/17 02:38:23  brianp
 * fixed bug when clearing color buffer with RGBA or index masking
 *
 * Revision 3.23  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.22  1999/01/30 04:29:16  brianp
 * stereo updates (KendallB@scitechsoft.com)
 *
 * Revision 3.21  1999/01/22 04:29:29  brianp
 * hooked in new GetString() device driver function
 *
 * Revision 3.20  1998/11/17 02:51:35  brianp
 * added a bunch of new device driver functions (Keith Harrison)
 *
 * Revision 3.19  1998/11/17 01:52:47  brianp
 * implemented GL_NV_texgen_reflection extension (MJK)
 *
 * Revision 3.18  1998/10/23 01:03:47  brianp
 * added GL_ARB_multitexture extension
 *
 * Revision 3.17  1998/10/20 02:27:12  brianp
 * removed GL_EXT_multitexture extension
 *
 * Revision 3.16  1998/10/05 00:40:15  brianp
 * added GL_INGR_blend_func_separate extension
 *
 * Revision 3.15  1998/10/03 13:16:44  brianp
 * implemented GL_EXT_stencil_wrap
 *
 * Revision 3.14  1998/09/18 02:32:02  brianp
 * fixed alpha clear problem reported by Sam Jordan
 *
 * Revision 3.13  1998/07/18 03:33:53  brianp
 * glRead/DrawBuffer() generated GL_INVALID_ENUM instead of GL_INVALID_OPERATION
 *
 * Revision 3.12  1998/06/19 02:37:58  brianp
 * fixed an error code in glReadBuffer()
 *
 * Revision 3.11  1998/06/07 22:18:52  brianp
 * implemented GL_EXT_multitexture extension
 *
 * Revision 3.10  1998/04/22 00:52:42  brianp
 * added GLcontext parameter to driver ExtensionString()
 *
 * Revision 3.9  1998/04/14 00:02:10  brianp
 * fixed bug in which whole buffer might be cleared instead of scissor rect
 *
 * Revision 3.8  1998/03/27 04:39:44  brianp
 * Driver.ExtensionString() must now return complete extension list
 *
 * Revision 3.7  1998/03/19 02:04:22  brianp
 * added GL_SGIS_texture_edge_clamp
 *
 * Revision 3.6  1998/03/15 18:50:25  brianp
 * added GL_EXT_abgr extension
 *
 * Revision 3.5  1998/03/15 18:32:12  brianp
 * applied DavidB's patches for v0.23 of fxmesa driver
 *
 * Revision 3.4  1998/02/20 04:50:44  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.3  1998/02/01 16:37:19  brianp
 * added GL_EXT_rescale_normal extension
 *
 * Revision 3.2  1998/02/01 15:23:52  brianp
 * added ExtensionString() function to device driver
 *
 * Revision 3.1  1998/01/31 23:58:19  brianp
 * new gl_Clear: removed ClearDepthBuffer and ClearColorAndDepth functions
 *
 * Revision 3.0  1998/01/31 20:59:27  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/glmisc.c,v 1.0tsi Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdlib.h>
#include <string.h>
#endif
#include "accum.h"
#include "alphabuf.h"
#include "context.h"
#include "depth.h"
#include "macros.h"
#include "masking.h"
#include "glmisc.h"
#include "stencil.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



void gl_ClearIndex( GLcontext *ctx, GLfloat c )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glClearIndex" );
      return;
   }
   ctx->Color.ClearIndex = (GLuint) c;
   if (!ctx->Visual->RGBAflag) {
      /* it's OK to call glClearIndex in RGBA mode but it should be a NOP */
      (*ctx->Driver.ClearIndex)( ctx, ctx->Color.ClearIndex );
   }
}



void gl_ClearColor( GLcontext *ctx, GLclampf red, GLclampf green,
                    GLclampf blue, GLclampf alpha )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glClearColor" );
      return;
   }

   ctx->Color.ClearColor[0] = CLAMP( red,   0.0F, 1.0F );
   ctx->Color.ClearColor[1] = CLAMP( green, 0.0F, 1.0F );
   ctx->Color.ClearColor[2] = CLAMP( blue,  0.0F, 1.0F );
   ctx->Color.ClearColor[3] = CLAMP( alpha, 0.0F, 1.0F );

   if (ctx->Visual->RGBAflag) {
      GLubyte r = (GLint) (ctx->Color.ClearColor[0] * 255.0F);
      GLubyte g = (GLint) (ctx->Color.ClearColor[1] * 255.0F);
      GLubyte b = (GLint) (ctx->Color.ClearColor[2] * 255.0F);
      GLubyte a = (GLint) (ctx->Color.ClearColor[3] * 255.0F);
      (*ctx->Driver.ClearColor)( ctx, r, g, b, a );
   }
}




/*
 * Clear the color buffer when glColorMask or glIndexMask is in effect.
 */
static void clear_color_buffer_with_masking( GLcontext *ctx )
{
   GLint x, y, height, width;

   /* Compute region to clear */
   if (ctx->Scissor.Enabled) {
      x = ctx->Buffer->Xmin;
      y = ctx->Buffer->Ymin;
      height = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
      width  = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
   }
   else {
      x = 0;
      y = 0;
      height = ctx->Buffer->Height;
      width  = ctx->Buffer->Width;
   }

   if (ctx->Visual->RGBAflag) {
      /* RGBA mode */
      GLubyte r = (GLint) (ctx->Color.ClearColor[0] * 255.0F);
      GLubyte g = (GLint) (ctx->Color.ClearColor[1] * 255.0F);
      GLubyte b = (GLint) (ctx->Color.ClearColor[2] * 255.0F);
      GLubyte a = (GLint) (ctx->Color.ClearColor[3] * 255.0F);
      GLint i;
      for (i=0;i<height;i++,y++) {
         GLubyte rgba[MAX_WIDTH][4];
         GLint j;
         for (j=0; j<width; j++) {
            rgba[j][RCOMP] = r;
            rgba[j][GCOMP] = g;
            rgba[j][BCOMP] = b;
            rgba[j][ACOMP] = a;
         }
         gl_mask_rgba_span( ctx, width, x, y, rgba );
         (*ctx->Driver.WriteRGBASpan)( ctx, width, x, y, rgba, NULL );
         if (ctx->RasterMask & ALPHABUF_BIT) {
            gl_write_alpha_span( ctx, width, x, y, rgba, NULL );
         }
      }
   }
   else {
      /* Color index mode */
      GLuint indx[MAX_WIDTH];
      GLubyte mask[MAX_WIDTH];
      GLint i, j;
      MEMSET( mask, 1, width );
      for (i=0;i<height;i++,y++) {
         for (j=0;j<width;j++) {
            indx[j] = ctx->Color.ClearIndex;
         }
         gl_mask_index_span( ctx, width, x, y, indx );
         (*ctx->Driver.WriteCI32Span)( ctx, width, x, y, indx, mask );
      }
   }
}



/*
 * Clear the front and/or back color buffers.  Also clear the alpha
 * buffer(s) if present.
 */
static void clear_color_buffers( GLcontext *ctx )
{
   if (ctx->Color.SWmasking) {
      clear_color_buffer_with_masking( ctx );
   }
   else {
      GLint x = ctx->Buffer->Xmin;
      GLint y = ctx->Buffer->Ymin;
      GLint height = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
      GLint width  = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
      (void) (*ctx->Driver.Clear)( ctx, GL_COLOR_BUFFER_BIT,
                                   !ctx->Scissor.Enabled,
                                   x, y, width, height );
      if (ctx->RasterMask & ALPHABUF_BIT) {
         /* front and/or back alpha buffers will be cleared here */
         gl_clear_alpha_buffers( ctx );
      }
   }

   if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
      /*** Also clear the back buffer ***/
      (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
      if (ctx->Color.SWmasking) {
         clear_color_buffer_with_masking( ctx );
      }
      else {
         GLint x = ctx->Buffer->Xmin;
         GLint y = ctx->Buffer->Ymin;
         GLint height = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
         GLint width  = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
         (void) (*ctx->Driver.Clear)( ctx, GL_COLOR_BUFFER_BIT,
                                      !ctx->Scissor.Enabled,
                                      x, y, width, height );
      }
      (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
   }
}



void gl_Clear( GLcontext *ctx, GLbitfield mask )
{
#ifdef PROFILE
   GLdouble t0 = gl_time();
#endif

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glClear" );
      return;
   }

   if (ctx->RenderMode==GL_RENDER) {
      GLint x, y, width, height;
      GLbitfield newMask;

      if (ctx->NewState) {
         gl_update_state( ctx );
      }

      x = ctx->Buffer->Xmin;
      y = ctx->Buffer->Ymin;
      height = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
      width  = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;

      /* let device driver try to clear the buffers */
      if ((mask & GL_COLOR_BUFFER_BIT) != 0 && ctx->Color.SWmasking) {
         /* driver can't clear color buffer with color/index masking */
         GLbitfield mask2 = mask & (~GL_COLOR_BUFFER_BIT);
         newMask = (*ctx->Driver.Clear)( ctx, mask2, !ctx->Scissor.Enabled,
                                      x, y, width, height );
         newMask |= GL_COLOR_BUFFER_BIT;
      }
      else {
         newMask = (*ctx->Driver.Clear)( ctx, mask, !ctx->Scissor.Enabled,
                                         x, y, width, height );
      }

      /* check if only clearing of alpha buffer needed */
      if ( (mask & GL_COLOR_BUFFER_BIT) &&
           (!(newMask & GL_COLOR_BUFFER_BIT)) &&
           (ctx->RasterMask & ALPHABUF_BIT) )
      {
         gl_clear_alpha_buffers( ctx );
      }

      if ((mask & GL_COLOR_BUFFER_BIT)
          && ctx->RasterMask & FRONT_AND_BACK_BIT) {
         /*** Also clear the back color buffer ***/
         (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
         (void) (*ctx->Driver.Clear)( ctx, GL_COLOR_BUFFER_BIT,
                                      !ctx->Scissor.Enabled,
                                      x, y, width, height );
         (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
      }

      if (newMask & GL_COLOR_BUFFER_BIT)    clear_color_buffers( ctx );
      if (newMask & GL_DEPTH_BUFFER_BIT)    gl_clear_depth_buffer( ctx );
      if (newMask & GL_ACCUM_BUFFER_BIT)    gl_clear_accum_buffer( ctx );
      if (newMask & GL_STENCIL_BUFFER_BIT)  gl_clear_stencil_buffer( ctx );

#ifdef PROFILE
      ctx->ClearTime += gl_time() - t0;
      ctx->ClearCount++;
#endif
   }
}



const GLubyte *gl_GetString( GLcontext *ctx, GLenum name )
{
   static char result[1000];
   static char *vendor = "Brian Paul";
   static char *version = "1.2 Mesa 3.1 beta";
   static char *extensions = "GL_EXT_blend_color GL_EXT_blend_minmax GL_EXT_blend_logic_op GL_EXT_blend_subtract GL_EXT_paletted_texture GL_EXT_point_parameters GL_EXT_polygon_offset GL_EXT_vertex_array GL_EXT_texture_object GL_EXT_texture3D GL_MESA_window_pos GL_MESA_resize_buffers GL_EXT_shared_texture_palette GL_EXT_rescale_normal GL_EXT_abgr GL_SGIS_texture_edge_clamp GL_SGIS_multitexture GL_EXT_stencil_wrap GL_INGR_blend_func_separate GL_ARB_multitexture GL_NV_texgen_reflection";

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetString" );
      return (GLubyte *) 0;
   }


   /* First see if device driver can satisfy this call */
   switch (name) {
      case GL_VENDOR:
      case GL_RENDERER:
      case GL_VERSION:
      case GL_EXTENSIONS:
         if (ctx->Driver.GetString) {
            const GLubyte *str = (*ctx->Driver.GetString)(ctx, name);
            if (str && str[0])
               return str;
         }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetString" );
         return (GLubyte *) 0;
   }

   /* If we get here, device driver didn't return a string */
   switch (name) {
      case GL_VENDOR:
         return (GLubyte *) vendor;
      case GL_RENDERER:
         strcpy(result, "Mesa");
         if (ctx->Driver.RendererString) {
            strcat(result, " ");
            strcat(result, (*ctx->Driver.RendererString)());
         }
         return (GLubyte *) result;
      case GL_VERSION:
         return (GLubyte *) version;
      case GL_EXTENSIONS:
         if (ctx->Driver.ExtensionString) {
            /* driver specifies the extensions */
            return (const GLubyte *) (*ctx->Driver.ExtensionString)(ctx);
         }
         else {
            /* return default extensions */
            return (const GLubyte *) extensions;
         }
      default:
         /* caught above */
         return NULL;
   }
}



void gl_Finish( GLcontext *ctx )
{
   /* Don't compile into display list */
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glFinish" );
      return;
   }
   if (ctx->Driver.Finish) {
      (*ctx->Driver.Finish)( ctx );
   }
}



void gl_Flush( GLcontext *ctx )
{
   /* Don't compile into display list */
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glFlush" );
      return;
   }
   if (ctx->Driver.Flush) {
      (*ctx->Driver.Flush)( ctx );
   }
}



void gl_Hint( GLcontext *ctx, GLenum target, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glHint" );
      return;
   }
   if (mode!=GL_DONT_CARE && mode!=GL_FASTEST && mode!=GL_NICEST) {
      gl_error( ctx, GL_INVALID_ENUM, "glHint(mode)" );
      return;
   }
   switch (target) {
      case GL_FOG_HINT:
         ctx->Hint.Fog = mode;
         break;
      case GL_LINE_SMOOTH_HINT:
         ctx->Hint.LineSmooth = mode;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
         ctx->Hint.PerspectiveCorrection = mode;
         break;
      case GL_POINT_SMOOTH_HINT:
         ctx->Hint.PointSmooth = mode;
         break;
      case GL_POLYGON_SMOOTH_HINT:
         ctx->Hint.PolygonSmooth = mode;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glHint(target)" );
         return;
   }
   ctx->NewState |= NEW_ALL;   /* just to be safe */

   if (ctx->Driver.Hint) {
      (*ctx->Driver.Hint)( ctx, target, mode );
   }
}



void gl_DrawBuffer( GLcontext *ctx, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
      return;
   }
   switch (mode) {
      case GL_FRONT_AND_BACK:
	 if ( (*ctx->Driver.SetBuffer)( ctx, GL_FRONT ) == GL_FALSE ) {
	    gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
	    return;
	 }
	 ctx->Color.DrawBuffer = mode;
	 ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
	 ctx->NewState |= NEW_RASTER_OPS;
	 break;
      case GL_FRONT:
      case GL_FRONT_LEFT:
      case GL_FRONT_RIGHT:
	 if ( (*ctx->Driver.SetBuffer)( ctx, mode ) == GL_FALSE ) {
	    gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
	    return;
	 }
	 ctx->Color.DrawBuffer = mode;
	 ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
	 ctx->NewState |= NEW_RASTER_OPS;
	 break;
      case GL_BACK:
      case GL_BACK_LEFT:
      case GL_BACK_RIGHT:
	 if ( (*ctx->Driver.SetBuffer)( ctx, mode ) == GL_FALSE) {
	    gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
	    return;
	 }
	 ctx->Color.DrawBuffer = mode;
	 ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
	 ctx->NewState |= NEW_RASTER_OPS;
	 break;
      case GL_LEFT:
	 switch (ctx->Color.DrawBuffer) {
	    case GL_BACK:
	    case GL_BACK_LEFT:
	    case GL_BACK_RIGHT:
	       mode = GL_BACK_LEFT;
	       break;
	    case GL_FRONT:
	    case GL_FRONT_LEFT:
	    case GL_FRONT_RIGHT:
	       mode = GL_FRONT_LEFT;
	       break;
            default:
               gl_problem( ctx, "default case in gl_DrawBuffer(GL_LEFT)");
         }
	 if ( (*ctx->Driver.SetBuffer)( ctx, mode ) == GL_FALSE) {
	    gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
	    return;
	 }
	 ctx->Color.DrawBuffer = mode;
	 ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
	 ctx->NewState |= NEW_RASTER_OPS;
	 break;
      case GL_RIGHT:
	 switch (ctx->Color.DrawBuffer) {
            case GL_BACK:
            case GL_BACK_LEFT:
            case GL_BACK_RIGHT:
               mode = GL_BACK_RIGHT;
               break;
            case GL_FRONT:
            case GL_FRONT_LEFT:
            case GL_FRONT_RIGHT:
               mode = GL_FRONT_RIGHT;
               break;
            default:
               gl_problem( ctx, "default case in gl_DrawBuffer(GL_RIGHT)");
         }
	 if ( (*ctx->Driver.SetBuffer)( ctx, mode ) == GL_FALSE) {
	    gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
	    return;
	 }
	 ctx->Color.DrawBuffer = mode;
	 ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
	 ctx->NewState |= NEW_RASTER_OPS;
	 break;
      case GL_NONE:
         ctx->Color.DrawBuffer = mode;
         ctx->Buffer->Alpha = NULL;
         ctx->NewState |= NEW_RASTER_OPS;
         break;
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
         gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawBuffer" );
   }
}



void gl_ReadBuffer( GLcontext *ctx, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
      return;
   }
   switch (mode) {
      case GL_FRONT:
      case GL_FRONT_LEFT:
         if ( (*ctx->Driver.SetBuffer)( ctx, GL_FRONT ) == GL_FALSE) {
            gl_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel.ReadBuffer = mode;
         ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
         ctx->NewState |= NEW_RASTER_OPS;
         break;
      case GL_BACK:
      case GL_BACK_LEFT:
         if ( (*ctx->Driver.SetBuffer)( ctx, GL_BACK ) == GL_FALSE) {
            gl_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel.ReadBuffer = mode;
         ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
         ctx->NewState |= NEW_RASTER_OPS;
         break;
      case GL_FRONT_RIGHT:
      case GL_BACK_RIGHT:
      case GL_LEFT:
      case GL_RIGHT:
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
         gl_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glReadBuffer" );
   }

   /* Remember, the draw buffer is the default state */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );
}
