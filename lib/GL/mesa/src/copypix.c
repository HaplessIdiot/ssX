/* Id: copypix.c,v 1.3 1999/02/26 08:52:31 martin Exp $ */

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
 * Log: copypix.c,v $
 * Revision 1.3  1999/02/26 08:52:31  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.12  1999/02/24 22:48:05  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.11  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.10  1998/10/03 13:22:40  brianp
 * stencil-related clean ups
 *
 * Revision 3.9  1998/09/25 03:03:01  brianp
 * fixed front/back alpha copy bug per Sam Jordan
 *
 * Revision 3.8  1998/07/17 03:24:16  brianp
 * added Pixel.ScaleOrBiasRGBA field
 *
 * Revision 3.7  1998/05/31 23:50:36  brianp
 * cleaned up a few Solaris compiler warnings
 *
 * Revision 3.6  1998/04/01 02:58:52  brianp
 * applied Miklos Fazekas's 3-31-98 Macintosh changes
 *
 * Revision 3.5  1998/03/27 03:30:36  brianp
 * fixed G++ warnings
 *
 * Revision 3.4  1998/02/15 01:31:41  brianp
 * removed unused variables
 *
 * Revision 3.3  1998/02/08 20:22:14  brianp
 * LOTS of clean-up and rewriting
 *
 * Revision 3.2  1998/02/04 00:33:45  brianp
 * fixed a few cast problems for Amiga StormC compiler
 *
 * Revision 3.1  1998/02/01 22:16:34  brianp
 * include different headers (zooming)
 *
 * Revision 3.0  1998/01/31 20:49:24  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/copypix.c,v 1.2 1999/03/14 03:20:41 dawes Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <string.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "copypix.h"
#include "depth.h"
#include "feedback.h"
#include "macros.h"
#include "pixel.h"
#include "span.h"
#include "stencil.h"
#include "types.h"
#include "zoom.h"
#endif



static void copy_rgba_pixels( GLcontext* ctx,
                              GLint srcx, GLint srcy,
                              GLint width, GLint height,
                              GLint destx, GLint desty )
{
   GLdepth zspan[MAX_WIDTH];
   GLubyte rgba[MAX_WIDTH][4];
   GLboolean quick_draw;
   GLint sy, dy, stepy;
   GLint i, j;
   GLboolean setbuffer;
   GLubyte *saveAlpha;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0F || ctx->Pixel.ZoomY!=1.0F;

   /* Determine if copy should be done bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (ctx->Depth.Test) {
      /* fill in array of z values */
      GLint z = (GLint) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
      for (i=0;i<width;i++) {
         zspan[i] = z;
      }
   }

   if (ctx->RasterMask==0 && !zoom
       && destx>=0 && destx+width<=ctx->Buffer->Width) {
      quick_draw = GL_TRUE;
   }
   else {
      quick_draw = GL_FALSE;
   }

   /* If read and draw buffer are different we must do buffer switching */
   saveAlpha = ctx->Buffer->Alpha;
   setbuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer;
   for (j=0; j<height; j++, sy+=stepy, dy+=stepy) {
      if (setbuffer) {
         (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.ReadBuffer );
         if (ctx->Pixel.ReadBuffer == GL_FRONT)
            ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
         else
            ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
      }
      gl_read_rgba_span( ctx, width, srcx, sy, rgba );
      if (ctx->Pixel.ScaleOrBiasRGBA) {
         gl_scale_and_bias_rgba( ctx, width, rgba );
      }
      if (ctx->Pixel.MapColorFlag) {
         gl_map_rgba( ctx, width, rgba );
      }
      if (setbuffer) {
         (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );
         if (ctx->Color.DrawBuffer == GL_FRONT)
            ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
         else
            ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
      }
      if (quick_draw && dy>=0 && dy<ctx->Buffer->Height) {
         (*ctx->Driver.WriteRGBASpan)( ctx, width, destx, dy, rgba, NULL );

      }
      else if (zoom) {
         gl_write_zoomed_rgba_span( ctx, width, destx, dy, zspan, rgba, desty);
      }
      else {
         gl_write_rgba_span( ctx, width, destx, dy, zspan, rgba, GL_BITMAP 
);
      }
   }

   /* Restore current alpha buffer pointer */
   ctx->Buffer->Alpha = saveAlpha;
}


static void copy_ci_pixels( GLcontext* ctx,
                            GLint srcx, GLint srcy, GLint width, GLint height,
                            GLint destx, GLint desty )
{
   GLdepth zspan[MAX_WIDTH];
   GLint sy, dy, stepy;
   GLint i, j;
   GLboolean setbuffer;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0F || ctx->Pixel.ZoomY!=1.0F;
   const GLboolean shift_or_offset = ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset;

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (ctx->Depth.Test) {
      /* fill in array of z values */
      GLint z = (GLint) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
      for (i=0;i<width;i++) {
         zspan[i] = z;
      }
   }

   /* If read and draw buffer are different we must do buffer switching */
   setbuffer = ctx->Pixel.ReadBuffer!=ctx->Color.DrawBuffer;

   for (j=0; j<height; j++, sy+=stepy, dy+=stepy) {
      GLuint indexes[MAX_WIDTH];
      if (setbuffer) {
         (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.ReadBuffer );
      }
      gl_read_index_span( ctx, width, srcx, sy, indexes );

      if (shift_or_offset) {
         gl_shift_and_offset_ci( ctx, width, indexes );
      }
      if (ctx->Pixel.MapColorFlag) {
         gl_map_ci( ctx, width, indexes );
      }

      if (setbuffer) {
         (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );
      }
      if (zoom) {
         gl_write_zoomed_index_span( ctx, width, destx, dy, zspan, indexes, desty );
      }
      else {
         gl_write_index_span( ctx, width, destx, dy, zspan, indexes, GL_BITMAP );
      }
   }
}



/*
 * TODO: Optimize!!!!
 */
static void copy_depth_pixels( GLcontext* ctx, GLint srcx, GLint srcy,
                               GLint width, GLint height,
                               GLint destx, GLint desty )
{
   GLfloat depth[MAX_WIDTH];
   GLdepth zspan[MAX_WIDTH];
   GLuint indexes[MAX_WIDTH];
   GLubyte rgba[MAX_WIDTH][4];
   GLint sy, dy, stepy;
   GLint i, j;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0F || ctx->Pixel.ZoomY!=1.0F;

   if (!ctx->Buffer->Depth) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   /* setup colors or indexes */
   if (ctx->Visual->RGBAflag) {
      GLuint *rgba32 = (GLuint *) rgba;
      GLuint color = *((GLuint *) ctx->Current.ByteColor);
      for (i=0; i<width; i++) {
         rgba32[i] = color;
      }
   }
   else {
      for (i=0;i<width;i++) {
         indexes[i] = ctx->Current.Index;
      }
   }

   for (j=0; j<height; j++, sy+=stepy, dy+=stepy) {
      (*ctx->Driver.ReadDepthSpanFloat)( ctx, width, srcx, sy, depth );

      for (i=0;i<width;i++) {
         GLfloat d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         zspan[i] = (GLint) (CLAMP( d, 0.0F, 1.0F ) * DEPTH_SCALE);
      }

      if (ctx->Visual->RGBAflag) {
         if (zoom) {
            gl_write_zoomed_rgba_span( ctx, width, destx, dy, zspan, rgba, desty );
         }
         else {
            gl_write_rgba_span( ctx, width, destx, dy, zspan, rgba, GL_BITMAP);
         }
      }
      else {
         if (zoom) {
            gl_write_zoomed_index_span( ctx, width, destx, dy,
                                        zspan, indexes, desty );
         }
         else {
            gl_write_index_span( ctx, width, destx, dy,
                                 zspan, indexes, GL_BITMAP );
         }
      }
   }
}



static void copy_stencil_pixels( GLcontext* ctx, GLint srcx, GLint srcy,
                                 GLint width, GLint height,
                                 GLint destx, GLint desty )
{
   GLint sy, dy, stepy;
   GLint j;
   const GLboolean zoom = (ctx->Pixel.ZoomX!=1.0F || ctx->Pixel.ZoomY!=1.0F);
   const GLboolean shift_or_offset = ctx->Pixel.IndexShift!=0 || ctx->Pixel.IndexOffset!=0;

   if (!ctx->Buffer->Stencil) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   for (j=0; j<height; j++, sy+=stepy, dy+=stepy) {
      GLstencil stencil[MAX_WIDTH];
      gl_read_stencil_span( ctx, width, srcx, sy, stencil );

      if (shift_or_offset) {
         gl_shift_and_offset_stencil( ctx, width, stencil );
      }
      if (ctx->Pixel.MapStencilFlag) {
         gl_map_stencil( ctx, width, stencil );
      }

      if (zoom) {
         gl_write_zoomed_stencil_span( ctx, width, destx, dy, stencil, desty );
      }
      else {
         gl_write_stencil_span( ctx, width, destx, dy, stencil );
      }
   }
}




void gl_CopyPixels( GLcontext* ctx, GLint srcx, GLint srcy, GLsizei width, GLsizei height,
		    GLenum type )
{
   GLint destx, desty;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   if (width<0 || height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyPixels" );
      return;
   }

   if (ctx->NewState) {
      gl_update_state(ctx);
   }

   if (ctx->RenderMode==GL_RENDER) {
      /* Destination of copy: */
      if (!ctx->Current.RasterPosValid) {
	 return;
      }
      destx = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      desty = (GLint) (ctx->Current.RasterPos[1] + 0.5F);

      if (type==GL_COLOR && ctx->Visual->RGBAflag) {
         copy_rgba_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else if (type==GL_COLOR && !ctx->Visual->RGBAflag) {
         copy_ci_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else if (type==GL_DEPTH) {
         copy_depth_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else if (type==GL_STENCIL) {
         copy_stencil_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else {
	 gl_error( ctx, GL_INVALID_ENUM, "glCopyPixels" );
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      GLfloat color[4];
      color[0] = (GLfloat) ctx->Current.ByteColor[0] / 255.0;
      color[1] = (GLfloat) ctx->Current.ByteColor[1] / 255.0;
      color[2] = (GLfloat) ctx->Current.ByteColor[2] / 255.0;
      color[3] = (GLfloat) ctx->Current.ByteColor[3] / 255.0;
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_COPY_PIXEL_TOKEN );
      gl_feedback_vertex( ctx, ctx->Current.RasterPos[0],
			  ctx->Current.RasterPos[1],
			  ctx->Current.RasterPos[2],
			  ctx->Current.RasterPos[3],
			  color, ctx->Current.Index,
			  ctx->Current.TexCoord );
   }
   else if (ctx->RenderMode==GL_SELECT) {
      gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }

}


