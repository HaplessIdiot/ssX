/* Id: accum.c,v 1.3 1999/02/26 08:52:30 martin Exp $ */

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
 * Log: accum.c,v $
 * Revision 1.3  1999/02/26 08:52:30  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.4  1999/02/24 22:48:04  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.3  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.2  1998/07/17 03:26:09  brianp
 * include masking.h
 *
 * Revision 3.1  1998/06/30 04:08:47  brianp
 * glAccum(GL_RETURN, s) didn't obey glColorMask() settings
 *
 * Revision 3.0  1998/01/31 20:46:29  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/accum.c,v 1.0tsi Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "accum.h"
#include "context.h"
#include "macros.h"
#include "masking.h"
#include "span.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif


void gl_alloc_accum_buffer( GLcontext *ctx )
{
   GLint n;

   if (ctx->Buffer->Accum) {
      free( ctx->Buffer->Accum );
      ctx->Buffer->Accum = NULL;
   }

   /* allocate accumulation buffer if not already present */
   n = ctx->Buffer->Width * ctx->Buffer->Height * 4 * sizeof(GLaccum);
   ctx->Buffer->Accum = (GLaccum *) malloc( n );
   if (!ctx->Buffer->Accum) {
      /* unable to setup accumulation buffer */
      gl_error( ctx, GL_OUT_OF_MEMORY, "glAccum" );
   }
}



void gl_ClearAccum( GLcontext *ctx,
                    GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glAccum" );
      return;
   }
   ctx->Accum.ClearColor[0] = CLAMP( red, -1.0, 1.0 );
   ctx->Accum.ClearColor[1] = CLAMP( green, -1.0, 1.0 );
   ctx->Accum.ClearColor[2] = CLAMP( blue, -1.0, 1.0 );
   ctx->Accum.ClearColor[3] = CLAMP( alpha, -1.0, 1.0 );
}




void gl_Accum( GLcontext *ctx, GLenum op, GLfloat value )
{
   GLuint xpos, ypos, width, height;
   GLfloat acc_scale;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glAccum" );
      return;
   }

   if (ctx->Visual->AccumBits==0 || !ctx->Buffer->Accum) {
      /* No accumulation buffer! */
      gl_warning(ctx, "Calling glAccum() without an accumulation buffer");
      return;
   }

   if (sizeof(GLaccum)==1) {
      acc_scale = 127.0;
   }
   else if (sizeof(GLaccum)==2) {
      acc_scale = 32767.0;
   }
   else {
      /* sizeof(GLaccum) > 2 (Cray) */
      acc_scale = (float) SHRT_MAX;
   }

   if (ctx->NewState)
      gl_update_state( ctx );

   /* Determine region to operate upon. */
   if (ctx->Scissor.Enabled) {
      xpos = ctx->Scissor.X;
      ypos = ctx->Scissor.Y;
      width = ctx->Scissor.Width;
      height = ctx->Scissor.Height;
   }
   else {
      /* whole window */
      xpos = 0;
      ypos = 0;
      width = ctx->Buffer->Width;
      height = ctx->Buffer->Height;
   }

   switch (op) {
      case GL_ADD:
         {
	    GLaccum ival, *acc;
	    GLuint i, j;

	    ival = (GLaccum) (value * acc_scale);
	    for (j=0;j<height;j++) {
	       acc = ctx->Buffer->Accum
                     + (ypos * ctx->Buffer->Width + xpos) * 4;
	       for (i=0;i<width;i++) {
		  *acc += ival;	  acc++;   /* red */
		  *acc += ival;	  acc++;   /* green */
		  *acc += ival;	  acc++;   /* blue */
		  *acc += ival;	  acc++;   /* alpha */
	       }
	       ypos++;
	    }
	 }
	 break;
      case GL_MULT:
	 {
	    GLaccum *acc;
	    GLuint i, j;

	    for (j=0;j<height;j++) {
	       acc = ctx->Buffer->Accum
                     + (ypos * ctx->Buffer->Width + xpos) * 4;
	       for (i=0;i<width;i++) {
		  *acc = (GLaccum) ( (GLfloat) *acc * value );	  acc++; /*r*/
		  *acc = (GLaccum) ( (GLfloat) *acc * value );	  acc++; /*g*/
		  *acc = (GLaccum) ( (GLfloat) *acc * value );	  acc++; /*g*/
		  *acc = (GLaccum) ( (GLfloat) *acc * value );	  acc++; /*a*/
	       }
	       ypos++;
	    }
	 }
	 break;
      case GL_ACCUM:
	 {
	    GLaccum *acc;
	    GLubyte rgba[MAX_WIDTH][4];
	    GLfloat rscale, gscale, bscale, ascale;
	    GLuint i, j;

	    (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.ReadBuffer );

	    /* Accumulate */
	    rscale = value * acc_scale / 255.0;
	    gscale = value * acc_scale / 255.0;
	    bscale = value * acc_scale / 255.0;
	    ascale = value * acc_scale / 255.0;
	    for (j=0;j<height;j++) {
               gl_read_rgba_span(ctx, width, xpos, ypos, rgba);
	       acc = ctx->Buffer->Accum
                     + (ypos * ctx->Buffer->Width + xpos) * 4;
	       for (i=0;i<width;i++) {
		  *acc += (GLaccum) ( (GLfloat) rgba[i][RCOMP] * rscale );  acc++;
		  *acc += (GLaccum) ( (GLfloat) rgba[i][GCOMP] * gscale );  acc++;
		  *acc += (GLaccum) ( (GLfloat) rgba[i][BCOMP] * bscale );  acc++;
		  *acc += (GLaccum) ( (GLfloat) rgba[i][ACOMP] * ascale );  acc++;
	       }
	       ypos++;
	    }

	    (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );
	 }
	 break;
      case GL_LOAD:
	 {
	    GLaccum *acc;
	    GLubyte rgba[MAX_WIDTH][4];
	    GLfloat rscale, gscale, bscale, ascale;
	    GLuint i, j;

	    (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.ReadBuffer );

	    /* Load accumulation buffer */
	    rscale = value * acc_scale / 255.0;
	    gscale = value * acc_scale / 255.0;
	    bscale = value * acc_scale / 255.0;
	    ascale = value * acc_scale / 255.0;
	    for (j=0;j<height;j++) {
               gl_read_rgba_span(ctx, width, xpos, ypos, rgba);
	       acc = ctx->Buffer->Accum
                     + (ypos * ctx->Buffer->Width + xpos) * 4;
	       for (i=0;i<width;i++) {
		  *acc++ = (GLaccum) ( (GLfloat) rgba[i][RCOMP] * rscale );
		  *acc++ = (GLaccum) ( (GLfloat) rgba[i][GCOMP] * gscale );
		  *acc++ = (GLaccum) ( (GLfloat) rgba[i][BCOMP] * bscale );
		  *acc++ = (GLaccum) ( (GLfloat) rgba[i][ACOMP] * ascale );
	       }
	       ypos++;
	    }

	    (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );
	 }
	 break;
      case GL_RETURN:
	 {
	    GLubyte rgba[MAX_WIDTH][4];
	    GLaccum *acc;
	    GLfloat rscale, gscale, bscale, ascale;
	    GLuint i, j;

	    rscale = value / acc_scale * 255.0F;
	    gscale = value / acc_scale * 255.0F;
	    bscale = value / acc_scale * 255.0F;
	    ascale = value / acc_scale * 255.0F;
	    for (j=0;j<height;j++) {
	       acc = ctx->Buffer->Accum
                     + (ypos * ctx->Buffer->Width + xpos) * 4;
	       for (i=0;i<width;i++) {
		  GLint r, g, b, a;
		  r = (GLint) ( (GLfloat) (*acc++) * rscale + 0.5F );
		  g = (GLint) ( (GLfloat) (*acc++) * gscale + 0.5F );
		  b = (GLint) ( (GLfloat) (*acc++) * bscale + 0.5F );
		  a = (GLint) ( (GLfloat) (*acc++) * ascale + 0.5F );
		  rgba[i][RCOMP] = CLAMP( r, 0, 255 );
		  rgba[i][GCOMP] = CLAMP( g, 0, 255 );
		  rgba[i][BCOMP] = CLAMP( b, 0, 255 );
		  rgba[i][ACOMP] = CLAMP( a, 0, 255 );
	       }
               if (ctx->Color.SWmasking) {
                  gl_mask_rgba_span( ctx, width, xpos, ypos, rgba );
               }
               (*ctx->Driver.WriteRGBASpan)( ctx, width, xpos, ypos, rgba, NULL );
	       ypos++;
	    }
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glAccum" );
   }
}




/*
 * Clear the accumulation Buffer->
 */
void gl_clear_accum_buffer( GLcontext *ctx )
{
   GLuint buffersize;
   GLfloat acc_scale;

   if (ctx->Visual->AccumBits==0) {
      /* No accumulation buffer! */
      return;
   }

   if (sizeof(GLaccum)==1) {
      acc_scale = 127.0;
   }
   else if (sizeof(GLaccum)==2) {
      acc_scale = 32767.0;
   }
   else {
      /* sizeof(GLaccum) > 2 (Cray) */
      acc_scale = (float) SHRT_MAX;
   }

   /* number of pixels */
   buffersize = ctx->Buffer->Width * ctx->Buffer->Height;

   if (!ctx->Buffer->Accum) {
      /* try to alloc accumulation buffer */
      ctx->Buffer->Accum = (GLaccum *)
	                   malloc( buffersize * 4 * sizeof(GLaccum) );
   }

   if (ctx->Buffer->Accum) {
      if (ctx->Scissor.Enabled) {
	 /* Limit clear to scissor box */
	 GLaccum r, g, b, a;
	 GLint i, j;
         GLint width, height;
         GLaccum *row;
	 r = (GLaccum) (ctx->Accum.ClearColor[0] * acc_scale);
	 g = (GLaccum) (ctx->Accum.ClearColor[1] * acc_scale);
	 b = (GLaccum) (ctx->Accum.ClearColor[2] * acc_scale);
	 a = (GLaccum) (ctx->Accum.ClearColor[3] * acc_scale);
         /* size of region to clear */
         width = 4 * (ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1);
         height = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
         /* ptr to first element to clear */
         row = ctx->Buffer->Accum
               + 4 * (ctx->Buffer->Ymin * ctx->Buffer->Width
                      + ctx->Buffer->Xmin);
         for (j=0;j<height;j++) {
            for (i=0;i<width;i+=4) {
               row[i+0] = r;
               row[i+1] = g;
               row[i+2] = b;
               row[i+3] = a;
	    }
            row += 4 * ctx->Buffer->Width;
	 }
      }
      else {
	 /* clear whole buffer */
	 if (ctx->Accum.ClearColor[0]==0.0 &&
	     ctx->Accum.ClearColor[1]==0.0 &&
	     ctx->Accum.ClearColor[2]==0.0 &&
	     ctx->Accum.ClearColor[3]==0.0) {
	    /* Black */
	    MEMSET( ctx->Buffer->Accum, 0, buffersize * 4 * sizeof(GLaccum) );
	 }
	 else {
	    /* Not black */
	    GLaccum *acc, r, g, b, a;
	    GLuint i;

	    acc = ctx->Buffer->Accum;
	    r = (GLaccum) (ctx->Accum.ClearColor[0] * acc_scale);
	    g = (GLaccum) (ctx->Accum.ClearColor[1] * acc_scale);
	    b = (GLaccum) (ctx->Accum.ClearColor[2] * acc_scale);
	    a = (GLaccum) (ctx->Accum.ClearColor[3] * acc_scale);
	    for (i=0;i<buffersize;i++) {
	       *acc++ = r;
	       *acc++ = g;
	       *acc++ = b;
	       *acc++ = a;
	    }
	 }
      }
   }
}
